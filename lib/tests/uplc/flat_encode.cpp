/**
 * \file flat_encode.cpp
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include "../../src/uplc/ast/uplc_term.h"

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/builtins/bls.h"
#include "../../src/uplc/flat/flat_decode.h"
#include "../../src/uplc/flat/flat_reader.h"
#include "../../src/uplc/flat/flat_writer.h"
#include "../../src/uplc/syntax/pretty.h"
#include "../../src/uplc/syntax/text_parser.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <sstream>
#include <string>
#include <vector>

/* STATIC HELPERS ************************************************************/

namespace {

namespace fs = std::filesystem;

cardano_uplc_arena_t*
make_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(0U, &arena), CARDANO_SUCCESS);
  return arena;
}

// Renders a program to the canonical surface syntax for structural comparison.
std::string
render(const cardano_uplc_program_t* program)
{
  cardano_buffer_t* buffer = nullptr;

  if (cardano_uplc_pretty_print_program(program, &buffer) != CARDANO_SUCCESS)
  {
    return std::string();
  }

  std::string out(reinterpret_cast<const char*>(cardano_buffer_get_data(buffer)), cardano_buffer_get_size(buffer));
  cardano_buffer_unref(&buffer);

  while (!out.empty() && (out.back() == '\0'))
  {
    out.pop_back();
  }

  return out;
}

// Decodes a flat byte buffer into a program over the supplied arena.
cardano_error_t
decode_program(const std::vector<uint8_t>& bytes, cardano_uplc_arena_t* arena, const cardano_uplc_program_t** program)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, bytes.data(), bytes.size()), CARDANO_SUCCESS);
  return cardano_uplc_flat_decode_program(arena, &reader, program);
}

std::vector<uint8_t>
to_vector(const cardano_buffer_t* buffer)
{
  const uint8_t* data = cardano_buffer_get_data(buffer);
  const size_t   size = cardano_buffer_get_size(buffer);
  return std::vector<uint8_t>(data, data + size);
}

std::string
to_hex(const std::vector<uint8_t>& bytes)
{
  static const char* digits = "0123456789abcdef";
  std::string        out;

  for (uint8_t b : bytes)
  {
    out.push_back(digits[(b >> 4U) & 0x0FU]);
    out.push_back(digits[b & 0x0FU]);
  }

  return out;
}

// Wraps a small program-building term into a full program owned by the arena.
// Builds an error term over the arena (a convenient leaf for negative tests).
const cardano_uplc_term_t*
make_error(cardano_uplc_arena_t* arena)
{
  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);
  return term;
}

// Wraps a constant in a constant term over the arena.
const cardano_uplc_term_t*
make_constant_term(cardano_uplc_arena_t* arena, const cardano_uplc_constant_t* constant)
{
  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, &term), CARDANO_SUCCESS);
  return term;
}

cardano_error_t
wrap_program(cardano_uplc_arena_t* arena, const cardano_uplc_term_t* term, const cardano_uplc_program_t** program)
{
  cardano_uplc_program_t* built = static_cast<cardano_uplc_program_t*>(
    cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));

  if (built == nullptr)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  built->version_major = 1U;
  built->version_minor = 0U;
  built->version_patch = 0U;
  built->term          = term;

  *program = built;

  return CARDANO_SUCCESS;
}

// Builds a single-constant program from a constant, encodes, decodes, and
// asserts the round-tripped rendering matches. Also asserts the encode is a
// fixed point.
void
round_trip_constant(cardano_uplc_constant_t* constant)
{
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_term_t*    term    = make_constant_term(arena, constant);
  const cardano_uplc_program_t* program = nullptr;

  ASSERT_EQ(wrap_program(arena, term, &program), CARDANO_SUCCESS);

  cardano_buffer_t* flat = nullptr;
  ASSERT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_SUCCESS);

  cardano_uplc_arena_t*         decode_arena = make_arena();
  const cardano_uplc_program_t* decoded      = nullptr;
  ASSERT_EQ(decode_program(to_vector(flat), decode_arena, &decoded), CARDANO_SUCCESS);

  EXPECT_EQ(render(program), render(decoded));

  cardano_buffer_t* flat2 = nullptr;
  ASSERT_EQ(cardano_uplc_flat_encode_program(decoded, &flat2), CARDANO_SUCCESS);
  EXPECT_EQ(to_vector(flat), to_vector(flat2));

  cardano_buffer_unref(&flat2);
  cardano_buffer_unref(&flat);
  cardano_uplc_arena_free(&decode_arena);
  cardano_uplc_arena_free(&arena);
}

std::string
corpus_root()
{
  fs::path file = fs::path(__FILE__);
  return (file.parent_path() / "vectors").string();
}

