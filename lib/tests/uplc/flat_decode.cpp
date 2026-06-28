/**
 * \file flat_decode.cpp
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/ast/uplc_int.h"
#include "../../src/uplc/data/uplc_data.h"
#include "../../src/uplc/flat/flat_decode.h"
#include "../../src/uplc/flat/flat_reader.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cstdint>
#include <cstring>
#include <gmock/gmock.h>
#include <string>
#include <vector>

/* STATIC HELPERS ************************************************************/

namespace
{

/**
 * MSB-first bit writer that mirrors the flat reader's bit ordering, used to build
 * test streams by hand.
 */
class BitWriter
{
  public:

    void
    bit(uint8_t value)
    {
      if (bit_pos_ == 0U)
      {
        bytes_.push_back(0U);
      }

      if (value != 0U)
      {
        bytes_.back() |= static_cast<uint8_t>(0x80U >> bit_pos_);
      }

      bit_pos_ = static_cast<uint8_t>((bit_pos_ + 1U) % 8U);
    }

    void
    bits(uint8_t value, uint8_t count)
    {
      for (uint8_t i = 0U; i < count; ++i)
      {
        const uint8_t shift = static_cast<uint8_t>(count - 1U - i);
        bit(static_cast<uint8_t>((value >> shift) & 1U));
      }
    }

    // Appends one const-type tag: a 1 continuation bit then the 4-bit tag.
    void
    type_tag(uint8_t tag)
    {
      bit(1U);
      bits(tag, 4U);
    }

    // Terminates the const-type list.
    void
    end_type_list()
    {
      bit(0U);
    }

    // Aligns to the next byte with a filler run: 0 bits then a terminating 1 bit
    // that lands on the final bit of the byte.
    void
    filler()
    {
      while (bit_pos_ != 7U)
      {
        bit(0U);
      }

      bit(1U);
    }

    // Encodes a flat bytestring: filler, then 255-capped length-prefixed blocks
    // terminated by a zero-length block. Assumes the writer is mid-byte.
    void
    byte_string(const std::vector<uint8_t>& data)
    {
      filler();

      size_t offset = 0U;

      while (offset < data.size())
      {
        const size_t remaining = data.size() - offset;
        const size_t chunk     = (remaining > 255U) ? 255U : remaining;

        bytes_.push_back(static_cast<uint8_t>(chunk));

        for (size_t i = 0U; i < chunk; ++i)
        {
          bytes_.push_back(data[offset + i]);
        }

        offset += chunk;
      }

      bytes_.push_back(0U);
    }

    // Encodes a non-negative magnitude as a flat word (7-bit little-endian groups).
    void
    word(uint64_t value)
    {
      while (true)
      {
        uint8_t group = static_cast<uint8_t>(value & 0x7FU);
        value         >>= 7U;

        if (value != 0U)
        {
          group |= 0x80U;
        }

        bits(group, 8U);

        if (value == 0U)
        {
          break;
        }
      }
    }

    // Encodes a signed integer constant value: zigzag then word.
    void
    integer(int64_t value)
    {
      uint64_t zigzag = (value >= 0)
        ? (static_cast<uint64_t>(value) << 1U)
        : (((~static_cast<uint64_t>(value)) << 1U) | 1U);

      word(zigzag);
    }

    const std::vector<uint8_t>&
    bytes() const
    {
      return bytes_;
    }

  private:

    std::vector<uint8_t> bytes_;
    uint8_t              bit_pos_ = 0U;
};

cardano_uplc_arena_t*
make_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(0U, &arena), CARDANO_SUCCESS);
  return arena;
}

cardano_error_t
decode(const std::vector<uint8_t>& bytes, cardano_uplc_arena_t* arena, cardano_uplc_constant_t** constant)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, bytes.data(), bytes.size()), CARDANO_SUCCESS);
  return cardano_uplc_flat_decode_constant(arena, &reader, constant);
}

cardano_error_t
decode_term(const std::vector<uint8_t>& bytes, cardano_uplc_arena_t* arena, const cardano_uplc_term_t** term)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, bytes.data(), bytes.size()), CARDANO_SUCCESS);
  return cardano_uplc_flat_decode_term(arena, &reader, term);
}

cardano_error_t
decode_program(const std::vector<uint8_t>& bytes, cardano_uplc_arena_t* arena, const cardano_uplc_program_t** program)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, bytes.data(), bytes.size()), CARDANO_SUCCESS);
  return cardano_uplc_flat_decode_program(arena, &reader, program);
}

int64_t
big_to_i64(const cardano_bigint_t* value)
{
  return cardano_bigint_to_int(value);
}

int64_t
int_const_i64(const cardano_uplc_constant_t* constant)
{
  if (cardano_uplc_constant_int_is_small(constant))
  {
    return cardano_uplc_constant_int_small(constant);
  }

  return cardano_bigint_to_int(constant->as.integer.big);
}

// Wraps a byte payload in a single definite-length CBOR bytestring (major type 2).
// Only supports payloads < 256 bytes: a length below 24 is folded into the initial
// byte, otherwise a single length byte follows the 0x58 header (no multi-byte length).
std::vector<uint8_t>
cbor_wrap(const std::vector<uint8_t>& payload)
{
  std::vector<uint8_t> out;

  if (payload.size() < 24U)
  {
    out.push_back(static_cast<uint8_t>(0x40U | payload.size()));
  }
  else
  {
    out.push_back(0x58U);
    out.push_back(static_cast<uint8_t>(payload.size()));
  }

  out.insert(out.end(), payload.begin(), payload.end());

  return out;
}

