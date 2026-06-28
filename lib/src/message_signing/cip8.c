/**
 * \file cip8.c
 *
 * \author angel.castillo
 * \date   Nov 21, 2025
 *
 * Copyright 2025 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include <assert.h>
#include <cardano/message_signing/cip8.h>
#include <src/string_safe.h>
#include <string.h>

/* STATIC CONSTANTS **********************************************************/

// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_HEADER_ALG_LABEL = 1; /* alg */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_HEADER_KID_LABEL = 4; /* kid */

// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_KEY_LABEL_KTY = 1; /* key type */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_KEY_LABEL_KID = 2; /* key id */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_KEY_LABEL_ALG = 3; /* algorithm */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_KEY_LABEL_CRV = -1; /* curve */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_KEY_LABEL_X = -2; /* public key bytes */

// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_KTY_OKP = 1; /* OKP */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_ALG_EDDSA = -8; /* EdDSA */
// cppcheck-suppress misra-c2012-8.9
static const int32_t COSE_CRV_ED25519 = 6; /* Ed25519 */

/* STATIC FUNCTIONS ***********************************************************/

/**
 * \brief Builds the protected header map used in CIP-8 COSE_Sign1 structures for
 *        address-bound signatures.
 *
 * This function constructs the CBOR-encoded protected headers required by CIP-8.
 * The resulting protected header map has the following shape:
 *
 * \code{.cbor}
 * {
 *   1:  -8,
 *   4:  <address_bytes>,
 *   "address": <address_bytes>
 * }
 * \endcode
 *
 * The encoded map is returned as a \ref cardano_buffer_t containing the CBOR
 * bytes for the protected header bstr. This buffer must be released with
 * \ref cardano_buffer_unref() by the caller.
 *
 * \param[in]  address_bytes   Pointer to the raw address bytes.
 * \param[in]  address_size    Size of \p address_bytes in bytes.
 * \param[out] protected_bstr_out
 *                             On success, receives a newly allocated buffer
 *                             containing the CBOR-encoded protected header bstr.
 *                             The caller takes ownership and must unref it.
 *
 * \retval CARDANO_SUCCESS                   The protected header was generated.
 * \retval CARDANO_ERROR_POINTER_IS_NULL     A required argument is NULL.
 * \retval CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *                                           Memory allocation failed.
 * \retval CARDANO_ERROR_CBOR_ENCODING       An encoding error occurred while
 *                                           writing the CBOR map.
 *
 * \note The protected headers are immutable, as required by COSE.
 * \note The address bytes are included twice: once under COSE header label `4`
 *       (`kid`), and once under the CIP-8 semantic field `"address"` to allow
 *       Cardano-specific verifiers to bind the signature to an address.
 */
static cardano_error_t
cip8_build_protected_headers(
  const byte_t*      address_bytes,
  const size_t       address_size,
  cardano_buffer_t** protected_bstr_out)
{
  assert(address_size > 0U);
  assert(protected_bstr_out != NULL);

  if (address_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *protected_bstr_out = NULL;

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_start_map(writer, 3);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_HEADER_ALG_LABEL);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_signed_int(writer, COSE_ALG_EDDSA);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_HEADER_KID_LABEL);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, address_bytes, address_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  static const char ADDRESS_LABEL[] = "address";

  result = cardano_cbor_writer_write_textstring(writer, ADDRESS_LABEL, sizeof(ADDRESS_LABEL) - 1U);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, address_bytes, address_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_encode_in_buffer(writer, protected_bstr_out);

  cardano_cbor_writer_unref(&writer);

  return result;
}

