/**
 * \file bigint.h
 *
 * \author luisd.bianchi
 * \date   Jun 05, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BIGINT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BIGINT_H

/* INCLUDES ******************************************************************/

#include <cardano/common/byte_order.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a large numeric value.
 *
 * The \ref cardano_bigint_t type is used for representing numeric values that are too large to be
 * represented by the standard numeric primitive types, such as int64_t or uint64_t.
 */
typedef struct cardano_bigint_t cardano_bigint_t;

/**
 * \brief Creates a copy of the given bigint object.
 *
 * This function duplicates the provided \ref cardano_bigint_t object.
 *
 * \param[in] bigint A constant pointer to the original \ref cardano_bigint_t object.
 * \param[out] clone A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t copy.
 *
 * \return \ref CARDANO_SUCCESS if the copy was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 * cardano_bigint_t* clone = NULL;
 * cardano_error_t result = cardano_bigint_from(original, &clone);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the clone
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&clone);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_clone(const cardano_bigint_t* bigint, cardano_bigint_t** clone);

/**
 * \brief Creates a bigint from a string representation.
 *
 * This function initializes a \ref cardano_bigint_t object from the given string.
 *
 * \param[in] string A pointer to the string containing the representation of the bigint.
 * \param[in] size The size of the string.
 * \param[in] base The base of the string representation (e.g., 10 for decimal, 16 for hexadecimal).
 * \param[out] bigint A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t object.
 *
 * \return \ref CARDANO_SUCCESS if the bigint was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* str = "1234567892356760";
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_string(str, strlen(str), 10, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bigint
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_from_string(const char* string, size_t size, int32_t base, cardano_bigint_t** bigint);

/**
 * \brief Calculates the size of the string representation of the bigint.
 *
 * This function calculates the size of the string needed to represent the given \ref cardano_bigint_t object
 * in the specified base. The size includes space for the null terminator.
 *
 * \param[in] bigint A pointer to the \ref cardano_bigint_t object.
 * \param[in] base The base to use for the string representation (e.g., 10 for decimal, 16 for hexadecimal).
 *
 * \return The size of the string representation, including the null terminator.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_string("1234567892356760", 16, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   size_t str_size = cardano_bigint_get_string_size(bigint, 16);
 *   char* str = (char*)malloc(str_size);
 *
 *   if (str != NULL)
 *   {
 *     result = cardano_bigint_to_string(bigint, str, str_size);
 *
 *     if (result == CARDANO_SUCCESS)
 *     {
 *       printf("Bigint string: %s\n", str);
 *     }
 *
 *     free(str);
 *   }
 *
 *   cardano_bigint_unref(&bigint);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_bigint_get_string_size(cardano_bigint_t* bigint, int32_t base);

/**
 * \brief Creates a bigint from an integer value.
 *
 * This function initializes a \ref cardano_bigint_t object from the given integer value.
 *
 * \param[in] value The integer value to convert to a bigint.
 * \param[out] bigint A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t object.
 *
 * \return \ref CARDANO_SUCCESS if the bigint was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * int64_t value = 12345;
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_int(value, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bigint
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_from_int(int64_t value, cardano_bigint_t** bigint);

/**
 * \brief Creates a bigint from an unsigned integer value.
 *
 * This function initializes a \ref cardano_bigint_t object from the given unsigned integer value.
 *
 * \param[in] value The unsigned integer value to convert to a bigint.
 * \param[out] bigint A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t object.
 *
 * \return \ref CARDANO_SUCCESS if the bigint was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t value = 12345;
 * cardano_bigint_t* bigint = NULL;
 *
 * cardano_error_t result = cardano_bigint_from_unsigned_int(value, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bigint
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_from_unsigned_int(uint64_t value, cardano_bigint_t** bigint);

/**
 * \brief Creates a bigint from a byte array.
 *
 * This function initializes a \ref cardano_bigint_t object from the given byte array.
 * The function also allows specifying the byte order of the input data (big-endian or little-endian).
 *
 * \param[in] data A pointer to the byte array containing the representation of the bigint.
 * \param[in] size The size of the byte array.
 * \param[in] byte_order An enum value indicating the byte order of the input data.
 *                       Use \ref CARDANO_BYTE_ORDER_BIG_ENDIAN for big-endian and \ref CARDANO_BYTE_ORDER_LITTLE_ENDIAN for little-endian.
 * \param[out] bigint A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t object.
 *
 * \return \ref CARDANO_SUCCESS if the bigint was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * byte_t bytes[] = { 0x01, 0x02, 0x03 };
 * cardano_bigint_t* bigint = NULL;
 * cardano_byte_order_t byte_order = CARDANO_ENDIANNESS_LITTLE;
 *
 * cardano_error_t result = cardano_bigint_from_bytes(bytes, sizeof(bytes), byte_order, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bigint
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_from_bytes(
  const byte_t*        data,
  size_t               size,
  cardano_byte_order_t byte_order,
  cardano_bigint_t**   bigint);

/**
 * \brief Converts a bigint to its string representation.
 *
 * This function converts a \ref cardano_bigint_t object to a string.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object.
 * \param[out] string A pointer to the buffer where the string representation will be stored.
 * \param[in] size The size of the buffer.
 * \param[in] base The numerical base for the string representation. Valid values are from 2 to 36.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code indicating the failure reason.
 *
 * \note It is recommended to use \ref cardano_bigint_get_string_size to get the actual size of the string needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized with a value
 * size_t str_size = cardano_bigint_get_string_size(bigint, 10);
 * char* buffer = (char*)malloc(str_size);
 *
 * cardano_error_t result = cardano_bigint_to_string(bigint, buffer, str_size, 10);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("BigInt as string: %s\n", buffer);
 * }
 *
 * // Clean up
 * free(buffer);
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_to_string(
  const cardano_bigint_t* bigint,
  char*                   string,
  size_t                  size,
  int32_t                 base);

/**
 * \brief Converts a bigint to an integer value.
 *
 * This function converts a \ref cardano_bigint_t object to an integer value.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object.
 *
 * \return The integer value of the bigint. If the bigint is too large to fit in an int64_t, the behavior is undefined.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 * int64_t value = cardano_bigint_to_int(bigint);
 *
 * // Use the integer value
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int64_t
cardano_bigint_to_int(const cardano_bigint_t* bigint);

/**
 * \brief Converts a bigint to an unsigned integer value.
 *
 * This function converts a \ref cardano_bigint_t object to an unsigned integer value.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object.
 *
 * \return The unsigned integer representation of the \ref cardano_bigint_t object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 * uint64_t value = cardano_bigint_to_unsigned_int(bigint);
 *
 * // Use the unsigned integer value
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t
cardano_bigint_to_unsigned_int(const cardano_bigint_t* bigint);

/**
 * \brief Gets the size of the byte array needed to represent a bigint.
 *
 * This function calculates the size of the byte array required to represent the given \ref cardano_bigint_t object.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object.
 *
 * \return The size of the byte array needed to represent the bigint.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 * size_t byte_size = cardano_bigint_get_bytes_size(bigint);
 *
 * // Use the byte size value
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bigint_get_bytes_size(const cardano_bigint_t* bigint);

/**
 * \brief Converts a bigint to its byte array representation.
 *
 * This function converts a \ref cardano_bigint_t object to a byte array.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object.
 * \param[in] byte_order The byte order for the byte array (big-endian or little-endian).
 * \param[out] data A pointer to the buffer where the byte array will be stored.
 * \param[in] size The size of the buffer.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code indicating the failure reason.
 *
 * \note It is recommended to use \ref cardano_bigint_get_bytes_size to get the exact size of the byte array needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized with a value
 * size_t byte_size = cardano_bigint_get_bytes_size(bigint);
 * byte_t* buffer = (byte_t*)malloc(byte_size);
 *
 * cardano_error_t result = cardano_bigint_to_bytes(bigint, CARDANO_ENDIANNESS_LITTLE, buffer, byte_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the byte array representation
 * }
 *
 * // Clean up
 * free(buffer);
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bigint_to_bytes(
  const cardano_bigint_t* bigint,
  cardano_byte_order_t    byte_order,
  byte_t*                 data,
  size_t                  size);

/**
 * \brief Adds two bigint objects.
 *
 * This function performs addition of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* lhs_str = "123456789012345678901234567890";
 * const char* rhs_str = "987654321098765432109876543210";
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 10, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 10, &rhs);
 * cardano_error_t res_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_add(lhs, rhs, result);
 *
 *   size_t result_size = cardano_bigint_get_string_size(result, 10);
 *   char* result_str = (char*)malloc(result_size);
 *
 *   cardano_error_t str_result = cardano_bigint_to_string(result, result_str, result_size, 10);
 *
 *   if (str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Result of addition: %s\n", result_str); // Should print "1111111110111111111011111111100"
 *   }
 *
 *   free(result_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_add(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Subtracts one bigint object from another.
 *
 * This function performs subtraction of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the minuend.
 * \param[in] rhs A constant pointer to the subtrahend.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* lhs_str = "987654321098765432109876543210";
 * const char* rhs_str = "123456789012345678901234567890";
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 10, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 10, &rhs);
 * cardano_error_t res_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_subtract(lhs, rhs, result);
 *
 *   size_t result_size = cardano_bigint_get_string_size(result, 10);
 *   char* result_str = (char*)malloc(result_size);
 *
 *   cardano_error_t str_result = cardano_bigint_to_string(result, result_str, result_size, 10);
 *
 *   if (str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Result of subtraction: %s\n", result_str); // Should print "864197532086419753208641975320"
 *   }
 *
 *   free(result_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_subtract(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Multiplies two bigint objects.
 *
 * This function performs multiplication of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* lhs_str = "123456789";
 * const char* rhs_str = "987654321";
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 10, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 10, &rhs);
 * cardano_error_t res_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_multiply(lhs, rhs, result);
 *
 *   size_t result_size = cardano_bigint_get_string_size(result, 10);
 *   char* result_str = (char*)malloc(result_size);
 *
 *   cardano_error_t str_result = cardano_bigint_to_string(result, result_str, result_size, 10);
 *
 *   if (str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Result of multiplication: %s\n", result_str); // Should print "121932631112635269"
 *   }
 *
 *   free(result_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_multiply(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Divides one bigint object by another.
 *
 * This function performs division of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] dividend A constant pointer to the dividend.
 * \param[in] divisor A constant pointer to the divisor.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p dividend, \p divisor, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* dividend_str = "987654321";
 * const char* divisor_str = "123456789";
 *
 * cardano_bigint_t* dividend = NULL;
 * cardano_bigint_t* divisor = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t div_result = cardano_bigint_from_string(dividend_str, strlen(dividend_str), 10, &dividend);
 * cardano_error_t divs_result = cardano_bigint_from_string(divisor_str, strlen(divisor_str), 10, &divisor);
 * cardano_error_t res_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (div_result == CARDANO_SUCCESS && divs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_divide(dividend, divisor, result);
 *
 *   size_t result_size = cardano_bigint_get_string_size(result, 10);
 *   char* result_str = (char*)malloc(result_size);
 *
 *   cardano_error_t str_result = cardano_bigint_to_string(result, result_str, result_size, 10);
 *
 *   if (str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Result of division: %s\n", result_str); // Should print "8"
 *   }
 *
 *   free(result_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&dividend);
 * cardano_bigint_unref(&divisor);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_divide(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       result);

/**
 * \brief Divides one bigint object by another and computes the remainder.
 *
 * This function performs division of two \ref cardano_bigint_t objects and stores the quotient in one bigint and the remainder in another bigint.
 *
 * \param[in] dividend A constant pointer to the dividend.
 * \param[in] divisor A constant pointer to the divisor.
 * \param[out] quotient A pointer to the pre-initialized \ref cardano_bigint_t object where the quotient will be stored.
 *                      This is not a factory method; \p quotient must be pre-initialized and cannot be NULL.
 * \param[out] reminder A pointer to the pre-initialized \ref cardano_bigint_t object where the remainder will be stored.
 *                      This is not a factory method; \p reminder must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p dividend, \p divisor, \p quotient, or \p reminder) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* dividend_str = "987654321";
 * const char* divisor_str = "123456789";
 *
 * cardano_bigint_t* dividend = NULL;
 * cardano_bigint_t* divisor = NULL;
 * cardano_bigint_t* quotient = NULL;
 * cardano_bigint_t* reminder = NULL;
 *
 * cardano_error_t div_result = cardano_bigint_from_string(dividend_str, strlen(dividend_str), 10, &dividend);
 * cardano_error_t divs_result = cardano_bigint_from_string(divisor_str, strlen(divisor_str), 10, &divisor);
 * cardano_error_t quo_result = cardano_bigint_from_int(0, &quotient); // Initialize quotient with 0
 * cardano_error_t rem_result = cardano_bigint_from_int(0, &reminder); // Initialize reminder with 0
 *
 * if (div_result == CARDANO_SUCCESS && divs_result == CARDANO_SUCCESS && quo_result == CARDANO_SUCCESS && rem_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_divide_and_reminder(dividend, divisor, quotient, reminder);
 *
 *   size_t quotient_size = cardano_bigint_get_string_size(quotient, 10);
 *   char* quotient_str = (char*)malloc(quotient_size);
 *
 *   cardano_error_t quo_str_result = cardano_bigint_to_string(quotient, quotient_str, quotient_size, 10);
 *
 *   size_t reminder_size = cardano_bigint_get_string_size(reminder, 10);
 *   char* reminder_str = (char*)malloc(reminder_size);
 *
 *   cardano_error_t rem_str_result = cardano_bigint_to_string(reminder, reminder_str, reminder_size, 10);
 *
 *   if (quo_str_result == CARDANO_SUCCESS && rem_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Quotient: %s\n", quotient_str); // Should print "8"
 *     printf("Reminder: %s\n", reminder_str); // Should print "9"
 *   }
 *
 *   free(quotient_str);
 *   free(reminder_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&dividend);
 * cardano_bigint_unref(&divisor);
 * cardano_bigint_unref(&quotient);
 * cardano_bigint_unref(&reminder);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_divide_and_reminder(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       quotient,
  cardano_bigint_t*       reminder);

/**
 * \brief Computes the remainder of dividing one bigint object by another.
 *
 * This function performs modulo operation on two \ref cardano_bigint_t objects and stores the remainder in a third bigint.
 *
 * \param[in] dividend A constant pointer to the dividend.
 * \param[in] divisor A constant pointer to the divisor.
 * \param[out] reminder A pointer to the pre-initialized \ref cardano_bigint_t object where the remainder will be stored.
 *                      This is not a factory method; \p reminder must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p dividend, \p divisor, or \p reminder) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* dividend_str = "987654321";
 * const char* divisor_str = "123456789";
 *
 * cardano_bigint_t* dividend = NULL;
 * cardano_bigint_t* divisor = NULL;
 * cardano_bigint_t* reminder = NULL;
 *
 * cardano_error_t div_result = cardano_bigint_from_string(dividend_str, strlen(dividend_str), 10, &dividend);
 * cardano_error_t divs_result = cardano_bigint_from_string(divisor_str, strlen(divisor_str), 10, &divisor);
 * cardano_error_t rem_result = cardano_bigint_from_int(0, &reminder); // Initialize reminder with 0
 *
 * if (div_result == CARDANO_SUCCESS && divs_result == CARDANO_SUCCESS && rem_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_reminder(dividend, divisor, reminder);
 *
 *   size_t reminder_size = cardano_bigint_get_string_size(reminder, 10);
 *   char* reminder_str = (char*)malloc(reminder_size);
 *
 *   cardano_error_t rem_str_result = cardano_bigint_to_string(reminder, reminder_str, reminder_size, 10);
 *
 *   if (rem_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Reminder: %s\n", reminder_str); // Should print "9"
 *   }
 *
 *   free(reminder_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&dividend);
 * cardano_bigint_unref(&divisor);
 * cardano_bigint_unref(&reminder);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_reminder(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       reminder);

/**
 * \brief Computes the absolute value of a bigint.
 *
 * This function computes the absolute value of the given \ref cardano_bigint_t object and stores the result in another bigint.
 *
 * \param[in] bignum A constant pointer to the bigint object.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p bignum or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* bignum_str = "-123456789";
 *
 * cardano_bigint_t* bignum = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t bignum_result = cardano_bigint_from_string(bignum_str, strlen(bignum_str), 10, &bignum);
 * cardano_error_t abs_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (bignum_result == CARDANO_SUCCESS && abs_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_abs(bignum, result);
 *
 *   size_t abs_size = cardano_bigint_get_string_size(result, 10);
 *   char* abs_str = (char*)malloc(abs_size);
 *
 *   cardano_error_t abs_str_result = cardano_bigint_to_string(result, abs_str, abs_size, 10);
 *
 *   if (abs_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Absolute value: %s\n", abs_str); // Should print "123456789"
 *   }
 *
 *   free(abs_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bignum);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_abs(const cardano_bigint_t* bignum, cardano_bigint_t* result);

/**
 * \brief Computes the greatest common divisor (GCD) of two bigints.
 *
 * This function computes the GCD of the given \ref cardano_bigint_t objects and stores the result in another bigint.
 *
 * \param[in] lhs A constant pointer to the first bigint object.
 * \param[in] rhs A constant pointer to the second bigint object.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* lhs_str = "48";
 * const char* rhs_str = "18";
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 10, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 10, &rhs);
 * cardano_error_t gcd_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && gcd_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_gcd(lhs, rhs, result);
 *
 *   size_t gcd_size = cardano_bigint_get_string_size(result, 10);
 *   char* gcd_str = (char*)malloc(gcd_size);
 *
 *   cardano_error_t gcd_str_result = cardano_bigint_to_string(result, gcd_str, gcd_size, 10);
 *
 *   if (gcd_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("GCD: %s\n", gcd_str); // Should print "6"
 *   }
 *
 *   free(gcd_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_gcd(const cardano_bigint_t* lhs, const cardano_bigint_t* rhs, cardano_bigint_t* result);

/**
 * \brief Negates a bigint.
 *
 * This function computes the negation of the given \ref cardano_bigint_t object and stores the result in another bigint.
 *
 * \param[in] bignum A constant pointer to the bigint object.
 * \param[out] result A pointer to the pre-initialized \ref cardano_bigint_t object where the result will be stored.
 *                    This is not a factory method; \p result must be pre-initialized and cannot be NULL.
 *
 * \remark If any of the pointers (\p bignum or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * const char* bignum_str = "12";
 *
 * cardano_bigint_t* bignum = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t bignum_result = cardano_bigint_from_string(bignum_str, strlen(bignum_str), 10, &bignum);
 * cardano_error_t negate_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (bignum_result == CARDANO_SUCCESS && negate_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_negate(bignum, result);
 *
 *   size_t neg_size = cardano_bigint_get_string_size(result, 10);
 *   char* neg_str = (char*)malloc(neg_size);
 *
 *   cardano_error_t neg_str_result = cardano_bigint_to_string(result, neg_str, neg_size, 10);
 *
 *   if (neg_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Negated value: %s\n", neg_str); // Should print "-12"
 *   }
 *
 *   free(neg_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bignum);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_negate(const cardano_bigint_t* bignum, cardano_bigint_t* result);

/**
 * \brief Computes the signum function of a bigint.
 *
 * This function returns the signum of the given \ref cardano_bigint_t object. The signum function returns:
 * -1 if the bigint is negative,
 *  0 if the bigint is zero,
 *  1 if the bigint is positive.
 *
 * \param[in] bignum A constant pointer to the bigint object.
 *
 * \return The signum of the bigint as an \c int32_t.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bignum = ...; // Assume this is initialized
 * int32_t sign = cardano_bigint_signum(bignum);
 * // Use the sign
 * // Clean up
 * cardano_bigint_unref(&bignum);
 * \endcode
 */
