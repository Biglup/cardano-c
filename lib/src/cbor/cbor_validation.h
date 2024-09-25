/**
 * \file cbor_validation.h
 *
 * \author angel.castillo
 * \date   Apr 13, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_VALIDATION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_VALIDATION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/common/credential_type.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* TYPES *********************************************************************/

typedef const char* (*enum_to_string_callback_t)(uint64_t value);

/* DECLARATIONS **************************************************************/

/**
 * \brief Validates that the next element in the CBOR stream is an array with exactly \p n elements.
 *
 * This function checks whether the next element to be read from the CBOR stream using the provided \p reader
 * is an array containing exactly \p n elements.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context
 *                           or field name where this array is expected, which will be included in any
 *                           error messages generated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be
 *                       advanced if the validation passes.
 * \param[in] n The expected number of elements in the array. The function checks for this exact count.
 *
 * \return \ref CARDANO_SUCCESS if the array contains exactly \p n elements. If the array contains a different
 *         number of elements, or if the next element is not an array, appropriate error codes are returned,
 *         such as \ref CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE or \ref CARDANO_ERROR_UNEXPECTED_CBOR_TYPE. If \p reader is NULL,
 *         \ref CARDANO_ERROR_POINTER_IS_NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * cardano_error_t result = cardano_cbor_validate_array_of_n_elements("field_name", reader, 2);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle error
 *   fprintf(stderr, "Invalid CBOR format for field_name: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 *
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_array_of_n_elements(const char* validator_name, cardano_cbor_reader_t* reader, uint32_t n);

/**
 * \brief Validates that the next element in the CBOR stream is an unsigned integer within a specified range.
 *
 * This function checks whether the next element to be read from the CBOR stream using the provided \p reader
 * is an unsigned integer and whether it falls within the inclusive range specified by \p min and \p max.
 * This check is crucial for ensuring that data conforms to expected value constraints, especially when specific
 * numerical ranges carry particular meanings or are required by the application logic.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context or field
 *                           name where this integer is expected, which will be included in any error messages generated.
 * \param[in] type_name A descriptive name for the type being validated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read and interpret
 *                       the CBOR encoded data stream. The reader's state will be advanced if the validation passes.
 * \param[out] type Pointer to \ref cardano_credential_type_t where the parsed and validated unsigned integer
 *                   will be stored if validation succeeds. This parameter is ignored if validation fails.
 * \param[in] min The minimum acceptable value for the unsigned integer.
 * \param[in] max The maximum acceptable value for the unsigned integer.
 *
 * \return \ref CARDANO_SUCCESS if the next element is an unsigned integer and it falls within the specified range.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * cardano_credential_type_t credential_type;
 * cardano_error_t result = cardano_cbor_validate_uint_in_range("field_name", "credential_type", reader, &credential_type, 0, 4);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the validated credential type
 *   printf("Credential type: %u\n", credential_type);
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Invalid CBOR format for field_name: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_uint_in_range(const char* validator_name, const char* type_name, cardano_cbor_reader_t* reader, uint64_t* type, uint64_t min, uint64_t max);

/**
 * \brief Validates and reads a byte string of a specified size from a CBOR stream.
 *
 * This function checks if the next element in the CBOR stream, read using the provided \p reader,
 * is a byte string of the exact length specified by \p size. If the validation succeeds, the function
 * reads this byte string and returns it via the \p byte_string parameter.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context or
 *                           the field name where this byte string is expected, which will be included in
 *                           any error messages generated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be advanced
 *                       if the byte string is successfully read.
 * \param[out] byte_string A pointer to a pointer of \ref cardano_buffer_t that will be allocated and
 *                         set to point to the new byte string if validation and reading succeed. The caller
 *                         is responsible for freeing this memory using \ref cardano_buffer_unref when it
 *                         is no longer needed.
 * \param[in] size The exact expected size of the byte string in bytes.
 *
 * \return \ref CARDANO_SUCCESS if the next element is a byte string and its size matches the specified \p size.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * cardano_buffer_t* byte_string = NULL;
 * cardano_error_t result = cardano_cbor_validate_byte_string_of_size("field_name", reader, &byte_string, expected_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the byte string
 *   // Process or inspect byte_string contents
 *
 *   cardano_buffer_unref(&byte_string);
 * }
 * else
 * {
 *   // Handle error
 *    fprintf(stderr, "Invalid CBOR format for field_name: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 *
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_byte_string_of_size(const char* validator_name, cardano_cbor_reader_t* reader, cardano_buffer_t** byte_string, uint32_t size);

/**
 * \brief Validates and reads a text string of a specified maximum size from a CBOR stream.
 *
 * This function checks if the next element in the CBOR stream, accessed through the provided \p reader,
 * is a text string whose length does not exceed the specified \p size. If the validation succeeds, the function
 * reads this text string and writes it to the \p text_string parameter, ensuring not to exceed the buffer capacity.
 *
 * \param[in] validator_name A descriptive name for the validator, indicating the context or
 *                           the field name where this text string is expected, which will be included in
 *                           any error messages generated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be advanced
 *                       if the text string is successfully read.
 * \param[out] text_string A pointer to a character array where the read text string will be stored.
 *                         The buffer must be large enough to hold the text string plus a terminating null character.
 * \param[in] size The maximum expected size of the text string in bytes, including the null terminator.
 *
 * \return \ref CARDANO_SUCCESS if the next element is a text string and its length is within the specified \p size.
 *         Returns an appropriate error code if the operation fails, which can be obtained using
 *         \ref cardano_cbor_reader_get_last_error for more details.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * char text_string[100];  // Define the buffer size based on the expected maximum
 * cardano_error_t result = cardano_cbor_validate_text_string_of_max_size("field_name", reader, text_string, sizeof(text_string));
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the text string
 *   printf("Read text string: %s\n", text_string);
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Invalid CBOR format for field_name: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_text_string_of_max_size(const char* validator_name, cardano_cbor_reader_t* reader, char* text_string, uint32_t size);

/**
 * \brief Validates the end of an array in a CBOR stream.
 *
 * This function checks if the current position in the CBOR stream, accessed through the provided
 * \p reader, correctly signifies the end of an array. It is typically used after reading all expected
 * elements of an array to ensure that there are no additional, unexpected elements in the array.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context or
 *                           the field name associated with the array, which will be included in any
 *                           error messages generated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be checked
 *                       to ensure it is at the end of an array structure.
 *
 * \return \ref CARDANO_SUCCESS if the reader is correctly positioned at the end of an array.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * // Assume we have read some elements from the array
 * cardano_error_t result = cardano_cbor_validate_end_array("field_name", reader);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Array was properly closed
 *   printf("Array reading completed successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Invalid CBOR format for field_name: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_end_array(const char* validator_name, cardano_cbor_reader_t* reader);

/**
 * \brief Validates the end of an map in a CBOR stream.
 *
 * This function checks if the current position in the CBOR stream, accessed through the provided
 * \p reader, correctly signifies the end of an map. It is typically used after reading all expected
 * elements of an map to ensure that there are no additional, unexpected elements in the map.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context or
 *                           the field name associated with the map, which will be included in any
 *                           error messages generated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be checked
 *                       to ensure it is at the end of an map structure.
 *
 * \return \ref CARDANO_SUCCESS if the reader is correctly positioned at the end of an map.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * // Assume we have read some elements from the map
 * cardano_error_t result = cardano_cbor_validate_end_map("field_name", reader);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Array was properly closed
 *   printf("Array reading completed successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Invalid CBOR format for field_name: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_end_map(const char* validator_name, cardano_cbor_reader_t* reader);

/**
 * \brief Validates a CBOR tag in a CBOR stream.
 *
 * This function validates the presence of a specific CBOR tag at the current position in the CBOR stream,
 * accessed through the provided \p reader. It is typically used to ensure that the expected CBOR tag is present
 * at the specified location in the CBOR data.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context or
 *                           the field name associated with the tag, which will be included in any
 *                           error messages generated.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be checked
 *                       to ensure it contains the specified CBOR tag.
 * \param[in] tag The CBOR tag to validate at the current position in the CBOR stream.
 *
 * \return \ref cardano_error_t indicating the outcome of the validation operation. Returns \ref CARDANO_SUCCESS
 *         if the specified CBOR tag is found at the current position in the CBOR stream, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, data_size);
 * // Assume we have read some data from the CBOR stream
 * cardano_error_t result = cardano_cbor_validate_tag("tag_validator", reader, MY_CBOR_TAG);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // CBOR tag was found at the current position
 *   printf("CBOR tag validation successful.\n");
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "CBOR tag validation failed for tag_validator: %s.\n", cardano_cbor_reader_get_last_error(reader));
 * }
 * \endcode
 */
