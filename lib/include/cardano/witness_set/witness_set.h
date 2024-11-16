/**
 * \file witness_set.h
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
 *
 * Copyright 2024 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_WITNESS_SET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_WITNESS_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/scripts/script.h>
#include <cardano/typedefs.h>
#include <cardano/witness_set/bootstrap_witness_set.h>
#include <cardano/witness_set/native_script_set.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/plutus_v1_script_set.h>
#include <cardano/witness_set/plutus_v2_script_set.h>
#include <cardano/witness_set/plutus_v3_script_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/vkey_witness_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A witness is a piece of information that allows you to efficiently verify the
 * authenticity of the transaction (also known as proof).
 *
 * In Cardano, transactions have multiple types of authentication proofs, these can range
 * from signatures for spending UTxOs, to scripts (with its arguments, datums and redeemers) for
 * smart contract execution.
 */
typedef struct cardano_witness_set_t cardano_witness_set_t;

/**
 * \brief Creates and initializes a new instance of a transaction witness set.
 *
 * This function allocates and initializes a new instance of a \ref cardano_witness_set_t object,
 * which is used to hold various witness data required to validate a transaction in the Cardano blockchain.
 *
 * \param[out] witness_set A pointer to a pointer of \ref cardano_witness_set_t.
 *                                     On successful initialization, this will point to a newly created
 *                                     transaction witness set object. The caller is responsible for managing
 *                                     the lifecycle of this object by calling \ref cardano_witness_set_unref
 *                                     when the witness set is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the witness set was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = NULL;
 * cardano_error_t result = cardano_witness_set_new(&witness_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the witness_set
 *
 *   // Once done, ensure to clean up and release the witness_set
 *   cardano_witness_set_unref(&witness_set);
 * }
 * else
 * {
 *   printf("Failed to create transaction witness set: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_witness_set_new(cardano_witness_set_t** witness_set);

/**
 * \brief Creates a \ref cardano_witness_set_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_witness_set_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a witness_set.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] witness_set A pointer to a pointer of \ref cardano_witness_set_t that will be set to the address
 *                        of the newly created witness_set object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the witness set were successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the witness_set and could invalidate any existing signatures.
 *         To prevent this, when a witness_set object is created using \ref cardano_witness_set_from_cbor, it caches the original
 *         CBOR representation of datums and redeemers internally. When \ref cardano_witness_set_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_witness_set_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_witness_set_t object by calling
 *       \ref cardano_witness_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_witness_set_t* witness_set = NULL;
 *
 * cardano_error_t result = cardano_witness_set_from_cbor(reader, &witness_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the witness_set
 *
 *   // Once done, ensure to clean up and release the witness_set
 *   cardano_witness_set_unref(&witness_set);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode witness_set: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_witness_set_from_cbor(cardano_cbor_reader_t* reader, cardano_witness_set_t** witness_set);

/**
 * \brief Serializes witness set into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_witness_set_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] witness_set A constant pointer to the \ref cardano_witness_set_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p witness_set or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the witness_set and could invalidate any existing signatures.
 *         To prevent this, when a witness_set object is created using \ref cardano_witness_set_from_cbor, it caches the original
 *         CBOR representation of datums and redeemers internally. When \ref cardano_witness_set_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_witness_set_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_witness_set_to_cbor(witness_set, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_witness_set_unref(&witness_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_to_cbor(
  const cardano_witness_set_t* witness_set,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Retrieves the vkey (verification key) witness set from a witness set.
 *
 * This function extracts the set of vkey witnesses (public key and signature pairs) from a given
 * \ref cardano_witness_set_t object. Vkey witnesses are used to validate that the transaction has
 * been signed by the correct private keys corresponding to the public keys provided.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object.
 *
 * \return A pointer to a \ref cardano_vkey_witness_set_t object containing the vkey witnesses.
 *         If the witness set contains no vkey witnesses or the witness_set pointer is NULL,
 *         this function will return NULL. The returned vkey witness set is a new reference,
 *         and the caller is responsible for managing its lifecycle by calling
 *         \ref cardano_vkey_witness_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_vkey_witness_set_t* vkeys = cardano_witness_set_get_vkeys(witness_set);
 *
 * if (vkeys != NULL)
 * {
 *   // Process the vkey witness set
 *   cardano_vkey_witness_set_unref(&vkeys); // Clean up when done
 * }
 * else
 * {
 *   printf("No vkey witnesses found or witness set is NULL.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_vkey_witness_set_t* cardano_witness_set_get_vkeys(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets or removes the vkey (verification key) witness set in a witness set.
 *
 * This function assigns a set of vkey witnesses (public key and signature pairs) to a given
 * \ref cardano_witness_set_t object. If \p vkeys is NULL, it will remove the vkey witnesses from
 * the witness set, effectively clearing the vkey witnesses.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object where the vkey witnesses will be set.
 * \param[in] vkeys A pointer to an initialized \ref cardano_vkey_witness_set_t object containing the vkey witnesses to set,
 *                  or NULL to remove the vkey witnesses from the witness set.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the vkey witnesses
 *         were successfully set or removed, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the witness_set pointer is NULL.
 *
 * \note This function increases the reference count of the `vkeys` object if it is not NULL. The caller retains ownership of their references,
 *       and it is their responsibility to manage the lifecycle of both the `witness_set` and `vkeys`.
 *       If `vkeys` is NULL, the function removes the vkey witnesses from the witness set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_vkey_witness_set_t* vkeys = ...;  // Assume vkeys is already initialized
 *
 * cardano_error_t result = cardano_witness_set_set_vkeys(witness_set, vkeys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Successfully set or removed vkey witnesses.\n");
 * }
 * else
 * {
 *   printf("Failed to set or remove vkey witnesses: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_vkeys(
  cardano_witness_set_t*      witness_set,
  cardano_vkey_witness_set_t* vkeys);

/**
 * \brief Retrieves the bootstrap witness set from a witness set.
 *
 * This function fetches the bootstrap witness set from a given \ref cardano_witness_set_t object. Bootstrap witnesses are
 * used in Cardano transactions that include inputs from the Byron era (the pre-Shelley era).
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object.
 *
 * \return A pointer to a \ref cardano_bootstrap_witness_set_t object representing the bootstrap witness set.
 *         This is a new reference, and the caller is responsible for releasing it with \ref cardano_bootstrap_witness_set_unref
 *         when it is no longer needed. If the witness set does not contain any bootstrap witnesses, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_bootstrap_witness_set_t* bootstrap_witness_set = cardano_witness_set_get_bootstrap(witness_set);
 *
 * if (bootstrap_witness_set != NULL)
 * {
 *   // Use the bootstrap witness set
 *
 *   // Once done, ensure to clean up and release the bootstrap witness set
 *   cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
 * }
 * else
 * {
 *   printf("No bootstrap witnesses found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_bootstrap_witness_set_t* cardano_witness_set_get_bootstrap(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the bootstrap witness set in a witness set.
 *
 * This function assigns a bootstrap witness set to a given \ref cardano_witness_set_t object. Bootstrap witnesses are
 * necessary for transactions that involve inputs from the Byron era (the pre-Shelley era).
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object, to which the bootstrap witness set will be added or replaced.
 * \param[in] bootstraps A pointer to an initialized \ref cardano_bootstrap_witness_set_t object representing the bootstrap witnesses to be set.
 *                       This parameter can be \c NULL to remove the bootstrap witnesses from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the bootstrap witnesses
 *         were successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the witness_set pointer is \c NULL.
 *
 * \note The function does not take ownership of the \p bootstraps object. The caller must ensure the lifecycle of the \p bootstraps object is properly managed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_bootstrap_witness_set_t* bootstraps = ...; // Assume bootstraps is already initialized
 *
 * cardano_error_t result = cardano_witness_set_set_bootstrap(witness_set, bootstraps);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Bootstrap witnesses successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set bootstrap witnesses: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_bootstrap(
  cardano_witness_set_t*           witness_set,
  cardano_bootstrap_witness_set_t* bootstraps);

/**
 * \brief Retrieves the native scripts from a witness set.
 *
 * This function extracts the native scripts associated with a given \ref cardano_witness_set_t object.
 * Native scripts are used in Cardano to define spending conditions, and they can include multi-signature scripts,
 * time-locks, and other types of scripts.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object from which to retrieve the native scripts.
 *
 * \return A pointer to the \ref cardano_native_script_set_t object representing the native scripts in the witness set.
 *         If the witness set does not contain any native scripts, or if the witness_set pointer is \c NULL, this function returns \c NULL.
 *         The returned \c cardano_native_script_set_t is a new reference, and the caller is responsible for releasing it
 *         with \ref cardano_native_script_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_native_script_set_t* native_scripts = cardano_witness_set_get_native_scripts(witness_set);
 *
 * if (native_scripts != NULL)
 * {
 *   // Process the native scripts
 *
 *   // Clean up once done
 *   cardano_native_script_set_unref(&native_scripts);
 * }
 * else
 * {
 *   printf("No native scripts found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_native_script_set_t* cardano_witness_set_get_native_scripts(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the native scripts for a witness set.
 *
 * This function assigns a set of native scripts to a given \ref cardano_witness_set_t object.
 * Native scripts are used to define spending conditions, including multi-signature scripts and time-locks.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object where the native scripts will be set.
 * \param[in] native_scripts A pointer to a \ref cardano_native_script_set_t object representing the native scripts to be assigned.
 *                           This can be \c NULL to remove any existing native scripts from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the native scripts were
 *         successfully set, or an appropriate error code such as \ref CARDANO_ERROR_POINTER_IS_NULL if the witness_set is \c NULL.
 *
 * \note If \p native_scripts is \c NULL, any existing native scripts in the \p witness_set will be removed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_native_script_set_t* native_scripts = ...; // Assume native_scripts is initialized
 *
 * cardano_error_t result = cardano_witness_set_native_scripts(witness_set, native_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Native scripts have been successfully set
 * }
 * else
 * {
 *   printf("Failed to set native scripts: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Optionally, remove native scripts by passing NULL
 * cardano_witness_set_native_scripts(witness_set, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_native_scripts(
  cardano_witness_set_t*       witness_set,
  cardano_native_script_set_t* native_scripts);

/**
 * \brief Retrieves the Plutus V1 scripts from a witness set.
 *
 * This function fetches the set of Plutus V1 scripts, if any, associated with a given \ref cardano_witness_set_t object.
 * Plutus scripts are smart contracts used in the Cardano blockchain, and the V1 scripts represent the first iteration of the Plutus language.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object from which the Plutus V1 scripts will be retrieved.
 *
 * \return A pointer to the \ref cardano_plutus_v1_script_set_t object representing the set of Plutus V1 scripts,
 *         or \c NULL if no Plutus V1 scripts are present in the witness set.
 *         The returned set is a new reference, and the caller is responsible for releasing it by calling \ref cardano_plutus_v1_script_set_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_v1_script_set_t* plutus_v1_scripts = cardano_witness_set_get_plutus_v1_scripts(witness_set);
 *
 * if (plutus_v1_scripts != NULL)
 * {
 *   // Process the Plutus V1 scripts
 *
 *   // Release the Plutus V1 scripts when done
 *   cardano_plutus_v1_script_set_unref(&plutus_v1_scripts);
 * }
 * else
 * {
 *   printf("No Plutus V1 scripts found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_v1_script_set_t* cardano_witness_set_get_plutus_v1_scripts(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the Plutus V1 scripts in a witness set.
 *
 * This function assigns a set of Plutus V1 scripts to the specified \ref cardano_witness_set_t object.
 * If the Plutus scripts are set, they will be included in the witness set used for transaction validation.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object where the Plutus V1 scripts will be set.
 * \param[in] plutus_scripts A pointer to an initialized \ref cardano_plutus_v1_script_set_t object representing the Plutus V1 scripts to be set.
 *                           This parameter can be \c NULL to remove any existing Plutus V1 scripts from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the Plutus V1 scripts were
 *         successfully set or removed, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_v1_script_set_t* plutus_scripts = ...; // Assume Plutus V1 scripts are initialized
 *
 * cardano_error_t result = cardano_witness_set_plutus_v1_scripts(witness_set, plutus_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus V1 scripts set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set Plutus V1 scripts.\n");
 * }
 *
 * // You can also remove the Plutus V1 scripts by passing NULL:
 * result = cardano_witness_set_plutus_v1_scripts(witness_set, NULL);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus V1 scripts removed successfully.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_plutus_v1_scripts(
  cardano_witness_set_t*          witness_set,
  cardano_plutus_v1_script_set_t* plutus_scripts);

/**
 * \brief Retrieves the Plutus V2 scripts from a witness set.
 *
 * This function fetches the set of Plutus V2 scripts, if any, associated with a given \ref cardano_witness_set_t object.
 * Plutus scripts are smart contracts used in the Cardano blockchain, and the V2 scripts represent the first iteration of the Plutus language.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object from which the Plutus V2 scripts will be retrieved.
 *
 * \return A pointer to the \ref cardano_plutus_v2_script_set_t object representing the set of Plutus V2 scripts,
 *         or \c NULL if no Plutus V2 scripts are present in the witness set.
 *         The returned set is a new reference, and the caller is responsible for releasing it by calling \ref cardano_plutus_v2_script_set_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_v2_script_set_t* plutus_v2_scripts = cardano_witness_set_get_plutus_v2_scripts(witness_set);
 *
 * if (plutus_v2_scripts != NULL)
 * {
 *   // Process the Plutus V2 scripts
 *
 *   // Release the Plutus V2 scripts when done
 *   cardano_plutus_v2_script_set_unref(&plutus_v2_scripts);
 * }
 * else
 * {
 *   printf("No Plutus V2 scripts found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_v2_script_set_t* cardano_witness_set_get_plutus_v2_scripts(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the Plutus V2 scripts in a witness set.
 *
 * This function assigns a set of Plutus V2 scripts to the specified \ref cardano_witness_set_t object.
 * If the Plutus scripts are set, they will be included in the witness set used for transaction validation.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object where the Plutus V2 scripts will be set.
 * \param[in] plutus_scripts A pointer to an initialized \ref cardano_plutus_v2_script_set_t object representing the Plutus V2 scripts to be set.
 *                           This parameter can be \c NULL to remove any existing Plutus V2 scripts from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the Plutus V2 scripts were
 *         successfully set or removed, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_v2_script_set_t* plutus_scripts = ...; // Assume Plutus V2 scripts are initialized
 *
 * cardano_error_t result = cardano_witness_set_plutus_v2_scripts(witness_set, plutus_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus V2 scripts set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set Plutus V2 scripts.\n");
 * }
 *
 * // You can also remove the Plutus V2 scripts by passing NULL:
 * result = cardano_witness_set_plutus_v2_scripts(witness_set, NULL);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus V2 scripts removed successfully.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_plutus_v2_scripts(
  cardano_witness_set_t*          witness_set,
  cardano_plutus_v2_script_set_t* plutus_scripts);

/**
 * \brief Retrieves the Plutus V3 scripts from a witness set.
 *
 * This function fetches the set of Plutus V3 scripts, if any, associated with a given \ref cardano_witness_set_t object.
 * Plutus scripts are smart contracts used in the Cardano blockchain, and the V3 scripts represent the first iteration of the Plutus language.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object from which the Plutus V3 scripts will be retrieved.
 *
 * \return A pointer to the \ref cardano_plutus_v3_script_set_t object representing the set of Plutus V3 scripts,
 *         or \c NULL if no Plutus V3 scripts are present in the witness set.
 *         The returned set is a new reference, and the caller is responsible for releasing it by calling \ref cardano_plutus_v3_script_set_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_v3_script_set_t* plutus_v3_scripts = cardano_witness_set_get_plutus_v3_scripts(witness_set);
 *
 * if (plutus_v3_scripts != NULL)
 * {
 *   // Process the Plutus V3 scripts
 *
 *   // Release the Plutus V3 scripts when done
 *   cardano_plutus_v3_script_set_unref(&plutus_v3_scripts);
 * }
 * else
 * {
 *   printf("No Plutus V3 scripts found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_v3_script_set_t* cardano_witness_set_get_plutus_v3_scripts(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the Plutus V3 scripts in a witness set.
 *
 * This function assigns a set of Plutus V3 scripts to the specified \ref cardano_witness_set_t object.
 * If the Plutus scripts are set, they will be included in the witness set used for transaction validation.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object where the Plutus V3 scripts will be set.
 * \param[in] plutus_scripts A pointer to an initialized \ref cardano_plutus_v3_script_set_t object representing the Plutus V3 scripts to be set.
 *                           This parameter can be \c NULL to remove any existing Plutus V3 scripts from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the Plutus V3 scripts were
 *         successfully set or removed, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_v3_script_set_t* plutus_scripts = ...; // Assume Plutus V3 scripts are initialized
 *
 * cardano_error_t result = cardano_witness_set_plutus_v3_scripts(witness_set, plutus_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus V3 scripts set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set Plutus V3 scripts.\n");
 * }
 *
 * // You can also remove the Plutus V3 scripts by passing NULL:
 * result = cardano_witness_set_plutus_v3_scripts(witness_set, NULL);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus V3 scripts removed successfully.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_plutus_v3_scripts(
  cardano_witness_set_t*          witness_set,
  cardano_plutus_v3_script_set_t* plutus_scripts);

/**
 * \brief Retrieves the Plutus data from a witness set.
 *
 * This function retrieves the set of Plutus data associated with the specified \ref cardano_witness_set_t object.
 * Plutus data is used in Plutus scripts to carry additional information during script execution.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object from which the Plutus data is retrieved.
 *
 * \return A pointer to the \ref cardano_plutus_data_set_t object representing the Plutus data.
 *         The returned pointer is a new reference, and the caller is responsible for managing its lifecycle by calling
 *         \ref cardano_plutus_data_set_unref when it is no longer needed. If no Plutus data is set, this function returns \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_data_set_t* plutus_data = cardano_witness_set_get_plutus_data(witness_set);
 *
 * if (plutus_data != NULL)
 * {
 *   // Use the Plutus data
 *
 *   // Once done, ensure to clean up and release the Plutus data
 *   cardano_plutus_data_set_unref(&plutus_data);
 * }
 * else
 * {
 *   printf("No Plutus data found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_data_set_t* cardano_witness_set_get_plutus_data(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the Plutus data in a witness set.
 *
 * This function sets the Plutus data associated with the specified \ref cardano_witness_set_t object.
 * Plutus data is used in Plutus scripts to carry additional information during script execution.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object where the Plutus data will be set.
 * \param[in] plutus_data A pointer to an initialized \ref cardano_plutus_data_set_t object representing the Plutus data to be added.
 *                        This parameter can be \c NULL to remove the Plutus data from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the Plutus data
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p witness_set is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_plutus_data_set_t* plutus_data = ...; // Assume plutus_data is already initialized
 *
 * cardano_error_t result = cardano_witness_set_plutus_data(witness_set, plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus data set successfully in the witness set.\n");
 * }
 * else
 * {
 *   printf("Failed to set Plutus data: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources if necessary
 * cardano_plutus_data_set_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_plutus_data(
  cardano_witness_set_t*     witness_set,
  cardano_plutus_data_set_t* plutus_data);

/**
 * \brief Retrieves the list of redeemers from a witness set.
 *
 * This function fetches the redeemer list from the specified \ref cardano_witness_set_t object. Redeemers are used in Plutus scripts
 * to provide the data necessary for validating script execution. A transaction can have multiple redeemers, each associated with a different Plutus script.
 *
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object.
 *
 * \return A pointer to the \ref cardano_redeemer_list_t object representing the list of redeemers. The caller is responsible for managing
 *         the lifecycle of the returned list, including freeing it with \ref cardano_redeemer_list_unref when it is no longer needed.
 *         If the witness set does not contain any redeemers, the function returns \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
 *
 * if (redeemers != NULL)
 * {
 *   // Process the redeemers
 *
 *   // Ensure to release the redeemers once done
 *   cardano_redeemer_list_unref(&redeemers);
 * }
 * else
 * {
 *   printf("No redeemers found in the witness set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_redeemer_list_t* cardano_witness_set_get_redeemers(
  cardano_witness_set_t* witness_set);

/**
 * \brief Sets the list of redeemers in a witness set.
 *
 * This function assigns a list of redeemers to the specified \ref cardano_witness_set_t object. Redeemers are used
 * in Plutus scripts to provide the necessary data for script validation. A transaction may have multiple redeemers
 * associated with different Plutus scripts.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object.
 * \param[in] redeemers A pointer to an initialized \ref cardano_redeemer_list_t object representing the list of redeemers.
 *                      This can be set to \c NULL to remove the redeemers from the witness set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the redeemers
 *         were successfully set, or an appropriate error code such as \ref CARDANO_ERROR_POINTER_IS_NULL if any required pointer is NULL.
 *
 * \note The caller retains ownership of the \p redeemers object. The function increases the reference count
 *       for the provided redeemers. If \p redeemers is \c NULL, the redeemers field in the witness set will be cleared.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 * cardano_redeemer_list_t* redeemers = ...; // Assume redeemers is already initialized
 *
 * cardano_error_t result = cardano_witness_set_redeemers(witness_set, redeemers);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Redeemers set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set redeemers.\n");
 * }
 *
 * // If redeemers are no longer needed:
 * cardano_redeemer_list_unref(&redeemers);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_witness_set_set_redeemers(
  cardano_witness_set_t*   witness_set,
  cardano_redeemer_list_t* redeemers);

/**
 * \brief Clears the cached CBOR representation from a witness_set.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_witness_set_t object.
 * It is useful when you have modified the witness_set after it was created from CBOR using
 * \ref cardano_witness_set_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the witness_set, rather than using the original cached CBOR.
 *
 * \param[in,out] witness_set A pointer to an initialized \ref cardano_witness_set_t object
 *                         from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the witness_set when
 *          serialized, which can alter the witness_set and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume witness_set was created using cardano_witness_set_from_cbor
 * cardano_witness_set_t* witness_set = ...;
 *
 * // Modify the witness_set as needed
 *
 * // Clear the CBOR cache to ensure serialization uses the updated witness_set
 * cardano_witness_set_clear_cbor_cache(witness_set);
 *
 * // Serialize the witness_set to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_witness_set_to_cbor(witness_set, writer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the CBOR data as needed
 * }
 * else
 * {
 *   const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *   printf("Serialization failed: %s\n", error_message);
 * }
 *
 * // Clean up resources
 * cardano_cbor_writer_unref(&writer);
 * cardano_witness_set_unref(&witness_set);
 * \endcode
 */
