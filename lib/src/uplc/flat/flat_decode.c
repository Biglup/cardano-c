/**
 * \file flat_decode.c
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

#include "flat_decode.h"

#include "../ast/uplc_int.h"
#include "../ast/uplc_lang_version.h"
#include "../builtins/builtins.h"
#include "../builtins/uplc_builtin.h"
#include "../data/uplc_data.h"

#include <cardano/cbor/cbor_reader.h>

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Flat const-type list tags.
 *
 * These are the 4-bit tags read from the const-type list, fixed by the protocol
 * (\c encodeUni in plutus-core/src/PlutusCore/Default/Universe.hs). They are
 * distinct from \ref cardano_uplc_type_kind_t enumerator values: the descriptor
 * enum places \c data at 7, the BLS kinds at 8..10, \c array at 11 and \c value
 * at 12, while the flat tags place \c apply at 7, \c data at 8, the BLS tags at
 * 9..11, \c array at 12 and \c value at 13.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_INTEGER = 0U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BYTE_STRING = 1U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_STRING = 2U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_UNIT = 3U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BOOL = 4U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_LIST = 5U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_PAIR = 6U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_APPLY = 7U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_DATA = 8U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BLS_G1 = 9U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BLS_G2 = 10U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BLS_ML_RESULT = 11U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_ARRAY = 12U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_VALUE = 13U;

/**
 * \brief Number of bits in a single const-type tag.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BITS = 4U;

/**
 * \brief Flat term tags.
 *
 * These are the 4-bit tags read for each term form, fixed by the protocol. They
 * mirror \ref cardano_uplc_term_kind_t one for one.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_VAR = 0U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_DELAY = 1U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_LAMBDA = 2U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_APPLY = 3U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_CONSTANT = 4U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_FORCE = 5U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_ERROR = 6U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_BUILTIN = 7U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_CONSTR = 8U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_CASE = 9U;

/**
 * \brief Number of bits in a single term tag.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TERM_TAG_BITS = 4U;

/**
 * \brief Number of bits in a builtin tag.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_BUILTIN_TAG_BITS = 7U;

/**
 * \brief The first program version, 1.1.0, whose terms may use \c constr and \c case.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t FLAT_CONSTR_CASE_MIN_MAJOR = 1U;
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t FLAT_CONSTR_CASE_MIN_MINOR = 1U;

/**
 * \brief Upper bound on the term nesting depth the decoder will follow.
 *
 * Term decoding recurses through \c Delay, \c Lambda, \c Apply, \c Force, \c Constr
 * and \c Case; recursing on an adversarial program without a bound would overflow
 * the C stack. The decoder refuses past this depth with \ref CARDANO_ERROR_DECODING.
 * The bound is generous relative to any real script yet small enough to keep the
 * worst-case recursion well inside the stack.
 */
enum
{
  FLAT_TERM_MAX_DEPTH = 4096
};

/**
 * \brief Upper bound on the nesting depth of a type descriptor or a value.
 *
 * A malformed program can encode an arbitrarily deep nesting of \c list and
 * \c pair applications; recursing on it without a bound would overflow the C
 * stack. The parser refuses past this depth with \ref CARDANO_ERROR_DECODING.
 * The bound is generous relative to any real script yet small enough to keep the
 * worst-case recursion well inside the stack. It is an enum constant so it can
 * also size the on-stack tag buffer.
 */
enum
{
  FLAT_TYPE_MAX_DEPTH = 128
};

/**
 * \brief The largest constant type header a ledger script may carry, counted in
 *        flat type tags (\c maxHeaderSize in
 *        plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t FLAT_SCRIPT_MAX_HEADER_SIZE = 32U;

/**
 * \brief The largest number of fields a \c constr term in a ledger script may
 *        carry (\c maxConstrFields in
 *        plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t FLAT_SCRIPT_MAX_CONSTR_FIELDS = 1024U;

/* STRUCTURES ****************************************************************/

/**
 * \brief The rules a program decode applies to every term it reads.
 *
 * \c allow_constr_case follows the program version. When \c ledger_checks is set
 * the decode also applies the ledger's script checks for \c language at
 * \c protocol_major: every builtin must be available, every constant type header
 * must fit \ref FLAT_SCRIPT_MAX_HEADER_SIZE and every \c constr must have at most
 * \ref FLAT_SCRIPT_MAX_CONSTR_FIELDS fields.
 */
typedef struct
{
    bool                        allow_constr_case;
    bool                        ledger_checks;
    cardano_uplc_lang_version_t language;
    uint64_t                    protocol_major;
} decode_rules_t;

/**
 * \brief The rules of a decode without the ledger script checks, in which
 *        \c constr and \c case are accepted until a program version says otherwise.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const decode_rules_t UNRESTRICTED_RULES = { true, false, CARDANO_UPLC_LANG_VERSION_V1, 0U };

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Validates that a byte range is well-formed UTF-8.
 *
 * Applies the Unicode well-formed-UTF-8 rules: ASCII bytes stand alone; lead
 * bytes 0xC2..0xDF, 0xE0..0xEF and 0xF0..0xF4 introduce 2-, 3- and 4-byte
 * sequences whose continuation bytes are 0x80..0xBF, with the ranges narrowed to
 * reject overlong encodings, UTF-16 surrogates and code points above U+10FFFF.
 *
 * \param[in] data The bytes to validate. May be NULL only when \p size is 0.
 * \param[in] size The number of bytes.
 *
 * \return \c true if the range is valid UTF-8, \c false otherwise.
 */
