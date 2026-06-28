/**
 * \file pretty.c
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

#include "pretty.h"

#include "../../allocators.h"
#include "../ast/uplc_int.h"
#include "../builtins/bls.h"
#include "../data/uplc_data.h"

#include <cardano/common/bigint.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Initial capacity, in bytes, of the output buffer.
 */
static const size_t PRETTY_INITIAL_CAPACITY = 256U;

/**
 * \brief Maximum term and data nesting the renderer descends before refusing.
 *
 * Bounds recursion so a deeply nested adversarial term cannot exhaust the C
 * stack while being printed; past it the renderer returns
 * \ref CARDANO_ERROR_ILLEGAL_STATE.
 */
static const uint32_t PRETTY_MAX_DEPTH = 2000U;

/**
 * \brief Size of the scratch buffer used to format a 64-bit value in decimal,
 *        wide enough for any uint64_t plus the NUL terminator.
 */
#define PRETTY_INDEX_BUFFER_SIZE 24

/**
 * \brief Numeric base used when rendering big integers.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int32_t PRETTY_DECIMAL_BASE = 10;

/**
 * \brief Surface-syntax name of every builtin, indexed by
 *        \ref cardano_uplc_builtin_t.
 *
 * The names are the textual UPLC surface form of each builtin, indexed by the
 * builtin tag (see uplc_builtin.h).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const char* const BUILTIN_NAME[CARDANO_UPLC_BUILTIN_COUNT] = {
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

/* STRUCTURES ****************************************************************/

/**
 * \brief Append-only writer over an output buffer with a sticky error.
 *
 * Every append goes through one of the writer helpers, which record the first
 * failure in \c status and become no-ops afterwards, so the renderer can run to
 * completion and report a single error rather than checking each append.
 */
typedef struct
{
    cardano_buffer_t* buffer;
    cardano_error_t   status;
} writer_t;

/* STATIC FUNCTION DECLARATIONS *********************************************/

static cardano_error_t print_term(writer_t* writer, const cardano_uplc_term_t* term, uint32_t depth, uint32_t binders);
static void            print_constant(writer_t* writer, const cardano_uplc_constant_t* constant, uint32_t depth);
static void            print_data(writer_t* writer, const cardano_uplc_data_t* data, uint32_t depth);

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Appends \p length bytes of \p text to the writer if no error is pending.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] text The bytes to append.
 * \param[in] length The number of bytes to append.
 */
static void
write_text(writer_t* writer, const char* text, const size_t length)
{
  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  writer->status = cardano_buffer_write(writer->buffer, (const byte_t*)text, length);
}

/**
 * \brief Appends a NUL-terminated string to the writer.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] text The NUL-terminated string to append.
 */
static void
write_str(writer_t* writer, const char* text)
{
  write_text(writer, text, strlen(text));
}

/**
 * \brief Appends a single character to the writer.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] character The character to append.
 */
static void
write_char(writer_t* writer, const char character)
{
  write_text(writer, &character, 1U);
}

/**
 * \brief Appends an unsigned 64-bit value in decimal to the writer.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] value The value to append.
 */
static void
write_u64(writer_t* writer, const uint64_t value)
{
  char buffer[PRETTY_INDEX_BUFFER_SIZE];
  int  written = snprintf(buffer, sizeof(buffer), "%" PRIu64, value);

  if (written < 0)
  {
    if (writer->status == CARDANO_SUCCESS)
    {
      writer->status = CARDANO_ERROR_ENCODING;
    }

    return;
  }

  write_text(writer, buffer, (size_t)written);
}

/**
 * \brief Appends a signed 64-bit value in decimal to the writer.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] value The value to append.
 */
static void
write_i64(writer_t* writer, const int64_t value)
{
  char buffer[PRETTY_INDEX_BUFFER_SIZE];
  int  written = snprintf(buffer, sizeof(buffer), "%" PRId64, value);

  if (written < 0)
  {
    if (writer->status == CARDANO_SUCCESS)
    {
      writer->status = CARDANO_ERROR_ENCODING;
    }

    return;
  }

  write_text(writer, buffer, (size_t)written);
}