CARDANO_EXPORT void cardano_witness_set_clear_cbor_cache(cardano_witness_set_t* witness_set);

/**
 * \brief Adds a script to the witness set of a transaction.
 *
 * This function incorporates a given script into the appropriate witness set within a transaction.
 *
 * \param[in,out] witness_set A pointer to the \ref cardano_witness_set_t to which the script will be added.
 * \param[in]     script      A pointer to the \ref cardano_script_t to be added to the witness set.
 *
 * \return \ref CARDANO_SUCCESS if the script was successfully added, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if `witness_set`
 *         or `script` is NULL, or \ref CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE if the script's language
 *         is unsupported.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = ...;
 * cardano_script_t* script = ...;
 *
 * cardano_error_t result = cardano_witness_add_script(witness_set, script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Script added to witness set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to add script to witness set: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_witness_set_unref(&witness_set);
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_witness_set_add_script(cardano_witness_set_t* witness_set, cardano_script_t* script);

/**
 * \brief Decrements the reference count of a cardano_witness_set_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_witness_set_t object
 * by decreasing its reference count. When the reference count reaches zero, the witness_set is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] witness_set A pointer to the pointer of the witness_set object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_witness_set_t* witness_set = cardano_witness_set_new(major, minor);
 *
 * // Perform operations with the witness_set...
 *
 * cardano_witness_set_unref(&witness_set);
 * // At this point, witness_set is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_witness_set_unref, the pointer to the \ref cardano_witness_set_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_witness_set_unref(cardano_witness_set_t** witness_set);

/**
 * \brief Increases the reference count of the cardano_witness_set_t object.
 *
 * This function is used to manually increment the reference count of an cardano_witness_set_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_witness_set_unref.
 *
 * \param witness_set A pointer to the cardano_witness_set_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming witness_set is a previously created witness_set object
 *
 * cardano_witness_set_ref(witness_set);
 *
 * // Now witness_set can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_witness_set_ref there is a corresponding
 * call to \ref cardano_witness_set_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_witness_set_ref(cardano_witness_set_t* witness_set);

/**
 * \brief Retrieves the current reference count of the cardano_witness_set_t object.
 *
 * This function returns the number of active references to an cardano_witness_set_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_witness_set_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param witness_set A pointer to the cardano_witness_set_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_witness_set_t object. If the object
 * is properly managed (i.e., every \ref cardano_witness_set_ref call is matched with a
 * \ref cardano_witness_set_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming witness_set is a previously created witness_set object
 *
 * size_t ref_count = cardano_witness_set_refcount(witness_set);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_witness_set_refcount(const cardano_witness_set_t* witness_set);

/**
 * \brief Sets the last error message for a given cardano_witness_set_t object.
 *
 * Records an error message in the witness_set's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] witness_set A pointer to the \ref cardano_witness_set_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the witness_set's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_witness_set_set_last_error(
  cardano_witness_set_t* witness_set,
  const char*            message);

/**
 * \brief Retrieves the last error message recorded for a specific witness_set.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_witness_set_set_last_error for the given
 * witness_set. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] witness_set A pointer to the \ref cardano_witness_set_t instance whose last error
 *                   message is to be retrieved. If the witness_set is NULL, the function
 *                   returns a generic error message indicating the null witness_set.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified witness_set. If the witness_set is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_witness_set_set_last_error for the same witness_set, or until
 *       the witness_set is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_witness_set_get_last_error(
  const cardano_witness_set_t* witness_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_WITNESS_SET_H