/**
 * \brief Builds the protected header map used in CIP-8 COSE_Sign1 structures
 *        for key-hash–bound signatures.
 *
 * This function constructs the CBOR-encoded protected headers required by CIP-8
 * when the signature is bound to a payment key hash rather than a full address.
 *
 * The resulting protected header map has the following form:
 *
 * \code{.cbor}
 * {
 *   1:  -8,
 *   "address": <key_hash_bytes>
 * }
 * \endcode
 *
 * The encoded map is returned as a \ref cardano_buffer_t containing the CBOR
 * bytes corresponding to the protected header bstr. The caller becomes the
 * owner of the returned buffer and must release it with
 * \ref cardano_buffer_unref().
 *
 * \param[in]  key_hash_bytes   Pointer to the raw Blake2b-224 key hash bytes.
 * \param[in]  key_hash_size    Size of \p key_hash_bytes in bytes.
 * \param[out] protected_bstr_out
 *                               On success, receives a newly allocated buffer
 *                               containing the CBOR-encoded protected headers.
 *                               Must be freed with \ref cardano_buffer_unref().
 *
 * \retval CARDANO_SUCCESS                    The protected headers were built.
 * \retval CARDANO_ERROR_POINTER_IS_NULL      A required pointer argument is NULL.
 * \retval CARDANO_ERROR_INVALID_ARGUMENT      The key hash is empty or invalid.
 * \retval CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *                                            Memory allocation failed.
 * \retval CARDANO_ERROR_CBOR_ENCODING        A CBOR encoding error occurred.
 *
 * \note The protected header is immutable by COSE design.
 */
static cardano_error_t
cip8_build_protected_headers_for_key_hash(
  const byte_t*      key_hash_bytes,
  const size_t       key_hash_size,
  cardano_buffer_t** protected_bstr_out)
{
  assert(key_hash_size > 0U);
  assert(protected_bstr_out != NULL);

  if (key_hash_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *protected_bstr_out = NULL;

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_start_map(writer, 2);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_HEADER_ALG_LABEL);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_signed_int(writer, COSE_ALG_EDDSA);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  static const char ADDRESS_LABEL[] = "address";

  result = cardano_cbor_writer_write_textstring(writer, ADDRESS_LABEL, sizeof(ADDRESS_LABEL) - 1U);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, key_hash_bytes, key_hash_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_encode_in_buffer(writer, protected_bstr_out);
  cardano_cbor_writer_unref(&writer);

  return result;
}

/**
 * \brief Builds and signs the CIP-8 Sig_structure for a COSE_Sign1 signature.
 *
 * This function constructs the COSE_Sign1 Sig_structure defined by RFC 8152
 * and required by CIP-8, then uses the provided Ed25519 private key to sign it.
 *
 * The Sig_structure is defined as:
 *
 * \code{.cbor}
 * Sig_structure = [
 *   "Signature1",
 *   protected,
 *   external_aad,
 *   payload
 * ]
 * \endcode
 *
 * Where:
 *   - `"Signature1"` is the fixed COSE context string.
 *   - `protected` is the raw bstr of the protected headers.
 *   - `external_aad` is always the empty bstr for CIP-8.
 *   - `payload` is the original message being signed.
 *
 * This function:
 *   1. CBOR-encodes the Sig_structure into a temporary buffer.
 *   2. Signs the encoded bytes using Ed25519.
 *   3. Returns the resulting signature object.
 *
 * The caller takes ownership of \p signature_out and must free it using
 * \ref cardano_ed25519_signature_unref().
 *
 * \param[in]  message         Pointer to the message to be signed.
 * \param[in]  message_size    Size of \p message in bytes.
 * \param[in]  protected_bstr  The CBOR-encoded protected header bstr produced
 *                             by CIP-8 header construction functions.
 * \param[in]  signing_key     The Ed25519 private key used to sign the structure.
 * \param[out] signature_out   On success, receives a newly allocated signature
 *                             object. Must be unref'd with
 *                             \ref cardano_ed25519_signature_unref().
 *
 * \retval CARDANO_SUCCESS                      The Sig_structure was signed successfully.
 * \retval CARDANO_ERROR_POINTER_IS_NULL        One of the arguments is NULL.
 * \retval CARDANO_ERROR_INVALID_ARGUMENT       The message size is zero.
 * \retval CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *                                              Memory allocation failed.
 * \retval CARDANO_ERROR_CBOR_ENCODING          Failed to encode the Sig_structure.
 * \retval CARDANO_ERROR_CRYPTO                 An error occurred during signing.
 *
 * \note This function does **not** modify \p protected_bstr or \p message.
 * \note The caller is responsible for passing a valid protected header bstr.
 */