// Builds the flat bytes for (program 1.0.0 (lam (var 1))), the aiken from_cbor
// reference program, whose CBOR-wrapped hex is 46010000200101.
std::vector<uint8_t>
sample_flat_program()
{
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(2U, 4U); // lambda
  w.bits(0U, 4U); // var
  w.word(1U);
  w.filler();

  return w.bytes();
}

cardano_error_t
decode_script(const std::vector<uint8_t>& bytes, cardano_uplc_arena_t* arena, const cardano_uplc_program_t** program)
{
  return cardano_uplc_program_from_script_bytes(arena, bytes.data(), bytes.size(), program);
}

void
expect_sample_program(const cardano_uplc_program_t* program)
{
  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->version_major, 1U);
  EXPECT_EQ(program->version_minor, 0U);
  EXPECT_EQ(program->version_patch, 0U);
  ASSERT_NE(program->term, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_LAMBDA);
  ASSERT_NE(program->term->as.unary, nullptr);
  EXPECT_EQ(program->term->as.unary->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(program->term->as.unary->as.var_index, 1U);
}

} // namespace

/* SCALAR TESTS *************************************************************/

TEST(cardano_uplc_flat_decode_constant, decodesInteger)
{
  // Arrange
  BitWriter w;
  w.type_tag(0U);
  w.end_type_list();
  w.integer(11);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(constant), 11);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesNegativeInteger)
{
  // Arrange
  BitWriter w;
  w.type_tag(0U);
  w.end_type_list();
  w.integer(-1234567);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(constant), -1234567);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesByteString)
{
  // Arrange
  const std::vector<uint8_t> payload = { 0xDEU, 0xADU, 0xBEU, 0xEFU };

  BitWriter w;
  w.type_tag(1U);
  w.end_type_list();
  w.byte_string(payload);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BYTE_STRING);
  ASSERT_EQ(constant->as.bytes.size, payload.size());
  EXPECT_EQ(std::memcmp(constant->as.bytes.data, payload.data(), payload.size()), 0);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesEmptyByteString)
{
  // Arrange
  BitWriter w;
  w.type_tag(1U);
  w.end_type_list();
  w.byte_string({});

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BYTE_STRING);
  EXPECT_EQ(constant->as.bytes.size, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesString)
{
  // Arrange
  const std::string          text = "h\xC3\xA9llo"; // "h", U+00E9, "llo"
  const std::vector<uint8_t> bytes(text.begin(), text.end());

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_STRING);
  ASSERT_EQ(constant->as.string.size, bytes.size());
  EXPECT_EQ(std::memcmp(constant->as.string.data, bytes.data(), bytes.size()), 0);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesUnit)
{
  // Arrange
  BitWriter w;
  w.type_tag(3U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_UNIT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesBoolTrue)
{
  // Arrange
  BitWriter w;
  w.type_tag(4U);
  w.end_type_list();
  w.bit(1U);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BOOL);
  EXPECT_TRUE(constant->as.boolean);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesBoolFalse)
{
  // Arrange
  BitWriter w;
  w.type_tag(4U);
  w.end_type_list();
  w.bit(0U);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BOOL);
  EXPECT_FALSE(constant->as.boolean);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesData)
{
  // Arrange: a plutus_data integer 42 encodes in CBOR as 0x18 0x2A.
  const std::vector<uint8_t> cbor = { 0x18U, 0x2AU };

  BitWriter w;
  w.type_tag(8U);
  w.end_type_list();
  w.byte_string(cbor);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_DATA);

  EXPECT_EQ(constant->as.data->kind, CARDANO_UPLC_DATA_KIND_INTEGER);
  EXPECT_TRUE(constant->as.data->as.integer.is_small);
  EXPECT_EQ(constant->as.data->as.integer.small, 42);

  cardano_uplc_arena_free(&arena);
}

/* LIST AND PAIR TESTS ******************************************************/