/**
 * \brief Appends a big integer in decimal to the writer.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] value The integer to append; a NULL value is printed as 0.
 */
static void
write_bigint(writer_t* writer, const cardano_bigint_t* value)
{
  size_t size   = 0U;
  char*  string = NULL;

  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  if (value == NULL)
  {
    write_char(writer, '0');
    return;
  }

  size   = cardano_bigint_get_string_size(value, PRETTY_DECIMAL_BASE);
  string = (char*)_cardano_malloc(size);

  if (string == NULL)
  {
    writer->status = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    return;
  }

  writer->status = cardano_bigint_to_string(value, string, size, PRETTY_DECIMAL_BASE);

  if (writer->status == CARDANO_SUCCESS)
  {
    write_str(writer, string);
  }

  _cardano_free(string);
}

/**
 * \brief Appends a raw byte span as a hex string prefixed with '#'.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] data The bytes to render; a NULL or empty span prints just '#'.
 * \param[in] size The number of bytes.
 */
static void
write_hex(writer_t* writer, const byte_t* data, size_t size)
{
  static const char digits[] = "0123456789abcdef";
  size_t            i        = 0U;

  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  write_char(writer, '#');

  if ((data == NULL) || (size == 0U))
  {
    return;
  }

  for (i = 0U; (i < size) && (writer->status == CARDANO_SUCCESS); ++i)
  {
    write_char(writer, digits[(data[i] >> 4) & 0x0FU]);
    write_char(writer, digits[data[i] & 0x0FU]);
  }
}

/**
 * \brief Appends a string constant's contents wrapped in double quotes.
 *
 * Backslash and double-quote bytes are escaped; every other byte is emitted
 * verbatim, which preserves the UTF-8 contents the constant was built with.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] data The text bytes; a NULL span prints an empty string.
 * \param[in] size The number of bytes.
 */
static void
write_string(writer_t* writer, const byte_t* data, size_t size)
{
  size_t i = 0U;

  write_char(writer, '"');

  for (i = 0U; i < size; ++i)
  {
    char c = (char)data[i];

    if ((c == '"') || (c == '\\'))
    {
      write_char(writer, '\\');
    }

    write_char(writer, c);
  }

  write_char(writer, '"');
}

/**
 * \brief Appends the type-list head of a constant type descriptor.
 *
 * Renders the type names the way they appear inside \c (con <type> ...): bare
 * \c integer, \c bytestring, \c string, \c unit, \c bool and \c data, and the
 * parenthesised \c (list T) and \c (pair A B) forms. BLS types are reserved and
 * print their reserved names.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] type The type descriptor; a NULL type prints \c unit.
 */