static cardano_error_t
cip8_sign_sig_structure(
  const byte_t*                        message,
  const size_t                         message_size,
  const cardano_buffer_t*              protected_bstr,
  const cardano_ed25519_private_key_t* signing_key,
  cardano_ed25519_signature_t**        signature_out)
{
  assert(message != NULL);
  assert(message_size > 0U);
  assert(signature_out != NULL);
  assert(signing_key != NULL);
  assert(protected_bstr != NULL);

  *signature_out = NULL;

  const byte_t* body_protected      = cardano_buffer_get_data(protected_bstr);
  const size_t  body_protected_size = cardano_buffer_get_size(protected_bstr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_start_array(writer, 4);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  static const char CONTEXT_SIGNATURE1[] = "Signature1";

  result = cardano_cbor_writer_write_textstring(writer, CONTEXT_SIGNATURE1, sizeof(CONTEXT_SIGNATURE1) - 1U);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, body_protected, body_protected_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  static const byte_t EMPTY_BYTES[1] = { 0 };
  result                             = cardano_cbor_writer_write_bytestring(writer, EMPTY_BYTES, 0U);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, message, message_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  cardano_buffer_t* sig_struct_buf = NULL;
  result                           = cardano_cbor_writer_encode_in_buffer(writer, &sig_struct_buf);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  const byte_t* sig_struct_bytes = cardano_buffer_get_data(sig_struct_buf);
  const size_t  sig_struct_size  = cardano_buffer_get_size(sig_struct_buf);

  result = cardano_ed25519_private_key_sign(
    signing_key,
    sig_struct_bytes,
    sig_struct_size,
    signature_out);

  cardano_buffer_unref(&sig_struct_buf);
  cardano_cbor_writer_unref(&writer);

  return result;
}

/**
 * \brief Constructs a COSE_Sign1 structure for CIP-8.
 *
 * This function builds the full COSE_Sign1 object required by CIP-8, using the
 * already-signed Ed25519 signature and the protected headers.
 *
 * The produced COSE_Sign1 structure has the canonical form:
 *
 * \code{.cbor}
 * COSE_Sign1 = [
 *   protected : bstr,
 *   unprotected : { "hashed" : false },
 *   payload : bstr,
 *   signature : bstr
 * ]
 * \endcode
 *
 * Where:
 *   - `protected` is the raw bstr of the CBOR-encoded protected header map.
 *   - `unprotected` always contains `"hashed" = false` for CIP-8.
 *   - `payload` is the message being signed (as a bstr).
 *   - `signature` is the raw Ed25519 signature.
 *
 * This function:
 *   1. Assembles the 4-element COSE_Sign1 array.
 *   2. Encodes it into a CBOR buffer.
 *   3. Returns the resulting buffer to the caller.
 *
 * Upon success, the caller takes ownership of \p cose_sign1_out, which must be
 * freed using \ref cardano_buffer_unref().
 *
 * \param[in]  message         The message payload to embed inside the COSE_Sign1.
 * \param[in]  message_size    Size of \p message in bytes.
 * \param[in]  protected_bstr  The CBOR-encoded protected header bstr previously
 *                             produced by CIP-8 header construction.
 * \param[in]  signature       The Ed25519 signature object returned by
 *                             \ref cip8_sign_sig_structure().
 * \param[out] cose_sign1_out  On success, receives the newly allocated COSE_Sign1
 *                             buffer. Must be released with \ref cardano_buffer_unref().
 *
 * \retval CARDANO_SUCCESS                      The COSE_Sign1 object was built successfully.
 * \retval CARDANO_ERROR_POINTER_IS_NULL        A required pointer argument is NULL.
 * \retval CARDANO_ERROR_INVALID_ARGUMENT       The message size was zero.
 * \retval CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *                                              Memory allocation failed.
 * \retval CARDANO_ERROR_CBOR_ENCODING          Failed to encode the COSE_Sign1 structure.
 *
 * \note This function does not validate the signature or message.
 * \note This function assumes the signature is valid and corresponds to the
 *       Sig_structure previously produced for this message.
 */
static cardano_error_t
cip8_build_cose_sign1(
  const byte_t*                      message,
  const size_t                       message_size,
  const cardano_buffer_t*            protected_bstr,
  const cardano_ed25519_signature_t* signature,
  cardano_buffer_t**                 cose_sign1_out)
{
  assert(message != NULL);
  assert(message_size > 0U);
  assert(cose_sign1_out != NULL);
  assert(protected_bstr != NULL);
  assert(signature != NULL);

  *cose_sign1_out = NULL;

  const byte_t* protected_bytes = cardano_buffer_get_data(protected_bstr);
  const size_t  protected_size  = cardano_buffer_get_size(protected_bstr);

  const byte_t* sig_bytes = cardano_ed25519_signature_get_data(signature);
  const size_t  sig_size  = cardano_ed25519_signature_get_bytes_size(signature);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_start_array(writer, 4);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, protected_bytes, protected_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_start_map(writer, 1);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  static const char HASHED_LABEL[] = "hashed";

  result = cardano_cbor_writer_write_textstring(writer, HASHED_LABEL, sizeof(HASHED_LABEL) - 1U);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bool(writer, false);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, message, message_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, sig_bytes, sig_size);
  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_encode_in_buffer(writer, cose_sign1_out);
  cardano_cbor_writer_unref(&writer);

  return result;
}

