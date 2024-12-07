/**
 * \file json_writer.h
 *
 * \author angel.castillo
 * \date   Dec 06, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_WRITER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_WRITER_H

/* INCLUDES ******************************************************************/

#include "json_object.h"
#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/json/json_context.h>
#include <cardano/json/json_format.h>
#include <cardano/json/json_object.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Provides a API for forward-only, non-cached writing of UTF-8 encoded JSON text.
 */
typedef struct cardano_json_writer_t cardano_json_writer_t;

/**
 * \brief Creates a new JSON writer instance.
 *
 * This function initializes a new instance of a \ref cardano_json_writer_t, which
 * can be used to encode data into JSON format. The created writer manages its
 * internal state and ensures that the generated JSON is syntactically valid.
 *
 * The JSON writer can be used to encode JSON incrementally by invoking various
 * write functions to add properties, values, and nested structures.
 *
 * \param[in] format The format of the JSON output (compact or pretty).
 *
 * \return A pointer to a newly allocated \ref cardano_json_writer_t instance.
 *      Returns \c NULL if the creation fails due to insufficient memory or
 *      other initialization errors.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 * if (writer == NULL)
 * {
 *   printf("Failed to create a JSON writer.\n");
 *   return EXIT_FAILURE;
 * }
 *
 * // Use the writer to encode JSON data...
 *
 * cardano_json_writer_unref(&writer); // Release resources when done
 * \endcode
 *
 * \note The caller is responsible for managing the memory of the returned
 *    JSON writer instance. Use \ref cardano_json_writer_unref to release
 *    resources when the writer is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_writer_t* cardano_json_writer_new(cardano_json_format_t format);

/**
 * \brief Writes a property name to the JSON output.
 *
 * This function writes the name of a JSON property to the output, preparing it
 * for the subsequent value. Property names used within JSON objects
 * must be followed by a value-writing function to form a valid key-value pair.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 * \param[in] name A pointer to the property name string. The string does not
 *        need to be null-terminated, as its length is specified separately.
 * \param[in] name_size The size of the property name string in bytes.
 *
 * \pre The JSON writer must currently be in an object context. Calling this function
 *   outside an object context will result in an error.
 * \pre The `name` string must not contain invalid JSON characters such as control characters.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 * cardano_json_writer_write_start_object(writer);
 * cardano_json_writer_write_property_name(writer, "name", 4); // Property: "name"
 * cardano_json_writer_write_string(writer, "John Doe", 8);    // Value: "John Doe"
 * cardano_json_writer_write_end_object(writer);
 *
 * ...
 *
 * cardano_json_writer_unref(&writer); // Release resources
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_property_name(
  cardano_json_writer_t* writer,
  const char*            name,
  size_t                 name_size);

/**
 * \brief Writes a boolean value to the JSON output.
 *
 * This function writes a boolean value (`true` or `false`) as a JSON value.
 * The value is represented in JSON's boolean format, ensuring compatibility
 * with all JSON parsers.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *                   This must be a valid, non-NULL JSON writer.
 * \param[in] value The boolean value to be written. Passing `true` writes the
 *                  JSON literal `true`, and passing `false` writes the JSON
 *                  literal `false`.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *      (e.g., after a property name in an object or as an element in an array).
 *      Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *       calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *       Once an error occurs, the writer enters an error state, and subsequent calls
 *       to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);                  // Start object: {
 *  cardano_json_writer_write_property_name(writer, "isEnabled", 9); // Property: "isEnabled"
 *  cardano_json_writer_write_bool(writer, true);                    // Value: true
 *  cardano_json_writer_write_end_object(writer);                    // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "isEnabled": true
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_bool(
  cardano_json_writer_t* writer,
  bool                   value);

/**
 * \brief Writes a JSON `null` value to the output.
 *
 * This function writes the literal `null` value to the JSON output.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in an array).
 *   Calling this function in an invalid context will result in error.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_json_writer_write_start_object(writer);
 *  cardano_json_writer_write_property_name(writer, "name", 4);
 *  cardano_json_writer_write_null(writer); // Write "name": null
 *  cardano_json_writer_write_end_object(writer);
 *
 *  char buffer[128];
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *    cardano_json_writer_unref(&writer);
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //  "name": null
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_null(
  cardano_json_writer_t* writer);

/**
 * \brief Writes a big integer value to the JSON output.
 *
 * This function writes a big integer (\ref cardano_bigint_t) as a JSON value. The
 * value is encoded as a JSON string to ensure compatibility with JSON parsers
 * that may not natively support extremely large integer values.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 * \param[in] bigint A pointer to the \ref cardano_bigint_t instance representing
 *          the big integer to be written.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in an array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note The big integer is serialized as a JSON string (e.g., `"123456789123456789"`) to
 *    preserve its precision and avoid loss of information due to JSON's numeric limitations.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Create a big integer value
 *  cardano_bigint_t* bigint = cardano_bigint_from_string("123456789123456789");
 *
 *  if (bigint == NULL)
 *  {
 *    printf("Failed to create big integer.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_json_writer_write_start_object(writer);
 *  cardano_json_writer_write_property_name(writer, "bigNumber", 9); // Property: "bigNumber"
 *  cardano_json_writer_write_bigint(writer, bigint);   // Value: "123456789123456789"
 *  cardano_json_writer_write_end_object(writer);
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128];
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_bigint_unref(&bigint);
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    cardano_bigint_unref(&bigint);
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "bigNumber": "123456789123456789"
 *  // }
 *
 *  // Clean up
 *  cardano_bigint_unref(&bigint);
 *  cardano_json_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_bigint(
  cardano_json_writer_t* writer,
  cardano_bigint_t*      bigint);

/**
 * \brief Begins a JSON array in the output.
 *
 * This function writes the opening bracket (`[`) of a JSON array. After calling this
 * function, subsequent calls to writer functions can add elements to the array.
 * The array must be closed with \ref cardano_json_writer_write_end_array to produce
 * a valid JSON structure.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in another array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_json_writer_write_start_object(writer);       // Start object: {
 *  cardano_json_writer_write_property_name(writer, "numbers", 7); // Property: "numbers"
 *  cardano_json_writer_write_start_array(writer);        // Start array: [
 *  cardano_json_writer_write_uint(writer, 1);         // Add element: 1
 *  cardano_json_writer_write_uint(writer, 2);         // Add element: 2
 *  cardano_json_writer_write_uint(writer, 3);         // Add element: 3
 *  cardano_json_writer_write_end_array(writer);          // End array: ]
 *  cardano_json_writer_write_end_object(writer);         // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128];
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //  "numbers": [1, 2, 3]
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_start_array(
  cardano_json_writer_t* writer);

/**
 * \brief Ends a JSON array in the output.
 *
 * This function writes the closing bracket (`]`) of a JSON array, marking the end
 * of the array structure. It must be preceded by a corresponding call to
 * \ref cardano_json_writer_write_start_array.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 *
 * \pre The JSON writer must currently be in an array context. Calling this function
 *   outside an array context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *   printf("Failed to create JSON writer.\n");
 *   return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object containing an array
 *  cardano_json_writer_write_start_object(writer);      // Start object: {
 *  cardano_json_writer_write_property_name(writer, "items", 5);  // Property: "items"
 *  cardano_json_writer_write_start_array(writer);       // Start array: [
 *  cardano_json_writer_write_string(writer, "item1", 5);      // Add element: "item1"
 *  cardano_json_writer_write_string(writer, "item2", 5);      // Add element: "item2"
 *  cardano_json_writer_write_end_array(writer);         // End array: ]
 *  cardano_json_writer_write_end_object(writer);        // End object: }
 *
 *  char buffer[128];
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *   printf("Buffer size is too small for the JSON output.\n");
 *   cardano_json_writer_unref(&writer);
 *   return -1;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return -1;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "items": ["item1", "item2"]
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_end_array(cardano_json_writer_t* writer);

/**
 * \brief Begins a JSON object in the output.
 *
 * This function writes the opening curly brace (`{`) of a JSON object. After calling
 * this function, subsequent calls to writer functions can add properties (key-value pairs)
 * to the object. The object must be closed with \ref cardano_json_writer_write_end_object
 * to produce a valid JSON structure.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in another object or as an element in an array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);       // Start object: {
 *  cardano_json_writer_write_property_name(writer, "name", 4); // Property: "name"
 *  cardano_json_writer_write_string(writer, "John Doe", 8);    // Value: "John Doe"
 *  cardano_json_writer_write_property_name(writer, "age", 3);  // Property: "age"
 *  cardano_json_writer_write_uint(writer, 30);        // Value: 30
 *  cardano_json_writer_write_end_object(writer);      // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128];
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *
 *    cardano_json_writer_unref(&writer);
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "name": "John Doe",
 *  //   "age": 30
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_start_object(
  cardano_json_writer_t* writer);

/**
 * \brief Ends a JSON object in the output.
 *
 * This function writes the closing curly brace (`}`) of a JSON object, marking the
 * end of the object structure. It must be preceded by a corresponding call to
 * \ref cardano_json_writer_write_start_object.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 *
 * \pre The JSON writer must currently be in an object context. Calling this function
 *   outside an object context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);       // Start object: {
 *  cardano_json_writer_write_property_name(writer, "name", 4); // Property: "name"
 *  cardano_json_writer_write_string(writer, "Alice", 5);    // Value: "Alice"
 *  cardano_json_writer_write_property_name(writer, "age", 3);  // Property: "age"
 *  cardano_json_writer_write_uint(writer, 25);        // Value: 25
 *  cardano_json_writer_write_end_object(writer);      // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "name": "Alice",
 *  //   "age": 25
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_end_object(cardano_json_writer_t* writer);

/**
 * \brief Writes a raw JSON value directly to the output.
 *
 * This function writes a raw JSON value to the output. The provided data must be
 * a syntactically valid JSON value (e.g., a number, a string, a boolean, or a null).
 * The JSON writer does not perform validation on the raw data; it assumes the caller
 * has ensured the correctness of the value.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 * \param[in] data A pointer to the raw JSON value as a string. The string does not
 *        need to be null-terminated, as its length is specified separately.
 * \param[in] size The size of the raw JSON value in bytes.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in an array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * \warning The caller is responsible for ensuring the raw value is a valid JSON value.
 *    Invalid data may result in syntactically incorrect JSON output.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *  printf("Failed to create JSON writer.\n");
 *  return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);       // Start object: {
 *  cardano_json_writer_write_property_name(writer, "rawData", 7); // Property: "rawData"
 *  cardano_json_writer_write_raw_value(writer, "[1, 2, 3]", 9);   // Write raw value: [1, 2, 3]
 *  cardano_json_writer_write_end_object(writer);         // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *    return -1;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    cardano_json_writer_unref(&writer);
 *    return -1;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //  "rawData": [1, 2, 3]
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_raw_value(
  cardano_json_writer_t* writer,
  const char*            data,
  size_t                 size);

CARDANO_EXPORT void cardano_json_writer_write_object(
  cardano_json_writer_t* writer,
  cardano_json_object_t* object);

/**
 * \brief Writes an unsigned integer value to the JSON output.
 *
 * This function writes an unsigned integer (\c uint64_t) as a JSON value.
 * The value is written in its numeric representation, which is valid for all
 * JSON parsers.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 * \param[in] value The unsigned integer value to be written.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in an array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);        // Start object
 *  cardano_json_writer_write_property_name(writer, "count", 5); // Property: "count"
 *  cardano_json_writer_write_uint(writer, 123456);        // Value: 123456
 *  cardano_json_writer_write_end_object(writer);       // End object
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "count": 123456
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_uint(
  cardano_json_writer_t* writer,
  uint64_t               value);

/**
 * \brief Writes an signed integer value to the JSON output.
 *
 * This function writes an signed integer (\c int64_t) as a JSON value.
 * The value is written in its numeric representation, which is valid for all
 * JSON parsers.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 * \param[in] value The signed integer value to be written.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in an array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);        // Start object
 *  cardano_json_writer_write_property_name(writer, "count", 5); // Property: "count"
 *  cardano_json_writer_write_signed_int(writer, -123456);    // Value: -123456
 *  cardano_json_writer_write_end_object(writer);       // End object
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "count": -123456
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_signed_int(
  cardano_json_writer_t* writer,
  int64_t                value);

/**
 * \brief Writes a double-precision floating-point value to the JSON output.
 *
 * This function writes a double-precision floating-point number (\c double) as a JSON value.
 * The value is written in its numeric representation, conforming to JSON's number format.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *          This must be a valid, non-NULL JSON writer.
 * \param[in] value The double value to be written.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *   (e.g., after a property name in an object or as an element in an array).
 *   Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *    calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *    Once an error occurs, the writer enters an error state, and subsequent calls
 *    to writer functions will have no effect.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);           // Start object: {
 *  cardano_json_writer_write_property_name(writer, "pi", 2); // Property: "pi"
 *  cardano_json_writer_write_double(writer, 3.14159265359);  // Value: 3.14159265359
 *  cardano_json_writer_write_property_name(writer, "e", 1);  // Property: "e"
 *  cardano_json_writer_write_double(writer, 2.71828182845);  // Value: 2.71828182845
 *  cardano_json_writer_write_end_object(writer);             // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *    printf("Buffer size is too small for the JSON output.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "pi": 3.14159265359,
 *  //   "e": 2.71828182845
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_double(
  cardano_json_writer_t* writer,
  double                 value);

/**
 * \brief Writes a string value to the JSON output.
 *
 * This function writes a string value to the JSON output. The string will be enclosed
 * in double quotes and any necessary escaping for special characters (e.g., quotes, backslashes)
 * will be handled automatically by the writer.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *                   This must be a valid, non-NULL JSON writer.
 * \param[in] value A pointer to the string to be written. The string does not need
 *                  to be null-terminated, as its length is specified separately.
 * \param[in] value_size The size of the string in bytes.
 *
 * \pre The JSON writer must currently be in a valid context for writing a value
 *      (e.g., after a property name in an object or as an element in an array).
 *      Calling this function in an invalid context will result in deferred error reporting.
 *
 * \note If an error occurs during this function, the error is deferred until the user
 *       calls \ref cardano_json_writer_encode or \ref cardano_json_writer_encode_in_buffer.
 *       Once an error occurs, the writer enters an error state, and subsequent calls
 *       to writer functions will have no effect.
 *
 * \note The writer will handle UTF-8 encoded strings. It is the caller's responsibility
 *       to ensure that the input string is valid UTF-8.
 *
 * Usage Example:
 * \code{.c}
 *  // Create a new JSON writer
 *  cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 *  if (writer == NULL)
 *  {
 *    printf("Failed to create JSON writer.\n");
 *    printf("Error: %s\n", cardano_json_writer_get_last_error(writer));

 *    return EXIT_FAILURE;
 *  }
 *
 *  // Start writing a JSON object
 *  cardano_json_writer_write_start_object(writer);                // Start object: {
 *  cardano_json_writer_write_property_name(writer, "message", 7); // Property: "message"
 *  cardano_json_writer_write_string(writer, "Hello, World!", 13); // Value: "Hello, World!"
 *  cardano_json_writer_write_end_object(writer);                  // End object: }
 *
 *  // Encode and print the resulting JSON
 *  char buffer[128] = { 0 };
 *  size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
 *
 *  if (encoded_size > sizeof(buffer))
 *  {
 *   printf("Buffer size is too small for the JSON output.\n");
 *   printf("Error: %s\n", cardano_json_writer_get_last_error(writer));
 *
 *   cardano_json_writer_unref(&writer);
 *
 *   return EXIT_FAILURE;
 *  }
 *
 *  cardano_error_t result = cardano_json_writer_encode(writer, (byte_t*)buffer, sizeof(buffer));
 *
 *  if (result != CARDANO_SUCCESS)
 *  {
 *    printf("Failed to encode JSON.\n");
 *    cardano_json_writer_unref(&writer);
 *
 *    return EXIT_FAILURE;
 *  }
 *
 *  printf("%s\n", buffer);
 *
 *  // Example output:
 *  // {
 *  //   "message": "Hello, World!"
 *  // }
 *
 *  // Clean up
 *  cardano_json_writer_unref(&writer);
 * \endcode
 */