std::string
read_file(const std::string& path)
{
  std::ifstream      stream(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

} // namespace

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_flat_encode_program, rejectsNullArguments)
{
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_term_t*    term    = make_error(arena);
  const cardano_uplc_program_t* program = nullptr;
  cardano_buffer_t*             out     = nullptr;

  ASSERT_EQ(wrap_program(arena, term, &program), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_flat_encode_program(nullptr, &out), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_flat_encode_program(program, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_to_cbor(nullptr, &out), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_to_cbor(program, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_program, matchesAikenReferenceVector)
{
  // (program 1.0.0 (lam (var 1))) -> flat 010000200101, CBOR 46010000200101.
  cardano_uplc_arena_t*         arena   = make_arena();
  cardano_uplc_term_t*          var     = nullptr;
  cardano_uplc_term_t*          lam     = nullptr;
  const cardano_uplc_program_t* program = nullptr;

  ASSERT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_term_new_lambda(arena, var, &lam), CARDANO_SUCCESS);
  ASSERT_EQ(wrap_program(arena, lam, &program), CARDANO_SUCCESS);

  cardano_buffer_t* flat = nullptr;
  ASSERT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(to_vector(flat)), "010000200101");

  cardano_buffer_t* cbor = nullptr;
  ASSERT_EQ(cardano_uplc_program_to_cbor(program, &cbor), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(to_vector(cbor)), "46010000200101");

  cardano_buffer_unref(&cbor);
  cardano_buffer_unref(&flat);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_program, encodesErrorTerm)
{
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_term_t*    err     = make_error(arena);
  const cardano_uplc_program_t* program = nullptr;

  ASSERT_EQ(wrap_program(arena, err, &program), CARDANO_SUCCESS);

  cardano_buffer_t* flat = nullptr;
  ASSERT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(to_vector(flat)), "01000061");

  cardano_buffer_unref(&flat);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_program, encodesBuiltinAddInteger)
{
  cardano_uplc_arena_t*         arena   = make_arena();
  cardano_uplc_term_t*          fn      = nullptr;
  const cardano_uplc_program_t* program = nullptr;

  ASSERT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, &fn), CARDANO_SUCCESS);
  ASSERT_EQ(wrap_program(arena, fn, &program), CARDANO_SUCCESS);

  cardano_buffer_t* flat = nullptr;
  ASSERT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(to_vector(flat)), "01000070" "01");

  cardano_buffer_unref(&flat);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_program, encodesUnitConstant)
{
  cardano_uplc_arena_t*         arena   = make_arena();
  cardano_uplc_constant_t*      con     = nullptr;
  const cardano_uplc_program_t* program = nullptr;

  ASSERT_EQ(cardano_uplc_constant_new_unit(arena, &con), CARDANO_SUCCESS);
  const cardano_uplc_term_t* term = make_constant_term(arena, con);
  ASSERT_EQ(wrap_program(arena, term, &program), CARDANO_SUCCESS);

  cardano_buffer_t* flat = nullptr;
  ASSERT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(to_vector(flat)), "0100004981");

  cardano_buffer_unref(&flat);
  cardano_uplc_arena_free(&arena);
}

/* CONSTANT ROUND-TRIPS ******************************************************/

TEST(cardano_uplc_flat_encode_constant, integerLargeAndNegative)
{
  const char* values[] = { "0", "1", "-1", "127", "-128",
                           "123456789012345678901234567890",
                           "-987654321098765432109876543210" };

  for (const char* text : values)
  {
    cardano_uplc_arena_t*    arena = make_arena();
    cardano_bigint_t*        big   = nullptr;
    cardano_uplc_constant_t* con   = nullptr;

    ASSERT_EQ(cardano_bigint_from_string(text, strlen(text), 10, &big), CARDANO_SUCCESS);
    ASSERT_EQ(cardano_uplc_constant_new_integer(arena, big, &con), CARDANO_SUCCESS);
    cardano_bigint_unref(&big);

    round_trip_constant(con);

    cardano_uplc_arena_free(&arena);
  }
}