CARDANO_EXPORT
int32_t cardano_bigint_signum(const cardano_bigint_t* bignum);

/**
 * \brief Computes the remainder of dividing one bigint object by another.
 *
 * This function performs the modulo operation on two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the dividend.
 * \param[in] rhs A constant pointer to the divisor.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * int64_t lhs_value = 10;
 * int64_t rhs_value = 3;
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_int(lhs_value, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_int(rhs_value, &rhs);
 * cardano_error_t mod_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && mod_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_mod(lhs, rhs, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 10);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 10);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Modulo result: %s\n", res_str); // Should print "1" for 10 % 3
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_mod(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Computes the result of raising a bigint to the power of another bigint modulo a third bigint.
 *
 * This function computes the modular exponentiation of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] base A constant pointer to the base bigint.
 * \param[in] exponent A constant pointer to the exponent bigint.
 * \param[in] modulus A constant pointer to the modulus bigint.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p base, \p exponent, \p modulus, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * int64_t base_value = 4;
 * int64_t exponent_value = 13;
 * int64_t modulus_value = 497;
 *
 * cardano_bigint_t* base = NULL;
 * cardano_bigint_t* exponent = NULL;
 * cardano_bigint_t* modulus = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t base_result = cardano_bigint_from_int(base_value, &base);
 * cardano_error_t exponent_result = cardano_bigint_from_int(exponent_value, &exponent);
 * cardano_error_t modulus_result = cardano_bigint_from_int(modulus_value, &modulus);
 * cardano_error_t result_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (base_result == CARDANO_SUCCESS && exponent_result == CARDANO_SUCCESS &&
 *     modulus_result == CARDANO_SUCCESS && result_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_mod_pow(base, exponent, modulus, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 10);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 10);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Modular exponentiation result: %s\n", res_str); // Should print "445" for 4^13 % 497
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&base);
 * cardano_bigint_unref(&exponent);
 * cardano_bigint_unref(&modulus);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_mod_pow(
  const cardano_bigint_t* base,
  const cardano_bigint_t* exponent,
  const cardano_bigint_t* modulus,
  cardano_bigint_t*       result);

/**
 * \brief Computes the modular multiplicative inverse of a bigint.
 *
 * This function computes the modular multiplicative inverse of a \ref cardano_bigint_t object and stores the result in another bigint.
 *
 * \param[in] bignum A constant pointer to the bigint whose modular inverse is to be computed.
 * \param[in] modulus A constant pointer to the modulus bigint.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p bignum, \p modulus, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * int64_t bignum_value = 3;
 * int64_t modulus_value = 11;
 *
 * cardano_bigint_t* bignum = NULL;
 * cardano_bigint_t* modulus = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t bignum_result = cardano_bigint_from_int(bignum_value, &bignum);
 * cardano_error_t modulus_result = cardano_bigint_from_int(modulus_value, &modulus);
 * cardano_error_t result_result = cardano_bigint_from_int(0, &result); // Initialize result with 0
 *
 * if (bignum_result == CARDANO_SUCCESS && modulus_result == CARDANO_SUCCESS && result_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_mod_inverse(bignum, modulus, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 10);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 10);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Modular inverse result: %s\n", res_str); // Should print "4" for 3^-1 % 11
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bignum);
 * cardano_bigint_unref(&modulus);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_mod_inverse(
  const cardano_bigint_t* bignum,
  const cardano_bigint_t* modulus,
  cardano_bigint_t*       result);

/**
 * \brief Performs bitwise AND operation on two bigint objects.
 *
 * This function performs bitwise AND of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect. The \p result pointer must be pre-initialized and cannot be NULL.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * const char* lhs_str = "101010"; // Binary: 101010
 * const char* rhs_str = "110011"; // Binary: 110011
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 2, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 2, &rhs);
 * cardano_error_t res_result = cardano_bigint_from_string("0", 1, 2, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_and(lhs, rhs, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 2);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 2);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Bitwise AND result: %s\n", res_str); // Should print "100010" for 101010 & 110011
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_and(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Performs bitwise OR operation on two bigint objects.
 *
 * This function performs bitwise OR of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect. The \p result pointer must be pre-initialized and cannot be NULL.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * const char* lhs_str = "101010"; // Binary: 101010
 * const char* rhs_str = "110011"; // Binary: 110011
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 2, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 2, &rhs);
 * cardano_error_t res_result = cardano_bigint_from_string("0", 1, 2, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_or(lhs, rhs, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 2);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 2);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Bitwise OR result: %s\n", res_str); // Should print "111011" for 101010 | 110011
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_or(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Performs bitwise XOR operation on two bigint objects.
 *
 * This function performs bitwise XOR of two \ref cardano_bigint_t objects and stores the result in a third bigint.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect. The \p result pointer must be pre-initialized and cannot be NULL.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * const char* lhs_str = "101010"; // Binary: 101010
 * const char* rhs_str = "110011"; // Binary: 110011
 *
 * cardano_bigint_t* lhs = NULL;
 * cardano_bigint_t* rhs = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t lhs_result = cardano_bigint_from_string(lhs_str, strlen(lhs_str), 2, &lhs);
 * cardano_error_t rhs_result = cardano_bigint_from_string(rhs_str, strlen(rhs_str), 2, &rhs);
 * cardano_error_t res_result = cardano_bigint_from_string("0", 1, 2, &result); // Initialize result with 0
 *
 * if (lhs_result == CARDANO_SUCCESS && rhs_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_xor(lhs, rhs, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 2);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 2);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Bitwise XOR result: %s\n", res_str); // Should print "011001" for 101010 ^ 110011
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_xor(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result);

/**
 * \brief Performs bitwise NOT operation on a bigint object.
 *
 * This function performs bitwise NOT of a \ref cardano_bigint_t object and stores the result in another bigint.
 *
 * \param[in] bigint A constant pointer to the operand.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p bigint or \p result) is NULL, the operation has no effect. The \p result pointer must be pre-initialized and cannot be NULL.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the big integers
 * const char* bigint_str = "101010"; // Binary: 101010
 *
 * cardano_bigint_t* bigint = NULL;
 * cardano_bigint_t* result = NULL;
 *
 * cardano_error_t bigint_result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &bigint);
 * cardano_error_t res_result = cardano_bigint_from_string("0", 1, 2, &result); // Initialize result with 0
 *
 * if (bigint_result == CARDANO_SUCCESS && res_result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_not(bigint, result);
 *
 *   size_t res_size = cardano_bigint_get_string_size(result, 2);
 *   char* res_str = (char*)malloc(res_size);
 *
 *   cardano_error_t res_str_result = cardano_bigint_to_string(result, res_str, res_size, 2);
 *
 *   if (res_str_result == CARDANO_SUCCESS)
 *   {
 *     printf("Bitwise NOT result: %s\n", res_str); // Example result depends on the bit length
 *   }
 *
 *   free(res_str);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_not(
  const cardano_bigint_t* bigint,
  cardano_bigint_t*       result);

/**
 * \brief Tests whether a specific bit is set in a bigint object.
 *
 * This function checks whether the bit at the specified position in a \ref cardano_bigint_t object is set.
 *
 * \param[in] bigint A constant pointer to the bigint object to be tested.
 * \param[in] n The index of the bit to be tested (0-based).
 *
 * \return \c true if the bit at the specified position is set, \c false otherwise.
 *
 * \remark If the \p bigint pointer is NULL, the operation has no effect and returns \c false.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the bigint
 * const char* bigint_str = "101010"; // Binary: 101010 (42 in decimal)
 *
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   uint32_t bit_index = 1;
 *   bool is_set = cardano_bigint_test_bit(bigint, bit_index);
 *
 *   if (is_set)
 *   {
 *     // The bit at index 1 is set
 *     printf("Bit at index %u is set.\n", bit_index);
 *   }
 *   else
 *   {
 *     // The bit at index 1 is not set
 *     printf("Bit at index %u is not set.\n", bit_index);
 *   }
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_bigint_test_bit(
  const cardano_bigint_t* bigint,
  uint32_t                n);

/**
 * \brief Sets a specific bit in a bigint object.
 *
 * This function sets the bit at the specified position in a \ref cardano_bigint_t object.
 *
 * \param[in,out] bigint A pointer to the bigint object in which the bit will be set.
 * \param[in] n The index of the bit to be set (0-based).
 *
 * \remark If the \p bigint pointer is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the bigint
 * const char* bigint_str = "101010"; // Binary: 101010 (42 in decimal)
 *
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   uint32_t bit_index = 5;
 *   cardano_bigint_set_bit(bigint, bit_index);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_set_bit(
  cardano_bigint_t* bigint,
  uint32_t          n);

/**
 * \brief Clears a specific bit in a bigint object.
 *
 * This function clears the bit at the specified position in a \ref cardano_bigint_t object.
 *
 * \param[in,out] bigint A pointer to the bigint object in which the bit will be cleared.
 * \param[in] n The index of the bit to be cleared (0-based).
 *
 * \remark If the \p bigint pointer is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the bigint
 * const char* bigint_str = "111111"; // Binary: 111111 (63 in decimal)
 *
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   uint32_t bit_index = 5;
 *   cardano_bigint_clear_bit(bigint, bit_index);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_clear_bit(
  cardano_bigint_t* bigint,
  uint32_t          n);

/**
 * \brief Flips a specific bit in a bigint object.
 *
 * This function toggles the bit at the specified position in a \ref cardano_bigint_t object.
 * If the bit is 1, it will be changed to 0. If the bit is 0, it will be changed to 1.
 *
 * \param[in,out] bigint A pointer to the bigint object in which the bit will be flipped.
 * \param[in] n The index of the bit to be flipped (0-based).
 *
 * \remark If the \p bigint pointer is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the bigint
 * const char* bigint_str = "101010"; // Binary: 101010 (42 in decimal)
 *
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   uint32_t bit_index = 1;
 *   cardano_bigint_flip_bit(bigint, bit_index);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_flip_bit(
  cardano_bigint_t* bigint,
  uint32_t          n);

/**
 * \brief Returns the number of bits in the two's complement representation of this BigInteger that differ from its sign bit.
 *
 * This function computes the number of bits in the two's complement representation of the given \ref cardano_bigint_t object that differ from its sign bit.
 *
 * \param[in] bigint A constant pointer to the bigint object.
 *
 * \return The number of bits that differ from the sign bit.
 *
 * \remark If the \p bigint pointer is NULL, the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 *
 * size_t bit_count = cardano_bigint_bit_count(bigint);
 * printf("The number of bits that differ from the sign bit is: %zu\n", bit_count);
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_bigint_bit_count(const cardano_bigint_t* bigint);

/**
 * \brief Returns the number of bits required to represent the bigint in a minimal two's-complement form.
 *
 * This function calculates the bit length required to represent the integer stored in a \ref cardano_bigint_t object in its minimal two's-complement form.
 * This count includes the necessary bits for the absolute value of the number and the sign bit.
 *
 * \param[in] bigint A constant pointer to the bigint object.
 *
 * \return The number of bits required to represent the bigint in two's-complement form, effectively equal to the actual bit length of the integer as stored.
 * If the bigint pointer is NULL, the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 *
 * size_t bit_length = cardano_bigint_bit_length(bigint);
 * printf("The bit length of the bigint is: %zu\n", bit_length);
 *
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_bigint_bit_length(const cardano_bigint_t* bigint);

/**
 * \brief Computes the minimum of two bigint objects.
 *
 * This function compares two \ref cardano_bigint_t objects and stores the smaller value in a third bigint object.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the bigint object where the minimum value will be stored.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * \return \c true if the minimum value was successfully computed and stored in \p result, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* lhs = ...; // Assume these are initialized
 * cardano_bigint_t* rhs = ...;
 * cardano_bigint_t* result = ...;
 *
 * cardano_bigint_min(lhs, rhs, result);
 *
 * // Use the result
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_min(
  cardano_bigint_t* lhs,
  cardano_bigint_t* rhs,
  cardano_bigint_t* result);

/**
 * \brief Computes the maximum of two bigint objects.
 *
 * This function compares two \ref cardano_bigint_t objects and stores the larger value in a third bigint object.
 *
 * \param[in] lhs A constant pointer to the first operand.
 * \param[in] rhs A constant pointer to the second operand.
 * \param[out] result A pointer to the bigint object where the maximum value will be stored.
 *
 * \remark If any of the pointers (\p lhs, \p rhs, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* lhs = ...; // Assume these are initialized
 * cardano_bigint_t* rhs = ...;
 * cardano_bigint_t* result ...;
 *
 * cardano_bigint_max(lhs, rhs, result);
 * // Use the result
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * cardano_bigint_unref(&result);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_max(
  cardano_bigint_t* lhs,
  cardano_bigint_t* rhs,
  cardano_bigint_t* result);

/**
 * \brief Performs left bitwise shift on a bigint object.
 *
 * This function shifts a \ref cardano_bigint_t object left by the specified number of bits and stores the result in another bigint.
 *
 * \param[in] n A constant pointer to the bigint to be shifted.
 * \param[in] bits The number of bits to shift.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p n or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the bigint from a binary string
 * const char* bigint_str = "1010"; // Binary: 1010 (10 in decimal)
 * cardano_bigint_t* n = NULL;
 * cardano_error_t result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &n);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_t* result_bigint = cardano_bigint_new();
 *   uint32_t bits = 2;
 *
 *   cardano_bigint_shift_left(n, bits, result_bigint);
 *
 *   // Convert the result to a string for display
 *   size_t size = cardano_bigint_get_string_size(result_bigint, 2);
 *   char* buffer = (char*)malloc(size);
 *   cardano_bigint_to_string(result_bigint, buffer, size, 2);
 *
 *   printf("Result: %s\n", buffer); // Should print: 101000
 *
 *   // Clean up
 *   free(buffer);
 *   cardano_bigint_unref(&result_bigint);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&n);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_shift_left(
  const cardano_bigint_t* n,
  uint32_t                bits,
  cardano_bigint_t*       result);

/**
 * \brief Performs right bitwise shift on a bigint object.
 *
 * This function shifts a \ref cardano_bigint_t object right by the specified number of bits and stores the result in another bigint.
 *
 * \param[in] n A constant pointer to the bigint to be shifted.
 * \param[in] bits The number of bits to shift.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p n or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the bigint from a binary string
 * const char* bigint_str = "1010"; // Binary: 1010 (10 in decimal)
 * cardano_bigint_t* n = NULL;
 * cardano_error_t result = cardano_bigint_from_string(bigint_str, strlen(bigint_str), 2, &n);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_t* result_bigint = cardano_bigint_new();
 *   uint32_t bits = 2;
 *
 *   cardano_bigint_shift_right(n, bits, result_bigint);
 *
 *   // Convert the result to a string for display
 *   size_t size = cardano_bigint_get_string_size(result_bigint, 2);
 *   char* buffer = (char*)malloc(size);
 *   cardano_bigint_to_string(result_bigint, buffer, size, 2);
 *
 *   printf("Result: %s\n", buffer); // Should print: 10
 *
 *   // Clean up
 *   free(buffer);
 *   cardano_bigint_unref(&result_bigint);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&n);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_shift_right(
  const cardano_bigint_t* n,
  uint32_t                bits,
  cardano_bigint_t*       result);

/**
 * \brief Checks if two bigint objects are equal.
 *
 * This function compares two \ref cardano_bigint_t objects to determine if they are equal.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_bigint_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_bigint_t object to be compared.
 *
 * \return \c true if the two bigints are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* lhs = ...; // Assume these are initialized
 * cardano_bigint_t* rhs = ...;
 * bool are_equal = cardano_bigint_equals(lhs, rhs);
 *
 * if (are_equal)
 * {
 *   // The two bigints are equal
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_bigint_equals(const cardano_bigint_t* lhs, const cardano_bigint_t* rhs);

/**
 * \brief Compares two bigint objects.
 *
 * This function compares two \ref cardano_bigint_t objects.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_bigint_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_bigint_t object to be compared.
 *
 * \return An integer less than, equal to, or greater than zero if \p lhs is found to be less than,
 * equal to, or greater than \p rhs, respectively.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* lhs = ...; // Assume these are initialized
 * cardano_bigint_t* rhs = ...;
 * int32 comparison = cardano_bigint_compare(lhs, rhs);
 *
 * if (comparison < 0)
 * {
 *   // lhs is less than rhs
 * }
 * else if (comparison > 0)
 * {
 *   // lhs is greater than rhs
 * }
 * else
 * {
 *   // lhs is equal to rhs
 * }
 * // Clean up
 * cardano_bigint_unref(&lhs);
 * cardano_bigint_unref(&rhs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int32_t cardano_bigint_compare(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs);

/**
 * \brief Checks if a bigint is zero.
 *
 * This function checks if a \ref cardano_bigint_t object is zero.
 *
 * \param[in] n A constant pointer to the \ref cardano_bigint_t object to be checked.
 *
 * \return 1 if the bigint is zero, 0 otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* n = ...; // Assume this is initialized
 * bool is_zero = cardano_bigint_is_zero(n);
 * if (is_zero)
 * {
 *   // n is zero
 * }
 * else
 * {
 *   // n is not zero
 * }
 * // Clean up
 * cardano_bigint_unref(&n);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_bigint_is_zero(const cardano_bigint_t* n);

/**
 * \brief Increments a bigint.
 *
 * This function increments a \ref cardano_bigint_t object by one.
 *
 * \param[in,out] n A pointer to the \ref cardano_bigint_t object to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* n = ...; // Assume this is initialized
 * cardano_bigint_increment(n);
 * // n is now incremented
 * // Clean up
 * cardano_bigint_unref(&n);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_increment(cardano_bigint_t* n);

/**
 * \brief Decrements a bigint.
 *
 * This function decrements a \ref cardano_bigint_t object by one.
 *
 * \param[in,out] n A pointer to the \ref cardano_bigint_t object to be decremented.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* n = ...; // Assume this is initialized
 * cardano_bigint_decrement(n);
 * // n is now decremented
 * // Clean up
 * cardano_bigint_unref(&n);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_decrement(cardano_bigint_t* n);

/**
 * \brief Raises a bigint to the power of another bigint.
 *
 * This function computes the power of a \ref cardano_bigint_t object raised to another \ref cardano_bigint_t object.
 *
 * \param[in] base A constant pointer to the base.
 * \param[in] exponent A constant pointer to the exponent.
 * \param[out] result A pointer to the bigint object where the result will be stored.
 *
 * \remark If any of the pointers (\p base, \p exponent, or \p result) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * // Initialize the base bigint from a binary string
 * const char* base_str = "10"; // Binary: 10 (2 in decimal)
 * cardano_bigint_t* base = NULL;
 * cardano_error_t result = cardano_bigint_from_string(base_str, strlen(base_str), 2, &base);
 *
 * // Initialize the exponent bigint from a binary string
 * const char* exponent_str = "11"; // Binary: 11 (3 in decimal)
 * cardano_bigint_t* exponent = NULL;
 * result = cardano_bigint_from_string(exponent_str, strlen(exponent_str), 2, &exponent);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_bigint_t* result_bigint = cardano_bigint_new();
 *
 *   cardano_bigint_pow(base, exponent, result_bigint);
 *
 *   // Convert the result to a string for display
 *   size_t size = cardano_bigint_get_string_size(result_bigint, 2);
 *   char* buffer = (char*)malloc(size);
 *   cardano_bigint_to_string(result_bigint, buffer, size, 2);
 *
 *   printf("Result: %s\n", buffer); // Should print: 1000 (Binary: 1000 is 8 in decimal)
 *
 *   // Clean up
 *   free(buffer);
 *   cardano_bigint_unref(&result_bigint);
 * }
 *
 * // Clean up
 * cardano_bigint_unref(&base);
 * cardano_bigint_unref(&exponent);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_pow(const cardano_bigint_t* base, uint64_t exponent, cardano_bigint_t* result);

/**
 * \brief Assigns one bigint to another.
 *
 * This function copies the value of one \ref cardano_bigint_t object to another.
 *
 * \param[in] source A constant pointer to the source bigint object.
 * \param[out] destination A pointer to the destination bigint object.
 *
 * \remark If any of the pointers (\p source or \p destination) is NULL, the operation has no effect.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* src = ...; // Assume these are initialized
 * cardano_bigint_t* dst = ...;
 * cardano_bigint_assign(src, dst);
 * // dst now has the same value as src
 * // Clean up
 * cardano_bigint_unref(&src);
 * cardano_bigint_unref(&dst);
 * \endcode
 */