cardano_error_t
cardano_cbor_validate_tag(const char* validator_name, cardano_cbor_reader_t* reader, cardano_cbor_tag_t tag);

/**
 * \brief Validates an enum value in a CBOR stream.
 *
 * This function validates that the actual enum value read from the CBOR stream matches the expected value.
 * It provides detailed error messages, including both friendly names and the actual values.
 *
 * \param[in] validator_name A descriptive name for the validator, typically indicating the context or
 *                           the field name associated with the value, which will be included in any
 *                           error messages generated.
 * \param[in] field_name A descriptive name for the field.
 * \param[in,out] reader The \ref cardano_cbor_reader_t object that provides the interface to read
 *                       and interpret the CBOR encoded data stream. The reader's state will be checked
 *                       to ensure it contains the expected enum value.
 * \param[in] expected_value The expected enum value to validate against.
 * \param[in] enum_to_string_callback A callback function that converts an enum value to a friendly string representation.
 * \param[out] actual_value Pointer to the unsigned int where the parsed and validated unsigned integer
 *                  will be stored if validation succeeds. This parameter is ignored if validation fails.
 *
 * \return \ref cardano_error_t indicating the outcome of the validation operation. Returns \ref CARDANO_SUCCESS
 *         if the actual enum value matches the expected value, or an appropriate error code
 *         indicating the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_cbor_validate_enum_value(
  const char*               validator_name,
  const char*               field_name,
  cardano_cbor_reader_t*    reader,
  uint64_t                  expected_value,
  enum_to_string_callback_t enum_to_string_callback,
  uint64_t*                 actual_value);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_VALIDATION_H
