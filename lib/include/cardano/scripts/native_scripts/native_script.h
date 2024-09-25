/**
 * \file native_script.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_NATIVE_SCRIPT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_NATIVE_SCRIPT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/scripts/native_scripts/native_script_type.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
 *
 * Note that it is recursive. There are no constraints on the nesting or size, except that imposed by the overall
 * transaction size limit (given that the script must be included in the transaction in a script witnesses).
 */
typedef struct cardano_native_script_t cardano_native_script_t;

/**
 * \brief This script evaluates to true if all the sub-scripts evaluate to true.
 *
 * If the list of sub-scripts is empty, this script evaluates to true.
 */
typedef struct cardano_script_all_t cardano_script_all_t;

/**
 * \brief This script evaluates to true if any the sub-scripts evaluate to true. That is, if one
 * or more evaluate to true.
 *
 * If the list of sub-scripts is empty, this script evaluates to false.
 */
typedef struct cardano_script_any_t cardano_script_any_t;

/**
 * \brief This script evaluates to true if at least M (required field) of the sub-scripts evaluate to true.
 */
typedef struct cardano_script_n_of_k_t cardano_script_n_of_k_t;

/**
 * \brief This script evaluates to true if the transaction also includes a valid key witness
 * where the witness verification key hashes to the given hash.
 *
 * In other words, this checks that the transaction is signed by a particular key, identified by its verification
 * key hash.
 */
typedef struct cardano_script_pubkey_t cardano_script_pubkey_t;

/**
 * \brief This script evaluates to true if the upper bound of the transaction validity interval is a
 * slot number Y, and X <= Y.
 *
 * This condition guarantees that the actual slot number in which the transaction is included is
 * (strictly) less than slot number X.
 */
typedef struct cardano_script_invalid_after_t cardano_script_invalid_after_t;

/**
 * \brief This script evaluates to true if the lower bound of the transaction validity interval is a
 * slot number Y, and Y >= X.
 *
 * This condition guarantees that the actual slot number in which the transaction is included
 * is greater than or equal to slot number X.
 */
typedef struct cardano_script_invalid_before_t cardano_script_invalid_before_t;

