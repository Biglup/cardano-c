/**
 * \file text_parser.c
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

/* INCLUDES ******************************************************************/

#include "text_parser.h"

#include "../../allocators.h"
#include "../builtins/bls.h"
#include "../ast/uplc_int.h"

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Maximum term and constant nesting the parser descends before refusing.
 *
 * Bounds recursion so a deeply nested adversarial input cannot exhaust the C
 * stack. It matches the flat term decoder's bound so the two front ends accept
 * the same depth of nesting. Past it the parser returns
 * \ref CARDANO_ERROR_DECODING.
 */
static const uint32_t PARSER_MAX_DEPTH = 4096U;

/**
 * \brief Numeric base used when parsing decimal integer literals.
 */
static const int32_t PARSER_DECIMAL_BASE = 10;

/**
 * \brief Number of hex digits that make up one byte.
 */
static const size_t PARSER_HEX_DIGITS_PER_BYTE = 2U;

/**
 * \brief Highest Unicode code point a numeric string escape may denote.
 */
static const uint32_t PARSER_MAX_CODE_POINT = 0x10FFFFU;

/* STRUCTURES ****************************************************************/

/**
 * \brief A binder name in scope, recorded as a slice into the source text.
 *
 * The scope stack holds one of these per \c lam currently open. A variable
 * resolves to the de Bruijn index counting from the top of the stack, so the
 * innermost binder is index 1.
 */
typedef struct binder_t
{
  const char* text;
  size_t      length;
} binder_t;

/**
 * \brief Parser state: a cursor over the source and the open-binder scope stack.
 *
 * \c text and \c len delimit the source; \c pos is the current read offset and is
 * never advanced past \c len. \c scope is a growable array of binders in scope,
 * \c scope_count its used length and \c scope_capacity its allocated length.
 */
typedef struct parser_t
{
  cardano_uplc_arena_t* arena;
  const char*           text;
  size_t                len;
  size_t                pos;
  binder_t*         scope;
  size_t                scope_count;
  size_t                scope_capacity;
  uint64_t              version_major;
  uint64_t              version_minor;
} parser_t;

/* STATIC FUNCTION DECLARATIONS *********************************************/

static cardano_error_t parse_term(parser_t* parser, uint32_t depth, const cardano_uplc_term_t** out);
static cardano_error_t parse_constant(parser_t* parser, uint32_t depth, const cardano_uplc_constant_t** out);
static cardano_error_t parse_type(parser_t* parser, uint32_t depth, const cardano_uplc_type_t** out);
static cardano_error_t parse_value(parser_t* parser, const cardano_uplc_type_t* type, uint32_t depth, const cardano_uplc_constant_t** out);
static cardano_error_t parse_data(parser_t* parser, uint32_t depth, cardano_plutus_data_t** out);

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Tests whether a byte is an ASCII whitespace character.
 *
 * \param[in] c The byte to test.
 *
 * \return \c true for space, tab, carriage return or newline, else \c false.
 */
