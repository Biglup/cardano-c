/**
 * \file script.h
 *
 * \author angel.castillo
 * \date   May 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/scripts/script_language.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Program that decides whether the transaction that spends the output is authorized to do so.
 */
typedef struct cardano_script_t cardano_script_t;

/**
 * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
 *
 * Note that it is recursive. There are no constraints on the nesting or size, except that imposed by the overall
 * transaction size limit (given that the script must be included in the transaction in a script witnesses).
 */
typedef struct cardano_native_script_t cardano_native_script_t;

/**
 * \brief V1 was the initial version of Plutus, introduced in the Alonzo hard fork.
 */
typedef struct cardano_plutus_v1_script_t cardano_plutus_v1_script_t;

/**
 * \brief V2 was introduced in the Vasil hard fork.
 *
 * The main changes in V2 of Plutus were to the interface to scripts. The ScriptContext was extended
 * to include the following information:
 *
 *  - The full “redeemers” structure, which contains all the redeemers used in the transaction
 *  - Reference inputs in the transaction (proposed in CIP-31)
 *  - Inline datums in the transaction (proposed in CIP-32)
 *  - Reference scripts in the transaction (proposed in CIP-33)
 */
typedef struct cardano_plutus_v2_script_t cardano_plutus_v2_script_t;

/**
 * \brief V3 was introduced in the Conway hard fork.
 *
 * The main changes in V3 of Plutus were to the interface to scripts. The ScriptContext was extended
 * to include the following information:
 *
 *  - A Map with all the votes that were included in the transaction.
 *  - A list with Proposals that will be turned into GovernanceActions, that everyone can vote on
 *  - Optional amount for the current treasury. If included it will be checked to be equal the current amount in the treasury.
 *  - Optional amount for donating to the current treasury. If included, specified amount will go into the treasury.
 */
typedef struct cardano_plutus_v3_script_t cardano_plutus_v3_script_t;

