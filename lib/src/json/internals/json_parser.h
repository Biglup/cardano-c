/**
 * \file json_parser.h
 *
 * \author angel.castillo
 * \date   Nov 08, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_PARSER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_PARSER_H

/* INCLUDES ******************************************************************/

#include <cardano/json/json_object.h>
#include <cardano/typedefs.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents the state and context of a JSON parsing operation.
 *
 * This structure is used internally to manage the state during JSON parsing operations.
 * It keeps track of the input data, current parsing position, nesting depth, and any
 * encountered errors.
 */
typedef struct
{
    const char* input;           /**< The input JSON string to be parsed. */
    size_t      length;          /**< The total length of the input JSON string, in bytes. */
    size_t      offset;          /**< The current position within the input JSON string. */
    size_t      depth;           /**< The current depth of the JSON structure being parsed. */
    char        last_error[256]; /**< Buffer for storing the last error message encountered. */
} cardano_json_parse_context_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Skips over any whitespace characters in the JSON input.
 *
 * This function advances the parsing position (`offset`) in the JSON input
 * until a non-whitespace character is encountered or the end of the input is reached.
 *
 * \param[in,out] ctx A pointer to the \ref cardano_json_parse_context_t structure
 *                    representing the current parsing context. This parameter must not
 *                    be NULL.
 */
void
cardano_skip_whitespace(cardano_json_parse_context_t* ctx);

/**
 * \brief Checks if a specific character exists within a given range of a character array.
 *
 * This function searches for the character \p to_match in the range specified by
 * \p begin and \p end. If the character is found within the range, the function
 * returns `true`. Otherwise, it returns `false`.
 *
 * \param[in] to_match The character to search for.
 * \param[in] begin A pointer to the start of the range to search. This must not be NULL.
 * \param[in] end A pointer to the end of the range to search. This must not be NULL and must be greater than or equal to \p begin.
 *
 * \return `true` if the character \p to_match is found in the range, `false` otherwise.
 */
bool
cardano_has_char(char to_match, const char* begin, const char* end);

/**
 * \brief Processes a UTF-8 sequence from the JSON input and writes it to a buffer.
 *
 * This function validates and processes a UTF-8 sequence starting at the current offset
 * in the JSON parse context. If the sequence is valid, it is written to the specified
 * buffer. If the sequence is invalid, the function records an error and returns `false`.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced by the length of the processed UTF-8 sequence.
 * \param[out] buf A pointer to the buffer where the processed UTF-8 sequence is written. This must not be NULL.
 *
 * \return `true` if the UTF-8 sequence was successfully processed and written to the buffer, `false` otherwise.
 */
bool
cardano_handle_utf8_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf);

/**
 * \brief Processes a Unicode escape sequence from the JSON input and writes it to a buffer.
 *
 * This function validates and processes a Unicode escape sequence (e.g., `\uXXXX`) starting
 * at the current offset in the JSON parse context. If the sequence is valid, it converts the
 * Unicode code point to its UTF-8 representation and writes it to the specified buffer.
 * Surrogate pairs are handled for sequences representing characters outside the Basic Multilingual Plane (BMP).
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced by the length of the processed Unicode escape sequence.
 * \param[out] buf A pointer to the buffer where the UTF-8 encoded representation of the Unicode code point is written.
 *                 This must not be NULL.
 *
 * \return `true` if the Unicode escape sequence was successfully processed and written to the buffer, `false` otherwise.
 */
bool
cardano_handle_unicode_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf);

/**
 * \brief Processes a JSON escape sequence and writes the corresponding character to a buffer.
 *
 * This function validates and processes a JSON escape sequence starting at the current offset
 * in the parse context. It converts the escape sequence to its corresponding character or
 * sequence of characters (e.g., Unicode or control characters) and writes it to the specified buffer.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced by the length of the processed escape sequence.
 * \param[out] buf A pointer to the buffer where the decoded character(s) are written. This must not be NULL.
 *
 * \return `true` if the escape sequence was successfully processed and written to the buffer, `false` otherwise.
 */
bool
cardano_handle_escape_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf);