/**
 * \brief Constructs a COSE_Key structure for CIP-8.
 *
 * This function builds a COSE_Key map compliant with CIP-8 and COSE (RFC 8152)
 * using the provided Ed25519 public key and key identifier (kid).
 *
 * The resulting structure has the canonical form:
 *
 * \code{.cbor}
 * COSE_Key = {
 *   1  : 1,
 *   3  : -8,
 *   2  : <kid>,
 *  -1  : 6,
 *  -2  : <public key>
 * }
 * \endcode
 *
 * The `kid` is application-defined within CIP-8:
 *   - When signing with a full address: it is the address bytes.
 *   - When signing with a payment key hash: it is the 28-byte key hash.
 *
 * On success, the function encodes the COSE_Key into a CBOR buffer and returns it
 * via \p cose_key_out. The caller becomes responsible for releasing the buffer
 * with \ref cardano_buffer_unref().
 *
 * \param[in]  kid_bytes     The key identifier bytes (address or key hash).
 * \param[in]  kid_size      Size of \p kid_bytes in bytes.
 * \param[in]  public_key    The Ed25519 public key associated with the signing key.
 * \param[out] cose_key_out  On success, receives a newly allocated COSE_Key CBOR buffer.
 *                           Must be freed with \ref cardano_buffer_unref().
 *
 * \retval CARDANO_SUCCESS                      The COSE_Key was constructed successfully.
 * \retval CARDANO_ERROR_POINTER_IS_NULL        One or more required pointers were NULL.
 * \retval CARDANO_ERROR_INVALID_ARGUMENT       \p kid_size was zero or invalid.
 * \retval CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *                                              Memory allocation failed.
 * \retval CARDANO_ERROR_CBOR_ENCODING          Failed to encode the COSE_Key.
 *
 * \note The caller must ensure \p public_key is a valid Ed25519 public key.
 * \note The COSE_Key contains no private material; it is safe to share and transmit.
 */
static cardano_error_t
cip8_build_cose_key(
  const byte_t*                       kid_bytes,
  const size_t                        kid_size,
  const cardano_ed25519_public_key_t* public_key,
  cardano_buffer_t**                  cose_key_out)
{
  assert(kid_bytes != NULL);
  assert(kid_size > 0U);
  assert(cose_key_out != NULL);
  assert(public_key != NULL);

  *cose_key_out = NULL;

  const byte_t* pub_bytes = cardano_ed25519_public_key_get_data(public_key);
  size_t        pub_size  = cardano_ed25519_public_key_get_bytes_size(public_key);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_map(writer, 5);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_KEY_LABEL_KTY);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_KTY_OKP);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_KEY_LABEL_KID);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, kid_bytes, kid_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_KEY_LABEL_ALG);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_signed_int(writer, COSE_ALG_EDDSA);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_signed_int(writer, COSE_KEY_LABEL_CRV);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, COSE_CRV_ED25519);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_signed_int(writer, COSE_KEY_LABEL_X);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, pub_bytes, pub_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = cardano_cbor_writer_encode_in_buffer(writer, cose_key_out);
  cardano_cbor_writer_unref(&writer);

  return result;
}