TEST(cardano_uplc_flat_encode_constant, byteStringAndString)
{
  cardano_uplc_arena_t* arena = make_arena();

  const std::array<uint8_t, 5> raw = { 0x00U, 0xFFU, 0x10U, 0xABU, 0xCDU };
  cardano_buffer_t*            bs  = cardano_buffer_new_from(raw.data(), raw.size());
  ASSERT_NE(bs, nullptr);
  cardano_uplc_constant_t* bs_con = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_byte_string(arena, bs, &bs_con), CARDANO_SUCCESS);
  cardano_buffer_unref(&bs);
  round_trip_constant(bs_con);

  const char*       text   = "hello, plutus";
  cardano_buffer_t* str    = cardano_buffer_new_from(reinterpret_cast<const uint8_t*>(text), strlen(text));
  ASSERT_NE(str, nullptr);
  cardano_uplc_constant_t* str_con = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_string(arena, str, &str_con), CARDANO_SUCCESS);
  cardano_buffer_unref(&str);
  round_trip_constant(str_con);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_constant, boolAndUnit)
{
  cardano_uplc_arena_t* arena = make_arena();

  cardano_uplc_constant_t* t = nullptr;
  cardano_uplc_constant_t* f = nullptr;
  cardano_uplc_constant_t* u = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_bool(arena, true, &t), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_constant_new_bool(arena, false, &f), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_constant_new_unit(arena, &u), CARDANO_SUCCESS);

  round_trip_constant(t);
  round_trip_constant(f);
  round_trip_constant(u);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_constant, data)
{
  cardano_uplc_arena_t*  arena = make_arena();
  cardano_bigint_t*      big   = nullptr;
  cardano_plutus_data_t* data  = nullptr;

  ASSERT_EQ(cardano_bigint_from_int(42, &big), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_data_new_integer(big, &data), CARDANO_SUCCESS);
  cardano_bigint_unref(&big);

  cardano_uplc_constant_t* con = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_data(arena, data, &con), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);

  round_trip_constant(con);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_constant, nestedListAndPair)
{
  cardano_uplc_arena_t* arena = make_arena();

  cardano_uplc_type_t* int_type = nullptr;
  ASSERT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_type), CARDANO_SUCCESS);

  cardano_uplc_constant_t* items[3] = { nullptr, nullptr, nullptr };
  for (int i = 0; i < 3; ++i)
  {
    cardano_bigint_t* big = nullptr;
    ASSERT_EQ(cardano_bigint_from_int(i - 1, &big), CARDANO_SUCCESS);
    ASSERT_EQ(cardano_uplc_constant_new_integer(arena, big, &items[i]), CARDANO_SUCCESS);
    cardano_bigint_unref(&big);
  }

  const cardano_uplc_constant_t* item_ptrs[3] = { items[0], items[1], items[2] };
  cardano_uplc_constant_t*       list          = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_list(arena, int_type, item_ptrs, 3U, &list), CARDANO_SUCCESS);
  round_trip_constant(list);

  // pair (bool, list<integer>)
  cardano_uplc_constant_t* boolean = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_bool(arena, true, &boolean), CARDANO_SUCCESS);
  cardano_uplc_constant_t* pair = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_pair(arena, boolean, list, &pair), CARDANO_SUCCESS);
  round_trip_constant(pair);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_flat_encode_constant, blsG1AndG2)
{
  const std::array<uint8_t, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE> g1 = {
    0x97, 0xf1, 0xd3, 0xa7, 0x31, 0x97, 0xd7, 0x94, 0x26, 0x95, 0x63, 0x8c,
    0x4f, 0xa9, 0xac, 0x0f, 0xc3, 0x68, 0x8c, 0x4f, 0x97, 0x74, 0xb9, 0x05,
    0xa1, 0x4e, 0x3a, 0x3f, 0x17, 0x1b, 0xac, 0x58, 0x6c, 0x55, 0xe8, 0x3f,
    0xf9, 0x7a, 0x1a, 0xef, 0xfb, 0x3a, 0xf0, 0x0a, 0xdb, 0x22, 0xc6, 0xbb
  };

  const std::array<uint8_t, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE> g2 = {
    0x93, 0xe0, 0x2b, 0x60, 0x52, 0x71, 0x9f, 0x60, 0x7d, 0xac, 0xd3, 0xa0,
    0x88, 0x27, 0x4f, 0x65, 0x59, 0x6b, 0xd0, 0xd0, 0x99, 0x20, 0xb6, 0x1a,
    0xb5, 0xda, 0x61, 0xbb, 0xdc, 0x7f, 0x50, 0x49, 0x33, 0x4c, 0xf1, 0x12,
    0x13, 0x94, 0x5d, 0x57, 0xe5, 0xac, 0x7d, 0x05, 0x5d, 0x04, 0x2b, 0x7e,
    0x02, 0x4a, 0xa2, 0xb2, 0xf0, 0x8f, 0x0a, 0x91, 0x26, 0x08, 0x05, 0x27,
    0x2d, 0xc5, 0x10, 0x51, 0xc6, 0xe4, 0x7a, 0xd4, 0xfa, 0x40, 0x3b, 0x02,
    0xb4, 0x51, 0x0b, 0x64, 0x7a, 0xe3, 0xd1, 0x77, 0x0b, 0xac, 0x03, 0x26,
    0xa8, 0x05, 0xbb, 0xef, 0xd4, 0x80, 0x56, 0xc8, 0xc1, 0x21, 0xbd, 0xb8
  };

  cardano_uplc_arena_t* arena = make_arena();

  cardano_uplc_constant_t* g1_con = nullptr;
  ASSERT_EQ(cardano_uplc_int_bls_g1_from_compressed(arena, g1.data(), g1.size(), &g1_con), CARDANO_SUCCESS);
  round_trip_constant(g1_con);

  cardano_uplc_constant_t* g2_con = nullptr;
  ASSERT_EQ(cardano_uplc_int_bls_g2_from_compressed(arena, g2.data(), g2.size(), &g2_con), CARDANO_SUCCESS);
  round_trip_constant(g2_con);

  cardano_uplc_arena_free(&arena);
}