/**
 * \brief Creates and initializes a new instance of a \ref cardano_native_script_t from a script_all.
 *
 * This function creates and initializes a new instance of a \ref cardano_native_script_t object
 * from a given \ref cardano_script_all_t object. It essentially converts a script_all
 * into a native_script. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script_all A constant pointer to the \ref cardano_script_all_t object from which
 *                       the native_script will be created. The object must not be NULL.
 * \param[out] native_script On successful initialization, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native_script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native_script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the native_script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_all_t* script_all = cardano_script_all_new(...);
 * cardano_native_script_t* native_script = NULL;
 *
 * // Attempt to create a new native_script from a script_all
 * cardano_error_t result = cardano_native_script_new_all(script_all, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the script_all object once done
 * cardano_script_all_unref(&script_all);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_new_all(
  cardano_script_all_t*     script_all,
  cardano_native_script_t** native_script);

/**
 * \brief Creates and initializes a new instance of a native_script from a script_any.
 *
 * This function creates and initializes a new instance of a \ref cardano_native_script_t object
 * from a given \ref cardano_script_any_t object. It essentially converts a script_any
 * into a native_script. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script_any A constant pointer to the \ref cardano_script_any_t object from which
 *                       the native_script will be created. The object must not be NULL.
 * \param[out] native_script On successful initialization, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native_script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native_script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the native_script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_any_t* script_any = cardano_script_any_new(...);
 * cardano_native_script_t* native_script = NULL;
 *
 * // Attempt to create a new native_script from a script_any
 * cardano_error_t result = cardano_native_script_new_any(script_any, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the script_any object once done
 * cardano_script_any_unref(&script_any);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_new_any(
  cardano_script_any_t*     script_any,
  cardano_native_script_t** native_script);

/**
 * \brief Creates and initializes a new instance of a native_script from a script_n_of_k.
 *
 * This function creates and initializes a new instance of a \ref cardano_native_script_t object
 * from a given \ref cardano_script_n_of_k_t object. It essentially converts a script_n_of_k
 * into a native_script. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script_n_of_k A constant pointer to the \ref cardano_script_n_of_k_t object from which
 *                        the native_script will be created. The object must not be NULL.
 * \param[out] native_script On successful initialization, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native_script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native_script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the native_script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_n_of_k_t* script_n_of_k = cardano_script_n_of_k_new(...);
 * cardano_native_script_t* native_script = NULL;
 *
 * // Attempt to create a new native_script from a script_n_of_k
 * cardano_error_t result = cardano_native_script_new_n_of_k(script_n_of_k, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the script_n_of_k object once done
 * cardano_script_n_of_k_unref(&script_n_of_k);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_new_n_of_k(
  cardano_script_n_of_k_t*  script_n_of_k,
  cardano_native_script_t** native_script);

/**
 * \brief Creates and initializes a new instance of a native_script from a script_pubkey.
 *
 * This function creates and initializes a new instance of a \ref cardano_native_script_t object
 * from a given \ref cardano_script_pubkey_t object. It essentially converts a script_pubkey
 * into a native_script. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] script_pubkey A constant pointer to the \ref cardano_script_pubkey_t object from which
 *                             the native_script will be created. The object must not be NULL.
 * \param[out] native_script On successful initialization, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native_script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native_script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the native_script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_pubkey_t* script_pubkey = cardano_script_pubkey_new(...);
 * cardano_native_script_t* native_script = NULL;
 *
 * // Attempt to create a new native_script from a script_pubkey
 * cardano_error_t result = cardano_native_script_new_pubkey(script_pubkey, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the script_pubkey object once done
 * cardano_script_pubkey_unref(&script_pubkey);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_new_pubkey(
  cardano_script_pubkey_t*  script_pubkey,
  cardano_native_script_t** native_script);

/**
 * \brief Creates and initializes a new instance of a native_script from a script_invalid_after.
 *
 * This function creates and initializes a new instance of a \ref cardano_native_script_t object
 * from a given \ref cardano_script_invalid_after_t object. It essentially converts a script_invalid_after
 * into a native_script. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] invalid_after A constant pointer to the \ref cardano_script_invalid_after_t object from which
 *                        the native_script will be created. The object must not be NULL.
 * \param[out] native_script On successful initialization, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native_script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native_script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the native_script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_invalid_after_t* invalid_after = cardano_script_invalid_after_new(...);
 * cardano_native_script_t* native_script = NULL;
 *
 * // Attempt to create a new native_script from a script_invalid_after
 * cardano_error_t result = cardano_native_script_new_invalid_after(invalid_after, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the invalid_after object once done
 * cardano_script_invalid_after_unref(&invalid_after);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_new_invalid_after(
  cardano_script_invalid_after_t* invalid_after,
  cardano_native_script_t**       native_script);

/**
 * \brief Creates and initializes a new instance of a native_script from a script_invalid_before.
 *
 * This function creates and initializes a new instance of a \ref cardano_native_script_t object
 * from a given \ref cardano_script_invalid_before_t object. It essentially converts a script_invalid_before
 * into a native_script. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] invalid_before A constant pointer to the \ref cardano_script_invalid_before_t object from which
 *                       the native_script will be created. The object must not be NULL.
 * \param[out] native_script On successful initialization, this will point to a newly created
 *                           \ref cardano_native_script_t object. This object represents a "strong reference"
 *                           to the native_script, meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the native_script is no longer needed, the caller must release it
 *                           by calling \ref cardano_native_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the native_script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_invalid_before_t* invalid_before = cardano_script_invalid_before_new(...);
 * cardano_native_script_t* native_script = NULL;
 *
 * // Attempt to create a new native_script from a script_invalid_before
 * cardano_error_t result = cardano_native_script_new_invalid_before(invalid_before, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 *
 * // Clean up the invalid_before object once done
 * cardano_script_invalid_before_unref(&invalid_before);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_new_invalid_before(
  cardano_script_invalid_before_t* invalid_before,
  cardano_native_script_t**        native_script);

/**
 * \brief Creates a native_script from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_native_script_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a native_script.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded native_script data.
 * \param[out] native_script A pointer to a pointer of \ref cardano_native_script_t that will be set to the address
 *                           of the newly created native_script object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the native_script was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_native_script_t object by calling
 *       \ref cardano_native_script_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_native_script_t* native_script = NULL;
 *
 * cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode native_script: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_from_cbor(cardano_cbor_reader_t* reader, cardano_native_script_t** native_script);

/**
 * \brief Serializes a native_script into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_native_script_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p native_script or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_native_script_to_cbor(native_script, writer);
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
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_cbor(
  const cardano_native_script_t* native_script,
  cardano_cbor_writer_t*         writer);

/**
 * \brief Creates a native_script from a JSON string.
 *
 * This function parses JSON data using a provided JSON string and constructs a \ref cardano_native_script_t object.
 * It assumes that the JSON data corresponds to the structure expected for a native_script.
 *
 * \param[in] json A pointer to a character array containing the JSON-encoded native_script data.
 * \param[in] json_size The size of the JSON data in bytes.
 * \param[out] native_script A pointer to a pointer of \ref cardano_native_script_t that will be set to the address
 *                           of the newly created native_script object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the native_script was successfully created, or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_data = "{...}"; // Assume this contains valid JSON-encoded native_script data
 * size_t data_size = strlen(json_data);
 * cardano_native_script_t* native_script = NULL;
 *
 * cardano_error_t result = cardano_native_script_from_json(json_data, data_size, &native_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the native_script
 *
 *   // Once done, ensure to clean up and release the native_script
 *   cardano_native_script_unref(&native_script);
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to decode native_script: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_native_script_from_json(const char* json, size_t json_size, cardano_native_script_t** native_script);

/**
 * \brief Retrieves the type of the native_script.
 *
 * This function retrieves the type of a given \ref cardano_native_script_t object and stores it in the provided
 * output parameter. The native script type is defined in the \ref cardano_native_script_type_t enumeration.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object from which
 *                          the type is to be retrieved. The object must not be NULL.
 * \param[out] type Pointer to a variable where the native_script type will be stored. This variable will
 *                  be set to the value from the \ref cardano_native_script_type_t enumeration.
 *
 * \return \ref CARDANO_SUCCESS if the type was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = cardano_native_script_new_pubkey(...);
 * cardano_native_script_type_t type;
 * cardano_error_t result = cardano_native_script_get_type(native_script, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (type == CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY)
 *   {
 *     // Handle require signature type native_script
 *   }
 *   else if (type == CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF)
 *   {
 *     // Handle require all of type native_script
 *   }
 *   // Add more cases as needed for other types
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_get_type(
  const cardano_native_script_t* native_script,
  cardano_native_script_type_t*  type);

/**
 * \brief Converts a native_script object to a script_all object.
 *
 * This function converts a \ref cardano_native_script_t object to a \ref cardano_script_all_t object.
 * It essentially creates a script_all object from the given native_script, allowing the conversion
 * between different types of native scripts. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object to be converted.
 * \param[out] script_all On successful conversion, this will point to a newly created
 *                        \ref cardano_script_all_t object. This object represents a "strong reference"
 *                        to the script_all, meaning that it is fully initialized and ready for use.
 *                        The caller is responsible for managing the lifecycle of this object.
 *                        Specifically, once the script_all is no longer needed, the caller must release it
 *                        by calling \ref cardano_script_all_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...; // Assume native_script is a valid native script object
 * cardano_script_all_t* script_all = NULL;
 * cardano_error_t result = cardano_native_script_to_all(native_script, &script_all);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_all
 *
 *   // Once done, ensure to clean up and release the script_all
 *   cardano_script_all_unref(&script_all);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_all(
  cardano_native_script_t* native_script,
  cardano_script_all_t**   script_all);

/**
 * \brief Converts a native_script object to a script_any object.
 *
 * This function converts a \ref cardano_native_script_t object to a \ref cardano_script_any_t object.
 * It essentially creates a script_any object from the given native_script, allowing the conversion
 * between different types of native scripts. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object to be converted.
 * \param[out] script_any On successful conversion, this will point to a newly created
 *                        \ref cardano_script_any_t object. This object represents a "strong reference"
 *                        to the script_any, meaning that it is fully initialized and ready for use.
 *                        The caller is responsible for managing the lifecycle of this object.
 *                        Specifically, once the script_any is no longer needed, the caller must release it
 *                        by calling \ref cardano_script_any_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...; // Assume native_script is a valid native script object
 * cardano_script_any_t* script_any = NULL;
 * cardano_error_t result = cardano_native_script_to_any(native_script, &script_any);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_any
 *
 *   // Once done, ensure to clean up and release the script_any
 *   cardano_script_any_unref(&script_any);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_any(
  cardano_native_script_t* native_script,
  cardano_script_any_t**   script_any);

/**
 * \brief Converts a native_script object to a script_n_of_k object.
 *
 * This function converts a \ref cardano_native_script_t object to a \ref cardano_script_n_of_k_t object.
 * It essentially creates a script_n_of_k object from the given native_script, allowing the conversion
 * between different types of native scripts. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object to be converted.
 * \param[out] script_n_of_k On successful conversion, this will point to a newly created
 *                         \ref cardano_script_n_of_k_t object. This object represents a "strong reference"
 *                         to the script_n_of_k, meaning that it is fully initialized and ready for use.
 *                         The caller is responsible for managing the lifecycle of this object.
 *                         Specifically, once the script_n_of_k is no longer needed, the caller must release it
 *                         by calling \ref cardano_script_n_of_k_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...; // Assume native_script is a valid native script object
 * cardano_script_n_of_k_t* script_n_of_k = NULL;
 * cardano_error_t result = cardano_native_script_to_n_of_k(native_script, &script_n_of_k);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_n_of_k
 *
 *   // Once done, ensure to clean up and release the script_n_of_k
 *   cardano_script_n_of_k_unref(&script_n_of_k);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_n_of_k(
  cardano_native_script_t*  native_script,
  cardano_script_n_of_k_t** script_n_of_k);

/**
 * \brief Converts a native_script object to a script_pubkey object.
 *
 * This function converts a \ref cardano_native_script_t object to a \ref cardano_script_pubkey_t object.
 * It essentially creates a script_pubkey object from the given native_script, allowing the conversion
 * between different types of native scripts. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object to be converted.
 * \param[out] script_pubkey On successful conversion, this will point to a newly created
 *                                  \ref cardano_script_pubkey_t object. This object represents a "strong reference"
 *                                  to the script_pubkey, meaning that it is fully initialized and ready for use.
 *                                  The caller is responsible for managing the lifecycle of this object.
 *                                  Specifically, once the script_pubkey is no longer needed, the caller must release it
 *                                  by calling \ref cardano_script_pubkey_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...; // Assume native_script is a valid native script object
 * cardano_script_pubkey_t* script_pubkey = NULL;
 * cardano_error_t result = cardano_native_script_to_pubkey(native_script, &script_pubkey);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_pubkey
 *
 *   // Once done, ensure to clean up and release the script_pubkey
 *   cardano_script_pubkey_unref(&script_pubkey);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_pubkey(
  cardano_native_script_t*  native_script,
  cardano_script_pubkey_t** script_pubkey);

/**
 * \brief Converts a native_script object to a script_invalid_after object.
 *
 * This function converts a \ref cardano_native_script_t object to a \ref cardano_script_invalid_after_t object.
 * It essentially creates a script_invalid_after object from the given native_script, allowing the conversion
 * between different types of native scripts. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object to be converted.
 * \param[out] invalid_after On successful conversion, this will point to a newly created
 *                         \ref cardano_script_invalid_after_t object. This object represents a "strong reference"
 *                         to the invalid_after, meaning that it is fully initialized and ready for use.
 *                         The caller is responsible for managing the lifecycle of this object.
 *                         Specifically, once the invalid_after is no longer needed, the caller must release it
 *                         by calling \ref cardano_script_invalid_after_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...; // Assume native_script is a valid native script object
 * cardano_script_invalid_after_t* invalid_after = NULL;
 * cardano_error_t result = cardano_native_script_to_invalid_after(native_script, &invalid_after);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the invalid_after
 *
 *   // Once done, ensure to clean up and release the invalid_after
 *   cardano_script_invalid_after_unref(&invalid_after);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_invalid_after(
  cardano_native_script_t*         native_script,
  cardano_script_invalid_after_t** invalid_after);

/**
 * \brief Converts a native_script object to a script_invalid_before object.
 *
 * This function converts a \ref cardano_native_script_t object to a \ref cardano_script_invalid_before_t object.
 * It essentially creates a script_invalid_before object from the given native_script, allowing the conversion
 * between different types of native scripts. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] native_script A constant pointer to the \ref cardano_native_script_t object to be converted.
 * \param[out] invalid_before On successful conversion, this will point to a newly created
 *                        \ref cardano_script_invalid_before_t object. This object represents a "strong reference"
 *                        to the invalid_before, meaning that it is fully initialized and ready for use.
 *                        The caller is responsible for managing the lifecycle of this object.
 *                        Specifically, once the invalid_before is no longer needed, the caller must release it
 *                        by calling \ref cardano_script_invalid_before_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = ...; // Assume native_script is a valid native script object
 * cardano_script_invalid_before_t* invalid_before = NULL;
 * cardano_error_t result = cardano_native_script_to_invalid_before(native_script, &invalid_before);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the invalid_before
 *
 *   // Once done, ensure to clean up and release the invalid_before
 *   cardano_script_invalid_before_unref(&invalid_before);
 * }
 *
 * // Clean up the native_script object once done
 * cardano_native_script_unref(&native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_native_script_to_invalid_before(
  cardano_native_script_t*          native_script,
  cardano_script_invalid_before_t** invalid_before);

/**
 * \brief Retrieves the hash associated with a native_script.
 *
 * This computes and returns the hash of a \ref cardano_native_script_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 *
 * \param native_script A constant pointer to the \ref cardano_native_script_t object from which
 *                      the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input native_script is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* original_native_script = cardano_native_script_new(...);
 * cardano_blake2b_hash_t* hash_native_script = cardano_native_script_get_hash(original_native_script);
 *
 * if (hash_native_script)
 * {
 *   // Use the hash native_script
 *
 *   // Once done, ensure to clean up and release the hash native_script
 *   cardano_blake2b_hash_unref(&hash_native_script);
 * }
 * // Release the original native_script after use
 * cardano_native_script_unref(&original_native_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_native_script_get_hash(
  const cardano_native_script_t* native_script);

/**
 * \brief Checks if two native_script objects are equal.
 *
 * This function compares two \ref cardano_native_script_t objects for equality.
 * It checks if the contents of the two native_script objects are identical.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_native_script_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_native_script_t object to be compared.
 *
 * \return \c true if the two native_script objects are equal (have the same contents), \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script1 = cardano_native_script_new(...);
 * cardano_native_script_t* native_script2 = cardano_native_script_new(...);
 *
 * if (cardano_native_script_equals(native_script1, native_script2))
 * {
 *   printf("native_script1 is equal to native_script2\n");
 * }
 * else
 * {
 *   printf("native_script1 is not equal to native_script2\n");
 * }
 *
 * // Clean up the native_script objects once done
 * cardano_native_script_unref(&native_script1);
 * cardano_native_script_unref(&native_script2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_native_script_equals(
  const cardano_native_script_t* lhs,
  const cardano_native_script_t* rhs);

/**
 * \brief Decrements the reference count of a native_script object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_native_script_t object
 * by decreasing its reference count. When the reference count reaches zero, the native_script is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] native_script A pointer to the pointer of the native_script object. This double
 *                              indirection allows the function to set the caller's pointer to
 *                              NULL, avoiding dangling pointer issues after the object has been
 *                              freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_t* native_script = cardano_native_script_new();
 *
 * // Perform operations with the native_script...
 *
 * cardano_native_script_unref(&native_script);
 * // At this point, native_script is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_native_script_unref, the pointer to the \ref cardano_native_script_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_native_script_unref(cardano_native_script_t** native_script);

/**
 * \brief Increases the reference count of the cardano_native_script_t object.
 *
 * This function is used to manually increment the reference count of a native_script
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_native_script_unref.
 *
 * \param native_script A pointer to the native_script object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming native_script is a previously created native_script object
 *
 * cardano_native_script_ref(native_script);
 *
 * // Now native_script can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_native_script_ref there is a corresponding
 * call to \ref cardano_native_script_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_native_script_ref(cardano_native_script_t* native_script);

/**
 * \brief Retrieves the current reference count of the cardano_native_script_t object.
 *
 * This function returns the number of active references to a native_script object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_native_script_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param native_script A pointer to the native_script object whose reference count is queried.
 *                      The object must not be NULL.
 *
 * \return The number of active references to the specified native_script object. If the object
 * is properly managed (i.e., every \ref cardano_native_script_ref call is matched with a
 * \ref cardano_native_script_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming native_script is a previously created native_script object
 *
 * size_t ref_count = cardano_native_script_refcount(native_script);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_native_script_refcount(const cardano_native_script_t* native_script);

/**
 * \brief Sets the last error message for a given native_script object.
 *
 * Records an error message in the native_script's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] native_script A pointer to the \ref cardano_native_script_t instance whose last error message is
 *                          to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the native_script's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 *       fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_native_script_set_last_error(
  cardano_native_script_t* native_script,
  const char*              message);

/**
 * \brief Retrieves the last error message recorded for a specific native_script.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_native_script_set_last_error for the given
 * native_script. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] native_script A pointer to the \ref cardano_native_script_t instance whose last error
 *                          message is to be retrieved. If the native_script is NULL, the function
 *                          returns a generic error message indicating the null native_script.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified native_script. If the native_script is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_native_script_set_last_error for the same native_script, or until
 *       the native_script is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_native_script_get_last_error(
  const cardano_native_script_t* native_script);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_NATIVE_SCRIPT_H