/**
 * \file blake2b_hash_internal.h
 *
 * \author angel.castillo
 * \date   Oct 31, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_INTERNAL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_INTERNAL_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Decodes a hash from a CBOR byte string that may be empty.
 *
 * Behaves like \ref cardano_blake2b_hash_from_cbor, except that a zero length byte string is accepted
 * and yields a hash of zero bytes. The ledger stores some hashes, such as the hash of the stake pool
 * metadata, as arbitrary byte arrays, so an empty value is valid on chain. A hash read this way encodes
 * back as an empty byte string and writes an empty string in CIP-116 JSON.
 *
 * \param[in] reader The CBOR reader positioned at the byte string.
 * \param[out] blake2b_hash On success, the decoded hash, owned by the caller and released with
 *             \ref cardano_blake2b_hash_unref.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_UNEXPECTED_CBOR_TYPE if the next item is
 *         not a byte string, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if memory cannot be allocated, or
 *         the error reported by the reader.
 */
cardano_error_t
_cardano_blake2b_hash_from_cbor_allow_empty(cardano_cbor_reader_t* reader, cardano_blake2b_hash_t** blake2b_hash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_INTERNAL_H */