static bool
is_space(const char c)
{
  return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

/**
 * \brief Tests whether a byte may appear in an identifier or builtin name.
 *
 * The set matches the corpus surface syntax: letters, digits, underscore, the
 * apostrophe, the tilde and the hyphen (binder names such as \c y-0 occur in the
 * corpus). The hyphen is part of the identifier set and not a token by itself.
 *
 * \param[in] c The byte to test.
 *
 * \return \c true if \p c is an identifier byte, else \c false.
 */
static bool
is_ident(const char c)
{
  const bool lower = (c >= 'a') && (c <= 'z');
  const bool upper = (c >= 'A') && (c <= 'Z');
  const bool digit = (c >= '0') && (c <= '9');

  return lower || upper || digit || (c == '_') || (c == '\'') || (c == '~') || (c == '-');
}

/**
 * \brief Tests whether a byte is a decimal digit.
 *
 * \param[in] c The byte to test.
 *
 * \return \c true for '0'..'9', else \c false.
 */
static bool
is_digit(const char c)
{
  return (c >= '0') && (c <= '9');
}

/**
 * \brief Tests whether a byte is a hexadecimal digit.
 *
 * \param[in] c The byte to test.
 *
 * \return \c true for '0'..'9', 'a'..'f' or 'A'..'F', else \c false.
 */
static bool
is_hex(const char c)
{
  const bool digit = (c >= '0') && (c <= '9');
  const bool lower = (c >= 'a') && (c <= 'f');
  const bool upper = (c >= 'A') && (c <= 'F');

  return digit || lower || upper;
}

/**
 * \brief Returns the byte at the cursor without consuming it.
 *
 * \param[in] parser The parser whose cursor to peek.
 *
 * \return The current byte, or '\\0' when the cursor is at the end of the source.
 */
static char
peek_char(const parser_t* parser)
{
  if (parser->pos >= parser->len)
  {
    return '\0';
  }

  return parser->text[parser->pos];
}

/**
 * \brief Skips a run of whitespace and \c "--" line comments.
 *
 * A line comment runs from \c "--" to the next newline or the end of the source.
 * Leaves the cursor on the first byte that is neither whitespace nor inside a
 * comment.
 *
 * \param[in,out] parser The parser whose cursor to advance.
 */
static void
skip_trivia(parser_t* parser)
{
  while (parser->pos < parser->len)
  {
    const char c = parser->text[parser->pos];

    if (is_space(c))
    {
      parser->pos += 1U;
    }
    else if ((c == '-') && ((parser->pos + 1U) < parser->len) && (parser->text[parser->pos + 1U] == '-'))
    {
      parser->pos += 2U;

      while ((parser->pos < parser->len) && (parser->text[parser->pos] != '\n'))
      {
        parser->pos += 1U;
      }
    }
    else
    {
      break;
    }
  }
}

/**
 * \brief Consumes a single expected byte after skipping leading trivia.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] expected The byte that must appear next.
 *
 * \return \ref CARDANO_SUCCESS if the byte was present and consumed, else
 *         \ref CARDANO_ERROR_DECODING.
 */
static cardano_error_t
expect_char(parser_t* parser, const char expected)
{
  skip_trivia(parser);

  if (peek_char(parser) != expected)
  {
    return CARDANO_ERROR_DECODING;
  }

  parser->pos += 1U;

  return CARDANO_SUCCESS;
}

/**
 * \brief Consumes an expected keyword followed by a non-identifier boundary.
 *
 * Skips leading trivia, matches \p word exactly, and requires the byte after the
 * word to be absent from the identifier set so that, for instance, \c "con" does
 * not match a prefix of \c "constr". The cursor is left just past the keyword on
 * success and is not moved on failure.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] word The NUL-terminated keyword to match.
 *
 * \return \c true if the keyword was matched and consumed, else \c false.
 */
static bool
match_keyword(parser_t* parser, const char* word)
{
  size_t      length = strlen(word);
  size_t      start  = 0U;
  const char* at     = NULL;

  skip_trivia(parser);

  start = parser->pos;

  if ((start + length) > parser->len)
  {
    return false;
  }

  at = &parser->text[start];

  if (memcmp(at, word, length) != 0)
  {
    return false;
  }

  if (((start + length) < parser->len) && is_ident(parser->text[start + length]))
  {
    return false;
  }

  parser->pos = start + length;

  return true;
}

/**
 * \brief Reads an identifier or builtin-name token.
 *
 * Skips leading trivia, then captures a maximal run of identifier bytes as a slice
 * into the source. An empty run is a parse error.
 *
 * \param[in,out] parser The parser to advance.
 * \param[out] text On success, set to the start of the identifier in the source.
 * \param[out] length On success, set to the identifier length in bytes.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING if no
 *         identifier byte is present.
 */
static cardano_error_t
read_ident(parser_t* parser, const char** text, size_t* length)
{
  size_t start = 0U;

  skip_trivia(parser);

  start = parser->pos;

  while ((parser->pos < parser->len) && is_ident(parser->text[parser->pos]))
  {
    parser->pos += 1U;
  }

  if (parser->pos == start)
  {
    return CARDANO_ERROR_DECODING;
  }

  *text   = &parser->text[start];
  *length = parser->pos - start;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads an unsigned decimal literal into a 64-bit value, detecting overflow.
 *
 * Skips leading trivia, then reads one or more decimal digits. Overflow past
 * \c UINT64_MAX is reported as a parse error so an out-of-range tag does not wrap.
 *
 * \param[in,out] parser The parser to advance.
 * \param[out] value On success, the parsed value.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         missing digit or an overflow.
 */
static cardano_error_t
read_u64(parser_t* parser, uint64_t* value)
{
  uint64_t acc   = 0U;
  size_t   start = 0U;

  skip_trivia(parser);

  start = parser->pos;

  while ((parser->pos < parser->len) && is_digit(parser->text[parser->pos]))
  {
    const uint64_t digit = (uint64_t)(parser->text[parser->pos] - '0');

    if (acc > ((UINT64_MAX - digit) / 10U))
    {
      return CARDANO_ERROR_DECODING;
    }

    acc = (acc * 10U) + digit;
    parser->pos += 1U;
  }

  if (parser->pos == start)
  {
    return CARDANO_ERROR_DECODING;
  }

  *value = acc;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a signed decimal integer literal into a fresh bigint.
 *
 * Skips leading trivia, accepts an optional leading sign, then requires at least
 * one decimal digit. A leading '+' denotes a positive value and is dropped before
 * the magnitude is handed to the bigint parser, which accepts only '-' or digits;
 * a leading '-' is kept. This matches the reference, where '+7' parses to 7. The
 * caller owns the returned reference and must release it.
 *
 * \param[in,out] parser The parser to advance.
 * \param[out] out On success, a newly created bigint the caller owns.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed literal, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
read_bigint(parser_t* parser, cardano_bigint_t** out)
{
  size_t          start    = 0U;
  size_t          digits   = 0U;
  size_t          length   = 0U;
  char*           terminated = NULL;
  cardano_error_t result   = CARDANO_SUCCESS;

  skip_trivia(parser);

  if (peek_char(parser) == '+')
  {
    parser->pos += 1U;
  }

  start = parser->pos;

  if (peek_char(parser) == '-')
  {
    parser->pos += 1U;
  }

  while ((parser->pos < parser->len) && is_digit(parser->text[parser->pos]))
  {
    parser->pos += 1U;
    digits += 1U;
  }

  if (digits == 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  length     = parser->pos - start;
  terminated = (char*)_cardano_malloc(length + 1U);

  if (terminated == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  memcpy(terminated, &parser->text[start], length);
  terminated[length] = '\0';

  result = cardano_bigint_from_string(terminated, length, PARSER_DECIMAL_BASE, out);

  _cardano_free(terminated);

  if (result == CARDANO_ERROR_MEMORY_ALLOCATION_FAILED)
  {
    return result;
  }

  if (result != CARDANO_SUCCESS)
  {
    return CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Converts a single hex digit to its value.
 *
 * \param[in] c The hex digit, assumed to satisfy \ref is_hex.
 *
 * \return The numeric value 0..15.
 */
static byte_t
hex_value(const char c)
{
  byte_t value = 0U;

  if ((c >= '0') && (c <= '9'))
  {
    value = (byte_t)(c - '0');
  }
  else if ((c >= 'a') && (c <= 'f'))
  {
    value = (byte_t)((c - 'a') + 10);
  }
  else
  {
    value = (byte_t)((c - 'A') + 10);
  }

  return value;
}

/**
 * \brief Reads a \c #hex byte-string literal into a fresh buffer.
 *
 * Skips leading trivia, requires a \c '#', then reads an even number of hex digits.
 * An empty \c '#' yields an empty buffer. An odd digit count or a non-hex byte in
 * the run is a parse error. The caller owns the returned reference.
 *
 * \param[in,out] parser The parser to advance.
 * \param[out] out On success, a newly created buffer the caller owns.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed literal, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
read_hex_bytes(parser_t* parser, cardano_buffer_t** out)
{
  size_t            start  = 0U;
  size_t            count  = 0U;
  size_t            i      = 0U;
  cardano_buffer_t* buffer = NULL;

  skip_trivia(parser);

  if (peek_char(parser) != '#')
  {
    return CARDANO_ERROR_DECODING;
  }

  parser->pos += 1U;
  start = parser->pos;

  while ((parser->pos < parser->len) && is_hex(parser->text[parser->pos]))
  {
    parser->pos += 1U;
  }

  count = parser->pos - start;

  if ((count % PARSER_HEX_DIGITS_PER_BYTE) != 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  buffer = cardano_buffer_new((count / PARSER_HEX_DIGITS_PER_BYTE) + 1U);

  if (buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (i = 0U; i < count; i += PARSER_HEX_DIGITS_PER_BYTE)
  {
    const byte_t high = hex_value(parser->text[start + i]);
    const byte_t low  = hex_value(parser->text[start + i + 1U]);
    const byte_t b    = (byte_t)((high << 4U) | low);

    if (cardano_buffer_write(buffer, &b, 1U) != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&buffer);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *out = buffer;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a \c 0x byte-string literal into a fresh buffer.
 *
 * Skips leading trivia, requires the \c 0x prefix, then reads an even number of
 * hex digits. The textual form BLS12-381 element constants use. An odd digit
 * count is a parse error. The caller owns the returned reference.
 *
 * \param[in,out] parser The parser to advance.
 * \param[out] out On success, a newly created buffer the caller owns.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed literal, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
read_0x_bytes(parser_t* parser, cardano_buffer_t** out)
{
  size_t            start  = 0U;
  size_t            count  = 0U;
  size_t            i      = 0U;
  cardano_buffer_t* buffer = NULL;

  skip_trivia(parser);

  if ((parser->pos + 1U) >= parser->len)
  {
    return CARDANO_ERROR_DECODING;
  }

  if ((parser->text[parser->pos] != '0') || (parser->text[parser->pos + 1U] != 'x'))
  {
    return CARDANO_ERROR_DECODING;
  }

  parser->pos += 2U;
  start = parser->pos;

  while ((parser->pos < parser->len) && is_hex(parser->text[parser->pos]))
  {
    parser->pos += 1U;
  }

  count = parser->pos - start;

  if ((count % PARSER_HEX_DIGITS_PER_BYTE) != 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  buffer = cardano_buffer_new((count / PARSER_HEX_DIGITS_PER_BYTE) + 1U);

  if (buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (i = 0U; i < count; i += PARSER_HEX_DIGITS_PER_BYTE)
  {
    const byte_t high = hex_value(parser->text[start + i]);
    const byte_t low  = hex_value(parser->text[start + i + 1U]);
    const byte_t b    = (byte_t)((high << 4U) | low);

    if (cardano_buffer_write(buffer, &b, 1U) != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&buffer);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *out = buffer;

  return CARDANO_SUCCESS;
}

/**
 * \brief Tests whether a byte is an octal digit.
 *
 * \param[in] c The byte to test.
 *
 * \return \c true for '0'..'7', else \c false.
 */
static bool
is_octal(const char c)
{
  return (c >= '0') && (c <= '7');
}

/**
 * \brief Appends the UTF-8 encoding of a code point to a buffer.
 *
 * Encodes \p code_point as one to four UTF-8 bytes so a numeric string escape that
 * denotes a code point above 127 is stored as the UTF-8 text the corpus expects.
 *
 * \param[in,out] buffer The buffer to append to.
 * \param[in] code_point The Unicode code point, assumed not above
 *            \ref PARSER_MAX_CODE_POINT.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the append fails.
 */
static cardano_error_t
utf8_encode(cardano_buffer_t* buffer, const uint32_t code_point)
{
  byte_t bytes[4] = { 0U, 0U, 0U, 0U };
  size_t length   = 0U;

  if (code_point < 0x80U)
  {
    bytes[0] = (byte_t)code_point;
    length   = 1U;
  }
  else if (code_point < 0x800U)
  {
    bytes[0] = (byte_t)(0xC0U | (code_point >> 6U));
    bytes[1] = (byte_t)(0x80U | (code_point & 0x3FU));
    length   = 2U;
  }
  else if (code_point < 0x10000U)
  {
    bytes[0] = (byte_t)(0xE0U | (code_point >> 12U));
    bytes[1] = (byte_t)(0x80U | ((code_point >> 6U) & 0x3FU));
    bytes[2] = (byte_t)(0x80U | (code_point & 0x3FU));
    length   = 3U;
  }
  else
  {
    bytes[0] = (byte_t)(0xF0U | (code_point >> 18U));
    bytes[1] = (byte_t)(0x80U | ((code_point >> 12U) & 0x3FU));
    bytes[2] = (byte_t)(0x80U | ((code_point >> 6U) & 0x3FU));
    bytes[3] = (byte_t)(0x80U | (code_point & 0x3FU));
    length   = 4U;
  }

  return cardano_buffer_write(buffer, bytes, length);
}

/**
 * \brief Reads a run of digits in a given base as a code point.
 *
 * Reads a maximal run of digits valid in \p base starting at the cursor and folds
 * them into a code point, rejecting an empty run or a value past
 * \ref PARSER_MAX_CODE_POINT. Used by the decimal, octal and hex numeric string
 * escapes.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] base The numeric base, 8, 10 or 16.
 * \param[out] code_point On success, the parsed code point.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for an
 *         empty run or an out-of-range value.
 */
static cardano_error_t
read_code_point(parser_t* parser, const uint32_t base, uint32_t* code_point)
{
  uint32_t acc    = 0U;
  size_t   digits = 0U;

  while (parser->pos < parser->len)
  {
    const char     c   = parser->text[parser->pos];
    uint32_t       val = 0U;

    if ((base == 16U) && is_hex(c))
    {
      val = (uint32_t)hex_value(c);
    }
    else if ((base == 8U) && is_octal(c))
    {
      val = (uint32_t)(c - '0');
    }
    else if ((base == 10U) && is_digit(c))
    {
      val = (uint32_t)(c - '0');
    }
    else
    {
      break;
    }

    if (acc > ((PARSER_MAX_CODE_POINT - val) / base))
    {
      return CARDANO_ERROR_DECODING;
    }

    acc = (acc * base) + val;
    parser->pos += 1U;
    digits += 1U;
  }

  if (digits == 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  *code_point = acc;

  return CARDANO_SUCCESS;
}

/**
 * \brief Matches a named control-code string escape at the cursor.
 *
 * Tries the ASCII control-code mnemonics (\c NUL through \c US plus \c SP and
 * \c DEL) at the cursor, preferring the longest match so \c "\\SOH" is not read as
 * \c "\\SO" followed by \c "H". On a match the cursor is advanced past the name and
 * the corresponding code point is returned.
 *
 * \param[in,out] parser The parser to advance, positioned just past the backslash.
 * \param[out] code_point On a match, the control-code value 0..127.
 *
 * \return \c true if a name matched and was consumed, else \c false.
 */
static bool
match_named_escape(parser_t* parser, uint32_t* code_point)
{
  static const struct
  {
    const char* name;
    uint32_t    value;
  } table[] = {
    { "NUL", 0U }, { "SOH", 1U }, { "STX", 2U }, { "ETX", 3U }, { "EOT", 4U },
    { "ENQ", 5U }, { "ACK", 6U }, { "DLE", 16U }, { "DC1", 17U }, { "DC2", 18U },
    { "DC3", 19U }, { "DC4", 20U }, { "NAK", 21U }, { "SYN", 22U }, { "ETB", 23U },
    { "CAN", 24U }, { "SUB", 26U }, { "ESC", 27U }, { "DEL", 127U }, { "BEL", 7U },
    { "BS", 8U }, { "HT", 9U }, { "LF", 10U }, { "VT", 11U }, { "FF", 12U },
    { "CR", 13U }, { "SO", 14U }, { "SI", 15U }, { "EM", 25U }, { "FS", 28U },
    { "GS", 29U }, { "RS", 30U }, { "US", 31U }, { "SP", 32U }
  };

  const size_t count    = sizeof(table) / sizeof(table[0]);
  size_t       best     = count;
  size_t       best_len = 0U;
  size_t       i        = 0U;

  for (i = 0U; i < count; ++i)
  {
    const size_t name_len = strlen(table[i].name);

    if (((parser->pos + name_len) <= parser->len) &&
        (memcmp(&parser->text[parser->pos], table[i].name, name_len) == 0) &&
        (name_len > best_len))
    {
      best     = i;
      best_len = name_len;
    }
  }

  if (best == count)
  {
    return false;
  }

  parser->pos += best_len;
  *code_point = table[best].value;

  return true;
}

/**
 * \brief Skips a string gap: a backslash, whitespace and a closing backslash.
 *
 * A string gap lets a literal span lines and contributes no character. The cursor
 * is positioned on the first whitespace byte after the opening backslash; this
 * consumes the whitespace run and the required closing backslash.
 *
 * \param[in,out] parser The parser to advance.
 *
 * \return \ref CARDANO_SUCCESS if a well-formed gap was consumed, else
 *         \ref CARDANO_ERROR_DECODING.
 */
static cardano_error_t
skip_string_gap(parser_t* parser)
{
  while ((parser->pos < parser->len) && is_space(parser->text[parser->pos]))
  {
    parser->pos += 1U;
  }

  if ((parser->pos >= parser->len) || (parser->text[parser->pos] != '\\'))
  {
    return CARDANO_ERROR_DECODING;
  }

  parser->pos += 1U;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a double-quoted string literal into a fresh UTF-8 buffer.
 *
 * Skips leading trivia, requires an opening quote, then reads bytes until the
 * closing quote, decoding the escape set: the simple
 * escapes \c \\n \c \\r \c \\t \c \\" \c \\' \c \\\\ \c \\a \c \\b \c \\f \c \\v, a
 * decimal numeric escape \c \\DDD, a hex escape \c \\xHH..., an octal escape
 * \c \\oNNN, the named ASCII control codes (for example \c \\DEL), the empty escape
 * \c \\& and a string gap (a backslash, whitespace and a closing backslash). A
 * numeric escape denoting a code point above 127 is UTF-8 encoded. Bytes outside an
 * escape are stored verbatim, so raw multi-byte UTF-8 in the source passes through.
 * An unterminated string or a malformed escape is a parse error. The caller owns the
 * returned reference.
 *
 * \param[in,out] parser The parser to advance.
 * \param[out] out On success, a newly created buffer the caller owns.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed literal, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
read_string(parser_t* parser, cardano_buffer_t** out)
{
  cardano_buffer_t* buffer = NULL;

  skip_trivia(parser);

  if (peek_char(parser) != '"')
  {
    return CARDANO_ERROR_DECODING;
  }

  parser->pos += 1U;

  buffer = cardano_buffer_new(1U);

  if (buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  while (true)
  {
    char            c          = '\0';
    bool            have_byte  = false;
    byte_t          emitted    = 0U;
    cardano_error_t result     = CARDANO_SUCCESS;

    if (parser->pos >= parser->len)
    {
      cardano_buffer_unref(&buffer);

      return CARDANO_ERROR_DECODING;
    }

    c = parser->text[parser->pos];
    parser->pos += 1U;

    if (c == '"')
    {
      break;
    }

    if (c != '\\')
    {
      emitted   = (byte_t)c;
      have_byte = true;
    }
    else
    {
      char esc = '\0';

      if (parser->pos >= parser->len)
      {
        cardano_buffer_unref(&buffer);

        return CARDANO_ERROR_DECODING;
      }

      esc = parser->text[parser->pos];

      switch (esc)
      {
        case 'n':  { emitted = (byte_t)'\n'; have_byte = true; parser->pos += 1U; break; }
        case 'r':  { emitted = (byte_t)'\r'; have_byte = true; parser->pos += 1U; break; }
        case 't':  { emitted = (byte_t)'\t'; have_byte = true; parser->pos += 1U; break; }
        case 'v':  { emitted = (byte_t)'\v'; have_byte = true; parser->pos += 1U; break; }
        case 'f':  { emitted = (byte_t)'\f'; have_byte = true; parser->pos += 1U; break; }
        case 'a':  { emitted = (byte_t)0x07U; have_byte = true; parser->pos += 1U; break; }
        case 'b':  { emitted = (byte_t)0x08U; have_byte = true; parser->pos += 1U; break; }
        case '"':  { emitted = (byte_t)'"';  have_byte = true; parser->pos += 1U; break; }
        case '\'': { emitted = (byte_t)'\''; have_byte = true; parser->pos += 1U; break; }
        case '\\': { emitted = (byte_t)'\\'; have_byte = true; parser->pos += 1U; break; }
        case '&':  { parser->pos += 1U; break; }
        case 'x':
        {
          uint32_t code_point = 0U;

          parser->pos += 1U;
          result = read_code_point(parser, 16U, &code_point);

          if (result == CARDANO_SUCCESS)
          {
            result = utf8_encode(buffer, code_point);
          }
          break;
        }
        case 'o':
        {
          uint32_t code_point = 0U;

          parser->pos += 1U;
          result = read_code_point(parser, 8U, &code_point);

          if (result == CARDANO_SUCCESS)
          {
            result = utf8_encode(buffer, code_point);
          }
          break;
        }
        default:
        {
          uint32_t code_point = 0U;

          if (is_digit(esc))
          {
            result = read_code_point(parser, 10U, &code_point);

            if (result == CARDANO_SUCCESS)
            {
              result = utf8_encode(buffer, code_point);
            }
          }
          else if (is_space(esc))
          {
            result = skip_string_gap(parser);
          }
          else if (match_named_escape(parser, &code_point))
          {
            result = utf8_encode(buffer, code_point);
          }
          else
          {
            result = CARDANO_ERROR_DECODING;
          }
          break;
        }
      }

      if (result != CARDANO_SUCCESS)
      {
        cardano_buffer_unref(&buffer);

        return result;
      }
    }

    if (have_byte && (cardano_buffer_write(buffer, &emitted, 1U) != CARDANO_SUCCESS))
    {
      cardano_buffer_unref(&buffer);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *out = buffer;

  return CARDANO_SUCCESS;
}

/**
 * \brief Pushes a binder onto the scope stack, growing it if needed.
 *
 * The stack array lives in arena-independent heap memory owned by the parser run
 * and freed when parsing finishes, regardless of outcome.
 *
 * \param[in,out] parser The parser whose scope to extend.
 * \param[in] text The binder name slice start in the source.
 * \param[in] length The binder name length.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the stack cannot grow.
 */
static cardano_error_t
scope_push(parser_t* parser, const char* text, const size_t length)
{
  if (parser->scope_count == parser->scope_capacity)
  {
    const size_t  next = (parser->scope_capacity == 0U) ? 8U : (parser->scope_capacity * 2U);
    binder_t* grown = (binder_t*)_cardano_realloc(parser->scope, next * sizeof(binder_t));

    if (grown == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    parser->scope          = grown;
    parser->scope_capacity = next;
  }

  parser->scope[parser->scope_count].text   = text;
  parser->scope[parser->scope_count].length = length;
  parser->scope_count += 1U;

  return CARDANO_SUCCESS;
}

/**
 * \brief Pops the innermost binder off the scope stack.
 *
 * \param[in,out] parser The parser whose scope to shrink.
 */
static void
scope_pop(parser_t* parser)
{
  if (parser->scope_count > 0U)
  {
    parser->scope_count -= 1U;
  }
}

/**
 * \brief Resolves a variable name to its 1-based de Bruijn index.
 *
 * Searches the scope stack from the innermost binder outward and returns the
 * distance to the matching binder, so the innermost binder is index 1. The
 * latest binding of a shadowed name wins, matching the lexical scope of the flat
 * encoding.
 *
 * A free (unbound) name is not a parse error: an open term is accepted and the
 * machine fails on it. An unbound name is assigned the index \c scope_count + 1,
 * which is one past the outermost binder in scope and therefore always out of
 * range for the environment built at evaluation. The CEK env lookup for that
 * index fails, so an open term yields a script error rather than a parse error.
 *
 * \param[in] parser The parser whose scope to search.
 * \param[in] text The variable name slice start.
 * \param[in] length The variable name length.
 * \param[out] index On success, the 1-based de Bruijn index, or
 *             \c scope_count + 1 for a free variable.
 *
 * \return \ref CARDANO_SUCCESS always.
 */
static cardano_error_t
resolve_name_index(const parser_t* parser, const char* text, const size_t length, uint64_t* index)
{
  size_t i = parser->scope_count;

  while (i > 0U)
  {
    const binder_t* binder = &parser->scope[i - 1U];

    if ((binder->length == length) && (memcmp(binder->text, text, length) == 0))
    {
      *index = (uint64_t)(parser->scope_count - (i - 1U));

      return CARDANO_SUCCESS;
    }

    i -= 1U;
  }

  *index = (uint64_t)parser->scope_count + 1U;

  return CARDANO_SUCCESS;
}

/**
 * \brief Resolves a builtin name to its tag.
 *
 * Looks the name up in the surface-syntax builtin name table. An unknown name is
 * a parse error.
 *
 * \param[in] text The builtin name slice start.
 * \param[in] length The builtin name length.
 * \param[out] builtin On success, the builtin tag.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING if the
 *         name is not a known builtin.
 */
static cardano_error_t
resolve_builtin(const char* text, const size_t length, cardano_uplc_builtin_t* builtin)
{
  static const char* const names[CARDANO_UPLC_BUILTIN_COUNT] = {
    [CARDANO_UPLC_BUILTIN_ADD_INTEGER]                        = "addInteger",
    [CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER]                   = "subtractInteger",
    [CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER]                   = "multiplyInteger",
    [CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER]                     = "divideInteger",
    [CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER]                   = "quotientInteger",
    [CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER]                  = "remainderInteger",
    [CARDANO_UPLC_BUILTIN_MOD_INTEGER]                        = "modInteger",
    [CARDANO_UPLC_BUILTIN_EQUALS_INTEGER]                     = "equalsInteger",
    [CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER]                  = "lessThanInteger",
    [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER]           = "lessThanEqualsInteger",
    [CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING]                 = "appendByteString",
    [CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING]                   = "consByteString",
    [CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING]                  = "sliceByteString",
    [CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING]              = "lengthOfByteString",
    [CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING]                  = "indexByteString",
    [CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING]                 = "equalsByteString",
    [CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING]              = "lessThanByteString",
    [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING]       = "lessThanEqualsByteString",
    [CARDANO_UPLC_BUILTIN_SHA2_256]                           = "sha2_256",
    [CARDANO_UPLC_BUILTIN_SHA3_256]                           = "sha3_256",
    [CARDANO_UPLC_BUILTIN_BLAKE2B_256]                        = "blake2b_256",
    [CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE]           = "verifyEd25519Signature",
    [CARDANO_UPLC_BUILTIN_APPEND_STRING]                      = "appendString",
    [CARDANO_UPLC_BUILTIN_EQUALS_STRING]                      = "equalsString",
    [CARDANO_UPLC_BUILTIN_ENCODE_UTF8]                        = "encodeUtf8",
    [CARDANO_UPLC_BUILTIN_DECODE_UTF8]                        = "decodeUtf8",
    [CARDANO_UPLC_BUILTIN_IF_THEN_ELSE]                       = "ifThenElse",
    [CARDANO_UPLC_BUILTIN_CHOOSE_UNIT]                        = "chooseUnit",
    [CARDANO_UPLC_BUILTIN_TRACE]                              = "trace",
    [CARDANO_UPLC_BUILTIN_FST_PAIR]                           = "fstPair",
    [CARDANO_UPLC_BUILTIN_SND_PAIR]                           = "sndPair",
    [CARDANO_UPLC_BUILTIN_CHOOSE_LIST]                        = "chooseList",
    [CARDANO_UPLC_BUILTIN_MK_CONS]                            = "mkCons",
    [CARDANO_UPLC_BUILTIN_HEAD_LIST]                          = "headList",
    [CARDANO_UPLC_BUILTIN_TAIL_LIST]                          = "tailList",
    [CARDANO_UPLC_BUILTIN_NULL_LIST]                          = "nullList",
    [CARDANO_UPLC_BUILTIN_CHOOSE_DATA]                        = "chooseData",
    [CARDANO_UPLC_BUILTIN_CONSTR_DATA]                        = "constrData",
    [CARDANO_UPLC_BUILTIN_MAP_DATA]                           = "mapData",
    [CARDANO_UPLC_BUILTIN_LIST_DATA]                          = "listData",
    [CARDANO_UPLC_BUILTIN_I_DATA]                             = "iData",
    [CARDANO_UPLC_BUILTIN_B_DATA]                             = "bData",
    [CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA]                     = "unConstrData",
    [CARDANO_UPLC_BUILTIN_UN_MAP_DATA]                        = "unMapData",
    [CARDANO_UPLC_BUILTIN_UN_LIST_DATA]                       = "unListData",
    [CARDANO_UPLC_BUILTIN_UN_I_DATA]                          = "unIData",
    [CARDANO_UPLC_BUILTIN_UN_B_DATA]                          = "unBData",
    [CARDANO_UPLC_BUILTIN_EQUALS_DATA]                        = "equalsData",
    [CARDANO_UPLC_BUILTIN_MK_PAIR_DATA]                       = "mkPairData",
    [CARDANO_UPLC_BUILTIN_MK_NIL_DATA]                        = "mkNilData",
    [CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA]                   = "mkNilPairData",
    [CARDANO_UPLC_BUILTIN_SERIALISE_DATA]                     = "serialiseData",
    [CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE]   = "verifyEcdsaSecp256k1Signature",
    [CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE] = "verifySchnorrSecp256k1Signature",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD]                   = "bls12_381_G1_add",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG]                   = "bls12_381_G1_neg",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL]            = "bls12_381_G1_scalarMul",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL]                 = "bls12_381_G1_equal",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS]              = "bls12_381_G1_compress",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS]            = "bls12_381_G1_uncompress",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP]         = "bls12_381_G1_hashToGroup",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD]                   = "bls12_381_G2_add",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG]                   = "bls12_381_G2_neg",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL]            = "bls12_381_G2_scalarMul",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL]                 = "bls12_381_G2_equal",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS]              = "bls12_381_G2_compress",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS]            = "bls12_381_G2_uncompress",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP]         = "bls12_381_G2_hashToGroup",
    [CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP]              = "bls12_381_millerLoop",
    [CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT]            = "bls12_381_mulMlResult",
    [CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY]             = "bls12_381_finalVerify",
    [CARDANO_UPLC_BUILTIN_KECCAK_256]                         = "keccak_256",
    [CARDANO_UPLC_BUILTIN_BLAKE2B_224]                        = "blake2b_224",
    [CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING]             = "integerToByteString",
    [CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER]             = "byteStringToInteger",
    [CARDANO_UPLC_BUILTIN_AND_BYTE_STRING]                    = "andByteString",
    [CARDANO_UPLC_BUILTIN_OR_BYTE_STRING]                     = "orByteString",
    [CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING]                    = "xorByteString",
    [CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING]             = "complementByteString",
    [CARDANO_UPLC_BUILTIN_READ_BIT]                           = "readBit",
    [CARDANO_UPLC_BUILTIN_WRITE_BITS]                         = "writeBits",
    [CARDANO_UPLC_BUILTIN_REPLICATE_BYTE]                     = "replicateByte",
    [CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING]                  = "shiftByteString",
    [CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING]                 = "rotateByteString",
    [CARDANO_UPLC_BUILTIN_COUNT_SET_BITS]                     = "countSetBits",
    [CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT]                 = "findFirstSetBit",
    [CARDANO_UPLC_BUILTIN_RIPEMD_160]                         = "ripemd_160",
    [CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER]                    = "expModInteger",
    [CARDANO_UPLC_BUILTIN_DROP_LIST]                          = "dropList",
    [CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY]                    = "lengthOfArray",
    [CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY]                      = "listToArray",
    [CARDANO_UPLC_BUILTIN_INDEX_ARRAY]                        = "indexArray",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL]      = "bls12_381_G1_multiScalarMul",
    [CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL]      = "bls12_381_G2_multiScalarMul",
    [CARDANO_UPLC_BUILTIN_INSERT_COIN]                        = "insertCoin",
    [CARDANO_UPLC_BUILTIN_LOOKUP_COIN]                        = "lookupCoin",
    [CARDANO_UPLC_BUILTIN_UNION_VALUE]                        = "unionValue",
    [CARDANO_UPLC_BUILTIN_VALUE_CONTAINS]                     = "valueContains",
    [CARDANO_UPLC_BUILTIN_VALUE_DATA]                         = "valueData",
    [CARDANO_UPLC_BUILTIN_UN_VALUE_DATA]                      = "unValueData",
    [CARDANO_UPLC_BUILTIN_SCALE_VALUE]                        = "scaleValue"
  };

  size_t i = 0U;

  for (i = 0U; i < (size_t)CARDANO_UPLC_BUILTIN_COUNT; ++i)
  {
    const char* name = names[i];

    if ((name != NULL) && (strlen(name) == length) && (memcmp(name, text, length) == 0))
    {
      *builtin = (cardano_uplc_builtin_t)i;

      return CARDANO_SUCCESS;
    }
  }

  return CARDANO_ERROR_DECODING;
}

