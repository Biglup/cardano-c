/**
 * \file flat_encode.c
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#include "flat_encode.h"
#include "../ast/uplc_program.h"
#include "../ast/uplc_term.h"
#include <cardano/cbor/cbor_writer.h>

#include "../ast/uplc_int.h"
#include "../builtins/bls.h"
#include "../data/uplc_data.h"
#include "flat_writer.h"

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Flat const-type list tags, the inverse of the tags in flat_decode.c.
 *
 * The application tag drives the two nested shapes: <tt>[7, 5, T]</tt> for
 * <tt>list(T)</tt> and <tt>[7, 7, 6, A, B]</tt> for <tt>pair(A, B)</tt>.
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

/**
 * \brief Number of bits in a single const-type tag.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_TYPE_TAG_BITS = 4U;

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

/* STATIC FUNCTIONS **********************************************************/

/* Forward declaration: constant encoding recurses on list and pair values. */
static cardano_error_t
write_constant(cardano_uplc_flat_writer_t* writer, const cardano_uplc_constant_t* constant);

/* Forward declaration: term encoding recurses on every nested term. */
static cardano_error_t
write_term(cardano_uplc_flat_writer_t* writer, const cardano_uplc_term_t* term);

/**
 * \brief Writes one const-type tag: a one continuation bit then the 4-bit tag.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] tag The 4-bit type tag.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated write error.
 */
static cardano_error_t
write_type_tag(cardano_uplc_flat_writer_t* writer, const uint8_t tag)
{
  cardano_error_t result = cardano_uplc_flat_writer_bit(writer, 1U);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_uplc_flat_writer_bits8(writer, tag, FLAT_TYPE_TAG_BITS);
}

/**
 * \brief Emits the const-type tag run for a type descriptor (no terminator).
 *
 * Leaf types emit a single tag; a list emits <tt>[apply, list, ...element]</tt>
 * and a pair emits <tt>[apply, apply, pair, ...fst, ...snd]</tt>, matching
 * \c parse_type in flat_decode.c. The terminating zero bit is written by the
 * caller. The \c array, \c value and \c bls12_381_mlresult kinds have no flat type
 * tag and are rejected.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] type The type descriptor to emit.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INVALID_ARGUMENT for
 *         a kind with no flat serialization, or a propagated write error.
 */
static cardano_error_t
write_type_tags(cardano_uplc_flat_writer_t* writer, const cardano_uplc_type_t* type)
{
  cardano_error_t result = CARDANO_SUCCESS;

  switch (type->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_INTEGER);
      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_BYTE_STRING);
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_STRING);
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_UNIT);
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_BOOL);
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_DATA);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_BLS_G1);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_BLS_G2);
      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_APPLY);

      if (result == CARDANO_SUCCESS)
      {
        result = write_type_tag(writer, FLAT_TYPE_TAG_LIST);
      }

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_type_tags(writer, type->fst);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      result = write_type_tag(writer, FLAT_TYPE_TAG_APPLY);

      if (result == CARDANO_SUCCESS)
      {
        result = write_type_tag(writer, FLAT_TYPE_TAG_APPLY);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = write_type_tag(writer, FLAT_TYPE_TAG_PAIR);
      }

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_type_tags(writer, type->fst);
      }

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_type_tags(writer, type->snd);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    case CARDANO_UPLC_TYPE_ARRAY:
    case CARDANO_UPLC_TYPE_VALUE:
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Writes the data carried by a value at a given type descriptor.
 *
 * Dispatches on \p type->kind: an integer is zig-zagged, byte-string/string/data
 * become flat bytestrings, a bool is one bit, a unit writes nothing, a list is a
 * cons-bit run of element values and a pair is its two component values. A G1 or
 * G2 value is compressed to its serialized form and written as a flat bytestring.
 * The \c array, \c value and \c bls12_381_mlresult kinds are rejected.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] type The type descriptor whose value is written.
 * \param[in] constant The constant whose value is written.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INVALID_ARGUMENT for
 *         a kind with no flat serialization, or a propagated write or allocation
 *         error.
 */