CARDANO_EXPORT void cardano_json_writer_write_string(
  cardano_json_writer_t* writer,
  const char*            value,
  size_t                 value_size);

/**
 * \brief Gets the current context of the JSON writer.
 *
 * This function retrieves the current context of the JSON writer, indicating whether
 * the writer is in a root state (brand new with no written output), inside an object,
 * or inside an array. This information can be used by objects or arrays that write
 * themselves to determine whether to initialize the root object or continue writing
 * within an existing context.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance.
 *                   This must be a valid, non-NULL JSON writer.
 *
 * \return A \ref cardano_json_context_t value representing the current context:
 *         - \c CARDANO_JSON_CONTEXT_ROOT: The writer is in the root state (no context set, no output started).
 *         - \c CARDANO_JSON_CONTEXT_OBJECT: The writer is inside an object.
 *         - \c CARDANO_JSON_CONTEXT_ARRAY: The writer is inside an array.
 *
 * \note If the writer is in an error state or is NULL, the function returns \c CARDANO_JSON_CONTEXT_ROOT.
 *
 * Usage Example:
 * \code{.c}
 *
 *  cardano_json_context_t context = cardano_json_writer_get_context(writer);
 *
 *  if (context == CARDANO_JSON_CONTEXT_ROOT)
 *  {
 *    cardano_json_writer_write_start_object(writer);
 *  }
 *
 *  cardano_json_writer_write_property_name(writer, "key", 3);
 *  cardano_json_writer_write_string(writer, "value", 5);
 *
 *  if (context == CARDANO_JSON_CONTEXT_ROOT)
 *  {
 *    // Close the root object if we opened it
 *    cardano_json_writer_write_end_object(writer);
 *  }
 * \endcode
 */