static bool
is_valid_utf8(const byte_t* data, const size_t size)
{
  size_t i = 0U;

  while (i < size)
  {
    const byte_t b0 = data[i];

    if (b0 <= 0x7FU)
    {
      i += 1U;
    }
    else if ((b0 >= 0xC2U) && (b0 <= 0xDFU))
    {
      if ((i + 1U) >= size)
      {
        return false;
      }

      if ((data[i + 1U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      i += 2U;
    }
    else if ((b0 >= 0xE0U) && (b0 <= 0xEFU))
    {
      byte_t lo = 0x80U;
      byte_t hi = 0xBFU;

      if ((i + 2U) >= size)
      {
        return false;
      }

      if (b0 == 0xE0U)
      {
        lo = 0xA0U;
      }
      else if (b0 == 0xEDU)
      {
        hi = 0x9FU;
      }
      else
      {
        lo = 0x80U;
        hi = 0xBFU;
      }

      if ((data[i + 1U] < lo) || (data[i + 1U] > hi))
      {
        return false;
      }

      if ((data[i + 2U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      i += 3U;
    }
    else if ((b0 >= 0xF0U) && (b0 <= 0xF4U))
    {
      byte_t lo = 0x80U;
      byte_t hi = 0xBFU;

      if ((i + 3U) >= size)
      {
        return false;
      }

      if (b0 == 0xF0U)
      {
        lo = 0x90U;
      }
      else if (b0 == 0xF4U)
      {
        hi = 0x8FU;
      }
      else
      {
        lo = 0x80U;
        hi = 0xBFU;
      }

      if ((data[i + 1U] < lo) || (data[i + 1U] > hi))
      {
        return false;
      }

      if ((data[i + 2U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      if ((data[i + 3U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      i += 4U;
    }
    else
    {
      return false;
    }
  }

  return true;
}

/**
 * \brief Maps a leaf flat type tag to its \ref cardano_uplc_type_kind_t.
 *
 * Handles only the tags that name a non-application type. The list, array and
 * pair applications are handled by the recursive type parser, and the apply tag
 * never reaches this function.
 *
 * \param[in] tag The flat type tag.
 * \param[out] kind On success, the corresponding descriptor kind.
 *
 * \return \ref CARDANO_SUCCESS on a known leaf tag, or \ref CARDANO_ERROR_DECODING
 *         for an unknown or non-leaf tag.
 */
static cardano_error_t
leaf_kind(const uint8_t tag, cardano_uplc_type_kind_t* kind)
{
  cardano_error_t result = CARDANO_SUCCESS;

  if (tag == FLAT_TYPE_TAG_INTEGER)
  {
    *kind = CARDANO_UPLC_TYPE_INTEGER;
  }
  else if (tag == FLAT_TYPE_TAG_BYTE_STRING)
  {
    *kind = CARDANO_UPLC_TYPE_BYTE_STRING;
  }
  else if (tag == FLAT_TYPE_TAG_STRING)
  {
    *kind = CARDANO_UPLC_TYPE_STRING;
  }
  else if (tag == FLAT_TYPE_TAG_UNIT)
  {
    *kind = CARDANO_UPLC_TYPE_UNIT;
  }
  else if (tag == FLAT_TYPE_TAG_BOOL)
  {
    *kind = CARDANO_UPLC_TYPE_BOOL;
  }
  else if (tag == FLAT_TYPE_TAG_DATA)
  {
    *kind = CARDANO_UPLC_TYPE_DATA;
  }
  else if (tag == FLAT_TYPE_TAG_BLS_G1)
  {
    *kind = CARDANO_UPLC_TYPE_BLS_G1;
  }
  else if (tag == FLAT_TYPE_TAG_BLS_G2)
  {
    *kind = CARDANO_UPLC_TYPE_BLS_G2;
  }
  else if (tag == FLAT_TYPE_TAG_BLS_ML_RESULT)
  {
    *kind = CARDANO_UPLC_TYPE_BLS_ML_RESULT;
  }
  else if (tag == FLAT_TYPE_TAG_VALUE)
  {
    *kind = CARDANO_UPLC_TYPE_VALUE;
  }
  else
  {
    result = CARDANO_ERROR_DECODING;
  }

  return result;
}

/**
 * \brief Builds a type descriptor from the collected const-type tag list.
 *
 * Consumes tags starting at \p index. A leaf tag yields a leaf type. The apply
 * tag drives the three application shapes used by flat: <tt>[7, 5, T]</tt> builds
 * <tt>list(T)</tt>, <tt>[7, 12, T]</tt> builds <tt>array(T)</tt> and
 * <tt>[7, 7, 6, A, B]</tt> builds <tt>pair(A, B)</tt>, exactly as \c decode_type
 * does in the references. Recursion is bounded by \ref FLAT_TYPE_MAX_DEPTH.
 *
 * \param[in] arena The arena every descriptor node is allocated from.
 * \param[in] tags The collected 4-bit type tags.
 * \param[in] tag_count The number of tags in \p tags.
 * \param[in,out] index The cursor into \p tags, advanced past the parsed type.
 * \param[in] depth The current nesting depth.
 * \param[out] type On success, the parsed type descriptor.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated or unknown tag sequence or nesting past the depth bound, or
 *         a propagated allocation error from the descriptor constructor.
 */
static cardano_error_t
parse_type(
  cardano_uplc_arena_t*       arena,
  const uint8_t*              tags,
  const size_t                tag_count,
  size_t*                     index,
  const size_t                depth,
  const cardano_uplc_type_t** type)
{
  uint8_t                  tag    = 0U;
  cardano_error_t          result = CARDANO_SUCCESS;
  cardano_uplc_type_kind_t kind   = CARDANO_UPLC_TYPE_INTEGER;

  if (depth > FLAT_TYPE_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (*index >= tag_count)
  {
    return CARDANO_ERROR_DECODING;
  }

  tag    = tags[*index];
  *index += 1U;

  if (tag == FLAT_TYPE_TAG_APPLY)
  {
    if (*index >= tag_count)
    {
      return CARDANO_ERROR_DECODING;
    }

    {
      const uint8_t applied = tags[*index];
      *index                += 1U;

      if ((applied == FLAT_TYPE_TAG_LIST) || (applied == FLAT_TYPE_TAG_ARRAY))
      {
        const cardano_uplc_type_t*     element   = NULL;
        cardano_uplc_type_t*           built     = NULL;
        const cardano_uplc_type_kind_t container = (applied == FLAT_TYPE_TAG_LIST) ? CARDANO_UPLC_TYPE_LIST : CARDANO_UPLC_TYPE_ARRAY;

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = parse_type(arena, tags, tag_count, index, depth + 1U, &element);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        result = cardano_uplc_type_new(arena, container, element, NULL, &built);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        *type = built;

        return CARDANO_SUCCESS;
      }

      if (applied == FLAT_TYPE_TAG_APPLY)
      {
        if (*index >= tag_count)
        {
          return CARDANO_ERROR_DECODING;
        }

        if (tags[*index] != FLAT_TYPE_TAG_PAIR)
        {
          return CARDANO_ERROR_DECODING;
        }

        *index += 1U;

        {
          const cardano_uplc_type_t* fst   = NULL;
          const cardano_uplc_type_t* snd   = NULL;
          cardano_uplc_type_t*       built = NULL;

          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = parse_type(arena, tags, tag_count, index, depth + 1U, &fst);

          if (result != CARDANO_SUCCESS)
          {
            return result;
          }

          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = parse_type(arena, tags, tag_count, index, depth + 1U, &snd);

          if (result != CARDANO_SUCCESS)
          {
            return result;
          }

          result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, fst, snd, &built);

          if (result != CARDANO_SUCCESS)
          {
            return result;
          }

          *type = built;

          return CARDANO_SUCCESS;
        }
      }

      return CARDANO_ERROR_DECODING;
    }
  }

  result = leaf_kind(tag, &kind);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  {
    cardano_uplc_type_t* built = NULL;

    result = cardano_uplc_type_new(arena, kind, NULL, NULL, &built);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    *type = built;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads the const-type list and parses it into a type descriptor.
 *
 * The const-type list is a sequence of 4-bit tags, each preceded by a one
 * continuation bit; a zero continuation bit ends the list. The collected tags are
 * then parsed into a (possibly nested) type descriptor.
 *
 * \param[in] arena The arena every descriptor node is allocated from.
 * \param[in,out] reader The flat reader, advanced past the type list.
 * \param[out] type On success, the parsed type descriptor.
 * \param[out] header_size On success, the number of tags in the type list.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated stream, an empty list, an unconsumed trailing tag or an
 *         unknown tag sequence, or a propagated allocation error.
 */
static cardano_error_t
read_type(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_type_t** type,
  size_t*                     header_size)
{
  uint8_t         tags[FLAT_TYPE_MAX_DEPTH];
  size_t          tag_count = 0U;
  size_t          index     = 0U;
  cardano_error_t result    = CARDANO_SUCCESS;
  uint8_t         more      = 0U;

  result = cardano_uplc_flat_reader_bit(reader, &more);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  while (more != 0U)
  {
    uint8_t tag = 0U;

    if (tag_count >= FLAT_TYPE_MAX_DEPTH)
    {
      return CARDANO_ERROR_DECODING;
    }

    result = cardano_uplc_flat_reader_bits8(reader, FLAT_TYPE_TAG_BITS, &tag);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    tags[tag_count] = tag;
    tag_count       += 1U;

    result = cardano_uplc_flat_reader_bit(reader, &more);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (tag_count == 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = parse_type(arena, tags, tag_count, &index, 0U, type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (index != tag_count)
  {
    return CARDANO_ERROR_DECODING;
  }

  *header_size = tag_count;

  return CARDANO_SUCCESS;
}

/* Forward declaration: value decoding recurses on list and pair types. */
static cardano_error_t
read_value(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_type_t*  type,
  size_t                      depth,
  cardano_uplc_constant_t**   constant);

/**
 * \brief Reads an integer constant value.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[out] constant On success, the integer constant.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated decode or allocation
 *         error.
 */
static cardano_error_t
read_integer(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  cardano_uplc_constant_t**   constant)
{
  cardano_bigint_t* value  = NULL;
  int64_t           small  = 0;
  cardano_error_t   result = cardano_uplc_flat_reader_integer(reader, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (cardano_uplc_int_bigint_fits_int64(value, &small))
  {
    result = cardano_uplc_constant_new_integer_small(arena, small, constant);
  }
  else
  {
    result = cardano_uplc_constant_new_integer(arena, value, constant);
  }

  cardano_bigint_unref(&value);

  return result;
}

/**
 * \brief Reads a byte-string constant value.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[out] constant On success, the byte-string constant.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated decode or allocation
 *         error.
 */
static cardano_error_t
read_byte_string(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  cardano_uplc_constant_t**   constant)
{
  cardano_buffer_t* bytes  = NULL;
  cardano_error_t   result = cardano_uplc_flat_reader_bytes(reader, &bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_constant_new_byte_string(arena, bytes, constant);

  cardano_buffer_unref(&bytes);

  return result;
}

/**
 * \brief Reads a UTF-8 string constant value.
 *
 * The bytes are read as a flat bytestring and then validated as UTF-8; invalid
 * UTF-8 is rejected with \ref CARDANO_ERROR_DECODING.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[out] constant On success, the string constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for
 *         invalid UTF-8, or a propagated decode or allocation error.
 */
static cardano_error_t
read_string(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  cardano_uplc_constant_t**   constant)
{
  cardano_buffer_t* bytes  = NULL;
  cardano_error_t   result = cardano_uplc_flat_reader_bytes(reader, &bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (!is_valid_utf8(cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes)))
  {
    cardano_buffer_unref(&bytes);

    return CARDANO_ERROR_DECODING;
  }

  result = cardano_uplc_constant_new_string(arena, bytes, constant);

  cardano_buffer_unref(&bytes);

  return result;
}

/**
 * \brief Reads a data constant value.
 *
 * The data is encoded as a flat bytestring whose contents are CBOR; those bytes
 * are decoded directly into an arena data node through
 * \ref cardano_uplc_data_from_cbor_bytes with no per-node caching.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[out] constant On success, the data constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for
 *         malformed CBOR, or a propagated decode or allocation error.
 */
static cardano_error_t
read_data(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  cardano_uplc_constant_t**   constant)
{
  cardano_buffer_t*    bytes  = NULL;
  cardano_uplc_data_t* data   = NULL;
  cardano_error_t      result = cardano_uplc_flat_reader_bytes(reader, &bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_data_from_cbor_bytes(arena, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes), &data);

  cardano_buffer_unref(&bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_uplc_int_constant_new_data_node(arena, data, constant);
}

/**
 * \brief Reads a cons-bit sequence of constant values of one element type.
 *
 * A one bit precedes each element and a zero bit ends the sequence. Each element
 * is read at \p element_type and the element pointers are gathered into an arena
 * array. This is the flat form of a list, of an array (the reference encodes a
 * vector as a list) and of each level of a value.
 *
 * \param[in] arena The arena the elements and the element array are allocated from.
 * \param[in,out] reader The flat reader, advanced past the sequence.
 * \param[in] element_type The element type descriptor.
 * \param[in] depth The current value nesting depth.
 * \param[out] items On success, the element array, or NULL when \p count is 0.
 * \param[out] count On success, the number of elements.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated stream or nesting past the depth bound, or a propagated
 *         allocation error.
 */
static cardano_error_t
read_items(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_type_t*  element_type,
  const size_t                depth,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_constant_t*** items,
  size_t*                          count)
{
  const cardano_uplc_constant_t** list     = NULL;
  size_t                          length   = 0U;
  size_t                          capacity = 0U;
  cardano_error_t                 result   = CARDANO_SUCCESS;
  uint8_t                         more     = 0U;

  result = cardano_uplc_flat_reader_bit(reader, &more);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  while (more != 0U)
  {
    cardano_uplc_constant_t* element = NULL;

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_value(arena, reader, element_type, depth + 1U, &element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (length == capacity)
    {
      const size_t                    next_capacity = (capacity == 0U) ? 4U : (capacity * 2U);
      const cardano_uplc_constant_t** grown         = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(
        arena,
        next_capacity * sizeof(const cardano_uplc_constant_t*),
        0U);

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      for (size_t i = 0U; i < length; ++i)
      {
        grown[i] = list[i];
      }

      list     = grown;
      capacity = next_capacity;
    }

    list[length] = element;
    length       += 1U;

    result = cardano_uplc_flat_reader_bit(reader, &more);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  *items = list;
  *count = length;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a typed list or array constant value.
 *
 * Both forms are a cons-bit sequence of elements read at the element type; the
 * array form is the reference's flat instance for a strict vector, which encodes
 * the vector as a list (plutus-core/src/Data/Vector/Orphans.hs).
 *
 * \param[in] arena The arena the constant and its element array are allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[in] type The list or array type descriptor.
 * \param[in] depth The current value nesting depth.
 * \param[out] constant On success, the list or array constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated stream or nesting past the depth bound, or a propagated
 *         allocation error.
 */
static cardano_error_t
read_sequence(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_type_t*  type,
  const size_t                depth,
  cardano_uplc_constant_t**   constant)
{
  const cardano_uplc_constant_t** items = NULL;
  size_t                          count = 0U;
  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  cardano_error_t result = read_items(arena, reader, type->fst, depth, &items, &count);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (type->kind == CARDANO_UPLC_TYPE_ARRAY)
  {
    return cardano_uplc_constant_new_array(arena, type->fst, items, count, constant);
  }

  return cardano_uplc_constant_new_list(arena, type->fst, items, count, constant);
}

/**
 * \brief Builds the value element type <tt>pair(bytestring, list(pair(bytestring, integer)))</tt>.
 *
 * \param[in] arena The arena every descriptor node is allocated from.
 * \param[out] type On success, the element type descriptor.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated allocation error.
 */
static cardano_error_t
value_element_type(cardano_uplc_arena_t* arena, const cardano_uplc_type_t** type)
{
  cardano_uplc_type_t* bytestring = NULL;
  cardano_uplc_type_t* integer    = NULL;
  cardano_uplc_type_t* token      = NULL;
  cardano_uplc_type_t* tokens     = NULL;
  cardano_uplc_type_t* policy     = NULL;
  cardano_error_t      result     = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BYTE_STRING, NULL, NULL, &bytestring);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, NULL, NULL, &integer);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, integer, &token);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, token, NULL, &tokens);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, tokens, &policy);
  }

  if (result == CARDANO_SUCCESS)
  {
    *type = policy;
  }

  return result;
}

/**
 * \brief Reads a multi-asset value constant.
 *
 * The flat form is the reference's encoding of the nested map: a cons-bit list of
 * (currency symbol, cons-bit list of (token name, quantity)) entries, keys as flat
 * byte strings and quantities as flat integers (plutus-core/src/PlutusCore/Value.hs,
 * \c Flat instance of \c Value). The entries must already be canonical, as the
 * reference's \c buildValueWith requires: strictly ascending keys of at most 32
 * bytes, no empty token map and non-zero quantities in the signed 128-bit range.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[in] depth The current value nesting depth.
 * \param[out] constant On success, the value constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated stream or a non-canonical value, or a propagated allocation
 *         error.
 */
static cardano_error_t
read_asset_value(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const size_t                depth,
  cardano_uplc_constant_t**   constant)
{
  const cardano_uplc_type_t*      element = NULL;
  const cardano_uplc_constant_t** items   = NULL;
  size_t                          count   = 0U;
  cardano_error_t                 result  = value_element_type(arena, &element);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  result = read_items(arena, reader, element, depth, &items, &count);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (!cardano_uplc_int_value_entries_are_canonical(arena, items, count))
  {
    return CARDANO_ERROR_DECODING;
  }

  return cardano_uplc_constant_new_value(arena, element, items, count, constant);
}

/**
 * \brief Reads a typed pair constant value.
 *
 * A pair value is the first component value followed by the second component
 * value, each read at its respective type.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[in] type The pair type descriptor.
 * \param[in] depth The current value nesting depth.
 * \param[out] constant On success, the pair constant.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated decode or allocation
 *         error.
 */
static cardano_error_t
read_pair(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_type_t*  type,
  const size_t                depth,
  cardano_uplc_constant_t**   constant)
{
  cardano_uplc_constant_t* fst = NULL;
  cardano_uplc_constant_t* snd = NULL;
  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  cardano_error_t result = read_value(arena, reader, type->fst, depth + 1U, &fst);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  result = read_value(arena, reader, type->snd, depth + 1U, &snd);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_uplc_constant_new_pair(arena, fst, snd, constant);
}

/**
 * \brief Reads a constant value for a parsed type descriptor.
 *
 * Dispatches on \p type->kind to the per-type readers. Recursion through list,
 * array, pair and value constants is bounded by \ref FLAT_TYPE_MAX_DEPTH. The
 * BLS12-381 G1, G2 and Miller-loop-result types have no flat value form: the
 * reference's \c Flat instances fail to decode them (\c G1.hs, \c G2.hs and
 * \c Pairing.hs under plutus-core/src/PlutusCore/Crypto/BLS12_381), so a value of
 * any of them is a decode error. The type alone is accepted, so an empty list of
 * such elements decodes as it does in the reference.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in,out] reader The flat reader, advanced past the value.
 * \param[in] type The type descriptor whose value is read.
 * \param[in] depth The current value nesting depth.
 * \param[out] constant On success, the decoded constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated or invalid value, a BLS12-381 value, a non-canonical value
 *         constant, or nesting past the depth bound, or a propagated allocation
 *         error.
 */
static cardano_error_t
read_value(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_type_t*  type,
  const size_t                depth,
  cardano_uplc_constant_t**   constant)
{
  cardano_error_t result = CARDANO_SUCCESS;

  if (depth > FLAT_TYPE_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  switch (type->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      result = read_integer(arena, reader, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      result = read_byte_string(arena, reader, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      result = read_string(arena, reader, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      result = cardano_uplc_constant_new_unit(arena, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      uint8_t bit = 0U;

      result = cardano_uplc_flat_reader_bit(reader, &bit);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_uplc_constant_new_bool(arena, bit != 0U, constant);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = read_sequence(arena, reader, type, depth, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = read_pair(arena, reader, type, depth, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      result = read_data(arena, reader, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_VALUE:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = read_asset_value(arena, reader, depth, constant);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    default:
    {
      result = CARDANO_ERROR_DECODING;
      break;
    }
  }

  return result;
}

/**
 * \brief Reads a constant: its const-type list, then a value of that type.
 *
 * Under the ledger script checks a type list longer than
 * \ref FLAT_SCRIPT_MAX_HEADER_SIZE tags is rejected before the value is read, as
 * the ledger's script decoder does
 * (plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs,
 * \c checkConstant).
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in,out] reader The flat reader, advanced past the constant.
 * \param[in] rules The program-wide decoding rules.
 * \param[out] constant On success, the decoded constant.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed type or value or an oversized type header under the ledger
 *         script checks, or a propagated allocation error.
 */
static cardano_error_t
read_constant(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  cardano_uplc_constant_t**   constant)
{
  const cardano_uplc_type_t* type        = NULL;
  size_t                     header_size = 0U;
  cardano_error_t            result      = read_type(arena, reader, &type, &header_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (rules->ledger_checks && (header_size > FLAT_SCRIPT_MAX_HEADER_SIZE))
  {
    return CARDANO_ERROR_DECODING;
  }

  return read_value(arena, reader, type, 0U, constant);
}

/* Forward declaration: term decoding recurses on every nested term. */
static cardano_error_t
read_term(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  size_t                      depth,
  const cardano_uplc_term_t** term);

/**
 * \brief Reads a cons-bit list of terms into an arena-allocated pointer array.
 *
 * A term list is a cons-bit sequence: a one bit precedes each element and a zero
 * bit ends the sequence, exactly like the constant list encoding. Each element is
 * a full term decoded at \p depth, so an empty list yields a NULL array and a zero
 * count. The element pointers are gathered into an arena array handed back to the
 * caller.
 *
 * \param[in] arena The arena the element array and every element are allocated from.
 * \param[in,out] reader The flat reader, advanced past the list.
 * \param[in] rules The program-wide decoding rules.
 * \param[in] depth The current term nesting depth.
 * \param[out] items On success, the element array, or NULL when \p count is 0.
 * \param[out] count On success, the number of elements.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         truncated stream or nesting past the depth bound, or a propagated
 *         allocation error.
 */
static cardano_error_t
read_term_list(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  const size_t                depth,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_term_t*** items,
  size_t*                      count)
{
  const cardano_uplc_term_t** list     = NULL;
  size_t                      length   = 0U;
  size_t                      capacity = 0U;
  cardano_error_t             result   = CARDANO_SUCCESS;
  uint8_t                     more     = 0U;

  result = cardano_uplc_flat_reader_bit(reader, &more);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  while (more != 0U)
  {
    const cardano_uplc_term_t* element = NULL;

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_term(arena, reader, rules, depth + 1U, &element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (length == capacity)
    {
      const size_t                next_capacity = (capacity == 0U) ? 4U : (capacity * 2U);
      const cardano_uplc_term_t** grown         = (const cardano_uplc_term_t**)cardano_uplc_arena_alloc(
        arena,
        next_capacity * sizeof(const cardano_uplc_term_t*),
        0U);

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      for (size_t i = 0U; i < length; ++i)
      {
        grown[i] = list[i];
      }

      list     = grown;
      capacity = next_capacity;
    }

    list[length] = element;
    length       += 1U;

    result = cardano_uplc_flat_reader_bit(reader, &more);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  *items = list;
  *count = length;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a builtin term, validating the 7-bit builtin tag.
 *
 * Under the ledger script checks a builtin that is not available in the script's
 * language at the protocol version is rejected, as the ledger's script decoder
 * does (plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs,
 * \c checkBuiltin).
 *
 * \param[in] arena The arena the term is allocated from.
 * \param[in,out] reader The flat reader, advanced past the builtin tag.
 * \param[in] rules The program-wide decoding rules.
 * \param[out] term On success, the builtin term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for an
 *         unknown or unavailable builtin tag or a truncated stream, or a
 *         propagated allocation error.
 */
static cardano_error_t
read_builtin(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  const cardano_uplc_term_t** term)
{
  uint8_t              tag    = 0U;
  cardano_uplc_term_t* built  = NULL;
  cardano_error_t      result = cardano_uplc_flat_reader_bits8(reader, FLAT_BUILTIN_TAG_BITS, &tag);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (tag >= (uint8_t)CARDANO_UPLC_BUILTIN_COUNT)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (rules->ledger_checks && !cardano_uplc_builtin_available((cardano_uplc_builtin_t)tag, rules->language, rules->protocol_major))
  {
    return CARDANO_ERROR_DECODING;
  }

  result = cardano_uplc_term_new_builtin(arena, (cardano_uplc_builtin_t)tag, &built);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *term = built;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a constructor term: a tag word then a cons-bit field list.
 *
 * A constructor in a program older than 1.1.0 is rejected before its payload is
 * read, as the reference decoder does
 * (plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs,
 * \c handleTerm 8). Under the ledger script checks a constructor with more than
 * \ref FLAT_SCRIPT_MAX_CONSTR_FIELDS fields is rejected once its fields are read
 * (plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs,
 * \c checkConstr).
 *
 * \param[in] arena The arena the term and its field array are allocated from.
 * \param[in,out] reader The flat reader, advanced past the constructor.
 * \param[in] rules The program-wide decoding rules.
 * \param[in] depth The current term nesting depth.
 * \param[out] term On success, the constructor term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING when
 *         constructors are not allowed, for too many fields under the ledger script
 *         checks, a truncated stream or nesting past the depth bound, or a
 *         propagated allocation error.
 */
static cardano_error_t
read_constr(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  const size_t                depth,
  const cardano_uplc_term_t** term)
{
  size_t                      tag         = 0U;
  const cardano_uplc_term_t** fields      = NULL;
  size_t                      field_count = 0U;
  cardano_uplc_term_t*        built       = NULL;
  cardano_error_t             result      = CARDANO_SUCCESS;

  if (!rules->allow_constr_case)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = cardano_uplc_flat_reader_word(reader, &tag);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  result = read_term_list(arena, reader, rules, depth, &fields, &field_count);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (rules->ledger_checks && (field_count > FLAT_SCRIPT_MAX_CONSTR_FIELDS))
  {
    return CARDANO_ERROR_DECODING;
  }

  result = cardano_uplc_term_new_constr(arena, (uint64_t)tag, fields, field_count, &built);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *term = built;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a case term: a scrutinee term then a cons-bit branch list.
 *
 * A case in a program older than 1.1.0 is rejected before its payload is read,
 * as the reference decoder does
 * (plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs,
 * \c handleTerm 9).
 *
 * \param[in] arena The arena the term and its branch array are allocated from.
 * \param[in,out] reader The flat reader, advanced past the case.
 * \param[in] rules The program-wide decoding rules.
 * \param[in] depth The current term nesting depth.
 * \param[out] term On success, the case term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING when
 *         case terms are not allowed, for a truncated stream or nesting past the
 *         depth bound, or a propagated allocation error.
 */
static cardano_error_t
read_case(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  const size_t                depth,
  const cardano_uplc_term_t** term)
{
  const cardano_uplc_term_t*  scrutinee    = NULL;
  const cardano_uplc_term_t** branches     = NULL;
  size_t                      branch_count = 0U;
  cardano_uplc_term_t*        built        = NULL;
  cardano_error_t             result       = CARDANO_SUCCESS;

  if (!rules->allow_constr_case)
  {
    return CARDANO_ERROR_DECODING;
  }

  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  result = read_term(arena, reader, rules, depth + 1U, &scrutinee);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
  result = read_term_list(arena, reader, rules, depth, &branches, &branch_count);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_term_new_case(arena, scrutinee, branches, branch_count, &built);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *term = built;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a single term, dispatching on the 4-bit term tag.
 *
 * Recursion through the nested forms is bounded by \ref FLAT_TERM_MAX_DEPTH so a
 * deeply nested adversarial input fails with \ref CARDANO_ERROR_DECODING instead of
 * overflowing the C stack. An unknown term tag is rejected with the same error.
 *
 * \param[in] arena The arena every node is allocated from.
 * \param[in,out] reader The flat reader, advanced past the term.
 * \param[in] rules The program-wide decoding rules.
 * \param[in] depth The current term nesting depth.
 * \param[out] term On success, the decoded term.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for an
 *         unknown term or builtin tag, a malformed constant, a truncated stream or
 *         nesting past the depth bound, or a propagated allocation error.
 */
static cardano_error_t
read_term(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const decode_rules_t*       rules,
  const size_t                depth,
  const cardano_uplc_term_t** term)
{
  uint8_t         tag    = 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  if (depth > FLAT_TERM_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = cardano_uplc_flat_reader_bits8(reader, FLAT_TERM_TAG_BITS, &tag);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (tag == FLAT_TERM_TAG_VAR)
  {
    size_t               index = 0U;
    cardano_uplc_term_t* built = NULL;

    result = cardano_uplc_flat_reader_word(reader, &index);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_uplc_term_new_var(arena, (uint64_t)index, &built);

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_DELAY)
  {
    const cardano_uplc_term_t* body  = NULL;
    cardano_uplc_term_t*       built = NULL;

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_term(arena, reader, rules, depth + 1U, &body);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_term_new_delay(arena, body, &built);
    }

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_LAMBDA)
  {
    const cardano_uplc_term_t* body  = NULL;
    cardano_uplc_term_t*       built = NULL;

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_term(arena, reader, rules, depth + 1U, &body);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_term_new_lambda(arena, body, &built);
    }

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_APPLY)
  {
    const cardano_uplc_term_t* function = NULL;
    const cardano_uplc_term_t* argument = NULL;
    cardano_uplc_term_t*       built    = NULL;

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_term(arena, reader, rules, depth + 1U, &function);

    if (result == CARDANO_SUCCESS)
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = read_term(arena, reader, rules, depth + 1U, &argument);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_term_new_apply(arena, function, argument, &built);
    }

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_CONSTANT)
  {
    cardano_uplc_constant_t* constant = NULL;
    cardano_uplc_term_t*     built    = NULL;

    result = read_constant(arena, reader, rules, &constant);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_term_new_constant(arena, constant, &built);
    }

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_FORCE)
  {
    const cardano_uplc_term_t* body  = NULL;
    cardano_uplc_term_t*       built = NULL;

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_term(arena, reader, rules, depth + 1U, &body);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_term_new_force(arena, body, &built);
    }

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_ERROR)
  {
    cardano_uplc_term_t* built = NULL;

    result = cardano_uplc_term_new_error(arena, &built);

    if (result == CARDANO_SUCCESS)
    {
      *term = built;
    }
  }
  else if (tag == FLAT_TERM_TAG_BUILTIN)
  {
    result = read_builtin(arena, reader, rules, term);
  }
  else if (tag == FLAT_TERM_TAG_CONSTR)
  {
    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_constr(arena, reader, rules, depth, term);
  }
  else if (tag == FLAT_TERM_TAG_CASE)
  {
    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = read_case(arena, reader, rules, depth, term);
  }
  else
  {
    result = CARDANO_ERROR_DECODING;
  }

  return result;
}

/**
 * \brief Reads a full program: three version words, a term, then the filler.
 *
 * \c Constr and \c Case are accepted only from version 1.1.0, matching the
 * reference decoder. The filler must end exactly at the end of the stream, since
 * the reference decoder rejects any bytes left over after the program.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in,out] reader The flat reader, advanced past the program.
 * \param[in] rules The ledger script checks to apply; its \c allow_constr_case
 *            is replaced by the program version gate.
 * \param[out] program On success, the decoded program.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for a
 *         malformed, truncated or over-long stream, a version word too large for
 *         32 bits, a term \p rules rejects or a \c Constr or \c Case before
 *         version 1.1.0, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve a node.
 */
static cardano_error_t
decode_program(
  cardano_uplc_arena_t*          arena,
  cardano_uplc_flat_reader_t*    reader,
  const decode_rules_t*          rules,
  const cardano_uplc_program_t** program)
{
  size_t                     major         = 0U;
  size_t                     minor         = 0U;
  size_t                     patch         = 0U;
  const cardano_uplc_term_t* term          = NULL;
  cardano_uplc_program_t*    result        = NULL;
  cardano_error_t            status        = CARDANO_SUCCESS;
  decode_rules_t             program_rules = *rules;

  status = cardano_uplc_flat_reader_word(reader, &major);

  if (status != CARDANO_SUCCESS)
  {
    return status;
  }

  status = cardano_uplc_flat_reader_word(reader, &minor);

  if (status != CARDANO_SUCCESS)
  {
    return status;
  }

  status = cardano_uplc_flat_reader_word(reader, &patch);

  if (status != CARDANO_SUCCESS)
  {
    return status;
  }

  if ((major > UINT32_MAX) || (minor > UINT32_MAX) || (patch > UINT32_MAX))
  {
    return CARDANO_ERROR_DECODING;
  }

  program_rules.allow_constr_case = (major > FLAT_CONSTR_CASE_MIN_MAJOR) || ((major == FLAT_CONSTR_CASE_MIN_MAJOR) && (minor >= FLAT_CONSTR_CASE_MIN_MINOR));

  status = read_term(arena, reader, &program_rules, 0U, &term);

  if (status != CARDANO_SUCCESS)
  {
    return status;
  }

  status = cardano_uplc_flat_reader_filler(reader);

  if (status != CARDANO_SUCCESS)
  {
    return status;
  }

  if (reader->byte_pos != reader->size)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = (cardano_uplc_program_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->version_major = (uint32_t)major;
  result->version_minor = (uint32_t)minor;
  result->version_patch = (uint32_t)patch;
  result->term          = term;

  *program = result;

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_flat_decode_constant(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  cardano_uplc_constant_t**   constant)
{
  if ((arena == NULL) || (reader == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return read_constant(arena, reader, &UNRESTRICTED_RULES, constant);
}

cardano_error_t
cardano_uplc_flat_decode_term(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_term_t** term)
{
  if ((arena == NULL) || (reader == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return read_term(arena, reader, &UNRESTRICTED_RULES, 0U, term);
}

cardano_error_t
cardano_uplc_flat_decode_program(
  cardano_uplc_arena_t*          arena,
  cardano_uplc_flat_reader_t*    reader,
  const cardano_uplc_program_t** program)
{
  if ((arena == NULL) || (reader == NULL) || (program == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return decode_program(arena, reader, &UNRESTRICTED_RULES, program);
}

cardano_error_t
cardano_uplc_flat_decode_script_program(
  cardano_uplc_arena_t*             arena,
  cardano_uplc_flat_reader_t*       reader,
  const cardano_uplc_lang_version_t language,
  const uint64_t                    protocol_major,
  const cardano_uplc_program_t**    program)
{
  decode_rules_t rules;

  if ((arena == NULL) || (reader == NULL) || (program == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  rules.allow_constr_case = false;
  rules.ledger_checks     = true;
  rules.language          = language;
  rules.protocol_major    = protocol_major;

  return decode_program(arena, reader, &rules, program);
}