CARDANO_EXPORT
void cardano_bigint_assign(
  const cardano_bigint_t* destination,
  cardano_bigint_t*       source);

/**
 * \brief Decrements the reference count of a bigint object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_bigint_t object
 * by decreasing its reference count. When the reference count reaches zero, the bigint is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] bigint A pointer to the pointer of the bigint object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = cardano_bigint_new();
 *
 * // Perform operations with the bigint...
 *
 * cardano_bigint_unref(&bigint);
 * // At this point, bigint is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_bigint_unref, the pointer to the \ref cardano_bigint_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_bigint_unref(cardano_bigint_t** bigint);

/**
 * \brief Increases the reference count of the cardano_bigint_t object.
 *
 * This function is used to manually increment the reference count of a bigint
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_bigint_unref.
 *
 * \param bigint A pointer to the bigint object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming bigint is a previously created bigint object
 *
 * cardano_bigint_ref(bigint);
 *
 * // Now bigint can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_bigint_ref there is a corresponding
 * call to \ref cardano_bigint_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_bigint_ref(cardano_bigint_t* bigint);

/**
 * \brief Retrieves the current reference count of the cardano_bigint_t object.
 *
 * This function returns the number of active references to a bigint object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_bigint_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param bigint A pointer to the bigint object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified bigint object. If the object
 * is properly managed (i.e., every \ref cardano_bigint_ref call is matched with a
 * \ref cardano_bigint_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming bigint is a previously created bigint object
 *
 * size_t ref_count = cardano_bigint_refcount(bigint);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bigint_refcount(const cardano_bigint_t* bigint);

/**
 * \brief Sets the last error message for a given bigint object.
 *
 * Records an error message in the bigint's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] bigint A pointer to the \ref cardano_bigint_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the bigint's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 * cardano_bigint_set_last_error(bigint, "Error message");
 * // Later, retrieve the error
 * const char* error = cardano_bigint_get_last_error(bigint);
 * printf("Error: %s\n", error);
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_EXPORT void cardano_bigint_set_last_error(cardano_bigint_t* bigint, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific bigint.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_bigint_set_last_error for the given
 * bigint. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] bigint A pointer to the \ref cardano_bigint_t instance whose last error
 *                   message is to be retrieved. If the bigint is NULL, the function
 *                   returns a generic error message indicating the null bigint.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified bigint. If the bigint is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_bigint_set_last_error for the same bigint, or until
 *       the bigint is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bigint_t* bigint = ...; // Assume this is initialized
 * const char* error = cardano_bigint_get_last_error(bigint);
 * printf("Error: %s\n", error);
 * // Clean up
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_bigint_get_last_error(const cardano_bigint_t* bigint);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BIGINT_H