/* PUBLIC FUNCTIONS ************************************************************/

CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_cip8_sign(
  const byte_t*                        message,
  const size_t                         message_size,
  const cardano_address_t*             address,
  const cardano_ed25519_private_key_t* signing_key,
  cardano_buffer_t**                   cose_sign1_out,
  cardano_buffer_t**                   cose_key_out)
{
  if ((message == NULL) || (address == NULL) || (signing_key == NULL) || (cose_sign1_out == NULL) || (cose_key_out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (message_size == 0U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *cose_sign1_out = NULL;
  *cose_key_out   = NULL;

  cardano_error_t result = CARDANO_SUCCESS;

  const byte_t* address_bytes = cardano_address_get_bytes(address);
  size_t        address_size  = cardano_address_get_bytes_size(address);

  cardano_buffer_t* protected_bstr = NULL;
  result                           = cip8_build_protected_headers(address_bytes, address_size, &protected_bstr);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_ed25519_signature_t* signature = NULL;
  result                                 = cip8_sign_sig_structure(
    message,
    message_size,
    protected_bstr,
    signing_key,
    &signature);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&protected_bstr);
    return result;
  }

  result = cip8_build_cose_sign1(
    message,
    message_size,
    protected_bstr,
    signature,
    cose_sign1_out);

  cardano_buffer_unref(&protected_bstr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_signature_unref(&signature);
    return result;
  }

  cardano_ed25519_public_key_t* public_key = NULL;

  result = cardano_ed25519_private_key_get_public_key(
    signing_key,
    &public_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_signature_unref(&signature);
    cardano_buffer_unref(cose_sign1_out);
    *cose_sign1_out = NULL;

    return result;
  }

  result = cip8_build_cose_key(address_bytes, address_size, public_key, cose_key_out);

  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_signature_unref(&signature);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(cose_sign1_out);
    *cose_sign1_out = NULL;
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cip8_sign_ex(
  const byte_t*                        message,
  const size_t                         message_size,
  const cardano_blake2b_hash_t*        key_hash,
  const cardano_ed25519_private_key_t* signing_key,
  cardano_buffer_t**                   cose_sign1_out,
  cardano_buffer_t**                   cose_key_out)
{
  if ((message == NULL) || (key_hash == NULL) || (signing_key == NULL) || (cose_sign1_out == NULL) || (cose_key_out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (message_size == 0U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *cose_sign1_out = NULL;
  *cose_key_out   = NULL;

  cardano_error_t result = CARDANO_SUCCESS;

  const byte_t* key_hash_bytes = cardano_blake2b_hash_get_data(key_hash);
  size_t        key_hash_size  = cardano_blake2b_hash_get_bytes_size(key_hash);

  if ((key_hash_bytes == NULL) || (key_hash_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_buffer_t* protected_bstr = NULL;
  result                           = cip8_build_protected_headers_for_key_hash(
    key_hash_bytes,
    key_hash_size,
    &protected_bstr);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_ed25519_signature_t* signature = NULL;
  result                                 = cip8_sign_sig_structure(
    message,
    message_size,
    protected_bstr,
    signing_key,
    &signature);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&protected_bstr);
    return result;
  }

  result = cip8_build_cose_sign1(
    message,
    message_size,
    protected_bstr,
    signature,
    cose_sign1_out);

  cardano_buffer_unref(&protected_bstr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_signature_unref(&signature);
    return result;
  }

  cardano_ed25519_public_key_t* public_key = NULL;

  result = cardano_ed25519_private_key_get_public_key(
    signing_key,
    &public_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_signature_unref(&signature);
    cardano_buffer_unref(cose_sign1_out);
    *cose_sign1_out = NULL;

    return result;
  }

  result = cip8_build_cose_key(
    key_hash_bytes,
    key_hash_size,
    public_key,
    cose_key_out);

  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_signature_unref(&signature);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(cose_sign1_out);
    *cose_sign1_out = NULL;
    return result;
  }

  return CARDANO_SUCCESS;
}