/**
 * \brief Parses one plutus-data value in the textual data syntax.
 *
 * Reads one of \c Constr <i> [d, ...], \c Map [(k, v), ...], \c List [d, ...],
 * \c I <integer> or \c B #hex into a fresh \ref cardano_plutus_data_t. Recursion
 * is bounded by \p depth. The caller owns the returned reference and must release
 * it. Every interior object created on an error path is released before return.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, a newly created data value the caller owns.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for
 *         malformed syntax or nesting past the bound, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
parse_data(parser_t* parser, const uint32_t depth, cardano_plutus_data_t** out)
{
  if (depth >= PARSER_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  skip_trivia(parser);

  if (match_keyword(parser, "Constr"))
  {
    uint64_t                      tag    = 0U;
    cardano_plutus_list_t*        fields = NULL;
    cardano_constr_plutus_data_t* constr = NULL;
    cardano_error_t               result = read_u64(parser, &tag);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = expect_char(parser, '[');

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (cardano_plutus_list_new(&fields) != CARDANO_SUCCESS)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    skip_trivia(parser);

    while (peek_char(parser) != ']')
    {
      cardano_plutus_data_t* element = NULL;

      if (cardano_plutus_list_get_length(fields) > 0U)
      {
        if (expect_char(parser, ',') != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&fields);

          return CARDANO_ERROR_DECODING;
        }
      }

      result = parse_data(parser, depth + 1U, &element);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_list_unref(&fields);

        return result;
      }

      result = cardano_plutus_list_add(fields, element);
      cardano_plutus_data_unref(&element);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_list_unref(&fields);

        return result;
      }

      skip_trivia(parser);
    }

    if (expect_char(parser, ']') != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&fields);

      return CARDANO_ERROR_DECODING;
    }

    result = cardano_constr_plutus_data_new(tag, fields, &constr);
    cardano_plutus_list_unref(&fields);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_data_new_constr(constr, out);
    cardano_constr_plutus_data_unref(&constr);

    return result;
  }

  if (match_keyword(parser, "Map"))
  {
    cardano_plutus_map_t* map    = NULL;
    cardano_error_t       result = expect_char(parser, '[');

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (cardano_plutus_map_new(&map) != CARDANO_SUCCESS)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    skip_trivia(parser);

    while (peek_char(parser) != ']')
    {
      cardano_plutus_data_t* key   = NULL;
      cardano_plutus_data_t* value = NULL;

      if (cardano_plutus_map_get_length(map) > 0U)
      {
        if (expect_char(parser, ',') != CARDANO_SUCCESS)
        {
          cardano_plutus_map_unref(&map);

          return CARDANO_ERROR_DECODING;
        }
      }

      if (expect_char(parser, '(') != CARDANO_SUCCESS)
      {
        cardano_plutus_map_unref(&map);

        return CARDANO_ERROR_DECODING;
      }

      result = parse_data(parser, depth + 1U, &key);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_map_unref(&map);

        return result;
      }

      if (expect_char(parser, ',') != CARDANO_SUCCESS)
      {
        cardano_plutus_data_unref(&key);
        cardano_plutus_map_unref(&map);

        return CARDANO_ERROR_DECODING;
      }

      result = parse_data(parser, depth + 1U, &value);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_unref(&key);
        cardano_plutus_map_unref(&map);

        return result;
      }

      if (expect_char(parser, ')') != CARDANO_SUCCESS)
      {
        cardano_plutus_data_unref(&key);
        cardano_plutus_data_unref(&value);
        cardano_plutus_map_unref(&map);

        return CARDANO_ERROR_DECODING;
      }

      result = cardano_plutus_map_insert(map, key, value);
      cardano_plutus_data_unref(&key);
      cardano_plutus_data_unref(&value);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_map_unref(&map);

        return result;
      }

      skip_trivia(parser);
    }

    if (expect_char(parser, ']') != CARDANO_SUCCESS)
    {
      cardano_plutus_map_unref(&map);

      return CARDANO_ERROR_DECODING;
    }

    result = cardano_plutus_data_new_map(map, out);
    cardano_plutus_map_unref(&map);

    return result;
  }

  if (match_keyword(parser, "List"))
  {
    cardano_plutus_list_t* list   = NULL;
    cardano_error_t        result = expect_char(parser, '[');

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (cardano_plutus_list_new(&list) != CARDANO_SUCCESS)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    skip_trivia(parser);

    while (peek_char(parser) != ']')
    {
      cardano_plutus_data_t* element = NULL;

      if (cardano_plutus_list_get_length(list) > 0U)
      {
        if (expect_char(parser, ',') != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&list);

          return CARDANO_ERROR_DECODING;
        }
      }

      result = parse_data(parser, depth + 1U, &element);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_list_unref(&list);

        return result;
      }

      result = cardano_plutus_list_add(list, element);
      cardano_plutus_data_unref(&element);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_list_unref(&list);

        return result;
      }

      skip_trivia(parser);
    }

    if (expect_char(parser, ']') != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&list);

      return CARDANO_ERROR_DECODING;
    }

    result = cardano_plutus_data_new_list(list, out);
    cardano_plutus_list_unref(&list);

    return result;
  }

  if (match_keyword(parser, "I"))
  {
    cardano_bigint_t* integer = NULL;
    cardano_error_t   result  = read_bigint(parser, &integer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_data_new_integer(integer, out);
    cardano_bigint_unref(&integer);

    return result;
  }

  if (match_keyword(parser, "B"))
  {
    cardano_buffer_t* bytes  = NULL;
    cardano_error_t   result = read_hex_bytes(parser, &bytes);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_data_new_bytes(cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes), out);
    cardano_buffer_unref(&bytes);

    return result;
  }

  return CARDANO_ERROR_DECODING;
}

/**
 * \brief Parses a constant type descriptor following the \c con keyword.
 *
 * Reads a leaf type name (\c integer, \c bytestring, \c string, \c unit, \c bool,
 * \c data) or a parenthesised \c (list T) or \c (pair A B) descriptor into an
 * arena-allocated \ref cardano_uplc_type_t. The BLS element type names are
 * rejected with \ref CARDANO_ERROR_NOT_IMPLEMENTED.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, the arena-allocated type descriptor.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for an
 *         unknown or malformed type, \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS
 *         type, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
parse_type(parser_t* parser, const uint32_t depth, const cardano_uplc_type_t** out)
{
  cardano_uplc_type_t* built  = NULL;
  cardano_error_t      result = CARDANO_SUCCESS;

  if (depth >= PARSER_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  skip_trivia(parser);

  if (peek_char(parser) == '(')
  {
    parser->pos += 1U;

    if (match_keyword(parser, "list"))
    {
      const cardano_uplc_type_t* element = NULL;

      result = parse_type(parser, depth + 1U, &element);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (expect_char(parser, ')') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_LIST, element, NULL, &built);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *out = built;

      return CARDANO_SUCCESS;
    }

    if (match_keyword(parser, "array"))
    {
      const cardano_uplc_type_t* element = NULL;

      result = parse_type(parser, depth + 1U, &element);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (expect_char(parser, ')') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_ARRAY, element, NULL, &built);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *out = built;

      return CARDANO_SUCCESS;
    }

    if (match_keyword(parser, "pair"))
    {
      const cardano_uplc_type_t* fst = NULL;
      const cardano_uplc_type_t* snd = NULL;

      result = parse_type(parser, depth + 1U, &fst);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = parse_type(parser, depth + 1U, &snd);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (expect_char(parser, ')') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_PAIR, fst, snd, &built);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *out = built;

      return CARDANO_SUCCESS;
    }

    return CARDANO_ERROR_DECODING;
  }

  if (match_keyword(parser, "integer"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_INTEGER, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "bytestring"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_BYTE_STRING, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "string"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_STRING, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "unit"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_UNIT, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "bool"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_BOOL, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "data"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_DATA, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "bls12_381_G1_element"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_BLS_G1, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "bls12_381_G2_element"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_BLS_G2, NULL, NULL, &built);
  }
  else if (match_keyword(parser, "value"))
  {
    result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_VALUE, NULL, NULL, &built);
  }
  else
  {
    return CARDANO_ERROR_DECODING;
  }

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *out = built;

  return CARDANO_SUCCESS;
}

/**
 * \brief Builds the canonical value element type from the arena.
 *
 * The value constant is stored as a list of pairs whose element type is
 * \c (pair bytestring (list (pair bytestring integer))): a policy paired with a
 * list of (token, amount) pairs.
 *
 * \param[in] arena The arena to allocate the descriptor from.
 * \param[out] out On success, the element type descriptor.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
value_element_type(cardano_uplc_arena_t* arena, const cardano_uplc_type_t** out)
{
  const cardano_uplc_type_t* bytestring  = NULL;
  const cardano_uplc_type_t* integer     = NULL;
  const cardano_uplc_type_t* token_pair  = NULL;
  const cardano_uplc_type_t* token_list  = NULL;
  cardano_uplc_type_t*       policy_pair = NULL;
  cardano_error_t            result      = CARDANO_SUCCESS;

  result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BYTE_STRING, NULL, NULL, (cardano_uplc_type_t**)((void*)&bytestring));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, NULL, NULL, (cardano_uplc_type_t**)((void*)&integer));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, integer, (cardano_uplc_type_t**)((void*)&token_pair));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, token_pair, NULL, (cardano_uplc_type_t**)((void*)&token_list));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, token_list, &policy_pair);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *out = policy_pair;

  return CARDANO_SUCCESS;
}

/**
 * \brief The maximum byte length of a value policy or token key.
 */