CARDANO_EXPORT cardano_json_context_t cardano_json_writer_get_context(cardano_json_writer_t* writer);

/**
 * \brief Calculates the required buffer size for the encoded data.
 *
 * This function determines the size of the buffer needed to store the  data encoded by the writer.
 * It is intended to be used before calling `cardano_json_writer_encode` to allocate a buffer of appropriate size.
 *
 * \param[in] writer The source writer whose encoded data size is being calculated.
 *
 * \return The size of the buffer required to store the writer's encoded data. Returns 0 if the writer is NULL
 * or contains no data.
 *
 * Example usage:
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 * // Assume writer has been used to encode some JSON data here
 *
 * size_t required_size = cardano_json_writer_get_encoded_size(writer);
 *
 * // Now required_size can be used to allocate a buffer of appropriate size
 * byte_t* data = (byte_t*)malloc(required_size);
 *
 * if (byte_t != NULL)
 * {
 *   if (cardano_json_writer_encode(writer, data, required_size) == CARDANO_SUCCESS)
 *   {
 *     // Operate on the encoded data
 *   }
 *
 *   free(data); // Free the allocated string after use
 * }
 *
 * cardano_json_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_json_writer_get_encoded_size(cardano_json_writer_t* writer);

/**
 * \brief Encodes data from the writer's context into JSON format and outputs it to a provided buffer.
 *
 * This function encodes data prepared in the writer's internal context into JSON format and writes it into an output
 * buffer specified by the caller.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t structure representing the context and state for the JSON encoding
 *          operation.
 * \param[in] data A pointer to a byte array where the encoded data will be written. The function will fill this buffer with
 *        the JSON-encoded data.
 * \param[in] size The size of the data byte array, indicating how much space is available for writing the encoded data.
 *
 * \return A cardano_error_t indicating the outcome of the operation. Returns CARDANO_SUCCESS if the data is successfully
 *      encoded into JSON format and stored in the provided buffer. If the operation fails, an error code is returned indicating
 *      the specific reason for failure. Detailed information on possible error codes and their meanings can be found in the
 *      cardano_error_t documentation.
 *
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 * byte_t buffer[256]; // Pre-allocated buffer with sufficient space for the encoded data
 *
 * cardano_error_t result = cardano_json_writer_encode(writer, buffer, sizeof(buffer));
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Encoded JSON data successfully. Bytes written: %zu\n", sizeof(buffer));
 * }
 * else
 * {
 *   printf("Failed to encode JSON data. Error code: %d\n", result);
 * }
 *
 * cardano_json_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_writer_encode(
  cardano_json_writer_t* writer,
  char*                  data,
  size_t                 size);

/**
 * \brief Encodes data from the writer's context into JSON format and outputs it into a new buffer.
 *
 * This function encodes data prepared in the writer's internal context into JSON format and writes it into a
 * buffer. The function will create a new buffer instance to store the encoded data.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t structure representing the context and state for the JSON encoding
 *          operation.
 * \param[out] buffer A pointer to a \ref cardano_buffer_t object where the encoded data will be stored. The function will allocate
 *        and fill this buffer with the JSON-encoded data. It is best to initialize this pointer as NULL. The caller is
 *        responsible for managing the lifecycle of the buffer, specifically releasing it by calling \ref cardano_buffer_unref
 *        when it is no longer needed.
 *
 * \return A cardano_error_t indicating the outcome of the operation. Returns CARDANO_SUCCESS if the data is successfully
 *      encoded into JSON format and stored in the provided buffer. If the operation fails, an error code is returned indicating
 *      the specific reason for failure. Detailed information on possible error codes and their meanings can be found in the
 *      cardano_error_t documentation.
 *
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 * cardano_buffer_t* buffer = NULL; // Buffer will be allocated by the function
 *
 * cardano_error_t result = cardano_json_writer_encode_in_buffer(writer, &buffer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Encoded JSON data successfully. Bytes written: %zu\n", cardano_buffer_get_size(buffer));
 * }
 * else
 * {
 *   printf("Failed to encode JSON data. Error code: %d\n", result);
 * }
 *
 * cardano_buffer_unref(&buffer); // Properly dispose of the buffer after use
 * cardano_json_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_writer_encode_in_buffer(
  cardano_json_writer_t* writer,
  cardano_buffer_t**     buffer);

/**
 * \brief Resets the writer, clearing all written data.
 *
 * This function resets the internal state of the JSON writer, effectively removing any data that has been written to it.
 * This is useful for reusing a writer instance without needing to create a new one, especially when working with
 * sequences of data encoding tasks.
 *
 * \param[in] writer The JSON writer instance to reset.
 *
 * \return A cardano_error_t indicating the outcome of the operation. CARDANO_SUCCESS is returned if the writer is
 *      successfully reset. If the operation fails, an error code is returned that indicates the specific reason for
 *      failure. For detailed information on possible error codes and their meanings, consult the cardano_error_t
 *      documentation.
 *
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 * // Write some data to the writer here
 *
 * cardano_error_t reset_result = cardano_json_writer_reset(writer);
 *
 * if (reset_result == CARDANO_SUCCESS)
 * {
 *  printf("Writer reset successfully.\n");
 * }
 * else
 * {
 *  printf("Failed to reset writer. Error code: %d\n", reset_result);
 * }
 * // The writer can now be reused for new data without creating a new instance
 *
 * cardano_json_writer_unref(&writer); // Clean up when done using the writer
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_writer_reset(cardano_json_writer_t* writer);

/**
 * \brief Decrements the reference count of a JSON writer object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_json_writer_t object
 * by decreasing its reference count. When the reference count reaches zero, the JSON writer is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] json_writer A pointer to the pointer of the JSON writer object. This double
 *             indirection allows the function to set the caller's pointer to
 *             NULL, avoiding dangling pointer issues after the object has been
 *             freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_writer_t* writer = cardano_json_writer_new();
 *
 * // Perform operations with the writer...
 *
 * cardano_json_writer_unref(&writer);
 * // At this point, writer is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_json_writer_unref, the pointer to the \ref cardano_json_writer_t object
 *    will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_json_writer_unref(cardano_json_writer_t** json_writer);

/**
 * \brief Increases the reference count of the cardano_json_writer_t object.
 *
 * This function is used to manually increment the reference count of a JSON writer
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_json_writer_unref.
 *
 * \param json_writer A pointer to the JSON writer object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming writer is a previously created JSON writer object
 *
 * cardano_json_writer_ref(writer);
 *
 * // Now writer can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_json_writer_ref there is a corresponding
 * call to \ref cardano_json_writer_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_json_writer_ref(cardano_json_writer_t* json_writer);

/**
 * \brief Retrieves the current reference count of the cardano_json_writer_t object.
 *
 * This function returns the number of active references to a JSON writer object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_json_writer_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param json_writer A pointer to the JSON writer object whose reference count is queried.
 *        The object must not be NULL.
 *
 * \return The number of active references to the specified JSON writer object. If the object
 * is properly managed (i.e., every \ref cardano_json_writer_ref call is matched with a
 * \ref cardano_json_writer_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming writer is a previously created JSON writer object
 *
 * size_t ref_count = cardano_json_writer_refcount(writer);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_json_writer_refcount(const cardano_json_writer_t* json_writer);

/**
 * \brief Sets the last error message for a given JSON writer object.
 *
 * Records an error message in the JSON writer's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance whose last error message is
 *          to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the writer's
 *        last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_json_writer_set_last_error(cardano_json_writer_t* writer, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific writer.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_json_writer_set_last_error for the given
 * writer. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t instance whose last error
 *          message is to be retrieved. If the writer is NULL, the function
 *          returns a generic error message indicating the null writer.
 *
 * \return A pointer to a null-terminated string containing the last error
 *      message for the specified writer. If the writer is NULL, "Object is NULL."
 *      is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *    must not be modified by the caller. The string remains valid until the
 *    next call to \ref cardano_json_writer_set_last_error for the same writer, or until
 *    the writer is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_json_writer_get_last_error(const cardano_json_writer_t* writer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_WRITER_H