TEST(cardano_uplc_flat_decode_constant, decodesListOfInteger)
{
  // Arrange: list(integer) [7, 5, 0] with elements 1, 2, 3.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(5U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.end_type_list();

  w.bit(1U);
  w.integer(1);
  w.bit(1U);
  w.integer(2);
  w.bit(1U);
  w.integer(3);
  w.bit(0U);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(constant->as.list.element_type->kind, CARDANO_UPLC_TYPE_INTEGER);
  ASSERT_EQ(constant->as.list.count, 3U);
  EXPECT_EQ(int_const_i64(constant->as.list.items[0]), 1);
  EXPECT_EQ(int_const_i64(constant->as.list.items[1]), 2);
  EXPECT_EQ(int_const_i64(constant->as.list.items[2]), 3);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesEmptyListOfBool)
{
  // Arrange: list(bool) [7, 5, 4] with no elements.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(5U, 4U);
  w.bit(1U);
  w.bits(4U, 4U);
  w.end_type_list();

  w.bit(0U);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(constant->as.list.element_type->kind, CARDANO_UPLC_TYPE_BOOL);
  EXPECT_EQ(constant->as.list.count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesNestedListOfListOfInteger)
{
  // Arrange: list(list(integer)) [7, 5, 7, 5, 0] with [[1], [], [2, 3]].
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(5U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(5U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.end_type_list();

  // outer element 0: [1]
  w.bit(1U);
  w.bit(1U);
  w.integer(1);
  w.bit(0U);
  // outer element 1: []
  w.bit(1U);
  w.bit(0U);
  // outer element 2: [2, 3]
  w.bit(1U);
  w.bit(1U);
  w.integer(2);
  w.bit(1U);
  w.integer(3);
  w.bit(0U);
  // end outer
  w.bit(0U);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(constant->as.list.element_type->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(constant->as.list.element_type->fst->kind, CARDANO_UPLC_TYPE_INTEGER);
  ASSERT_EQ(constant->as.list.count, 3U);

  ASSERT_EQ(constant->as.list.items[0]->as.list.count, 1U);
  EXPECT_EQ(int_const_i64(constant->as.list.items[0]->as.list.items[0]), 1);
  EXPECT_EQ(constant->as.list.items[1]->as.list.count, 0U);
  ASSERT_EQ(constant->as.list.items[2]->as.list.count, 2U);
  EXPECT_EQ(int_const_i64(constant->as.list.items[2]->as.list.items[0]), 2);
  EXPECT_EQ(int_const_i64(constant->as.list.items[2]->as.list.items[1]), 3);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesPairOfIntegerAndByteString)
{
  // Arrange: pair(integer, bytestring) [7, 7, 6, 0, 1] with (7, 0xAB).
  const std::vector<uint8_t> payload = { 0xABU };

  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(6U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.bit(1U);
  w.bits(1U, 4U);
  w.end_type_list();

  w.integer(7);
  w.byte_string(payload);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_PAIR);
  EXPECT_EQ(constant->as.pair.fst->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(constant->as.pair.fst), 7);
  EXPECT_EQ(constant->as.pair.snd->kind, CARDANO_UPLC_TYPE_BYTE_STRING);
  ASSERT_EQ(constant->as.pair.snd->as.bytes.size, payload.size());
  EXPECT_EQ(constant->as.pair.snd->as.bytes.data[0], 0xABU);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesNestedPair)
{
  // Arrange: pair(integer, pair(bool, integer))
  // [7, 7, 6, 0, 7, 7, 6, 4, 0] with (5, (true, 9)).
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(6U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(6U, 4U);
  w.bit(1U);
  w.bits(4U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.end_type_list();

  w.integer(5);
  w.bit(1U);
  w.integer(9);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_PAIR);
  EXPECT_EQ(int_const_i64(constant->as.pair.fst), 5);
  EXPECT_EQ(constant->as.pair.snd->kind, CARDANO_UPLC_TYPE_PAIR);
  EXPECT_TRUE(constant->as.pair.snd->as.pair.fst->as.boolean);
  EXPECT_EQ(int_const_i64(constant->as.pair.snd->as.pair.snd), 9);

  cardano_uplc_arena_free(&arena);
}

/* MALFORMED INPUT TESTS ****************************************************/

TEST(cardano_uplc_flat_decode_constant, rejectsNullArguments)
{
  // Arrange
  cardano_uplc_arena_t*      arena    = make_arena();
  cardano_uplc_constant_t*   constant = nullptr;
  cardano_uplc_flat_reader_t reader   = { nullptr, 0U, 0U, 0U };
  const byte_t               data[]   = { 0x00U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, data, sizeof(data)), CARDANO_SUCCESS);

  // Act + Assert
  EXPECT_EQ(cardano_uplc_flat_decode_constant(nullptr, &reader, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_decode_constant(arena, nullptr, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_decode_constant(arena, &reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsUnknownTypeTag)
{
  // Arrange: a leaf tag 12 that names no type.
  BitWriter w;
  w.type_tag(12U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsEmptyTypeList)
{
  // Arrange: a type list that terminates immediately.
  BitWriter w;
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTrailingTypeTag)
{
  // Arrange: integer leaf followed by an unconsumed trailing tag.
  BitWriter w;
  w.type_tag(0U);
  w.type_tag(0U);
  w.end_type_list();
  w.integer(1);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedApplyType)
{
  // Arrange: apply (7) then list (5) then the stream ends with no element type.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(5U, 4U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsApplyWithUnknownConstructor)
{
  // Arrange: apply (7) then a non-application constructor (0) is invalid.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsApplyApplyWithoutPair)
{
  // Arrange: [7, 7, 0] - the double apply must be followed by the pair tag 6.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsApplyApplyTruncated)
{
  // Arrange: [7, 7] with the stream ending before the inner constructor.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(7U, 4U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedTypeListBit)
{
  // Arrange: an entirely empty stream cannot supply the first continuation bit.
  const std::vector<uint8_t> empty;

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(empty, arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedTagBits)
{
  // Arrange: a single set bit promises a tag the stream cannot supply.
  const std::vector<uint8_t> data = { 0x80U };

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(data, arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedIntegerValue)
{
  // Arrange: integer type but the magnitude word never terminates.
  BitWriter w;
  w.type_tag(0U);
  w.end_type_list();
  w.bits(0x80U, 8U); // continuation set, no following group

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedBoolValue)
{
  // Arrange: list(bool) whose first cons bit promises an element, but the stream
  // ends before that element's value bit can be read.
  BitWriter w;
  w.bit(1U);
  w.bits(7U, 4U);
  w.bit(1U);
  w.bits(5U, 4U);
  w.bit(1U);
  w.bits(4U, 4U);
  w.end_type_list();
  w.bit(1U); // cons bit: an element follows, but no value bit is supplied

  std::vector<uint8_t> bytes = w.bytes();
  bytes.resize(2U); // drop trailing zero-padding so the value bit is absent

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(bytes, arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsInvalidUtf8String)
{
  // Arrange: 0xFF is never a valid UTF-8 byte.
  const std::vector<uint8_t> bytes = { 0xFFU };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsInvalidUtf8Continuation)
{
  // Arrange: a 2-byte lead 0xC3 with a non-continuation second byte.
  const std::vector<uint8_t> bytes = { 0xC3U, 0x20U };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedUtf8Lead)
{
  // Arrange: a 2-byte lead 0xC3 with no continuation byte at all.
  const std::vector<uint8_t> bytes = { 0xC3U };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesThreeAndFourByteUtf8)
{
  // Arrange: U+20AC (E2 82 AC) and U+1F600 (F0 9F 98 80).
  const std::vector<uint8_t> bytes = { 0xE2U, 0x82U, 0xACU, 0xF0U, 0x9FU, 0x98U, 0x80U };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_STRING);
  EXPECT_EQ(constant->as.string.size, bytes.size());

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsOverlongUtf8)
{
  // Arrange: 0xE0 0x80 0x80 is an overlong encoding of U+0000.
  const std::vector<uint8_t> bytes = { 0xE0U, 0x80U, 0x80U };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsSurrogateUtf8)
{
  // Arrange: 0xED 0xA0 0x80 encodes a UTF-16 surrogate (U+D800).
  const std::vector<uint8_t> bytes = { 0xEDU, 0xA0U, 0x80U };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsOutOfRangeUtf8)
{
  // Arrange: 0xF4 0x90 0x80 0x80 encodes U+110000, above the Unicode ceiling.
  const std::vector<uint8_t> bytes = { 0xF4U, 0x90U, 0x80U, 0x80U };

  BitWriter w;
  w.type_tag(2U);
  w.end_type_list();
  w.byte_string(bytes);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsMalformedData)
{
  // Arrange: a single 0x1F CBOR byte starts a reserved additional-info value.
  const std::vector<uint8_t> cbor = { 0x1FU };

  BitWriter w;
  w.type_tag(8U);
  w.end_type_list();
  w.byte_string(cbor);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTruncatedDataBytes)
{
  // Arrange: data type but the bytestring blocks are never terminated.
  BitWriter w;
  w.type_tag(8U);
  w.end_type_list();
  w.filler();
  std::vector<uint8_t> bytes = w.bytes();
  bytes.push_back(0x04U); // a block claiming 4 bytes that are absent

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(bytes, arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsTypeNestingPastDepthBound)
{
  // Arrange: 129 nested list applications exceed the depth and tag-buffer bound.
  BitWriter w;

  for (int i = 0; i < 129; ++i)
  {
    w.bit(1U);
    w.bits(7U, 4U);
    w.bit(1U);
    w.bits(5U, 4U);
  }

  w.bit(1U);
  w.bits(0U, 4U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

/* BLS TESTS ****************************************************************/

TEST(cardano_uplc_flat_decode_constant, decodesBlsG1CompressedPoint)
{
  // Arrange: a BLS G1 type tag (9) followed by the compressed G1 generator (48
  // bytes) decodes to a G1 constant.
  const std::vector<uint8_t> g1_generator = {
    0x97, 0xf1, 0xd3, 0xa7, 0x31, 0x97, 0xd7, 0x94, 0x26, 0x95, 0x63, 0x8c, 0x4f, 0xa9, 0xac, 0x0f, 0xc3, 0x68, 0x8c, 0x4f, 0x97, 0x74, 0xb9, 0x05, 0xa1, 0x4e, 0x3a, 0x3f, 0x17, 0x1b, 0xac, 0x58, 0x6c, 0x55, 0xe8, 0x3f, 0xf9, 0x7a, 0x1a, 0xef, 0xfb, 0x3a, 0xf0, 0x0a, 0xdb, 0x22, 0xc6, 0xbb
  };

  BitWriter w;
  w.type_tag(9U);
  w.end_type_list();
  w.byte_string(g1_generator);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BLS_G1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsBlsG1PointOutsideSubgroup)
{
  // Arrange: 48 bytes that do not uncompress to a valid subgroup point are a
  // decode error.
  const std::vector<uint8_t> bad(48U, 0xFFU);

  BitWriter w;
  w.type_tag(9U);
  w.end_type_list();
  w.byte_string(bad);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, decodesBlsG2CompressedPoint)
{
  // Arrange: a BLS G2 type tag (10) followed by the compressed G2 generator (96
  // bytes) decodes to a G2 constant.
  const std::vector<uint8_t> g2_generator = {
    0x93, 0xe0, 0x2b, 0x60, 0x52, 0x71, 0x9f, 0x60, 0x7d, 0xac, 0xd3, 0xa0, 0x88, 0x27, 0x4f, 0x65, 0x59, 0x6b, 0xd0, 0xd0, 0x99, 0x20, 0xb6, 0x1a, 0xb5, 0xda, 0x61, 0xbb, 0xdc, 0x7f, 0x50, 0x49, 0x33, 0x4c, 0xf1, 0x12, 0x13, 0x94, 0x5d, 0x57, 0xe5, 0xac, 0x7d, 0x05, 0x5d, 0x04, 0x2b, 0x7e, 0x02, 0x4a, 0xa2, 0xb2, 0xf0, 0x8f, 0x0a, 0x91, 0x26, 0x08, 0x05, 0x27, 0x2d, 0xc5, 0x10, 0x51, 0xc6, 0xe4, 0x7a, 0xd4, 0xfa, 0x40, 0x3b, 0x02, 0xb4, 0x51, 0x0b, 0x64, 0x7a, 0xe3, 0xd1, 0x77, 0x0b, 0xac, 0x03, 0x26, 0xa8, 0x05, 0xbb, 0xef, 0xd4, 0x80, 0x56, 0xc8, 0xc1, 0x21, 0xbd, 0xb8
  };

  BitWriter w;
  w.type_tag(10U);
  w.end_type_list();
  w.byte_string(g2_generator);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BLS_G2);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, rejectsBlsMlResultValue)
{
  // Arrange: an ML-result has no flat serialization, so its value tag is a decode
  // error.
  BitWriter w;
  w.type_tag(11U);
  w.end_type_list();

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

/* ALLOCATION FAILURE TESTS *************************************************/

TEST(cardano_uplc_flat_decode_constant, propagatesTypeNodeAllocationFailure)
{
  // Arrange: integer type, allocation fails before the type descriptor node.
  BitWriter w;
  w.type_tag(0U);
  w.end_type_list();
  w.integer(1);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_constant, propagatesValueAllocationFailure)
{
  // Arrange: integer value allocation fails partway through the decode.
  BitWriter w;
  w.type_tag(0U);
  w.end_type_list();
  w.integer(123456789);

  cardano_uplc_arena_t*    arena    = make_arena();
  cardano_uplc_constant_t* constant = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = decode(w.bytes(), arena, &constant);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(constant, nullptr);

  cardano_uplc_arena_free(&arena);
}

/* TERM TESTS ***************************************************************/

TEST(cardano_uplc_flat_decode_term, decodesVar)
{
  // Arrange: term tag 0, de Bruijn index 5.
  BitWriter w;
  w.bits(0U, 4U);
  w.word(5U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.var_index, 5U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesDelay)
{
  // Arrange: term tag 1 wrapping an error term (tag 6).
  BitWriter w;
  w.bits(1U, 4U);
  w.bits(6U, 4U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_DELAY);
  ASSERT_NE(term->as.unary, nullptr);
  EXPECT_EQ(term->as.unary->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesLambda)
{
  // Arrange: term tag 2 wrapping a var (index 1).
  BitWriter w;
  w.bits(2U, 4U);
  w.bits(0U, 4U);
  w.word(1U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_LAMBDA);
  ASSERT_NE(term->as.unary, nullptr);
  EXPECT_EQ(term->as.unary->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.unary->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesApplyNested)
{
  // Arrange: [ (var 1) (var 2) ] - term tag 3, then two vars.
  BitWriter w;
  w.bits(3U, 4U);
  w.bits(0U, 4U);
  w.word(1U);
  w.bits(0U, 4U);
  w.word(2U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_APPLY);
  ASSERT_NE(term->as.apply.function, nullptr);
  ASSERT_NE(term->as.apply.argument, nullptr);
  EXPECT_EQ(term->as.apply.function->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.apply.function->as.var_index, 1U);
  EXPECT_EQ(term->as.apply.argument->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.apply.argument->as.var_index, 2U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesConstant)
{
  // Arrange: term tag 4, then (con integer 11).
  BitWriter w;
  w.bits(4U, 4U);
  w.type_tag(0U);
  w.end_type_list();
  w.integer(11);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CONSTANT);
  ASSERT_NE(term->as.constant, nullptr);
  EXPECT_EQ(term->as.constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(term->as.constant), 11);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesForce)
{
  // Arrange: term tag 5 wrapping a builtin (addInteger, tag 0).
  BitWriter w;
  w.bits(5U, 4U);
  w.bits(7U, 4U);
  w.bits(0U, 7U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_FORCE);
  ASSERT_NE(term->as.unary, nullptr);
  EXPECT_EQ(term->as.unary->kind, CARDANO_UPLC_TERM_BUILTIN);
  EXPECT_EQ(term->as.unary->as.builtin, CARDANO_UPLC_BUILTIN_ADD_INTEGER);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesError)
{
  // Arrange: term tag 6, no payload.
  BitWriter w;
  w.bits(6U, 4U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesBuiltin)
{
  // Arrange: term tag 7, builtin tag 34 (tailList).
  BitWriter w;
  w.bits(7U, 4U);
  w.bits(34U, 7U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_BUILTIN);
  EXPECT_EQ(term->as.builtin, CARDANO_UPLC_BUILTIN_TAIL_LIST);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, rejectsUnknownBuiltinTag)
{
  // Arrange: term tag 7, builtin tag 101 which names no builtin.
  BitWriter w;
  w.bits(7U, 4U);
  w.bits(101U, 7U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(term, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesConstrWithFields)
{
  // Arrange: (constr 3 (var 1) (var 2)) - tag 8, tag word 3, cons-bit field list.
  BitWriter w;
  w.bits(8U, 4U);
  w.word(3U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.word(1U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.word(2U);
  w.bit(0U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(term->as.constr.tag, 3U);
  ASSERT_EQ(term->as.constr.field_count, 2U);
  EXPECT_EQ(term->as.constr.fields[0]->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.constr.fields[0]->as.var_index, 1U);
  EXPECT_EQ(term->as.constr.fields[1]->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.constr.fields[1]->as.var_index, 2U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesEmptyConstr)
{
  // Arrange: (constr 0) - tag 8, tag word 0, empty field list.
  BitWriter w;
  w.bits(8U, 4U);
  w.word(0U);
  w.bit(0U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(term->as.constr.tag, 0U);
  EXPECT_EQ(term->as.constr.field_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, decodesCaseWithBranches)
{
  // Arrange: (case (var 1) (var 2) (var 3)) - tag 9, scrutinee, cons-bit branches.
  BitWriter w;
  w.bits(9U, 4U);
  w.bits(0U, 4U);
  w.word(1U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.word(2U);
  w.bit(1U);
  w.bits(0U, 4U);
  w.word(3U);
  w.bit(0U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(term, nullptr);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CASE);
  ASSERT_NE(term->as.cases.scrutinee, nullptr);
  EXPECT_EQ(term->as.cases.scrutinee->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.cases.scrutinee->as.var_index, 1U);
  ASSERT_EQ(term->as.cases.branch_count, 2U);
  EXPECT_EQ(term->as.cases.branches[0]->as.var_index, 2U);
  EXPECT_EQ(term->as.cases.branches[1]->as.var_index, 3U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, rejectsUnknownTermTag)
{
  // Arrange: term tag 10 names no term form.
  BitWriter w;
  w.bits(10U, 4U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(term, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, rejectsTruncatedTerm)
{
  // Arrange: an apply (tag 3) whose two operand terms are absent.
  BitWriter w;
  w.bits(3U, 4U);

  std::vector<uint8_t> bytes = w.bytes();
  bytes.resize(1U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(bytes, arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(term, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, rejectsOverDeepNesting)
{
  // Arrange: a chain of 5000 delays exceeds the recursion depth bound.
  BitWriter w;

  for (int i = 0; i < 5000; ++i)
  {
    w.bits(1U, 4U);
  }

  w.bits(6U, 4U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(term, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, rejectsNullArguments)
{
  // Arrange
  cardano_uplc_arena_t*      arena  = make_arena();
  const cardano_uplc_term_t* term   = nullptr;
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  const byte_t               data[] = { 0x60U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, data, sizeof(data)), CARDANO_SUCCESS);

  // Act + Assert
  EXPECT_EQ(cardano_uplc_flat_decode_term(nullptr, &reader, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_decode_term(arena, nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_decode_term(arena, &reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_term, propagatesAllocationFailure)
{
  // Arrange: a var term whose node allocation fails right away.
  BitWriter w;
  w.bits(0U, 4U);
  w.word(1U);

  cardano_uplc_arena_t*      arena = make_arena();
  const cardano_uplc_term_t* term  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = decode_term(w.bytes(), arena, &term);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(term, nullptr);

  cardano_uplc_arena_free(&arena);
}

/* PROGRAM TESTS ***********************************************************/

TEST(cardano_uplc_flat_decode_program, decodesLamApplySelf)
{
  // Arrange: (program 1.0.0 (lam [ (var 1) (var 1) ])).
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(2U, 4U);
  w.bits(3U, 4U);
  w.bits(0U, 4U);
  w.word(1U);
  w.bits(0U, 4U);
  w.word(1U);
  w.filler();

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(w.bytes(), arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->version_major, 1U);
  EXPECT_EQ(program->version_minor, 0U);
  EXPECT_EQ(program->version_patch, 0U);
  ASSERT_NE(program->term, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_LAMBDA);
  ASSERT_NE(program->term->as.unary, nullptr);
  EXPECT_EQ(program->term->as.unary->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(program->term->as.unary->as.apply.function->as.var_index, 1U);
  EXPECT_EQ(program->term->as.unary->as.apply.argument->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, decodesBuiltinAppliedToConstants)
{
  // Arrange: (program 1.0.0 [ [ (builtin addInteger) (con integer 2) ] (con integer 3) ]).
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(3U, 4U);
  w.bits(3U, 4U);
  w.bits(7U, 4U);
  w.bits(0U, 7U);
  w.bits(4U, 4U);
  w.type_tag(0U);
  w.end_type_list();
  w.integer(2);
  w.bits(4U, 4U);
  w.type_tag(0U);
  w.end_type_list();
  w.integer(3);
  w.filler();

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(w.bytes(), arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  ASSERT_NE(program->term, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_APPLY);

  const cardano_uplc_term_t* outer = program->term;
  const cardano_uplc_term_t* inner = outer->as.apply.function;
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(inner->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(inner->as.apply.function->kind, CARDANO_UPLC_TERM_BUILTIN);
  EXPECT_EQ(inner->as.apply.function->as.builtin, CARDANO_UPLC_BUILTIN_ADD_INTEGER);
  EXPECT_EQ(inner->as.apply.argument->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(inner->as.apply.argument->as.constant), 2);
  EXPECT_EQ(outer->as.apply.argument->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(outer->as.apply.argument->as.constant), 3);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, decodesConstrProgramV11)
{
  // Arrange: (program 1.1.0 (constr 0)) cross-checks the corpus constr-01 shape.
  BitWriter w;
  w.word(1U);
  w.word(1U);
  w.word(0U);
  w.bits(8U, 4U);
  w.word(0U);
  w.bit(0U);
  w.filler();

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(w.bytes(), arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->version_major, 1U);
  EXPECT_EQ(program->version_minor, 1U);
  EXPECT_EQ(program->version_patch, 0U);
  ASSERT_NE(program->term, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(program->term->as.constr.tag, 0U);
  EXPECT_EQ(program->term->as.constr.field_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, acceptsTrailingZeroByteAfterFiller)
{
  // Arrange: a valid, properly filled program followed by a trailing zero byte.
  // The flat decoder consumes only one filler after the term and does not inspect
  // the rest of the buffer, so a trailing byte left unread is not an error.
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(6U, 4U);
  w.filler();

  std::vector<uint8_t> bytes = w.bytes();
  bytes.push_back(0x00U);

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(bytes, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, acceptsArbitraryTrailingBytesAfterFiller)
{
  // The reference flat decoders read the term and a single filler then return
  // without checking that the buffer is fully consumed. Rejecting trailing bytes
  // would make this decoder stricter than the ledger, a consensus hazard, so a
  // valid program followed by arbitrary trailing bytes still decodes, and to the
  // same program as the unpadded form.
  std::vector<uint8_t> clean = sample_flat_program();

  cardano_uplc_arena_t*         clean_arena   = make_arena();
  const cardano_uplc_program_t* clean_program = nullptr;
  ASSERT_EQ(decode_program(clean, clean_arena, &clean_program), CARDANO_SUCCESS);
  expect_sample_program(clean_program);

  std::vector<uint8_t> trailing = clean;
  trailing.push_back(0xDEU);
  trailing.push_back(0xADU);
  trailing.push_back(0xBEU);
  trailing.push_back(0xEFU);

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  EXPECT_EQ(decode_program(trailing, arena, &program), CARDANO_SUCCESS);
  expect_sample_program(program);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&clean_arena);
}

TEST(cardano_uplc_flat_decode_program, rejectsFillerRunningPastEnd)
{
  // Arrange: an error term followed by a whole byte of zero filler bits with no
  // terminating one bit, so the final filler runs off the end of the stream.
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(6U, 4U);

  std::vector<uint8_t> bytes = w.bytes();
  bytes.back()               &= 0xF0U; // clear the four trailing bits in the term byte
  bytes.push_back(0x00U);              // a full zero byte with no terminating filler one bit

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(bytes, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, rejectsTruncatedVersion)
{
  // Arrange: a single version word then end of stream.
  BitWriter w;
  w.word(1U);

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(w.bytes(), arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, rejectsVersionWordTooLarge)
{
  // Arrange: a major version word that overflows 32 bits (2^33).
  BitWriter w;
  w.word(static_cast<uint64_t>(1) << 33U);
  w.word(0U);
  w.word(0U);
  w.bits(6U, 4U);
  w.filler();

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(w.bytes(), arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, rejectsMissingFiller)
{
  // Arrange: version triple and an error term, but the stream ends with no filler.
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(6U, 4U);

  std::vector<uint8_t> bytes = w.bytes();
  bytes.back()               &= 0xF0U;

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_program(bytes, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, rejectsNullArguments)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  cardano_uplc_flat_reader_t    reader  = { nullptr, 0U, 0U, 0U };
  const byte_t                  data[]  = { 0x01U, 0x00U, 0x00U, 0x61U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, data, sizeof(data)), CARDANO_SUCCESS);

  // Act + Assert
  EXPECT_EQ(cardano_uplc_flat_decode_program(nullptr, &reader, &program), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_decode_program(arena, nullptr, &program), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_decode_program(arena, &reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_decode_program, propagatesAllocationFailure)
{
  // Arrange: a minimal program whose first node allocation fails right away.
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(6U, 4U);
  w.filler();

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = decode_program(w.bytes(), arena, &program);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

/* SCRIPT UNWRAP TESTS ******************************************************/

TEST(cardano_uplc_program_from_script_bytes, matchesAikenReferenceVector)
{
  // Arrange: the aiken from_cbor reference, 46010000200101, is a single CBOR
  // bytestring (0x46) wrapping the flat bytes 010000200101.
  const std::vector<uint8_t> script = { 0x46U, 0x01U, 0x00U, 0x00U, 0x20U, 0x01U, 0x01U };

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  expect_sample_program(program);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, decodesSingleWrapped)
{
  // Arrange: flat bytes wrapped in one CBOR bytestring.
  const std::vector<uint8_t> script = cbor_wrap(sample_flat_program());

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  expect_sample_program(program);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, doesNotTransparentlyAcceptDoubleWrapped)
{
  // Arrange: flat bytes wrapped in a CBOR bytestring of a CBOR bytestring. This
  // entry peels EXACTLY ONE bytestring (matching aiken from_cbor). After the single
  // peel the buffer still carries the inner CBOR bytestring header, so it is never
  // the original flat program: depending on the payload the flat decoder either
  // rejects it with CARDANO_ERROR_DECODING or yields some unrelated program, but it
  // must NOT silently reconstruct the wrapped program. A caller holding a
  // double-wrapped script removes the extra layer itself before calling.
  const std::vector<uint8_t> script = cbor_wrap(cbor_wrap(sample_flat_program()));

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert: the wrapped (program 1.0.0 (lam (var 1))) is not reproduced.
  if (result == CARDANO_SUCCESS)
  {
    ASSERT_NE(program, nullptr);
    const bool is_sample = (program->version_major == 1U) && (program->version_minor == 0U) && (program->version_patch == 0U) && (program->term != nullptr) && (program->term->kind == CARDANO_UPLC_TERM_LAMBDA);
    EXPECT_FALSE(is_sample);
  }
  else
  {
    EXPECT_EQ(result, CARDANO_ERROR_DECODING);
    EXPECT_EQ(program, nullptr);
  }

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, decodesCorrectVersionWhenFlatLooksLikeCbor)
{
  // Regression for the old double-peel bug. The program is
  // (program 69.0.0 (delay (force (delay (error))))). Its major version word 69
  // encodes as the single flat byte 0x45, a CBOR type-2 header for a 5-byte
  // bytestring, and the whole flat encoding is exactly 6 bytes (45 00 00 15 16 01).
  // Single-wrapped, the peeled inner buffer is therefore a complete, all-consuming
  // CBOR bytestring: 0x45 followed by exactly five bytes. The old code took that
  // bait, peeled a SECOND time, and decoded the five payload bytes (00 00 15 16 01)
  // as a flat program, returning SUCCESS with version 0.0.0 -- the WRONG program.
  // With the single-peel fix the inner bytes are decoded as flat directly, so the
  // correct version 69 comes out.
  BitWriter w;
  w.word(69U);
  w.word(0U);
  w.word(0U);
  w.bits(1U, 4U); // delay
  w.bits(5U, 4U); // force
  w.bits(1U, 4U); // delay
  w.bits(6U, 4U); // error
  w.filler();

  const std::vector<uint8_t> flat = w.bytes();
  ASSERT_EQ(flat.size(), 6U); // 0x45 + 5 bytes => the inner buffer is a complete bytestring
  ASSERT_EQ(flat[0], 0x45U);  // first flat byte is a cbor type-2 header for 5 bytes

  const std::vector<uint8_t> script = cbor_wrap(flat);

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert: the correct version, not the 0.0.0 the old second peel produced.
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->version_major, 69U);
  EXPECT_EQ(program->version_minor, 0U);
  EXPECT_EQ(program->version_patch, 0U);
  ASSERT_NE(program->term, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_DELAY);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, rejectsNotABytestring)
{
  // Arrange: a CBOR unsigned integer (0x01) is not a bytestring.
  const std::vector<uint8_t> script = { 0x01U };

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, rejectsTruncatedCbor)
{
  // Arrange: a bytestring header claiming six bytes with none present.
  const std::vector<uint8_t> script = { 0x46U };

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, rejectsTrailingBytesAfterOuter)
{
  // Arrange: a complete CBOR bytestring of flat bytes followed by a stray byte.
  std::vector<uint8_t> script = cbor_wrap(sample_flat_program());
  script.push_back(0x00U);

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, rejectsInnerFlatDecodeFailure)
{
  // Arrange: a well-formed outer bytestring wrapping bytes that are not a valid
  // flat program (a single version word then end of stream).
  BitWriter w;
  w.word(1U);

  const std::vector<uint8_t> script = cbor_wrap(w.bytes());

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, rejectsEmptyInput)
{
  // Arrange: an empty buffer has no CBOR item to peel.
  const std::vector<uint8_t> script;

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, rejectsNullArguments)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  const byte_t                  data[]  = { 0x46U, 0x01U, 0x00U, 0x00U, 0x20U, 0x01U, 0x01U };

  // Act + Assert
  EXPECT_EQ(cardano_uplc_program_from_script_bytes(nullptr, data, sizeof(data), &program), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_from_script_bytes(arena, data, sizeof(data), nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_from_script_bytes(arena, nullptr, sizeof(data), &program), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, acceptsNullDataWhenSizeIsZero)
{
  // Arrange: a NULL buffer with size 0 is a well-formed empty stream, which has no
  // CBOR item, so it is rejected as a decode failure rather than a NULL-pointer error.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = cardano_uplc_program_from_script_bytes(arena, nullptr, 0U, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, propagatesReaderAllocationFailure)
{
  // Arrange: the first allocation (the CBOR reader) fails right away.
  const std::vector<uint8_t> script = cbor_wrap(sample_flat_program());

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert: the CBOR reader is the first allocation, so the failure surfaces as a
  // memory-allocation error from the peel.
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_from_script_bytes, propagatesLaterAllocationFailure)
{
  // Arrange: allocation fails after the first successful allocation, exercising a
  // failure once the CBOR reader exists but the bytestring read cannot allocate.
  const std::vector<uint8_t> script = cbor_wrap(sample_flat_program());

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = decode_script(script, arena, &program);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(program, nullptr);

  cardano_uplc_arena_free(&arena);
}