#define PRV_VALUE_KEY_MAX_LEN ((size_t)32U)

/**
 * \brief Tests whether a quantity fits the signed 128-bit range.
 *
 * The valid range is \c -(2^127) .. \c (2^127 - 1).
 *
 * \param[in] amount The quantity to check.
 *
 * \return \c true when in range, \c false otherwise.
 */
static bool
value_amount_in_range(const cardano_bigint_t* amount)
{
  cardano_bigint_t* one   = NULL;
  cardano_bigint_t* limit = NULL;
  bool              ok    = false;

  if (cardano_bigint_bit_length(amount) < 128U)
  {
    return true;
  }

  if (cardano_bigint_from_int(1, &one) != CARDANO_SUCCESS)
  {
    return false;
  }

  if (cardano_bigint_from_int(0, &limit) != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&one);

    return false;
  }

  cardano_bigint_shift_left(one, 127U, limit);

  if (cardano_bigint_signum(amount) >= 0)
  {
    ok = (cardano_bigint_compare(amount, limit) < 0);
  }
  else
  {
    cardano_bigint_t* neg = NULL;

    if (cardano_bigint_from_int(0, &neg) == CARDANO_SUCCESS)
    {
      cardano_bigint_negate(limit, neg);
      ok = (cardano_bigint_compare(amount, neg) >= 0);
      cardano_bigint_unref(&neg);
    }
  }

  cardano_bigint_unref(&one);
  cardano_bigint_unref(&limit);

  return ok;
}