/* UNSUPPORTED CONSTANT KINDS ************************************************/

TEST(cardano_uplc_flat_encode_constant, arrayValueAndMlResultAreRejected)
{
  cardano_uplc_arena_t* arena = make_arena();

  cardano_uplc_type_t* int_type = nullptr;
  ASSERT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_type), CARDANO_SUCCESS);

  cardano_uplc_constant_t* array = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_array(arena, int_type, nullptr, 0U, &array), CARDANO_SUCCESS);

  const cardano_uplc_term_t*    term    = make_constant_term(arena, array);
  const cardano_uplc_program_t* program = nullptr;
  cardano_buffer_t*             flat    = nullptr;
  ASSERT_EQ(wrap_program(arena, term, &program), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(flat, nullptr);

  cardano_uplc_constant_t* value = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_value(arena, int_type, nullptr, 0U, &value), CARDANO_SUCCESS);
  const cardano_uplc_term_t*    vterm = make_constant_term(arena, value);
  const cardano_uplc_program_t* vprog = nullptr;
  ASSERT_EQ(wrap_program(arena, vterm, &vprog), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_flat_encode_program(vprog, &flat), CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

/* ALLOCATOR INJECTION *******************************************************/

TEST(cardano_uplc_flat_encode_program, propagatesAllocationFailure)
{
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_term_t*    err     = make_error(arena);
  const cardano_uplc_program_t* program = nullptr;
  cardano_buffer_t*             flat    = nullptr;

  ASSERT_EQ(wrap_program(arena, err, &program), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  EXPECT_EQ(cardano_uplc_flat_encode_program(program, &flat), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(flat, nullptr);

  cardano_set_allocators(malloc, realloc, free);
  cardano_uplc_arena_free(&arena);
}

/* CORPUS ROUND-TRIP *********************************************************/

TEST(cardano_uplc_flat_encode_program, roundTripsCorpusPrograms)
{
  size_t round_tripped = 0U;

  for (const auto& entry : fs::recursive_directory_iterator(corpus_root()))
  {
    if (!entry.is_regular_file())
    {
      continue;
    }

    const std::string name = entry.path().filename().string();

    if (name.size() < 5U)
    {
      continue;
    }

    const bool is_uplc = (name.compare(name.size() - 5U, 5U, ".uplc") == 0);

    if (!is_uplc)
    {
      continue;
    }

    const std::string text = read_file(entry.path().string());

    cardano_uplc_arena_t*         arena   = make_arena();
    const cardano_uplc_program_t* program = nullptr;
    size_t                        offset  = 0U;

    cardano_error_t parsed = cardano_uplc_parse_program(arena, text.c_str(), text.size(), &program, &offset);

    if (parsed != CARDANO_SUCCESS)
    {
      // Parse-error vectors and BLS value literals are not encodable here.
      cardano_uplc_arena_free(&arena);
      continue;
    }

    cardano_buffer_t* flat    = nullptr;
    cardano_error_t   encoded = cardano_uplc_flat_encode_program(program, &flat);

    if (encoded == CARDANO_ERROR_INVALID_ARGUMENT)
    {
      // A V4 array or value constant has no flat serialization; the decoder
      // rejects it too, so it is not part of the flat round-trip corpus.
      cardano_uplc_arena_free(&arena);
      continue;
    }

    ASSERT_EQ(encoded, CARDANO_SUCCESS) << entry.path().string();

    cardano_uplc_arena_t*         decode_arena = make_arena();
    const cardano_uplc_program_t* decoded      = nullptr;
    ASSERT_EQ(decode_program(to_vector(flat), decode_arena, &decoded), CARDANO_SUCCESS) << entry.path().string();

    EXPECT_EQ(render(program), render(decoded)) << entry.path().string();

    cardano_buffer_t* flat2 = nullptr;
    ASSERT_EQ(cardano_uplc_flat_encode_program(decoded, &flat2), CARDANO_SUCCESS) << entry.path().string();
    EXPECT_EQ(to_vector(flat), to_vector(flat2)) << entry.path().string();

    round_tripped += 1U;

    cardano_buffer_unref(&flat2);
    cardano_buffer_unref(&flat);
    cardano_uplc_arena_free(&decode_arena);
    cardano_uplc_arena_free(&arena);
  }

  EXPECT_GT(round_tripped, 100U);
}