/**
 * \brief Parses a JSON string value from the input context.
 *
 * This function reads a JSON string value from the input provided in the parse context.
 * It handles escape sequences, Unicode sequences, and validates the format of the string.
 * The parsed value is returned as a \ref cardano_json_object_t of type `CARDANO_JSON_OBJECT_TYPE_STRING`.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced as the string is processed.
 *
 * \return A pointer to a \ref cardano_json_object_t representing the parsed string,
 *         or `NULL` if parsing fails. The caller is responsible for releasing the returned object
 *         using \ref cardano_json_object_unref.
 */
cardano_json_object_t*
cardano_parse_string_value(cardano_json_parse_context_t* ctx);

/**
 * \brief Parses a JSON number value from the input context.
 *
 * This function reads a JSON number from the input provided in the parse context.
 * It validates the format of the number and stores it as a \ref cardano_json_object_t
 * with the appropriate numeric type (integer, unsigned integer, or real number).
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced as the number is processed.
 *
 * \return A pointer to a \ref cardano_json_object_t representing the parsed number,
 *         or `NULL` if parsing fails. The caller is responsible for releasing the returned object
 *         using \ref cardano_json_object_unref.
 */
cardano_json_object_t*
cardano_parse_number_value(cardano_json_parse_context_t* ctx);

/**
 * \brief Parses a JSON object from the input context.
 *
 * This function reads and parses a JSON object from the input provided in the parse context.
 * It validates the object structure, including keys and values, and constructs a corresponding
 * \ref cardano_json_object_t representation.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced as the object is parsed.
 *
 * \return A pointer to a \ref cardano_json_object_t representing the parsed object,
 *         or `NULL` if parsing fails. The caller is responsible for releasing the returned object
 *         using \ref cardano_json_object_unref.
 */
cardano_json_object_t*
cardano_parse_object_value(cardano_json_parse_context_t* ctx);

/**
 * \brief Parses a JSON array from the input context.
 *
 * This function reads and parses a JSON array from the input provided in the parse context.
 * It validates the array structure and constructs a corresponding \ref cardano_json_object_t representation.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced as the array is parsed.
 *
 * \return A pointer to a \ref cardano_json_object_t representing the parsed array,
 *         or `NULL` if parsing fails. The caller is responsible for releasing the returned object
 *         using \ref cardano_json_object_unref.
 */
cardano_json_object_t*
cardano_parse_array_value(cardano_json_parse_context_t* ctx);

/**
 * \brief Parses a specific JSON literal (e.g., `true`, `false`, `null`) from the input context.
 *
 * This function validates and parses a specific JSON literal from the input provided in the parse context.
 * It creates a corresponding \ref cardano_json_object_t with the specified type.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced as the literal is parsed.
 * \param[in] literal A pointer to the expected literal string (e.g., `"true"`, `"false"`, or `"null"`).
 *                    This must not be NULL and must point to a valid, null-terminated string.
 * \param[in] literal_size The length of the expected literal string, in bytes.
 * \param[in] type The \ref cardano_json_object_type_t representing the type of the literal
 *                 (e.g., `CARDANO_JSON_OBJECT_TYPE_BOOLEAN` for `true` or `false`,
 *                 `CARDANO_JSON_OBJECT_TYPE_NULL` for `null`).
 *
 * \return A pointer to a \ref cardano_json_object_t representing the parsed literal, or `NULL` if parsing fails.
 *         The caller is responsible for releasing the returned object using \ref cardano_json_object_unref.
 */
cardano_json_object_t*
cardano_parse_literal(
  cardano_json_parse_context_t* ctx,
  const char*                   literal,
  size_t                        literal_size,
  cardano_json_object_type_t    type);

/**
 * \brief Parses a JSON value from the input context.
 *
 * This function identifies and parses a JSON value (e.g., object, array, string, number, boolean, or null)
 * from the input context. It creates and returns a corresponding \ref cardano_json_object_t.
 *
 * \param[in,out] ctx A pointer to the JSON parse context. This must not be NULL.
 *                    The context's offset is advanced as the value is parsed.
 *
 * \return A pointer to a \ref cardano_json_object_t representing the parsed JSON value, or `NULL` if parsing fails.
 *         The caller is responsible for releasing the returned object using \ref cardano_json_object_unref.
 */
cardano_json_object_t*
cardano_parse_value(cardano_json_parse_context_t* ctx);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_PARSER_H