static cardano_error_t
write_value(
  cardano_uplc_flat_writer_t*    writer,
  const cardano_uplc_type_t*     type,
  const cardano_uplc_constant_t* constant)
{
  cardano_error_t result = CARDANO_SUCCESS;

  switch (type->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      if (cardano_uplc_constant_int_is_small(constant))
      {
        cardano_bigint_t* temp = NULL;

        result = cardano_bigint_from_int(cardano_uplc_constant_int_small(constant), &temp);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_uplc_flat_writer_integer(writer, temp);
          cardano_bigint_unref(&temp);
        }
      }
      else
      {
        result = cardano_uplc_flat_writer_integer(writer, constant->as.integer.big);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      result = cardano_uplc_flat_writer_bytes(
        writer,
        constant->as.bytes.data,
        constant->as.bytes.size);
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      result = cardano_uplc_flat_writer_bytes(
        writer,
        constant->as.string.data,
        constant->as.string.size);
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      result = cardano_uplc_flat_writer_bit(writer, constant->as.boolean ? 1U : 0U);
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      cardano_cbor_writer_t* cbor  = cardano_cbor_writer_new();
      cardano_buffer_t*      bytes = NULL;

      if (cbor == NULL)
      {
        result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        break;
      }

      result = cardano_uplc_data_to_cbor(constant->as.data, cbor);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_cbor_writer_encode_in_buffer(cbor, &bytes);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_uplc_flat_writer_bytes(writer, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes));
      }

      cardano_buffer_unref(&bytes);
      cardano_cbor_writer_unref(&cbor);
      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    {
      for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < constant->as.list.count); ++i)
      {
        result = cardano_uplc_flat_writer_bit(writer, 1U);

        if (result == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = write_value(writer, type->fst, constant->as.list.items[i]);
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_uplc_flat_writer_bit(writer, 0U);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = write_value(writer, type->fst, constant->as.pair.fst);

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_value(writer, type->snd, constant->as.pair.snd);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    {
      byte_t compressed[CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE];

      result = cardano_uplc_int_bls_g1_compress(constant, compressed);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_uplc_flat_writer_bytes(writer, compressed, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      byte_t compressed[CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE];

      result = cardano_uplc_int_bls_g2_compress(constant, compressed);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_uplc_flat_writer_bytes(writer, compressed, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    case CARDANO_UPLC_TYPE_ARRAY:
    case CARDANO_UPLC_TYPE_VALUE:
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Recovers the type descriptor a constant carries.
 *
 * Leaf constants map their kind directly. A list builds <tt>list(element_type)</tt>
 * and a pair builds <tt>pair(fst-type, snd-type)</tt> from the recovered component
 * types. The descriptor and any nested nodes are allocated from \p arena.
 *
 * \param[in] arena The arena every descriptor node is allocated from.
 * \param[in] constant The constant whose type is recovered.
 * \param[out] type On success, the recovered type descriptor; left untouched on
 *             failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INVALID_ARGUMENT for
 *         a kind with no flat serialization, or a propagated allocation error.
 */
static cardano_error_t
constant_type(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* constant,
  const cardano_uplc_type_t**    type)
{
  cardano_error_t      result = CARDANO_SUCCESS;
  cardano_uplc_type_t* built  = NULL;

  switch (constant->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    case CARDANO_UPLC_TYPE_STRING:
    case CARDANO_UPLC_TYPE_UNIT:
    case CARDANO_UPLC_TYPE_BOOL:
    case CARDANO_UPLC_TYPE_DATA:
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      result = cardano_uplc_type_new(arena, constant->kind, NULL, NULL, &built);

      if (result == CARDANO_SUCCESS)
      {
        *type = built;
      }

      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    {
      result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, constant->as.list.element_type, NULL, &built);

      if (result == CARDANO_SUCCESS)
      {
        *type = built;
      }

      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      const cardano_uplc_type_t* fst = NULL;
      const cardano_uplc_type_t* snd = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = constant_type(arena, constant->as.pair.fst, &fst);

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = constant_type(arena, constant->as.pair.snd, &snd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, fst, snd, &built);
      }

      if (result == CARDANO_SUCCESS)
      {
        *type = built;
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    case CARDANO_UPLC_TYPE_ARRAY:
    case CARDANO_UPLC_TYPE_VALUE:
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Writes a constant: its const-type list then its value.
 *
 * Recovers the constant's type into a scratch arena, emits the const-type tag run
 * followed by the terminating zero bit, then writes the value. The scratch arena is
 * freed on every exit path.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] constant The constant to write.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INVALID_ARGUMENT for
 *         a kind with no flat serialization, or a propagated write or allocation
 *         error.
 */
static cardano_error_t
write_constant(cardano_uplc_flat_writer_t* writer, const cardano_uplc_constant_t* constant)
{
  cardano_uplc_arena_t*      arena  = NULL;
  const cardano_uplc_type_t* type   = NULL;
  cardano_error_t            result = cardano_uplc_arena_new(0U, &arena);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = constant_type(arena, constant, &type);

  if (result == CARDANO_SUCCESS)
  {
    result = write_type_tags(writer, type);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_writer_bit(writer, 0U);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = write_value(writer, type, constant);
  }

  cardano_uplc_arena_free(&arena);

  return result;
}

/**
 * \brief Writes a cons-bit list of terms: a one bit before each, a zero to stop.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] items The element terms, or NULL when \p count is 0.
 * \param[in] count The number of elements.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated write error.
 */
static cardano_error_t
write_term_list(
  cardano_uplc_flat_writer_t*       writer,
  const cardano_uplc_term_t* const* items,
  const size_t                      count)
{
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < count); ++i)
  {
    result = cardano_uplc_flat_writer_bit(writer, 1U);

    if (result == CARDANO_SUCCESS)
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = write_term(writer, items[i]);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_writer_bit(writer, 0U);
  }

  return result;
}

/**
 * \brief Writes a single term: its 4-bit tag then its payload.
 *
 * The exact inverse of \c read_term in flat_decode.c. The term kind enum value
 * is itself the 4-bit tag.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] term The term to write.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INVALID_ARGUMENT for
 *         an unknown term kind, or a propagated write or allocation error.
 */
static cardano_error_t
write_term(cardano_uplc_flat_writer_t* writer, const cardano_uplc_term_t* term)
{
  cardano_error_t result = cardano_uplc_flat_writer_bits8(writer, (uint8_t)term->kind, FLAT_TERM_TAG_BITS);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (term->kind)
  {
    case CARDANO_UPLC_TERM_VAR:
    {
      result = cardano_uplc_flat_writer_word(writer, (size_t)term->as.var_index);
      break;
    }
    case CARDANO_UPLC_TERM_DELAY:
    case CARDANO_UPLC_TERM_LAMBDA:
    case CARDANO_UPLC_TERM_FORCE:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = write_term(writer, term->as.unary);
      break;
    }
    case CARDANO_UPLC_TERM_APPLY:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = write_term(writer, term->as.apply.function);

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_term(writer, term->as.apply.argument);
      }

      break;
    }
    case CARDANO_UPLC_TERM_CONSTANT:
    {
      result = write_constant(writer, term->as.constant);
      break;
    }
    case CARDANO_UPLC_TERM_ERROR:
    {
      break;
    }
    case CARDANO_UPLC_TERM_BUILTIN:
    {
      result = cardano_uplc_flat_writer_bits8(writer, (uint8_t)term->as.builtin, FLAT_BUILTIN_TAG_BITS);
      break;
    }
    case CARDANO_UPLC_TERM_CONSTR:
    {
      result = cardano_uplc_flat_writer_word(writer, (size_t)term->as.constr.tag);

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_term_list(writer, term->as.constr.fields, term->as.constr.field_count);
      }

      break;
    }
    case CARDANO_UPLC_TERM_CASE:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = write_term(writer, term->as.cases.scrutinee);

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = write_term_list(writer, term->as.cases.branches, term->as.cases.branch_count);
      }

      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_int_flat_encode_program(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out_flat)
{
  cardano_uplc_flat_writer_t writer = { NULL, 0U, 0U };
  cardano_error_t            result = CARDANO_SUCCESS;

  if ((program == NULL) || (program->term == NULL) || (out_flat == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_flat_writer_init(&writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_flat_writer_word(&writer, (size_t)program->version_major);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_writer_word(&writer, (size_t)program->version_minor);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_writer_word(&writer, (size_t)program->version_patch);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = write_term(&writer, program->term);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_writer_filler(&writer);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_writer_finish(&writer, out_flat);
  }

  cardano_uplc_flat_writer_dispose(&writer);

  return result;
}