/**
 * \brief Creates and initializes a new instance of a script from a native_script.
 *
 * This function creates and initializes a new instance of a \ref cardano_script_t object
 * from a given \ref cardano_native_script_t object. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object from which
 *                          the script will be created. The object must not be NULL.
 * \param[out] script On successful initialization, this will point to a newly created
 *             \ref cardano_script_t object. This object represents a "strong reference"
 *             to the script, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script is no longer needed, the caller must release it
 *             by calling \ref cardano_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = cardano_native_script_new(...);
 * cardano_script_t* script = NULL;
 *
 * // Attempt to create a new script from a native_script
 * cardano_error_t result = cardano_script_new_native(native_script, &script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script
 *
 *   // Once done, ensure to clean up and release the script
 *   cardano_script_unref(&script);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_new_native(
  cardano_native_script_t* native_script,
  cardano_script_t**       script);

/**
 * \brief Creates and initializes a new instance of a script from a Plutus V1 script.
 *
 * This function creates and initializes a new instance of a \ref cardano_script_t object
 * from a given \ref cardano_plutus_v1_script_t object. It returns an error code to indicate
 * the success or failure of the operation.
 *
 * \param[in] plutus_v1_script A constant pointer to the \ref cardano_plutus_v1_script_t object from which
 *                             the script will be created. The object must not be NULL.
 * \param[out] script On successful initialization, this will point to a newly created
 *             \ref cardano_script_t object. This object represents a "strong reference"
 *             to the script, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script is no longer needed, the caller must release it
 *             by calling \ref cardano_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v1_script_t* plutus_v1_script = cardano_plutus_v1_script_new(...);
 * cardano_script_t* script = NULL;
 *
 * // Attempt to create a new script from a Plutus V1 script
 * cardano_error_t result = cardano_script_new_plutus_v1(plutus_v1_script, &script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script
 *
 *   // Once done, ensure to clean up and release the script
 *   cardano_script_unref(&script);
 * }
 *
 * // Clean up the plutus_v1_script object once done
 * cardano_plutus_v1_script_unref(&plutus_v1_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_new_plutus_v1(
  cardano_plutus_v1_script_t* plutus_v1_script,
  cardano_script_t**          script);

/**
 * \brief Creates and initializes a new instance of a script from a Plutus V2 script.
 *
 * This function creates and initializes a new instance of a \ref cardano_script_t object
 * from a given \ref cardano_plutus_v2_script_t object. It returns an error code to indicate
 * the success or failure of the operation.
 *
 * \param[in] plutus_v2_script A constant pointer to the \ref cardano_plutus_v2_script_t object from which
 *                             the script will be created. The object must not be NULL.
 * \param[out] script On successful initialization, this will point to a newly created
 *             \ref cardano_script_t object. This object represents a "strong reference"
 *             to the script, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script is no longer needed, the caller must release it
 *             by calling \ref cardano_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_t* plutus_v2_script = cardano_plutus_v2_script_new(...);
 * cardano_script_t* script = NULL;
 *
 * // Attempt to create a new script from a Plutus V2 script
 * cardano_error_t result = cardano_script_new_plutus_v2(plutus_v2_script, &script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script
 *
 *   // Once done, ensure to clean up and release the script
 *   cardano_script_unref(&script);
 * }
 *
 * // Clean up the plutus_v2_script object once done
 * cardano_plutus_v2_script_unref(&plutus_v2_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_new_plutus_v2(
  cardano_plutus_v2_script_t* plutus_v2_script,
  cardano_script_t**          script);

/**
 * \brief Creates and initializes a new instance of a script from a Plutus V3 script.
 *
 * This function creates and initializes a new instance of a \ref cardano_script_t object
 * from a given \ref cardano_plutus_v3_script_t object. It returns an error code to indicate
 * the success or failure of the operation.
 *
 * \param[in] plutus_v3_script A constant pointer to the \ref cardano_plutus_v3_script_t object from which
 *                             the script will be created. The object must not be NULL.
 * \param[out] script On successful initialization, this will point to a newly created
 *             \ref cardano_script_t object. This object represents a "strong reference"
 *             to the script, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script is no longer needed, the caller must release it
 *             by calling \ref cardano_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v3_script_t* plutus_v3_script = cardano_plutus_v3_script_new(...);
 * cardano_script_t* script = NULL;
 *
 * // Attempt to create a new script from a Plutus V3 script
 * cardano_error_t result = cardano_script_new_plutus_v3(plutus_v3_script, &script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script
 *
 *   // Once done, ensure to clean up and release the script
 *   cardano_script_unref(&script);
 * }
 *
 * // Clean up the plutus_v3_script object once done
 * cardano_plutus_v3_script_unref(&plutus_v3_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_new_plutus_v3(
  cardano_plutus_v3_script_t* plutus_v3_script,
  cardano_script_t**          script);

/**
 * \brief Creates a script from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_script_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a script.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded script data.
 * \param[out] script A pointer to a pointer of \ref cardano_script_t that will be set to the address
 *                        of the newly created script object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_script_t object by calling
 *       \ref cardano_script_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_script_t* script = NULL;
 *
 * cardano_error_t result = cardano_script_from_cbor(reader, &script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script
 *
 *   // Once done, ensure to clean up and release the script
 *   cardano_script_unref(&script);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode script: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_from_cbor(cardano_cbor_reader_t* reader, cardano_script_t** script);

/**
 * \brief Serializes a script into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_script_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] script A constant pointer to the \ref cardano_script_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p script or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_script_to_cbor(script, writer);
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
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_to_cbor(
  const cardano_script_t* script,
  cardano_cbor_writer_t*  writer);

/**
 * \brief Retrieves the language of the script.
 *
 * This function retrieves the language of a given \ref cardano_script_t object and stores it in the provided
 * output parameter. The script language is defined in the \ref cardano_script_language_t enumeration.
 *
 * \param[in] script A constant pointer to the \ref cardano_script_t object from which
 *                        the language is to be retrieved. The object must not be NULL.
 * \param[out] language Pointer to a variable where the script language will be stored. This variable will
 *                      be set to the value from the \ref cardano_script_language_t enumeration.
 *
 * \return \ref CARDANO_SUCCESS if the language was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = cardano_script_new_bytes_from_hex(...);
 * cardano_script_language_t language;
 * cardano_error_t result = cardano_script_get_language(script, &language);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (language == CARDANO_SCRIPT_LANGUAGE_NATIVE)
 *   {
 *     // Handle native script
 *   }
 *   else if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1)
 *   {
 *     // Handle Plutus V1 script
 *   }
 *   // Add more cases as needed for other languages
 * }
 *
 * // Clean up the script object once done
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_get_language(
  const cardano_script_t*    script,
  cardano_script_language_t* language);

/**
 * \brief Converts a script object to a native script object.
 *
 * This function converts a \ref cardano_script_t object to a \ref cardano_native_script_t object.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script A constant pointer to the \ref cardano_script_t object to be converted.
 * \param[out] native_script On successful conversion, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = ...; // Assume script is a valid script object
 * cardano_native_script_t* native_script = NULL;
 * cardano_error_t result = cardano_script_to_native(script, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the script object once done
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_to_native(
  cardano_script_t*         script,
  cardano_native_script_t** native_script);

/**
 * \brief Converts a script object to a Plutus V1 script object.
 *
 * This function converts a \ref cardano_script_t object to a \ref cardano_plutus_v1_script_t object.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script A constant pointer to the \ref cardano_script_t object to be converted.
 * \param[out] plutus_v1 On successful conversion, this will point to a newly created
 *                       \ref cardano_plutus_v1_script_t object. This object represents a "strong reference"
 *                       to the Plutus V1 script, meaning that it is fully initialized and ready for use.
 *                       The caller is responsible for managing the lifecycle of this object.
 *                       Specifically, once the Plutus V1 script is no longer needed, the caller must release it
 *                       by calling \ref cardano_plutus_v1_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = ...; // Assume script is a valid script object
 * cardano_plutus_v1_script_t* plutus_v1 = NULL;
 * cardano_error_t result = cardano_script_to_plutus_v1(script, &plutus_v1);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the Plutus V1 script
 *
 *   // Once done, ensure to clean up and release the Plutus V1 script
 *   cardano_plutus_v1_script_unref(&plutus_v1);
 * }
 *
 * // Clean up the script object once done
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_to_plutus_v1(
  cardano_script_t*            script,
  cardano_plutus_v1_script_t** plutus_v1);

/**
 * \brief Converts a script object to a Plutus V2 script object.
 *
 * This function converts a \ref cardano_script_t object to a \ref cardano_plutus_v2_script_t object.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script A constant pointer to the \ref cardano_script_t object to be converted.
 * \param[out] plutus_v2 On successful conversion, this will point to a newly created
 *                       \ref cardano_plutus_v2_script_t object. This object represents a "strong reference"
 *                       to the Plutus V2 script, meaning that it is fully initialized and ready for use.
 *                       The caller is responsible for managing the lifecycle of this object.
 *                       Specifically, once the Plutus V2 script is no longer needed, the caller must release it
 *                       by calling \ref cardano_plutus_v2_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = ...; // Assume script is a valid script object
 * cardano_plutus_v2_script_t* plutus_v2 = NULL;
 * cardano_error_t result = cardano_script_to_plutus_v2(script, &plutus_v2);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the Plutus V2 script
 *
 *   // Once done, ensure to clean up and release the Plutus V2 script
 *   cardano_plutus_v2_script_unref(&plutus_v2);
 * }
 *
 * // Clean up the script object once done
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_to_plutus_v2(
  cardano_script_t*            script,
  cardano_plutus_v2_script_t** plutus_v2);

/**
 * \brief Converts a script object to a Plutus V3 script object.
 *
 * This function converts a \ref cardano_script_t object to a \ref cardano_plutus_v3_script_t object.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script A constant pointer to the \ref cardano_script_t object to be converted.
 * \param[out] plutus_v3 On successful conversion, this will point to a newly created
 *                       \ref cardano_plutus_v3_script_t object. This object represents a "strong reference"
 *                       to the Plutus V3 script, meaning that it is fully initialized and ready for use.
 *                       The caller is responsible for managing the lifecycle of this object.
 *                       Specifically, once the Plutus V3 script is no longer needed, the caller must release it
 *                       by calling \ref cardano_plutus_v3_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = ...; // Assume script is a valid script object
 * cardano_plutus_v3_script_t* plutus_v3 = NULL;
 * cardano_error_t result = cardano_script_to_plutus_v3(script, &plutus_v3);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the Plutus V3 script
 *
 *   // Once done, ensure to clean up and release the Plutus V3 script
 *   cardano_plutus_v3_script_unref(&plutus_v3);
 * }
 *
 * // Clean up the script object once done
 * cardano_script_unref(&script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_to_plutus_v3(
  cardano_script_t*            script,
  cardano_plutus_v3_script_t** plutus_v3);

/**
 * \brief Retrieves the hash associated with a script.
 *
 * This function computes the hash of a \ref cardano_script_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 * It the caller's responsibility to release it by calling \ref cardano_blake2b_hash_unref when
 * it is no longer needed.
 *
 * \param script A constant pointer to the \ref cardano_script_t object from which
 *                   the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input script is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* original_script = cardano_script_new(...);
 * cardano_blake2b_hash_t* hash_script = cardano_script_get_hash(original_script);
 *
 * if (hash_script)
 * {
 *   // Use the hash script
 *
 *   // Once done, ensure to clean up and release the hash script
 *   cardano_blake2b_hash_unref(&hash_script);
 * }
 * // Release the original script after use
 * cardano_script_unref(&original_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_script_get_hash(const cardano_script_t* script);

/**
 * \brief Checks if two script objects are equal.
 *
 * This function compares two \ref cardano_script_t objects for equality.
 * It checks if the contents of the two script objects are identical.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_script_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_script_t object to be compared.
 *
 * \return \c true if the two script objects are equal (have the same contents), \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script1 = ...; // Assume script1 is initialized
 * cardano_script_t* script2 = ...; // Assume script2 is initialized
 *
 * if (cardano_script_equals(script1, script2))
 * {
 *   printf("script1 is equal to script2\n");
 * }
 * else
 * {
 *   printf("script1 is not equal to script2\n");
 * }
 *
 * // Clean up the script objects once done
 * cardano_script_unref(&script1);
 * cardano_script_unref(&script2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_script_equals(const cardano_script_t* lhs, const cardano_script_t* rhs);

/**
 * \brief Decrements the reference count of a script object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_script_t object
 * by decreasing its reference count. When the reference count reaches zero, the script is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] script A pointer to the pointer of the script object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = cardano_script_new();
 *
 * // Perform operations with the script...
 *
 * cardano_script_unref(&script);
 * // At this point, script is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_script_unref, the pointer to the \ref cardano_script_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_script_unref(cardano_script_t** script);

/**
 * \brief Increases the reference count of the cardano_script_t object.
 *
 * This function is used to manually increment the reference count of a script
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_script_unref.
 *
 * \param script A pointer to the script object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script is a previously created script object
 *
 * cardano_script_ref(script);
 *
 * // Now script can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_script_ref there is a corresponding
 * call to \ref cardano_script_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_script_ref(cardano_script_t* script);

/**
 * \brief Retrieves the current reference count of the cardano_script_t object.
 *
 * This function returns the number of active references to a script object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_script_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param script A pointer to the script object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified script object. If the object
 * is properly managed (i.e., every \ref cardano_script_ref call is matched with a
 * \ref cardano_script_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script is a previously created script object
 *
 * size_t ref_count = cardano_script_refcount(script);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_script_refcount(const cardano_script_t* script);

/**
 * \brief Sets the last error message for a given script object.
 *
 * Records an error message in the script's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] script A pointer to the \ref cardano_script_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the script's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_script_set_last_error(cardano_script_t* script, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific script.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_script_set_last_error for the given
 * script. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] script A pointer to the \ref cardano_script_t instance whose last error
 *                   message is to be retrieved. If the script is NULL, the function
 *                   returns a generic error message indicating the null script.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified script. If the script is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_script_set_last_error for the same script, or until
 *       the script is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_script_get_last_error(const cardano_script_t* script);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_H