/**
 * \brief A borrowed view of arena bytes used as a value-map key during parsing.
 */
typedef struct bytes_view_t
{
  const byte_t* data;
  size_t        size;
} bytes_view_t;

/**
 * \brief Compares two byte views lexicographically.
 *
 * \param[in] a The first view.
 * \param[in] b The second view.
 *
 * \return A negative, zero, or positive value as \p a orders before, equal to, or
 *         after \p b.
 */
static int
value_compare_buffers(bytes_view_t a, bytes_view_t b)
{
  size_t n = (a.size < b.size) ? a.size : b.size;
  size_t i = 0U;

  for (i = 0U; i < n; ++i)
  {
    if (a.data[i] != b.data[i])
    {
      return (a.data[i] < b.data[i]) ? -1 : 1;
    }
  }

  if (a.size == b.size)
  {
    return 0;
  }

  return (a.size < b.size) ? -1 : 1;
}

/**
 * \brief A single accumulated token amount keyed by token bytes.
 */
typedef struct token_acc_t
{
  bytes_view_t  name;
  cardano_bigint_t* amount;
} token_acc_t;

/**
 * \brief A single accumulated policy entry keyed by policy bytes.
 */
typedef struct policy_acc_t
{
  bytes_view_t policy;
  token_acc_t* tokens;
  size_t           token_count;
  size_t           token_capacity;
} policy_acc_t;