static void
print_type(writer_t* writer, const cardano_uplc_type_t* type)
{
  if (type == NULL)
  {
    write_str(writer, "unit");
    return;
  }

  switch (type->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      write_str(writer, "integer");
      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      write_str(writer, "bytestring");
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      write_str(writer, "string");
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      write_str(writer, "unit");
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      write_str(writer, "bool");
      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    {
      write_str(writer, "(list ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_type(writer, type->fst);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      write_str(writer, "(array ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_type(writer, type->fst);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      write_str(writer, "(pair ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_type(writer, type->fst);
      write_char(writer, ' ');
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_type(writer, type->snd);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      write_str(writer, "data");
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    {
      write_str(writer, "bls12_381_G1_element");
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      write_str(writer, "bls12_381_G2_element");
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    {
      write_str(writer, "bls12_381_mlresult");
      break;
    }
    case CARDANO_UPLC_TYPE_VALUE:
    {
      write_str(writer, "value");
      break;
    }
    default:
    {
      if (writer->status == CARDANO_SUCCESS)
      {
        writer->status = CARDANO_ERROR_INVALID_ARGUMENT;
      }
      break;
    }
  }
}

/**
 * \brief Appends an arena data value in the textual data syntax.
 *
 * Renders the same forms the conformance corpus uses for the contents of a
 * \c (con data ...) constant: \c Constr <i> [..], \c Map [(k, v), ..],
 * \c List [..], \c I <integer> and \c B #hex. Recursion is bounded by \p depth.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] data The arena data node to render.
 * \param[in] depth The current nesting depth, checked against the ceiling.
 */
static void
print_data(writer_t* writer, const cardano_uplc_data_t* data, const uint32_t depth)
{
  size_t i = 0U;

  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  if (depth >= PRETTY_MAX_DEPTH)
  {
    writer->status = CARDANO_ERROR_ILLEGAL_STATE;
    return;
  }

  if (data == NULL)
  {
    writer->status = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      write_str(writer, "Constr ");
      write_u64(writer, data->as.constr.tag);
      write_str(writer, " [");

      for (i = 0U; i < data->as.constr.count; ++i)
      {
        if (i > 0U)
        {
          write_str(writer, ", ");
        }

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        print_data(writer, data->as.constr.fields[i], depth + 1U);
      }

      write_char(writer, ']');
      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      write_str(writer, "Map [");

      for (i = 0U; i < data->as.map.count; ++i)
      {
        if (i > 0U)
        {
          write_str(writer, ", ");
        }

        write_char(writer, '(');
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        print_data(writer, data->as.map.entries[i].key, depth + 1U);
        write_str(writer, ", ");
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        print_data(writer, data->as.map.entries[i].value, depth + 1U);
        write_char(writer, ')');
      }

      write_char(writer, ']');
      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      write_str(writer, "List [");

      for (i = 0U; i < data->as.list.count; ++i)
      {
        if (i > 0U)
        {
          write_str(writer, ", ");
        }

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        print_data(writer, data->as.list.items[i], depth + 1U);
      }

      write_char(writer, ']');
      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    {
      write_str(writer, "I ");

      if (data->as.integer.is_small)
      {
        write_i64(writer, data->as.integer.small);
      }
      else
      {
        write_bigint(writer, data->as.integer.big);
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_BYTES:
    {
      write_str(writer, "B ");
      write_hex(writer, data->as.bytes.data, data->as.bytes.size);

      break;
    }
    default:
    {
      writer->status = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }
}

/**
 * \brief Appends a raw byte span as a \c 0x-prefixed lowercase hex string.
 *
 * The surface form a BLS12-381 element constant uses for its compressed point.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] data The bytes to render.
 * \param[in] size The number of bytes.
 */
static void
write_0x_hex(writer_t* writer, const byte_t* data, const size_t size)
{
  static const char digits[] = "0123456789abcdef";
  size_t            i        = 0U;

  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  write_str(writer, "0x");

  for (i = 0U; i < size; ++i)
  {
    char pair[2];

    pair[0] = digits[(data[i] >> 4U) & 0x0FU];
    pair[1] = digits[data[i] & 0x0FU];

    write_text(writer, pair, 2U);
  }
}

/**
 * \brief Appends a BLS12-381 element constant as its \c 0x compressed-point hex.
 *
 * G1 and G2 elements compress to their fixed serialization; an ML-result has no
 * surface form, so it is rejected with \ref CARDANO_ERROR_NOT_IMPLEMENTED.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] constant The BLS constant whose compressed point to render.
 */
static void
write_bls(writer_t* writer, const cardano_uplc_constant_t* constant)
{
  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  if (constant->kind == CARDANO_UPLC_TYPE_BLS_G1)
  {
    byte_t out[CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE];

    writer->status = cardano_uplc_int_bls_g1_compress(constant, out);

    if (writer->status == CARDANO_SUCCESS)
    {
      write_0x_hex(writer, out, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE);
    }
  }
  else if (constant->kind == CARDANO_UPLC_TYPE_BLS_G2)
  {
    byte_t out[CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE];

    writer->status = cardano_uplc_int_bls_g2_compress(constant, out);

    if (writer->status == CARDANO_SUCCESS)
    {
      write_0x_hex(writer, out, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE);
    }
  }
  else
  {
    writer->status = CARDANO_ERROR_NOT_IMPLEMENTED;
  }
}

/**
 * \brief Appends a constant value's contents without the \c con head, as it
 *        appears as an item of a list or a component of a pair.
 *
 * This is the bare form: \c 12345, \c #hex, a quoted string, \c (), \c True,
 * a bracketed list, a parenthesised pair, the data forms, or a \c 0x BLS point.
 * The leading type head belongs to the surrounding \c (con <type> ...) or
 * \c (list T) and is not repeated here.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] constant The constant whose value to render.
 * \param[in] depth The current nesting depth, checked against the ceiling.
 */
static void
print_value(writer_t* writer, const cardano_uplc_constant_t* constant, const uint32_t depth)
{
  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  if (depth >= PRETTY_MAX_DEPTH)
  {
    writer->status = CARDANO_ERROR_ILLEGAL_STATE;
    return;
  }

  if (constant == NULL)
  {
    writer->status = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  switch (constant->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      if (cardano_uplc_constant_int_is_small(constant))
      {
        write_i64(writer, cardano_uplc_constant_int_small(constant));
      }
      else
      {
        write_bigint(writer, constant->as.integer.big);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      write_hex(writer, constant->as.bytes.data, constant->as.bytes.size);
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      write_string(writer, constant->as.string.data, constant->as.string.size);
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      write_str(writer, "()");
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      write_str(writer, constant->as.boolean ? "True" : "False");
      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    case CARDANO_UPLC_TYPE_VALUE:
    {
      size_t i = 0U;

      write_char(writer, '[');

      for (i = 0U; i < constant->as.list.count; ++i)
      {
        if (i > 0U)
        {
          write_str(writer, ", ");
        }

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        print_value(writer, constant->as.list.items[i], depth + 1U);
      }

      write_char(writer, ']');
      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      write_char(writer, '(');
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_value(writer, constant->as.pair.fst, depth + 1U);
      write_str(writer, ", ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_value(writer, constant->as.pair.snd, depth + 1U);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      print_data(writer, constant->as.data, depth + 1U);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    {
      write_bls(writer, constant);
      break;
    }
    default:
    {
      writer->status = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }
}

/**
 * \brief Appends the type head of a constant for the \c (con <type> ...) form.
 *
 * The leaf kinds map to their bare names. A list takes its element type from the
 * stored \c element_type descriptor; a pair has no stored component-type
 * descriptors, so its component types are reconstructed from the kinds of its
 * actual component constants, recursing for nested lists and pairs.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] constant The constant whose type head to render.
 */
static void
print_constant_type(writer_t* writer, const cardano_uplc_constant_t* constant)
{
  if (constant == NULL)
  {
    if (writer->status == CARDANO_SUCCESS)
    {
      writer->status = CARDANO_ERROR_POINTER_IS_NULL;
    }
    return;
  }

  switch (constant->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      write_str(writer, "integer");
      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      write_str(writer, "bytestring");
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      write_str(writer, "string");
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    {
      write_str(writer, "unit");
      break;
    }
    case CARDANO_UPLC_TYPE_BOOL:
    {
      write_str(writer, "bool");
      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    {
      write_str(writer, "(list ");
      print_type(writer, constant->as.list.element_type);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      write_str(writer, "(array ");
      print_type(writer, constant->as.list.element_type);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      write_str(writer, "(pair ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_constant_type(writer, constant->as.pair.fst);
      write_char(writer, ' ');
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      print_constant_type(writer, constant->as.pair.snd);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      write_str(writer, "data");
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    {
      write_str(writer, "bls12_381_G1_element");
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      write_str(writer, "bls12_381_G2_element");
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    {
      write_str(writer, "bls12_381_mlresult");
      break;
    }
    case CARDANO_UPLC_TYPE_VALUE:
    {
      write_str(writer, "value");
      break;
    }
    default:
    {
      if (writer->status == CARDANO_SUCCESS)
      {
        writer->status = CARDANO_ERROR_INVALID_ARGUMENT;
      }
      break;
    }
  }
}

/**
 * \brief Appends a constant as the contents of a \c (con ...) term.
 *
 * Writes the type head followed by a space and the bare value, for example
 * \c integer 23 or \c (pair integer bool) (12345, True).
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] constant The constant to render.
 * \param[in] depth The current nesting depth, checked against the ceiling.
 */
static void
print_constant(writer_t* writer, const cardano_uplc_constant_t* constant, const uint32_t depth)
{
  if (writer->status != CARDANO_SUCCESS)
  {
    return;
  }

  if (constant == NULL)
  {
    writer->status = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  print_constant_type(writer, constant);
  write_char(writer, ' ');

  if (constant->kind == CARDANO_UPLC_TYPE_DATA)
  {
    write_char(writer, '(');
    print_value(writer, constant, depth);
    write_char(writer, ')');
  }
  else
  {
    print_value(writer, constant, depth);
  }
}

/**
 * \brief Appends the binder name introduced at de Bruijn level \p level.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] prefix The base name of the binder.
 * \param[in] level The zero-based level counting binders from the outermost.
 */
static void
write_binder(writer_t* writer, const char* prefix, const uint64_t level)
{
  write_str(writer, prefix);
  write_char(writer, '-');
  write_u64(writer, level);
}

/**
 * \brief Appends a variable term resolved against the binders in scope.
 *
 * A variable with de Bruijn index \p index (1 names the innermost binder) under
 * \p binders binders in scope names the binder at level \c binders-index, which
 * is printed as \c v-<level>. An index that escapes the binders in scope (a free
 * variable) is printed as \c free-<index>, so rendering never fails on a
 * malformed term.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] index The de Bruijn index.
 * \param[in] binders The number of binders currently in scope.
 */
static void
print_var(writer_t* writer, const uint64_t index, const uint32_t binders)
{
  if ((index >= 1U) && (index <= (uint64_t)binders))
  {
    write_binder(writer, "v", (uint64_t)binders - index);
  }
  else
  {
    write_binder(writer, "free", index);
  }
}

/**
 * \brief Appends a term in the textual surface syntax.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] term The term to render.
 * \param[in] depth The current nesting depth, checked against the ceiling.
 * \param[in] binders The number of binders in scope, used to name variables and
 *            lambda binders.
 *
 * \return The writer status after rendering: \ref CARDANO_SUCCESS, or the first
 *         error recorded.
 */
static cardano_error_t
print_term(writer_t* writer, const cardano_uplc_term_t* term, const uint32_t depth, const uint32_t binders)
{
  if (writer->status != CARDANO_SUCCESS)
  {
    return writer->status;
  }

  if (depth >= PRETTY_MAX_DEPTH)
  {
    writer->status = CARDANO_ERROR_ILLEGAL_STATE;
    return writer->status;
  }

  if (term == NULL)
  {
    writer->status = CARDANO_ERROR_POINTER_IS_NULL;
    return writer->status;
  }

  switch (term->kind)
  {
    case CARDANO_UPLC_TERM_VAR:
    {
      print_var(writer, term->as.var_index, binders);
      break;
    }
    case CARDANO_UPLC_TERM_DELAY:
    {
      write_str(writer, "(delay ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      (void)print_term(writer, term->as.unary, depth + 1U, binders);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TERM_LAMBDA:
    {
      write_str(writer, "(lam ");
      write_binder(writer, "v", binders);
      write_char(writer, ' ');
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      (void)print_term(writer, term->as.unary, depth + 1U, binders + 1U);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TERM_APPLY:
    {
      write_str(writer, "[ ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      (void)print_term(writer, term->as.apply.function, depth + 1U, binders);
      write_char(writer, ' ');
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      (void)print_term(writer, term->as.apply.argument, depth + 1U, binders);
      write_str(writer, " ]");
      break;
    }
    case CARDANO_UPLC_TERM_CONSTANT:
    {
      write_str(writer, "(con ");
      print_constant(writer, term->as.constant, depth + 1U);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TERM_FORCE:
    {
      write_str(writer, "(force ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      (void)print_term(writer, term->as.unary, depth + 1U, binders);
      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TERM_ERROR:
    {
      write_str(writer, "(error)");
      break;
    }
    case CARDANO_UPLC_TERM_BUILTIN:
    {
      write_str(writer, "(builtin ");

      if (((int)term->as.builtin >= 0) && ((size_t)term->as.builtin < (size_t)CARDANO_UPLC_BUILTIN_COUNT))
      {
        write_str(writer, BUILTIN_NAME[(size_t)term->as.builtin]);
      }
      else if (writer->status == CARDANO_SUCCESS)
      {
        writer->status = CARDANO_ERROR_INVALID_ARGUMENT;
      }
      else
      {
        /* status already records a prior error; leave it untouched */
      }

      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TERM_CONSTR:
    {
      size_t i = 0U;

      write_str(writer, "(constr ");
      write_u64(writer, term->as.constr.tag);

      for (i = 0U; i < term->as.constr.field_count; ++i)
      {
        write_char(writer, ' ');
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        (void)print_term(writer, term->as.constr.fields[i], depth + 1U, binders);
      }

      write_char(writer, ')');
      break;
    }
    case CARDANO_UPLC_TERM_CASE:
    {
      size_t i = 0U;

      write_str(writer, "(case ");
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      (void)print_term(writer, term->as.cases.scrutinee, depth + 1U, binders);

      for (i = 0U; i < term->as.cases.branch_count; ++i)
      {
        write_char(writer, ' ');
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        (void)print_term(writer, term->as.cases.branches[i], depth + 1U, binders);
      }

      write_char(writer, ')');
      break;
    }
    default:
    {
      if (writer->status == CARDANO_SUCCESS)
      {
        writer->status = CARDANO_ERROR_INVALID_ARGUMENT;
      }
      break;
    }
  }

  return writer->status;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_pretty_print_term(const cardano_uplc_term_t* term, cardano_buffer_t** out)
{
  writer_t writer = { NULL, CARDANO_SUCCESS };
  byte_t   nul    = 0U;

  if ((term == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  writer.buffer = cardano_buffer_new(PRETTY_INITIAL_CAPACITY);

  if (writer.buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (void)print_term(&writer, term, 0U, 0U);

  if (writer.status == CARDANO_SUCCESS)
  {
    writer.status = cardano_buffer_write(writer.buffer, &nul, 1U);
  }

  if (writer.status != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&writer.buffer);
    return writer.status;
  }

  *out = writer.buffer;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_pretty_print_program(const cardano_uplc_program_t* program, cardano_buffer_t** out)
{
  writer_t writer = { NULL, CARDANO_SUCCESS };
  byte_t   nul    = 0U;

  if ((program == NULL) || (out == NULL) || (program->term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  writer.buffer = cardano_buffer_new(PRETTY_INITIAL_CAPACITY);

  if (writer.buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  write_str(&writer, "(program ");
  write_u64(&writer, (uint64_t)program->version_major);
  write_char(&writer, '.');
  write_u64(&writer, (uint64_t)program->version_minor);
  write_char(&writer, '.');
  write_u64(&writer, (uint64_t)program->version_patch);
  write_char(&writer, ' ');
  (void)print_term(&writer, program->term, 0U, 0U);
  write_char(&writer, ')');

  if (writer.status == CARDANO_SUCCESS)
  {
    writer.status = cardano_buffer_write(writer.buffer, &nul, 1U);
  }

  if (writer.status != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&writer.buffer);
    return writer.status;
  }

  *out = writer.buffer;

  return CARDANO_SUCCESS;
}
