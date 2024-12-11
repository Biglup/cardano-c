/**
 * \file json_object.h
 *
 * \author angel.castillo
 * \date   Dec 07, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/json/json_format.h>
#include <cardano/json/json_object_type.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \struct cardano_json_object_t
 *
 * \brief Represents a parsed JSON object, array, or value.
 *
 * This opaque structure serves as the primary data type for interacting with JSON data in the library.
 * It can represent any valid JSON value, including objects, arrays, strings, numbers, booleans, or null.
 *
 * JSON objects are immutable once created, providing thread safety and consistency in shared contexts.
 */
typedef struct cardano_json_object_t cardano_json_object_t;

/**
 * \brief Parses a JSON string into a \ref cardano_json_object_t.
 *
 * This function takes a JSON string and parses it into a \ref cardano_json_object_t representation.
 * The returned object can be used to query the parsed JSON data.
 *
 * \param[in] json A pointer to the JSON string to parse. The string does not need to be null-terminated
 *                 as its length is specified by \p size. This parameter must not be NULL.
 * \param[in] size The length of the JSON string, in bytes.
 *
 * \return A pointer to the parsed \ref cardano_json_object_t if the parsing is successful, or \c NULL if
 *         the input is invalid, the JSON string is malformed, or there is insufficient memory to complete the operation.
 *
 * \note The returned \ref cardano_json_object_t has an initial reference count of 1. The caller is responsible
 *       for releasing the object by calling \ref cardano_json_object_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_data = "{\"name\":\"Alice\",\"age\":30,\"active\":true}";
 * cardano_json_object_t* json_object = cardano_json_object_parse(json_data, strlen(json_data));
 *
 * if (json_object != NULL)
 * {
 *   // Query the JSON object (e.g., get the number of keys)
 *   size_t size = cardano_json_object_get_property_count(json_object);
 *   printf("Number of keys: %zu\n", size);
 *
 *   // Always release the JSON object when done
 *   cardano_json_object_unref(&json_object);
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_object_t* cardano_json_object_parse(const char* json, size_t size);

/**
 * \brief Serializes a JSON object into a JSON string.
 *
 * This function takes a JSON object and converts it into its JSON string representation.
 * The resulting string is valid JSON and encoded in UTF-8. The caller must ensure the
 * JSON object is valid and properly initialized before calling this function.
 *
 * \param[in] json_object A pointer to the JSON object to be serialized. Must not be NULL.
 * \param[in] format The format to use when serializing the JSON object.
 * \param[out] length A pointer to a size_t variable where the length of the resulting JSON
 *                    string will be stored (excluding the null terminator). Must not be NULL.
 *
 * \return A pointer to a null-terminated UTF-8 JSON string representing the serialized object.
 *         This points to an internal buffer and must not be modified or freed by the caller. The
 *         string is valid until the JSON object is released.
 *
 * \note If the function returns NULL, the value of \p length is undefined.
 *
 * \code{.c}
 * cardano_json_object_t* obj = ...
 * size_t length = 0;
 *
 * const char* json_string = cardano_json_object_to_json_string(obj, CARDANO_JSON_FORMAT_COMPACT, &length);
 *
 * if (json_string != NULL)
 * {
 *   printf("JSON: %s\n", json_string);
 *   printf("Length: %zu\n", length);
 * }
 * else
 * {
 *   printf("Failed to serialize JSON object.\n");
 * }
 *
 * cardano_json_object_unref(&obj);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_json_object_to_json_string(
  cardano_json_object_t* json_object,
  cardano_json_format_t  format,
  size_t*                length);

/**
 * \brief Retrieves the type of a JSON object.
 *
 * This function determines the type of the specified \ref cardano_json_object_t. The type
 * indicates whether the object represents a JSON object, array, string, number, boolean, or null.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t whose type is to be determined.
 *                        This parameter must not be NULL.
 *
 * \return A value of type \ref cardano_json_object_type_t representing the type of the JSON object.
 *         If \p json_object is NULL, the function returns \c CARDANO_JSON_OBJECT_TYPE_NULL.
 *
 * The possible return values are:
 * - \c CARDANO_JSON_OBJECT_TYPE_OBJECT: The JSON object represents a key-value structure.
 * - \c CARDANO_JSON_OBJECT_TYPE_ARRAY: The JSON object represents an array of elements.
 * - \c CARDANO_JSON_OBJECT_TYPE_STRING: The JSON object represents a string.
 * - \c CARDANO_JSON_OBJECT_TYPE_NUMBER: The JSON object represents a numeric value.
 * - \c CARDANO_JSON_OBJECT_TYPE_BOOLEAN: The JSON object represents a boolean value (true or false).
 * - \c CARDANO_JSON_OBJECT_TYPE_NULL: The JSON object is null or the parameter is invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"key\":\"value\"}", 15);
 * if (json_object != NULL)
 * {
 *   cardano_json_object_type_t type = cardano_json_object_get_type(json_object);
 *
 *   switch (type)
 *   {
 *     case CARDANO_JSON_OBJECT_TYPE_OBJECT:
 *       printf("The JSON object is an object.\n");
 *       break;
 *     case CARDANO_JSON_OBJECT_TYPE_ARRAY:
 *       printf("The JSON object is an array.\n");
 *       break;
 *     case CARDANO_JSON_OBJECT_TYPE_STRING:
 *       printf("The JSON object is a string.\n");
 *       break;
 *     case CARDANO_JSON_OBJECT_TYPE_NUMBER:
 *       printf("The JSON object is a number.\n");
 *       break;
 *     case CARDANO_JSON_OBJECT_TYPE_BOOLEAN:
 *       printf("The JSON object is a boolean.\n");
 *       break;
 *     case CARDANO_JSON_OBJECT_TYPE_NULL:
 *     default:
 *       printf("The JSON object is null or invalid.\n");
 *       break;
 *    }
 *   // Always release the JSON object when done
 *   cardano_json_object_unref(&json_object);
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 *
 * \see cardano_json_object_parse
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_object_type_t cardano_json_object_get_type(
  const cardano_json_object_t* json_object);

/**
 * \brief Checks if a JSON object contains a specific property.
 *
 * This function determines whether a JSON object contains a property with the specified key.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be NULL.
 * \param[in] key A pointer to the key string to check. The key does not need to be null-terminated, as its length
 *                is specified by \p size. This parameter must not be NULL.
 * \param[in] size The length of the key string, in bytes.
 *
 * \return \c true if the JSON object contains the specified property; \c false otherwise.
 *
 * \note This function is only valid for JSON objects. If the \p json_object is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, the function will return \c false.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   bool has_name = cardano_json_object_has_property(json_object, "name", 4);
 *   bool has_age = cardano_json_object_has_property(json_object, "age", 3);
 *   bool has_active = cardano_json_object_has_property(json_object, "active", 6);
 *
 *   printf("Has 'name': %s\n", has_name ? "true" : "false");
 *   printf("Has 'age': %s\n", has_age ? "true" : "false");
 *   printf("Has 'active': %s\n", has_active ? "true" : "false");
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_json_object_has_property(
  const cardano_json_object_t* json_object,
  const char*                  key,
  size_t                       size);

/**
 * \brief Retrieves the number of properties in a JSON object.
 *
 * This function returns the number of key-value pairs (properties) in the specified JSON object.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be NULL.
 *
 * \return The number of properties in the JSON object, or \c 0 if the \p json_object is not
 *         of type \ref CARDANO_JSON_OBJECT_TYPE_OBJECT or is \c NULL.
 *
 * \note This function is only valid for JSON objects. If the \p json_object is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, the function will return \c 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   size_t property_count = cardano_json_object_get_property_count(json_object);
 *   printf("Number of properties: %zu\n", property_count);
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_json_object_get_property_count(
  const cardano_json_object_t* json_object);

/**
 * \brief Retrieves the key at a specific position in a JSON object.
 *
 * This function provides access to the key at a specified index within a JSON object.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be NULL.
 * \param[in] index The zero-based index of the key to retrieve. The index must be less than
 *                  the number of properties in the JSON object, as determined by
 *                  \ref cardano_json_object_get_property_count.
 * \param[out] key_length A reference to a size_t variable where the length of the key will be stored.
 *                        Does not include the null terminator.
 *
 * \return A pointer to the null-terminated key string if the index is valid, or \c NULL if the index is out of range,
 *         the \p json_object is not of type \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, or any parameter is invalid.
 *
 * \note The returned key remains valid as long as the \p json_object is not deallocated. The caller must
 *       not attempt to modify or free the returned key.
 *
 * \note This function is only valid for JSON objects. If the \p json_object is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, the function will return \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   size_t property_count = cardano_json_object_get_property_count(json_object);
 *   printf("Number of properties: %zu\n", property_count);
 *
 *   for (size_t i = 0; i < property_count; ++i)
 *   {
 *     size_t key_length = 0;
 *     const char* key = cardano_json_object_get_key_at(json_object, i, &key_length);
 *
 *     if (key != NULL)
 *     {
 *       printf("Key %zu: %.*s\n", i, (int)key_length, key);
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve key at index %zu\n", i);
 *     }
 *   }
 *
 *  cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_json_object_get_key_at(
  const cardano_json_object_t* json_object,
  size_t                       index,
  size_t*                      key_length);

/**
 * \brief Retrieves the value at a specific position in a JSON object.
 *
 * This function provides access to the value associated with a specific index within a JSON object.
 * The index corresponds to the order of key-value pairs in the JSON object. The returned value has
 * its reference count incremented, so it must be explicitly released when no longer needed by calling
 * \ref cardano_json_object_unref.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be NULL.
 * \param[in] index The zero-based index of the value to retrieve. The index must be less than
 *                  the number of properties in the JSON object, as determined by
 *                  \ref cardano_json_object_get_property_count.
 *
 * \return A new reference to a \ref cardano_json_object_t representing the value at the specified index,
 *         or \c NULL if the index is out of range, the \p json_object is not of type
 *         \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, or any parameter is invalid.
 *
 * \note This function increments the reference count of the returned \ref cardano_json_object_t.
 *       The caller is responsible for managing the lifecycle of the returned object. To release
 *       the object, call \ref cardano_json_object_unref.
 *
 * \note This function is only valid for JSON objects. If the \p json_object is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, the function will return \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   size_t property_count = cardano_json_object_get_property_count(json_object);
 *   printf("Number of properties: %zu\n", property_count);
 *
 *   for (size_t i = 0; i < property_count; ++i)
 *   {
 *     size_t key_length = 0;
 *     const char* key = cardano_json_object_get_key_at(json_object, i, key_length);
 *     cardano_json_object_t* value = cardano_json_object_get_value_at(json_object, i);
 *
 *     if (key != NULL && value != NULL)
 *     {
 *       printf("Key %zu: %.*s\n", i, (int)key_length, key);
 *       cardano_json_object_type_t type = cardano_json_object_get_type(value);
 *
 *       // Process the value
 *       cardano_json_object_unref(&value); // Release the value when done
 *      }
 *      else
 *      {
 *        printf("Failed to retrieve key or value at index %zu\n", i);
 *     }
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_object_t* cardano_json_object_get_value_at(
  const cardano_json_object_t* json_object,
  size_t                       index);

/**
 * \brief Retrieves the value at a specific index in a JSON object without incrementing the reference count.
 *
 * This function provides access to the value associated with a specific index within a JSON object.
 * It is an extended version of \ref cardano_json_object_get_value_at that does not increment the
 * reference count of the returned object. This is useful in scenarios where the caller does not need
 * to take ownership of the returned value.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be \c NULL.
 * \param[in] index The zero-based index of the value to retrieve. The index must be less than the
 *                  number of properties in the JSON object, as determined by \ref cardano_json_object_get_property_count.
 *
 * \return A pointer to the \ref cardano_json_object_t representing the value at the specified index,
 *         or \c NULL if the index is out of range, the \p json_object is not of type
 *         \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, or any parameter is invalid.
 *
 * \note Unlike \ref cardano_json_object_get_value_at, this function does not increment the reference
 *       count of the returned object. The caller must ensure the lifetime of the \p json_object is valid
 *       while the returned value is in use.
 *
 * \warning Do not modify or release the returned value directly unless you first increment its reference
 *          count using \ref cardano_json_object_ref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   size_t property_count = cardano_json_object_get_property_count(json_object);
 *   printf("Number of properties: %zu\n", property_count);
 *
 *   for (size_t i = 0; i < property_count; ++i)
 *   {
 *     cardano_json_object_t* value = cardano_json_object_get_value_at_ex(json_object, i);
 *
 *     if (value != NULL)
 *     {
 *       cardano_json_object_type_t type = cardano_json_object_get_type(value);
 *
 *       // Process the value without taking ownership
 *       switch (type)
 *       {
 *         case CARDANO_JSON_OBJECT_TYPE_STRING:
 *           printf("Property %zu is a string: %s\n", i, cardano_json_object_get_string(value, NULL));
 *           break;
 *         case CARDANO_JSON_OBJECT_TYPE_NUMBER:
 *           printf("Property %zu is a number.\n", i);
 *           break;
 *         default:
 *           printf("Property %zu has an unknown type.\n", i);
 *           break;
 *       }
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve property value at index %zu.\n", i);
 *     }
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_object_t* cardano_json_object_get_value_at_ex(
  const cardano_json_object_t* json_object,
  size_t                       index);

/**
 * \brief Retrieves the value associated with a specified key in a JSON object.
 *
 * This function allows you to access the value of a property in a JSON object by specifying its key.
 * The returned value has its reference count incremented, so it must be explicitly released
 * when no longer needed by calling \ref cardano_json_object_unref.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be NULL.
 * \param[in] key A pointer to the key string to look up. The key does not need to be null-terminated,
 *                as its length is specified by \p size. This parameter must not be NULL.
 * \param[in] size The length of the key string, in bytes.
 * \param[out] value A pointer to a \ref cardano_json_object_t pointer where the value associated with
 *                   the key will be stored. The retrieved value has its reference count incremented.
 *                   The caller is responsible for releasing the value when it is no longer needed.
 *
 * \return \c true if the key was found and \p value was successfully populated, or \c false otherwise.
 *
 * \note This function is only valid for JSON objects. If the \p json_object is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_OBJECT, the function will return \c false and \p value will remain unmodified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "name", 4, &value))
 *   {
 *     cardano_json_object_type_t type = cardano_json_object_get_type(value);
 *
 *     if (type == CARDANO_JSON_OBJECT_TYPE_STRING)
 *     {
 *       const char* name = cardano_json_object_get_string(value, NULL);
 *       printf("Name: %s\n", name);
 *     }
 *
 *     // Release the value when done
 *     cardano_json_object_unref(&value);
 *   }
 *   else
 *   {
 *     printf("Key 'name' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_json_object_get(
  const cardano_json_object_t* json_object,
  const char*                  key,
  size_t                       size,
  cardano_json_object_t**      value);

/**
 * \brief Retrieves the value associated with a specified key in a JSON object without incrementing the reference count.
 *
 * This function allows access to the value of a property in a JSON object by specifying its key.
 * It is an extended version of \ref cardano_json_object_get that does not increment the reference count
 * of the returned value. This is useful in scenarios where the caller does not need to take ownership
 * of the returned object.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON object and must not be \c NULL.
 * \param[in] key A pointer to the key string to look up. The key does not need to be null-terminated,
 *                as its length is specified by \p size. This parameter must not be \c NULL.
 * \param[in] size The length of the key string, in bytes.
 * \param[out] value A pointer to a \ref cardano_json_object_t pointer where the value associated with
 *                   the key will be stored. The returned value does not have its reference count incremented.
 *
 * \return \c true if the key was found and \p value was successfully populated; otherwise, \c false.
 *         On failure, \p value is set to \c NULL.
 *
 * \note Unlike \ref cardano_json_object_get, this function does not increment the reference count
 *       of the returned object. The caller must ensure the lifetime of the \p json_object is valid
 *       while the returned value is in use.
 *
 * \warning Do not modify or release the returned value directly unless you first increment its reference
 *          count using \ref cardano_json_object_ref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\",\"age\":30}", 29);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* value = NULL;
 *
 *   if (cardano_json_object_get_ex(json_object, "name", 4, &value))
 *   {
 *     cardano_json_object_type_t type = cardano_json_object_get_type(value);
 *
 *     if (type == CARDANO_JSON_OBJECT_TYPE_STRING)
 *     {
 *       const char* name = cardano_json_object_get_string(value, NULL);
 *       printf("Name: %s\n", name);
 *     }
 *     else
 *     {
 *       printf("The 'name' property is not a string.\n");
 *     }
 *   }
 *   else
 *   {
 *     printf("Key 'name' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_json_object_get_ex(
  const cardano_json_object_t* json_object,
  const char*                  key,
  size_t                       size,
  cardano_json_object_t**      value);

/**
 * \brief Retrieves the number of elements in a JSON array.
 *
 * This function returns the length (number of elements) of a JSON array.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t representing a JSON array.
 *                        This parameter must not be NULL and must represent a JSON array.
 *
 * \return The number of elements in the JSON array. Returns 0 if the \p json_object is NULL,
 *         or if it is not of type \ref CARDANO_JSON_OBJECT_TYPE_ARRAY.
 *
 * \note This function is only valid for JSON arrays. If the \p json_object is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_ARRAY, the function will return 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("[1, 2, 3, 4]", 9);
 *
 * if (json_object != NULL)
 * {
 *   size_t length = cardano_json_object_array_get_length(json_object);
 *   printf("Array length: %zu\n", length);
 *
 *   for (size_t i = 0; i < length; ++i)
 *   {
 *     cardano_json_object_t* element = cardano_json_object_array_get_element_at(json_object, i);
 *     if (element != NULL)
 *     {
 *       double value = cardano_json_object_get_number(element);
 *       printf("Element %zu: %f\n", i, value);
 *       cardano_json_object_unref(&element); // Release the element when done
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve element at index %zu\n", i);
 *     }
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_json_object_array_get_length(
  const cardano_json_object_t* json_object);

/**
 * \brief Retrieves the value at a specified index in a JSON array.
 *
 * This function provides access to the value at a specific position within a JSON array.
 * The returned value has its reference count incremented, so it must be explicitly released
 * when no longer needed by calling \ref cardano_json_object_unref.
 *
 * \param[in] json_array A pointer to the \ref cardano_json_object_t to query.
 *                       This parameter must represent a JSON array and must not be NULL.
 * \param[in] index The zero-based index of the value to retrieve. The index must be less than
 *                  the number of elements in the JSON array, as determined by
 *                  \ref cardano_json_object_array_get_length.
 *
 * \return A new reference to a \ref cardano_json_object_t representing the value at the specified index,
 *         or \c NULL if the index is out of range, the \p json_array is not of type
 *         \ref CARDANO_JSON_OBJECT_TYPE_ARRAY, or any parameter is invalid.
 *
 * \note This function increments the reference count of the returned \ref cardano_json_object_t.
 *       The caller is responsible for managing the lifecycle of the returned object. To release
 *       the object, call \ref cardano_json_object_unref.
 *
 * \note This function is only valid for JSON arrays. If the \p json_array is not of type
 *       \ref CARDANO_JSON_OBJECT_TYPE_ARRAY, the function will return \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_array = cardano_json_object_parse("[\"Alice\", 30, true]", 19);
 *
 * if (json_array != NULL)
 * {
 *   size_t array_length = cardano_json_object_array_get_length(json_array);
 *   printf("Array length: %zu\n", array_length);
 *
 *   for (size_t i = 0; i < array_length; ++i)
 *   {
 *     cardano_json_object_t* element = cardano_json_object_array_get(json_array, i);
 *
 *     if (element != NULL)
 *     {
 *       cardano_json_object_type_t type = cardano_json_object_get_type(element);
 *
 *       switch (type)
 *       {
 *         case CARDANO_JSON_OBJECT_TYPE_STRING:
 *           printf("Element %zu is a string: %s\n", i, cardano_json_object_get_string(element, NULL));
 *           break;
 *         case CARDANO_JSON_OBJECT_TYPE_NUMBER:
 *           printf("Element %zu is a number.\n", i);
 *           break;
 *         case CARDANO_JSON_OBJECT_TYPE_BOOL:
 *           printf("Element %zu is a boolean.\n", i);
 *           break;
 *         default:
 *           printf("Element %zu is of unknown type.\n", i);
 *           break;
 *       }
 *
 *       // Release the element when done
 *       cardano_json_object_unref(&element);
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve element at index %zu\n", i);
 *     }
 *   }
 *
 *   cardano_json_object_unref(&json_array); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON array.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_object_t* cardano_json_object_array_get(
  const cardano_json_object_t* json_array,
  size_t                       index);

/**
 * \brief Retrieves the value at a specified index in a JSON array without incrementing the reference count.
 *
 * This function provides access to the value at a specific position within a JSON array.
 * It is an extended version of \ref cardano_json_object_array_get that does not increment the
 * reference count of the returned value. This is useful in scenarios where ownership of the
 * returned object is not required.
 *
 * \param[in] json_array A pointer to the \ref cardano_json_object_t representing a JSON array.
 *                       This parameter must not be \c NULL and must represent a JSON array.
 * \param[in] index The zero-based index of the value to retrieve. The index must be less than
 *                  the number of elements in the JSON array, as determined by
 *                  \ref cardano_json_object_array_get_length.
 *
 * \return A pointer to the \ref cardano_json_object_t representing the value at the specified index,
 *         or \c NULL if:
 *         - The \p index is out of range.
 *         - The \p json_array is not of type \ref CARDANO_JSON_OBJECT_TYPE_ARRAY.
 *         - The \p json_array is \c NULL.
 *
 * \note Unlike \ref cardano_json_object_array_get, this function does not increment the reference count
 *       of the returned object. The caller must ensure the lifetime of the returned object is valid for
 *       the duration of its usage.
 *
 * \warning Avoid modifying or releasing the returned object directly. If you need to take ownership
 *          of the returned object, use \ref cardano_json_object_ref to increment its reference count.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_array = cardano_json_object_parse("[\"Alice\", 30, true]", 19);
 *
 * if (json_array != NULL)
 * {
 *   size_t array_length = cardano_json_object_array_get_length(json_array);
 *   printf("Array length: %zu\n", array_length);
 *
 *   for (size_t i = 0; i < array_length; ++i)
 *   {
 *     cardano_json_object_t* element = cardano_json_object_array_get_ex(json_array, i);
 *
 *     if (element != NULL)
 *     {
 *       cardano_json_object_type_t type = cardano_json_object_get_type(element);
 *
 *       switch (type)
 *       {
 *         case CARDANO_JSON_OBJECT_TYPE_STRING:
 *           printf("Element %zu is a string: %s\n", i, cardano_json_object_get_string(element, NULL));
 *           break;
 *         case CARDANO_JSON_OBJECT_TYPE_NUMBER:
 *           printf("Element %zu is a number.\n", i);
 *           break;
 *         case CARDANO_JSON_OBJECT_TYPE_BOOLEAN:
 *           printf("Element %zu is a boolean.\n", i);
 *           break;
 *         default:
 *           printf("Element %zu is of unknown type.\n", i);
 *           break;
 *       }
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve element at index %zu\n", i);
 *     }
 *   }
 *
 *   cardano_json_object_unref(&json_array); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON array.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_json_object_t* cardano_json_object_array_get_ex(
  const cardano_json_object_t* json_array,
  size_t                       index);

/**
 * \brief Retrieves the string value from a JSON object.
 *
 * This function provides access to the string value of a JSON object. The caller can retrieve the
 * string along with its length. The returned string points to internal storage and must not be
 * modified or freed by the caller.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON string and must not be NULL.
 * \param[out] string_length A reference to a \c size_t variable where the length of the string will
 *                           be stored. If the parameter is NULL, the length will not be returned.
 *
 * \return A pointer to the string value, or \c NULL if the \p json_object is not of type
 *         \ref CARDANO_JSON_OBJECT_TYPE_STRING or any parameter is invalid.
 *
 * \note The returned string is a pointer to the internal storage of the JSON object. The caller must
 *       not modify or free this string. If the JSON object is released or modified, the pointer may
 *       become invalid.
 *
 * \note This function is only valid for JSON objects of type \ref CARDANO_JSON_OBJECT_TYPE_STRING. If the
 *       \p json_object is of a different type, the function will return \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"name\":\"Alice\"}", 16);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* name_value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "name", 4, &name_value))
 *   {
 *     size_t string_length = 0;
 *     const char* name = cardano_json_object_get_string(name_value, &string_length);
 *
 *     if (name != NULL)
 *     {
 *       printf("Name: %.*s\n", (int)string_length, name);
 *     }
 *     else
 *     {
 *       printf("The 'name' property is not a string.\n");
 *     }
 *
 *     cardano_json_object_unref(&name_value); // Release the reference
 *   }
 *   else
 *   {
 *     printf("Key 'name' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_json_object_get_string(
  const cardano_json_object_t* json_object,
  size_t*                      string_length);

/**
 * \brief Determines if a JSON object represents a negative number.
 *
 * This function checks whether the given \ref cardano_json_object_t represents a numeric value
 * that is negative. A negative number is defined as any numeric value less than zero,
 * regardless of whether it is an integer or a real number.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query. This parameter must represent
 *                        a JSON number and must not be \c NULL.
 *
 * \return \c true if the JSON object represents a negative number; otherwise, \c false. If the \p json_object
 *         is \c NULL, or if the JSON object is not a number, the function will return \c false.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"temperature\": -15.5, \"count\": 42}", 38);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "temperature", 11, &value))
 *   {
 *     bool is_negative = cardano_json_object_get_is_negative_number(value);
 *     printf("\"temperature\" is negative: %s\n", is_negative ? "true" : "false");
 *
 *     cardano_json_object_unref(&value);
 *   }
 *
 *   if (cardano_json_object_get(json_object, "count", 5, &value))
 *   {
 *     bool is_negative = cardano_json_object_get_is_negative_number(value);
 *     printf("\"count\" is negative: %s\n", is_negative ? "true" : "false");
 *
 *     cardano_json_object_unref(&value);
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_json_object_get_is_negative_number(const cardano_json_object_t* json_object);

/**
 * \brief Determines if a JSON object represents a real (floating-point) number.
 *
 * This function checks whether the given \ref cardano_json_object_t represents a real number. A real number
 * in JSON is a numeric value that includes a fractional part (e.g., `3.14`, `-0.01`).
 *
 * This function is specifically designed to distinguish between integral and real numbers within the JSON
 * context. While JSON itself does not have distinct types for integers and floating-point numbers, this
 * function interprets numbers with a fractional component or numbers expressed in scientific notation
 * as real numbers.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query. This parameter must represent
 *                        a JSON number and must not be \c NULL.
 *
 * \return \c true if the JSON object represents a real number; otherwise, \c false. If the \p json_object
 *         is \c NULL, the function will also return \c false.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"pi\": 3.14159, \"count\": 42}", 29);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "pi", 2, &value))
 *   {
 *     bool is_real = cardano_json_object_get_is_real_number(value);
 *     printf("\"pi\" is a real number: %s\n", is_real ? "true" : "false");
 *
 *     cardano_json_object_unref(&value);
 *   }
 *
 *   if (cardano_json_object_get(json_object, "count", 5, &value))
 *   {
 *     bool is_real = cardano_json_object_get_is_real_number(value);
 *     printf("\"count\" is a real number: %s\n", is_real ? "true" : "false");
 *
 *     cardano_json_object_unref(&value);
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_json_object_get_is_real_number(const cardano_json_object_t* json_object);

/**
 * \brief Retrieves an unsigned integer value from a JSON object.
 *
 * This function provides access to the unsigned integer value of a JSON object. It ensures
 * that the value is a valid unsigned integer and retrieves it into the specified output parameter.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON number and must not be NULL.
 * \param[out] value A pointer to a \c uint64_t variable where the unsigned integer value will
 *                   be stored. This parameter must not be NULL.
 *
 * \return A status code indicating the result of the operation. If the operation is successful,
 *        the function returns \ref CARDANO_SUCCESS.
 *
 * \note This function is only valid for JSON objects of type \ref CARDANO_JSON_OBJECT_TYPE_NUMBER.
 *       If the \p json_object is of a different type the function will return \ref CARDANO_ERROR_JSON_TYPE_MISMATCH.
 *
 * \warning If the value exceeds the range of a \c uint64_t or is not a valid number, the function
 *          will return an appropriate error code, and \p value will remain unmodified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"age\":30}", 10);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* age_value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "age", 3, &age_value))
 *   {
 *     uint64_t age = 0;
 *     cardano_error_t result = cardano_json_object_get_uint(age_value, &age);
 *
 *     if (result == CARDANO_SUCCESS)
 *     {
 *       printf("Age: %llu\n", (unsigned long long)age);
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve 'age' as an unsigned integer. Error: %d\n", result);
 *     }
 *
 *     cardano_json_object_unref(&age_value); // Release the reference
 *   }
 *   else
 *   {
 *     printf("Key 'age' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_object_get_uint(
  const cardano_json_object_t* json_object,
  uint64_t*                    value);

/**
 * \brief Retrieves a signed integer value from a JSON object.
 *
 * This function provides access to the signed integer value of a JSON object. It ensures
 * that the value is a valid signed integer and retrieves it into the specified output parameter.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON number and must not be NULL.
 * \param[out] value A pointer to an \c int64_t variable where the signed integer value will
 *                   be stored. This parameter must not be NULL.
 *
 * \return A status code indicating the result of the operation. If the operation is successful,
 *       the function returns \ref CARDANO_SUCCESS.
 *
 * \note This function is only valid for JSON objects of type \ref CARDANO_JSON_OBJECT_TYPE_NUMBER.
 *       If the \p json_object is of a different type, the function will return
 *       \ref CARDANO_ERROR_JSON_TYPE_MISMATCH.
 *
 * \warning If the value exceeds the range of an \c int64_t or is not a valid number, the function
 *          will return an appropriate error code, and \p value will remain unmodified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"age\":-25}", 12);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* age_value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "age", 3, &age_value))
 *   {
 *     int64_t age = 0;
 *     cardano_error_t result = cardano_json_object_get_signed_int(age_value, &age);
 *
 *     if (result == CARDANO_SUCCESS)
 *     {
 *       printf("Age: %lld\n", (long long)age);
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve 'age' as a signed integer. Error: %d\n", result);
 *     }
 *
 *     cardano_json_object_unref(&age_value); // Release the reference
 *   }
 *   else
 *   {
 *     printf("Key 'age' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_object_get_signed_int(
  const cardano_json_object_t* json_object,
  int64_t*                     value);

/**
 * \brief Retrieves a double-precision floating-point value from a JSON object.
 *
 * This function provides access to the double value of a JSON object. It ensures
 * that the value is a valid number and retrieves it into the specified output parameter.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON number and must not be NULL.
 * \param[out] value A pointer to a \c double variable where the floating-point value will
 *                   be stored. This parameter must not be NULL.
 *
 * \return A status code indicating the result of the operation. If the operation is successful,
 *       the function returns \ref CARDANO_SUCCESS.
 *
 * \note This function is only valid for JSON objects of type \ref CARDANO_JSON_OBJECT_TYPE_NUMBER.
 *       If the \p json_object is of a different type, the function will return
 *       \ref CARDANO_ERROR_JSON_TYPE_MISMATCH.
 *
 * \warning If the value is not a valid floating-point number or cannot be represented as a \c double,
 *          the function will return an appropriate error code, and \p value will remain unmodified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"height\":5.75}", 17);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* height_value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "height", 6, &height_value))
 *   {
 *     double height = 0.0;
 *     cardano_error_t result = cardano_json_object_get_double(height_value, &height);
 *
 *     if (result == CARDANO_SUCCESS)
 *     {
 *       printf("Height: %f\n", height);
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve 'height' as a double. Error: %d\n", result);
 *     }
 *
 *     cardano_json_object_unref(&height_value); // Release the reference
 *   }
 *   else
 *   {
 *     printf("Key 'height' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_object_get_double(
  const cardano_json_object_t* json_object,
  double*                      value);

/**
 * \brief Retrieves a boolean value from a JSON object.
 *
 * This function provides access to the boolean value of a JSON object. It ensures
 * that the value is a valid boolean and retrieves it into the specified output parameter.
 *
 * \param[in] json_object A pointer to the \ref cardano_json_object_t to query.
 *                        This parameter must represent a JSON boolean and must not be NULL.
 * \param[out] value A pointer to a \c bool variable where the boolean value will
 *                   be stored. This parameter must not be NULL.
 *
 * \return A status code indicating the result of the operation. If the operation is successful,
 *      the function returns \ref CARDANO_SUCCESS.
 *
 * \note This function is only valid for JSON objects of type \ref CARDANO_JSON_OBJECT_TYPE_BOOLEAN.
 *       If the \p json_object is of a different type, the function will return
 *       \ref CARDANO_ERROR_JSON_TYPE_MISMATCH.
 *
 * \warning If the value is not a valid boolean, the function will return an appropriate
 *          error code, and \p value will remain unmodified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* json_object = cardano_json_object_parse("{\"active\":true}", 15);
 *
 * if (json_object != NULL)
 * {
 *   cardano_json_object_t* active_value = NULL;
 *
 *   if (cardano_json_object_get(json_object, "active", 6, &active_value))
 *   {
 *     bool is_active = false;
 *     cardano_error_t result = cardano_json_object_get_boolean(active_value, &is_active);
 *
 *     if (result == CARDANO_SUCCESS)
 *     {
 *       printf("Active: %s\n", is_active ? "true" : "false");
 *     }
 *     else
 *     {
 *       printf("Failed to retrieve 'active' as a boolean. Error: %d\n", result);
 *     }
 *
 *     cardano_json_object_unref(&active_value); // Release the reference
 *   }
 *   else
 *   {
 *     printf("Key 'active' not found.\n");
 *   }
 *
 *   cardano_json_object_unref(&json_object); // Clean up
 * }
 * else
 * {
 *   printf("Failed to parse JSON data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_json_object_get_boolean(
  const cardano_json_object_t* json_object,
  bool*                        value);

/**
 * \brief Decrements the reference count of a JSON object object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_json_object_t object
 * by decreasing its reference count. When the reference count reaches zero, the JSON object is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] json_object A pointer to the pointer of the JSON object object. This double
 *             indirection allows the function to set the caller's pointer to
 *             NULL, avoiding dangling pointer issues after the object has been
 *             freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_json_object_t* object = cardano_json_object_new();
 *
 * // Perform operations with the object...
 *
 * cardano_json_object_unref(&object);
 * // At this point, object is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_json_object_unref, the pointer to the \ref cardano_json_object_t object
 *    will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_json_object_unref(cardano_json_object_t** json_object);

/**
 * \brief Increases the reference count of the cardano_json_object_t object.
 *
 * This function is used to manually increment the reference count of a JSON object
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_json_object_unref.
 *
 * \param json_object A pointer to the JSON object object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming object is a previously created JSON object object
 *
 * cardano_json_object_ref(object);
 *
 * // Now object can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_json_object_ref there is a corresponding
 * call to \ref cardano_json_object_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_json_object_ref(cardano_json_object_t* json_object);

/**
 * \brief Retrieves the current reference count of the cardano_json_object_t object.
 *
 * This function returns the number of active references to a JSON object object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_json_object_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param json_object A pointer to the JSON object object whose reference count is queried.
 *        The object must not be NULL.
 *
 * \return The number of active references to the specified JSON object object. If the object
 * is properly managed (i.e., every \ref cardano_json_object_ref call is matched with a
 * \ref cardano_json_object_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming object is a previously created JSON object object
 *
 * size_t ref_count = cardano_json_object_refcount(object);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_json_object_refcount(const cardano_json_object_t* json_object);

/**
 * \brief Sets the last error message for a given JSON object object.
 *
 * Records an error message in the JSON object's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] object A pointer to the \ref cardano_json_object_t instance whose last error message is
 *          to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the object's
 *        last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_json_object_set_last_error(cardano_json_object_t* object, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_json_object_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] object A pointer to the \ref cardano_json_object_t instance whose last error
 *          message is to be retrieved. If the object is NULL, the function
 *          returns a generic error message indicating the null object.
 *
 * \return A pointer to a null-terminated string containing the last error
 *      message for the specified object. If the object is NULL, "Object is NULL."
 *      is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *    must not be modified by the caller. The string remains valid until the
 *    next call to \ref cardano_json_object_set_last_error for the same object, or until
 *    the object is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_json_object_get_last_error(const cardano_json_object_t* object);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_H