/**
 * \brief Releases the working accumulator and all owned bigints.
 *
 * \param[in,out] policies The policy accumulator array.
 * \param[in] count The number of policy entries.
 */
static void
value_acc_free(policy_acc_t* policies, size_t count)
{
  size_t i = 0U;

  if (policies == NULL)
  {
    return;
  }

  for (i = 0U; i < count; ++i)
  {
    size_t j = 0U;

    for (j = 0U; j < policies[i].token_count; ++j)
    {
      cardano_bigint_unref(&policies[i].tokens[j].amount);
    }

    _cardano_free(policies[i].tokens);
  }

  _cardano_free(policies);
}

/**
 * \brief Inserts or sums a (policy, token, amount) triple into the accumulator.
 *
 * Keeps policies and tokens in ascending order; a repeated key sums the amount.
 *
 * \param[in,out] policies The policy accumulator array (grown as needed).
 * \param[in,out] count The number of policy entries.
 * \param[in,out] capacity The policy array capacity.
 * \param[in] policy The policy byte view.
 * \param[in] name The token byte view.
 * \param[in] amount The amount to add.
 *
 * \return \ref CARDANO_SUCCESS or an allocation error.
 */
static cardano_error_t
value_acc_insert(
  policy_acc_t**      policies,
  size_t*                 count,
  size_t*                 capacity,
  bytes_view_t        policy,
  bytes_view_t        name,
  const cardano_bigint_t* amount)
{
  size_t pi = 0U;
  size_t ti = 0U;

  while ((pi < *count) && (value_compare_buffers((*policies)[pi].policy, policy) < 0))
  {
    pi += 1U;
  }

  if ((pi >= *count) || (value_compare_buffers((*policies)[pi].policy, policy) != 0))
  {
    if (*count == *capacity)
    {
      size_t            next  = (*capacity == 0U) ? 4U : (*capacity * 2U);
      policy_acc_t* grown = (policy_acc_t*)_cardano_realloc(*policies, next * sizeof(policy_acc_t));

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      *policies = grown;
      *capacity = next;
    }

    {
      size_t k = *count;

      while (k > pi)
      {
        (*policies)[k] = (*policies)[k - 1U];
        k -= 1U;
      }
    }

    (*policies)[pi].policy         = policy;
    (*policies)[pi].tokens         = NULL;
    (*policies)[pi].token_count    = 0U;
    (*policies)[pi].token_capacity = 0U;
    *count += 1U;
  }

  {
    policy_acc_t* entry = &(*policies)[pi];

    while ((ti < entry->token_count) && (value_compare_buffers(entry->tokens[ti].name, name) < 0))
    {
      ti += 1U;
    }

    if ((ti < entry->token_count) && (value_compare_buffers(entry->tokens[ti].name, name) == 0))
    {
      cardano_bigint_t* sum = NULL;

      if (cardano_bigint_from_int(0, &sum) != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      cardano_bigint_add(entry->tokens[ti].amount, amount, sum);
      cardano_bigint_unref(&entry->tokens[ti].amount);
      entry->tokens[ti].amount = sum;

      return CARDANO_SUCCESS;
    }

    if (entry->token_count == entry->token_capacity)
    {
      size_t           next  = (entry->token_capacity == 0U) ? 4U : (entry->token_capacity * 2U);
      token_acc_t* grown = (token_acc_t*)_cardano_realloc(entry->tokens, next * sizeof(token_acc_t));

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      entry->tokens         = grown;
      entry->token_capacity = next;
    }

    {
      size_t k = entry->token_count;

      while (k > ti)
      {
        entry->tokens[k] = entry->tokens[k - 1U];
        k -= 1U;
      }
    }

    {
      cardano_bigint_t* clone = NULL;

      if (cardano_bigint_clone(amount, &clone) != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      entry->tokens[ti].name   = name;
      entry->tokens[ti].amount = clone;
      entry->token_count += 1U;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Builds a token-pair constant from a token buffer and an amount.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in] name The token byte view; copied into the arena.
 * \param[in] amount The amount; the constructor clones it.
 * \param[out] out On success, the token-pair constant.
 *
 * \return \ref CARDANO_SUCCESS or an allocation error.
 */
static cardano_error_t
value_make_token(
  cardano_uplc_arena_t*           arena,
  bytes_view_t                name,
  const cardano_bigint_t*         amount,
  const cardano_uplc_constant_t** out)
{
  cardano_bigint_t*        amt_copy   = NULL;
  cardano_uplc_constant_t* name_const = NULL;
  cardano_uplc_constant_t* amt_const  = NULL;
  cardano_uplc_constant_t* pair_const = NULL;
  cardano_error_t          error      = cardano_uplc_int_constant_new_byte_string_copy(arena, name.data, name.size, &name_const);

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_bigint_clone(amount, &amt_copy);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_constant_new_integer(arena, amt_copy, &amt_const);
    cardano_bigint_unref(&amt_copy);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_constant_new_pair(arena, name_const, amt_const, &pair_const);
  }

  if (error == CARDANO_SUCCESS)
  {
    *out = pair_const;
  }

  return error;
}

/**
 * \brief Normalizes a raw parsed value list into a canonical value constant.
 *
 * Validates key lengths (at most 32 bytes) and amounts (signed 128-bit range),
 * sums repeated keys, drops zero amounts and empty token maps, and sorts policies
 * and tokens ascending. A bound or key-length violation yields
 * \ref CARDANO_ERROR_DECODING (a parse error).
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] element_type The canonical value element type.
 * \param[in] raw The raw parsed list of policy entries.
 * \param[out] out On success, the normalized value constant.
 *
 * \return \ref CARDANO_SUCCESS, \ref CARDANO_ERROR_DECODING on a validation
 *         failure, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
normalize_value(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_type_t*     element_type,
  const cardano_uplc_constant_t* raw,
  cardano_uplc_constant_t**      out)
{
  policy_acc_t*               policies     = NULL;
  size_t                          count        = 0U;
  size_t                          capacity     = 0U;
  const cardano_uplc_constant_t** entries      = NULL;
  size_t                          out_count    = 0U;
  size_t                          i            = 0U;
  cardano_error_t                 error        = CARDANO_SUCCESS;
  const cardano_uplc_type_t*      token_pair   = element_type->snd->fst;

  for (i = 0U; (i < raw->as.list.count) && (error == CARDANO_SUCCESS); ++i)
  {
    const cardano_uplc_constant_t* entry  = raw->as.list.items[i];
    bytes_view_t               policy  = { entry->as.pair.fst->as.bytes.data, entry->as.pair.fst->as.bytes.size };
    const cardano_uplc_constant_t* tlist  = entry->as.pair.snd;
    size_t                         j      = 0U;

    if (policy.size > PRV_VALUE_KEY_MAX_LEN)
    {
      error = CARDANO_ERROR_DECODING;
      break;
    }

    for (j = 0U; (j < tlist->as.list.count) && (error == CARDANO_SUCCESS); ++j)
    {
      const cardano_uplc_constant_t* tok    = tlist->as.list.items[j];
      bytes_view_t               name   = { tok->as.pair.fst->as.bytes.data, tok->as.pair.fst->as.bytes.size };
      const cardano_bigint_t*        amount = NULL;

      if (cardano_uplc_constant_int_materialize(arena, tok->as.pair.snd, &amount) != CARDANO_SUCCESS)
      {
        error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        break;
      }

      if ((name.size > PRV_VALUE_KEY_MAX_LEN) || !value_amount_in_range(amount))
      {
        error = CARDANO_ERROR_DECODING;
        break;
      }

      error = value_acc_insert(&policies, &count, &capacity, policy, name, amount);
    }
  }

  if (error == CARDANO_SUCCESS)
  {
    entries = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, (count == 0U) ? 1U : (count * sizeof(const cardano_uplc_constant_t*)), 0U);

    if (entries == NULL)
    {
      error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  for (i = 0U; (i < count) && (error == CARDANO_SUCCESS); ++i)
  {
    const cardano_uplc_constant_t** tokens     = NULL;
    size_t                          tout       = 0U;
    size_t                          j          = 0U;
    cardano_uplc_constant_t*        policy_const = NULL;
    cardano_uplc_constant_t*        token_list   = NULL;
    cardano_uplc_constant_t*        pair_const   = NULL;

    tokens = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, (policies[i].token_count == 0U) ? 1U : (policies[i].token_count * sizeof(const cardano_uplc_constant_t*)), 0U);

    if (tokens == NULL)
    {
      error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      break;
    }

    for (j = 0U; (j < policies[i].token_count) && (error == CARDANO_SUCCESS); ++j)
    {
      if (cardano_bigint_is_zero(policies[i].tokens[j].amount))
      {
        continue;
      }

      error = value_make_token(arena, policies[i].tokens[j].name, policies[i].tokens[j].amount, &tokens[tout]);

      if (error == CARDANO_SUCCESS)
      {
        tout += 1U;
      }
    }

    if ((error != CARDANO_SUCCESS) || (tout == 0U))
    {
      continue;
    }

    error = cardano_uplc_int_constant_new_byte_string_copy(arena, policies[i].policy.data, policies[i].policy.size, &policy_const);

    if (error == CARDANO_SUCCESS)
    {
      error = cardano_uplc_constant_new_list(arena, token_pair, tokens, tout, &token_list);
    }

    if (error == CARDANO_SUCCESS)
    {
      error = cardano_uplc_constant_new_pair(arena, policy_const, token_list, &pair_const);
    }

    if (error == CARDANO_SUCCESS)
    {
      entries[out_count] = pair_const;
      out_count += 1U;
    }
  }

  value_acc_free(policies, count);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  return cardano_uplc_constant_new_value(arena, element_type, entries, out_count, out);
}

/**
 * \brief Parses a constant value for a known type descriptor.
 *
 * Reads the bare value form for \p type: a signed integer, a \c #hex byte string,
 * a quoted string, \c () for unit, \c True / \c False for bool, a bracketed list
 * \c [v, ...], a parenthesised pair \c (a, b), or the wrapped data form. The value
 * is built into an arena-allocated \ref cardano_uplc_constant_t. The BLS arms are
 * unreachable because \ref parse_type rejects BLS types first.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] type The type descriptor whose value to read.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, the arena-allocated constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed value or nesting past the bound, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
parse_value(parser_t* parser, const cardano_uplc_type_t* type, const uint32_t depth, const cardano_uplc_constant_t** out)
{
  cardano_uplc_constant_t* built  = NULL;
  cardano_error_t          result = CARDANO_SUCCESS;

  if (depth >= PARSER_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  skip_trivia(parser);

  switch (type->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      cardano_bigint_t* integer = NULL;

      result = read_bigint(parser, &integer);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_constant_new_integer(parser->arena, integer, &built);
      cardano_bigint_unref(&integer);
      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      cardano_buffer_t* bytes = NULL;

      result = read_hex_bytes(parser, &bytes);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_constant_new_byte_string(parser->arena, bytes, &built);
      cardano_buffer_unref(&bytes);
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      cardano_buffer_t* text = NULL;

      result = read_string(parser, &text);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_constant_new_string(parser->arena, text, &built);
      cardano_buffer_unref(&text);
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      if (expect_char(parser, '(') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      if (expect_char(parser, ')') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = cardano_uplc_constant_new_unit(parser->arena, &built);
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      if (match_keyword(parser, "True"))
      {
        result = cardano_uplc_constant_new_bool(parser->arena, true, &built);
      }
      else if (match_keyword(parser, "False"))
      {
        result = cardano_uplc_constant_new_bool(parser->arena, false, &built);
      }
      else
      {
        return CARDANO_ERROR_DECODING;
      }

      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      const cardano_uplc_constant_t** items    = NULL;
      size_t                          count    = 0U;
      size_t                          capacity = 0U;

      if (expect_char(parser, '[') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      skip_trivia(parser);

      while (peek_char(parser) != ']')
      {
        const cardano_uplc_constant_t* element = NULL;

        if (count > 0U)
        {
          if (expect_char(parser, ',') != CARDANO_SUCCESS)
          {
            return CARDANO_ERROR_DECODING;
          }
        }

        result = parse_value(parser, type->fst, depth + 1U, &element);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        if (count == capacity)
        {
          const size_t                    next  = (capacity == 0U) ? 4U : (capacity * 2U);
          const cardano_uplc_constant_t** grown = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(parser->arena, next * sizeof(const cardano_uplc_constant_t*), 0U);
          size_t                          i     = 0U;

          if (grown == NULL)
          {
            return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
          }

          for (i = 0U; i < count; ++i)
          {
            grown[i] = items[i];
          }

          items    = grown;
          capacity = next;
        }

        items[count] = element;
        count += 1U;

        skip_trivia(parser);
      }

      if (expect_char(parser, ']') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      if (type->kind == CARDANO_UPLC_TYPE_ARRAY)
      {
        result = cardano_uplc_constant_new_array(parser->arena, type->fst, items, count, &built);
      }
      else
      {
        result = cardano_uplc_constant_new_list(parser->arena, type->fst, items, count, &built);
      }
      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      const cardano_uplc_constant_t* fst = NULL;
      const cardano_uplc_constant_t* snd = NULL;

      if (expect_char(parser, '(') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = parse_value(parser, type->fst, depth + 1U, &fst);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (expect_char(parser, ',') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = parse_value(parser, type->snd, depth + 1U, &snd);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (expect_char(parser, ')') != CARDANO_SUCCESS)
      {
        return CARDANO_ERROR_DECODING;
      }

      result = cardano_uplc_constant_new_pair(parser->arena, fst, snd, &built);
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      cardano_plutus_data_t* data = NULL;

      result = parse_data(parser, depth + 1U, &data);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_constant_new_data(parser->arena, data, &built);
      cardano_plutus_data_unref(&data);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      cardano_buffer_t* bytes = NULL;

      result = read_0x_bytes(parser, &bytes);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (type->kind == CARDANO_UPLC_TYPE_BLS_G1)
      {
        result = cardano_uplc_int_bls_g1_from_compressed(parser->arena, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes), &built);
      }
      else
      {
        result = cardano_uplc_int_bls_g2_from_compressed(parser->arena, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes), &built);
      }

      cardano_buffer_unref(&bytes);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    {
      return CARDANO_ERROR_DECODING;
    }
    case CARDANO_UPLC_TYPE_VALUE:
    {
      const cardano_uplc_type_t*     element_type = NULL;
      const cardano_uplc_type_t*     list_type    = NULL;
      const cardano_uplc_constant_t* list_const   = NULL;

      result = value_element_type(parser->arena, &element_type);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_type_new(parser->arena, CARDANO_UPLC_TYPE_LIST, element_type, NULL, (cardano_uplc_type_t**)((void*)&list_type));

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = parse_value(parser, list_type, depth + 1U, &list_const);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = normalize_value(parser->arena, element_type, list_const, &built);
      break;
    }
    default:
    {
      return CARDANO_ERROR_DECODING;
    }
  }

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *out = built;

  return CARDANO_SUCCESS;
}

/**
 * \brief Parses a constant body following the \c con keyword.
 *
 * Reads the type descriptor and then the value at that type. The \c data form is
 * written \c data ( <data> ), with the data wrapped in its own parentheses, which
 * this matches.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, the arena-allocated constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed constant, \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS type,
 *         or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
parse_constant(parser_t* parser, const uint32_t depth, const cardano_uplc_constant_t** out)
{
  const cardano_uplc_type_t* type   = NULL;
  cardano_error_t            result = parse_type(parser, depth, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (type->kind == CARDANO_UPLC_TYPE_DATA)
  {
    if (expect_char(parser, '(') != CARDANO_SUCCESS)
    {
      return CARDANO_ERROR_DECODING;
    }

    result = parse_value(parser, type, depth, out);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (expect_char(parser, ')') != CARDANO_SUCCESS)
    {
      return CARDANO_ERROR_DECODING;
    }

    return CARDANO_SUCCESS;
  }

  return parse_value(parser, type, depth, out);
}

/**
 * \brief Collects a space-separated run of terms until a closing delimiter.
 *
 * Parses zero or more terms, each preceded by trivia, into an arena-allocated
 * pointer array, stopping when the next non-trivia byte is \p terminator. Used for
 * the field list of \c constr and the branch list of \c case.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] depth The current nesting depth, passed to each term parse.
 * \param[in] terminator The byte that ends the run.
 * \param[out] items On success, the arena-allocated array, or NULL when empty.
 * \param[out] count On success, the number of terms read.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated parse or allocation
 *         error.
 */
static cardano_error_t
parse_term_run(parser_t* parser, const uint32_t depth, const char terminator, const cardano_uplc_term_t* const** items, size_t* count)
{
  const cardano_uplc_term_t** array    = NULL;
  size_t                      used     = 0U;
  size_t                      capacity = 0U;

  skip_trivia(parser);

  while (peek_char(parser) != terminator)
  {
    const cardano_uplc_term_t* element = NULL;
    cardano_error_t            result  = parse_term(parser, depth + 1U, &element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (used == capacity)
    {
      const size_t                next  = (capacity == 0U) ? 4U : (capacity * 2U);
      const cardano_uplc_term_t** grown = (const cardano_uplc_term_t**)cardano_uplc_arena_alloc(parser->arena, next * sizeof(const cardano_uplc_term_t*), 0U);
      size_t                      i     = 0U;

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      for (i = 0U; i < used; ++i)
      {
        grown[i] = array[i];
      }

      array    = grown;
      capacity = next;
    }

    array[used] = element;
    used += 1U;

    skip_trivia(parser);
  }

  *items = array;
  *count = used;

  return CARDANO_SUCCESS;
}

/**
 * \brief Tests whether the program version admits the \c constr and \c case forms.
 *
 * The sum-of-products terms were introduced at UPLC version 1.1.0, so a program
 * with an earlier version header must reject them with a parse error.
 *
 * \param[in] parser The parser holding the parsed version triple.
 *
 * \return \c true if the version is at least 1.1.0, else \c false.
 */
static bool
supports_sums(const parser_t* parser)
{
  if (parser->version_major != 1U)
  {
    return parser->version_major > 1U;
  }

  return parser->version_minor >= 1U;
}

/**
 * \brief Parses a parenthesised term form after the opening \c '('.
 *
 * Dispatches on the keyword that follows the paren: \c con, \c lam, \c delay,
 * \c force, \c error, \c builtin, \c constr or \c case, then consumes the closing
 * paren. The \c constr and \c case forms require version 1.1.0 or later and are a
 * parse error under an earlier version. Only \c lam introduces a binder; it pushes
 * the parameter name on the scope stack for the body and pops it afterward.
 *
 * \param[in,out] parser The parser to advance, positioned just past the \c '('.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, the arena-allocated term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed form, \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS constant,
 *         or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
parse_paren_term(parser_t* parser, const uint32_t depth, const cardano_uplc_term_t** out)
{
  cardano_uplc_term_t* built  = NULL;
  cardano_error_t      result = CARDANO_SUCCESS;

  if (match_keyword(parser, "con"))
  {
    const cardano_uplc_constant_t* constant = NULL;

    result = parse_constant(parser, depth + 1U, &constant);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_constant(parser->arena, constant, &built);
  }
  else if (match_keyword(parser, "lam"))
  {
    const char*                name    = NULL;
    size_t                     length  = 0U;
    const cardano_uplc_term_t* body    = NULL;

    result = read_ident(parser, &name, &length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = scope_push(parser, name, length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = parse_term(parser, depth + 1U, &body);
    scope_pop(parser);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_lambda(parser->arena, body, &built);
  }
  else if (match_keyword(parser, "delay"))
  {
    const cardano_uplc_term_t* body = NULL;

    result = parse_term(parser, depth + 1U, &body);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_delay(parser->arena, body, &built);
  }
  else if (match_keyword(parser, "force"))
  {
    const cardano_uplc_term_t* body = NULL;

    result = parse_term(parser, depth + 1U, &body);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_force(parser->arena, body, &built);
  }
  else if (match_keyword(parser, "error"))
  {
    result = cardano_uplc_term_new_error(parser->arena, &built);
  }
  else if (match_keyword(parser, "builtin"))
  {
    const char*            name    = NULL;
    size_t                 length  = 0U;
    cardano_uplc_builtin_t builtin = CARDANO_UPLC_BUILTIN_ADD_INTEGER;

    result = read_ident(parser, &name, &length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = resolve_builtin(name, length, &builtin);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_builtin(parser->arena, builtin, &built);
  }
  else if (match_keyword(parser, "constr"))
  {
    uint64_t                          tag    = 0U;
    const cardano_uplc_term_t* const* fields = NULL;
    size_t                            count  = 0U;

    if (!supports_sums(parser))
    {
      return CARDANO_ERROR_DECODING;
    }

    result = read_u64(parser, &tag);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = parse_term_run(parser, depth, ')', &fields, &count);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_constr(parser->arena, tag, fields, count, &built);
  }
  else if (match_keyword(parser, "case"))
  {
    const cardano_uplc_term_t*        scrutinee = NULL;
    const cardano_uplc_term_t* const* branches  = NULL;
    size_t                            count     = 0U;

    if (!supports_sums(parser))
    {
      return CARDANO_ERROR_DECODING;
    }

    result = parse_term(parser, depth + 1U, &scrutinee);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = parse_term_run(parser, depth, ')', &branches, &count);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_case(parser->arena, scrutinee, branches, count, &built);
  }
  else
  {
    return CARDANO_ERROR_DECODING;
  }

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (expect_char(parser, ')') != CARDANO_SUCCESS)
  {
    return CARDANO_ERROR_DECODING;
  }

  *out = built;

  return CARDANO_SUCCESS;
}

/**
 * \brief Parses an application after the opening \c '['.
 *
 * Reads two or more terms and folds them left-associatively into nested apply
 * terms, so \c [ f x y ] builds \c ((f x) y). An application with fewer than two
 * terms is a parse error. The closing \c ']' is consumed.
 *
 * \param[in,out] parser The parser to advance, positioned just past the \c '['.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, the arena-allocated apply term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed application, or a propagated allocation error.
 */
static cardano_error_t
parse_apply(parser_t* parser, const uint32_t depth, const cardano_uplc_term_t** out)
{
  const cardano_uplc_term_t* accumulator = NULL;
  cardano_error_t            result       = parse_term(parser, depth + 1U, &accumulator);
  size_t                     argument_count = 0U;

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  skip_trivia(parser);

  while (peek_char(parser) != ']')
  {
    const cardano_uplc_term_t* argument = NULL;
    cardano_uplc_term_t*       applied  = NULL;

    result = parse_term(parser, depth + 1U, &argument);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_apply(parser->arena, accumulator, argument, &applied);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    accumulator = applied;
    argument_count += 1U;

    skip_trivia(parser);
  }

  if (expect_char(parser, ']') != CARDANO_SUCCESS)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (argument_count == 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  *out = accumulator;

  return CARDANO_SUCCESS;
}

/**
 * \brief Parses one term: a variable, an application, or a parenthesised form.
 *
 * A leading \c '[' starts an application, a leading \c '(' a parenthesised form,
 * and a leading identifier byte a variable, which is resolved to its de Bruijn
 * index against the scope stack. Any other byte is a parse error. Recursion is
 * bounded by \p depth.
 *
 * \param[in,out] parser The parser to advance.
 * \param[in] depth The current nesting depth, checked against the bound.
 * \param[out] out On success, the arena-allocated term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed term, an unbound variable or nesting past the bound,
 *         \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS constant, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED.
 */
static cardano_error_t
parse_term(parser_t* parser, const uint32_t depth, const cardano_uplc_term_t** out)
{
  char c = '\0';

  if (depth >= PARSER_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  skip_trivia(parser);

  c = peek_char(parser);

  if (c == '[')
  {
    parser->pos += 1U;

    return parse_apply(parser, depth, out);
  }

  if (c == '(')
  {
    parser->pos += 1U;

    return parse_paren_term(parser, depth, out);
  }

  if (is_ident(c) && !is_digit(c))
  {
    const char*          name   = NULL;
    size_t               length = 0U;
    uint64_t             index  = 0U;
    cardano_uplc_term_t* built  = NULL;
    cardano_error_t      result = read_ident(parser, &name, &length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = resolve_name_index(parser, name, length, &index);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_var(parser->arena, index, &built);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    *out = built;

    return CARDANO_SUCCESS;
  }

  return CARDANO_ERROR_DECODING;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_uplc_parse_program(
  cardano_uplc_arena_t*          arena,
  const char*                    text,
  size_t                         len,
  const cardano_uplc_program_t** program,
  size_t*                        error_offset)
{
  parser_t               parser = { NULL, NULL, 0U, 0U, NULL, 0U, 0U, 0U, 0U };
  cardano_uplc_program_t*    result = NULL;
  const cardano_uplc_term_t* term   = NULL;
  uint64_t                   major  = 0U;
  uint64_t                   minor  = 0U;
  uint64_t                   patch  = 0U;
  cardano_error_t            status = CARDANO_SUCCESS;

  if ((arena == NULL) || (program == NULL) || ((text == NULL) && (len != 0U)))
  {
    if (error_offset != NULL)
    {
      *error_offset = 0U;
    }

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  parser.arena = arena;
  parser.text  = (text != NULL) ? text : "";
  parser.len   = len;

  status = expect_char(&parser, '(');

  if (status == CARDANO_SUCCESS)
  {
    status = match_keyword(&parser, "program") ? CARDANO_SUCCESS : CARDANO_ERROR_DECODING;
  }

  if (status == CARDANO_SUCCESS)
  {
    status = read_u64(&parser, &major);
  }

  if (status == CARDANO_SUCCESS)
  {
    status = expect_char(&parser, '.');
  }

  if (status == CARDANO_SUCCESS)
  {
    status = read_u64(&parser, &minor);
  }

  if (status == CARDANO_SUCCESS)
  {
    status = expect_char(&parser, '.');
  }

  if (status == CARDANO_SUCCESS)
  {
    status = read_u64(&parser, &patch);
  }

  if ((status == CARDANO_SUCCESS) && ((major > UINT32_MAX) || (minor > UINT32_MAX) || (patch > UINT32_MAX)))
  {
    status = CARDANO_ERROR_DECODING;
  }

  if (status == CARDANO_SUCCESS)
  {
    parser.version_major = major;
    parser.version_minor = minor;

    status = parse_term(&parser, 0U, &term);
  }

  if (status == CARDANO_SUCCESS)
  {
    status = expect_char(&parser, ')');
  }

  if (status == CARDANO_SUCCESS)
  {
    skip_trivia(&parser);

    if (parser.pos != parser.len)
    {
      status = CARDANO_ERROR_DECODING;
    }
  }

  if (status == CARDANO_SUCCESS)
  {
    result = (cardano_uplc_program_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U);

    if (result == NULL)
    {
      status = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  if (status == CARDANO_SUCCESS)
  {
    result->version_major = (uint32_t)major;
    result->version_minor = (uint32_t)minor;
    result->version_patch = (uint32_t)patch;
    result->term          = term;

    *program = result;
  }

  if (error_offset != NULL)
  {
    *error_offset = parser.pos;
  }

  _cardano_free(parser.scope);

  return status;
}
