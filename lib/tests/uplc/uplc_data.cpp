/**
 * \file uplc_data.cpp
 *
 * \author angel.castillo
 * \date   Jun 20, 2026
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
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/cost/uplc_ex_mem.h"
#include "../../src/uplc/data/uplc_data.h"

#include <gmock/gmock.h>
#include <string>
#include <vector>

/* HELPERS *******************************************************************/

namespace
{

cardano_uplc_arena_t*
make_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);
  return arena;
}

std::vector<uint8_t>
from_hex(const std::string& hex)
{
  cardano_buffer_t* buffer = cardano_buffer_from_hex(hex.c_str(), hex.size());
  EXPECT_NE(buffer, nullptr);

  std::vector<uint8_t> out(cardano_buffer_get_data(buffer), cardano_buffer_get_data(buffer) + cardano_buffer_get_size(buffer));
  cardano_buffer_unref(&buffer);

  return out;
}

std::string
to_hex(const uint8_t* data, size_t size)
{
  static const char* digits = "0123456789abcdef";
  std::string        out;

  for (size_t i = 0U; i < size; ++i)
  {
    out.push_back(digits[(data[i] >> 4U) & 0x0FU]);
    out.push_back(digits[data[i] & 0x0FU]);
  }

  return out;
}

cardano_plutus_data_t*
plutus_from_hex(const std::string& hex)
{
  std::vector<uint8_t>   bytes  = from_hex(hex);
  cardano_cbor_reader_t* reader = cardano_cbor_reader_new(bytes.data(), bytes.size());
  cardano_plutus_data_t* data   = nullptr;

  EXPECT_EQ(cardano_plutus_data_from_cbor(reader, &data), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  return data;
}

std::string
arena_serialise_hex(const cardano_uplc_data_t* node)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_buffer_t*      bytes  = nullptr;

  EXPECT_EQ(cardano_uplc_data_to_cbor(node, writer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &bytes), CARDANO_SUCCESS);

  std::string out = to_hex(cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes));

  cardano_buffer_unref(&bytes);
  cardano_cbor_writer_unref(&writer);

  return out;
}

std::string
plutus_canonical_hex(cardano_plutus_data_t* data)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_buffer_t*      bytes  = nullptr;

  cardano_plutus_data_clear_cbor_cache(data);
  EXPECT_EQ(cardano_plutus_data_to_cbor(data, writer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &bytes), CARDANO_SUCCESS);

  std::string out = to_hex(cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes));

  cardano_buffer_unref(&bytes);
  cardano_cbor_writer_unref(&writer);

  return out;
}

const char* kCorpus[] = {
  "182a",                     /* Integer 42 */
  "3863",                     /* Integer -100 */
  "00",                       /* Integer 0 */
  "1bffffffffffffffff",       /* Integer 2^64-1 (uint64 max) */
  "c249010000000000000000",   /* Integer 2^64 (bignum tag 2) */
  "c349010000000000000000",   /* negative bignum (tag 3) */
  "4401020304",               /* Bytes 01020304 */
  "40",                       /* empty Bytes */
  "9f0102ff",                 /* List [1, 2] (indefinite) */
  "80",                       /* empty List (definite) */
  "a10102",                   /* Map {1: 2} (definite) */
  "d8799f0102ff",             /* Constr 0 [1, 2] (tag 121) */
  "d87a9f43abcdefff",         /* Constr 1 [bytes] (tag 122) */
  "d9050080",                 /* Constr 7 [] (tag 1280) */
  "d8668218ff9f01ff",         /* Constr 255 [1] general form (tag 102) */
  "d8799f0102d8799f0304ffff", /* nested Constr */
};

} // namespace

/* PARSE VS LIBRARY ***********************************************************/

TEST(cardano_uplc_data_from_cbor_bytes, parsesEveryCorpusVectorStructurallyLikeTheLibrary)
{
  for (const char* hex: kCorpus)
  {
    cardano_uplc_arena_t* arena = make_arena();
    std::vector<uint8_t>  bytes = from_hex(hex);
    cardano_uplc_data_t*  node  = nullptr;

    ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS) << hex;
    ASSERT_NE(node, nullptr) << hex;

    cardano_plutus_data_t* library_node     = nullptr;
    cardano_plutus_data_t* arena_as_library = nullptr;
    ASSERT_EQ(cardano_uplc_data_to_plutus_data(node, &arena_as_library), CARDANO_SUCCESS) << hex;

    library_node = plutus_from_hex(hex);
    EXPECT_TRUE(cardano_plutus_data_equals(arena_as_library, library_node)) << hex;

    cardano_plutus_data_unref(&arena_as_library);
    cardano_plutus_data_unref(&library_node);
    cardano_uplc_arena_free(&arena);
  }
}

/* SERIALISE CANONICAL BYTES **************************************************/

TEST(cardano_uplc_data_to_cbor, producesTheSameCanonicalBytesAsTheLibrary)
{
  for (const char* hex: kCorpus)
  {
    cardano_uplc_arena_t* arena = make_arena();
    std::vector<uint8_t>  bytes = from_hex(hex);
    cardano_uplc_data_t*  node  = nullptr;

    ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS) << hex;

    cardano_plutus_data_t* library_node = plutus_from_hex(hex);

    EXPECT_EQ(arena_serialise_hex(node), plutus_canonical_hex(library_node)) << hex;

    cardano_plutus_data_unref(&library_node);
    cardano_uplc_arena_free(&arena);
  }
}

/* ROUND TRIP LIBRARY -> ARENA -> LIBRARY ************************************/

TEST(cardano_uplc_data_round_trip, libraryToArenaToLibraryIsExact)
{
  for (const char* hex: kCorpus)
  {
    cardano_uplc_arena_t*  arena   = make_arena();
    cardano_plutus_data_t* library = plutus_from_hex(hex);
    cardano_uplc_data_t*   node    = nullptr;
    cardano_plutus_data_t* back    = nullptr;

    ASSERT_EQ(cardano_uplc_data_from_plutus_data(arena, library, &node), CARDANO_SUCCESS) << hex;
    ASSERT_EQ(cardano_uplc_data_to_plutus_data(node, &back), CARDANO_SUCCESS) << hex;

    EXPECT_TRUE(cardano_plutus_data_equals(library, back)) << hex;

    cardano_plutus_data_unref(&back);
    cardano_plutus_data_unref(&library);
    cardano_uplc_arena_free(&arena);
  }
}

/* STRUCTURAL EQUALITY ******************************************************/

TEST(cardano_uplc_data_equals, distinguishesAndMatchesArenaTrees)
{
  cardano_uplc_arena_t* arena = make_arena();

  std::vector<uint8_t> a_bytes = from_hex("d8799f0102ff");
  std::vector<uint8_t> b_bytes = from_hex("d8799f0102ff");
  std::vector<uint8_t> c_bytes = from_hex("d8799f0103ff");

  cardano_uplc_data_t* a = nullptr;
  cardano_uplc_data_t* b = nullptr;
  cardano_uplc_data_t* c = nullptr;

  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, a_bytes.data(), a_bytes.size(), &a), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, b_bytes.data(), b_bytes.size(), &b), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, c_bytes.data(), c_bytes.size(), &c), CARDANO_SUCCESS);

  EXPECT_TRUE(cardano_uplc_data_equals(a, a));
  EXPECT_TRUE(cardano_uplc_data_equals(a, b));
  EXPECT_FALSE(cardano_uplc_data_equals(a, c));
  EXPECT_TRUE(cardano_uplc_data_equals(nullptr, nullptr));
  EXPECT_FALSE(cardano_uplc_data_equals(a, nullptr));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_data_equals, matchesAcrossInlineAndBignumIntegers)
{
  cardano_uplc_arena_t* arena = make_arena();
  cardano_uplc_data_t*  small = nullptr;
  cardano_bigint_t*     big   = nullptr;
  cardano_uplc_data_t*  wide  = nullptr;

  ASSERT_EQ(cardano_uplc_data_new_integer_small(arena, 7, &small), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_bigint_from_int(7, &big), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_data_new_integer(arena, big, &wide), CARDANO_SUCCESS);
  cardano_bigint_unref(&big);

  EXPECT_TRUE(cardano_uplc_data_equals(small, wide));

  cardano_uplc_arena_free(&arena);
}

/* NEGATIVE INTEGER DECODE **************************************************/

TEST(cardano_uplc_data_from_cbor_bytes, negativeIntegerBeyondInt64MatchesLibrary)
{
  const char* kNegatives[] = {
    "3bffffffffffffffff", /* major-type-1 with magnitude 2^64-1 (value > INT64_MAX) */
    "3b8000000000000000", /* -(2^63 + 1), magnitude just past INT64_MAX */
    "3b7fffffffffffffff", /* -(2^63), the inline boundary */
    "3b0000000000000000", /* -1 */
  };

  for (const char* hex: kNegatives)
  {
    cardano_uplc_arena_t* arena = make_arena();
    std::vector<uint8_t>  bytes = from_hex(hex);
    cardano_uplc_data_t*  node  = nullptr;

    ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS) << hex;
    ASSERT_NE(node, nullptr) << hex;

    cardano_plutus_data_t* arena_as_library = nullptr;
    ASSERT_EQ(cardano_uplc_data_to_plutus_data(node, &arena_as_library), CARDANO_SUCCESS) << hex;

    cardano_plutus_data_t* library_node = plutus_from_hex(hex);

    EXPECT_TRUE(cardano_plutus_data_equals(arena_as_library, library_node)) << hex;
    EXPECT_EQ(arena_serialise_hex(node), plutus_canonical_hex(library_node)) << hex;

    cardano_plutus_data_unref(&arena_as_library);
    cardano_plutus_data_unref(&library_node);
    cardano_uplc_arena_free(&arena);
  }
}

/* MEMOIZED EX-MEM **********************************************************/

TEST(cardano_uplc_data_node_ex_mem, deeplyNestedTreeIsMemoizedAndStable)
{
  cardano_uplc_arena_t* arena = make_arena();

  std::vector<uint8_t> bytes = from_hex("d8799f0102d8799f0304ffff");
  cardano_uplc_data_t* node  = nullptr;

  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS);

  const int64_t first  = cardano_uplc_data_node_ex_mem(node);
  const int64_t second = cardano_uplc_data_node_ex_mem(node);

  EXPECT_GT(first, 0);
  EXPECT_EQ(first, second);
  EXPECT_EQ(first, cardano_uplc_data_ex_mem(node));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_data_node_ex_mem, matchesTheKnownLeafFormula)
{
  cardano_uplc_arena_t* arena = make_arena();
  cardano_uplc_data_t*  zero  = nullptr;
  cardano_uplc_data_t*  bytes = nullptr;

  ASSERT_EQ(cardano_uplc_data_new_integer_small(arena, 0, &zero), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_data_new_bytes(arena, nullptr, 0U, &bytes), CARDANO_SUCCESS);

  /* a single Integer(0) leaf costs 4 (node) + 1 (leaf) = 5. */
  EXPECT_EQ(cardano_uplc_data_node_ex_mem(zero), 5);
  /* a single empty Bytes leaf costs 4 + 1 = 5. */
  EXPECT_EQ(cardano_uplc_data_node_ex_mem(bytes), 5);
  EXPECT_EQ(cardano_uplc_data_node_ex_mem(nullptr), 0);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_data_node_count, countsEveryNodeOnce)
{
  cardano_uplc_arena_t* arena = make_arena();

  std::vector<uint8_t> bytes = from_hex("d8799f0102d8799f0304ffff");
  cardano_uplc_data_t* node  = nullptr;

  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS);

  /* Constr[1, 2, Constr[3, 4]]: outer + 2 leaves + inner + 2 leaves = 6. */
  EXPECT_EQ(cardano_uplc_data_node_count(node), 6);
  EXPECT_EQ(cardano_uplc_data_node_count(node), 6);
  EXPECT_EQ(cardano_uplc_data_node_count(nullptr), 0);

  cardano_uplc_arena_free(&arena);
}

/* DIRECT PARSER EDGE CASES ***************************************************/

namespace
{

const char* kEdgeCorpus[] = {
  "00",                           /* integer 0 */
  "1b7fffffffffffffff",           /* INT64_MAX (inline boundary) */
  "1b8000000000000000",           /* INT64_MAX + 1 (spills to bignum) */
  "1bffffffffffffffff",           /* UINT64_MAX (spills to bignum) */
  "c249010000000000000000",       /* 2^64 positive bignum (tag 2) */
  "c349010000000000000000",       /* negative bignum (tag 3) */
  "3bffffffffffffffff",           /* large negative integer */
  "40",                           /* empty byte string */
  "4401020304",                   /* definite byte string */
  "80",                           /* empty definite list */
  "8403040506",                   /* definite list [3,4,5,6] */
  "9f0102ff",                     /* indefinite list [1,2] */
  "9fff",                         /* empty indefinite list */
  "a201020304",                   /* definite map {1:2,3:4} */
  "a0",                           /* empty definite map */
  "bf0102ff",                     /* indefinite map {1:2} */
  "bfff",                         /* empty indefinite map */
  "d8799f0102d87a9f43abcdefffff", /* nested constr with bytes field */
  "d8799fa101029f0203ffff",       /* constr holding map and list */
  "d9050080",                     /* ranged constr tag 1280 (alt 7), empty */
  "d8668218ff9f01ff",             /* general form constr alt 255 */
};

void
expect_parse_matches_library(const std::string& hex)
{
  cardano_uplc_arena_t* arena = make_arena();
  std::vector<uint8_t>  bytes = from_hex(hex);
  cardano_uplc_data_t*  node  = nullptr;

  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS) << hex;
  ASSERT_NE(node, nullptr) << hex;

  cardano_plutus_data_t* arena_as_library = nullptr;
  ASSERT_EQ(cardano_uplc_data_to_plutus_data(node, &arena_as_library), CARDANO_SUCCESS) << hex;

  cardano_plutus_data_t* library_node = plutus_from_hex(hex);

  EXPECT_TRUE(cardano_plutus_data_equals(arena_as_library, library_node)) << hex;
  EXPECT_EQ(arena_serialise_hex(node), plutus_canonical_hex(library_node)) << hex;

  cardano_plutus_data_unref(&arena_as_library);
  cardano_plutus_data_unref(&library_node);
  cardano_uplc_arena_free(&arena);
}

} // namespace

TEST(cardano_uplc_data_direct_parser, matchesLibraryAcrossEdgeConstructs)
{
  for (const char* hex: kEdgeCorpus)
  {
    expect_parse_matches_library(hex);
  }
}

TEST(cardano_uplc_data_direct_parser, parsesChunkedIndefiniteByteStringStructurallyLikeTheLibrary)
{
  /* Build a 200-byte string; the canonical serializer emits it as a chunked
   * indefinite byte string with 64-byte chunks, exercising the concatenation path. */
  std::vector<uint8_t> payload(200U);
  for (size_t i = 0U; i < payload.size(); ++i)
  {
    payload[i] = static_cast<uint8_t>(i & 0xFFU);
  }

  cardano_plutus_data_t* library = nullptr;
  ASSERT_EQ(cardano_plutus_data_new_bytes(payload.data(), payload.size(), &library), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_buffer_t*      cbor   = nullptr;
  cardano_plutus_data_clear_cbor_cache(library);
  ASSERT_EQ(cardano_plutus_data_to_cbor(library, writer), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &cbor), CARDANO_SUCCESS);

  /* The encoding must be the indefinite/chunked form (head byte 0x5f). */
  ASSERT_GT(cardano_buffer_get_size(cbor), 0U);
  EXPECT_EQ(cardano_buffer_get_data(cbor)[0], 0x5fU);

  cardano_uplc_arena_t* arena = make_arena();
  cardano_uplc_data_t*  node  = nullptr;
  ASSERT_EQ(
    cardano_uplc_data_from_cbor_bytes(arena, cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &node),
    CARDANO_SUCCESS);

  cardano_plutus_data_t* arena_as_library = nullptr;
  ASSERT_EQ(cardano_uplc_data_to_plutus_data(node, &arena_as_library), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_plutus_data_equals(arena_as_library, library));

  cardano_plutus_data_unref(&arena_as_library);
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&cbor);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&library);
}

TEST(cardano_uplc_data_direct_parser, rejectsMalformedCbor)
{
  cardano_uplc_arena_t* arena = make_arena();
  cardano_uplc_data_t*  node  = nullptr;

  const char* malformed[] = {
    "18",           /* truncated 1-byte argument */
    "1b00000000",   /* truncated 8-byte argument */
    "9f0102",       /* unterminated indefinite list */
    "bf0102",       /* unterminated indefinite map */
    "5f4401020304", /* unterminated indefinite byte string */
    "d81e80",       /* unknown tag 30 */
    "44010203",     /* byte string shorter than declared length */
    "e0",           /* simple value (unsupported major type) */
  };

  for (const char* hex: malformed)
  {
    std::vector<uint8_t> bytes = from_hex(hex);
    node                       = nullptr;
    EXPECT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_ERROR_DECODING) << hex;
  }

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_data_direct_parser, emptyByteStringRoundTrips)
{
  cardano_uplc_arena_t* arena = make_arena();
  std::vector<uint8_t>  bytes = from_hex("40");
  cardano_uplc_data_t*  node  = nullptr;

  ASSERT_EQ(cardano_uplc_data_from_cbor_bytes(arena, bytes.data(), bytes.size(), &node), CARDANO_SUCCESS);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CARDANO_UPLC_DATA_KIND_BYTES);
  EXPECT_EQ(node->as.bytes.size, 0U);
  EXPECT_EQ(arena_serialise_hex(node), "40");

  cardano_uplc_arena_free(&arena);
}

/* DEGENERATE DEPTH STACK SAFETY ********************************************/

namespace
{

const cardano_uplc_data_t*
build_constr_chain(cardano_uplc_arena_t* arena, size_t depth)
{
  cardano_uplc_data_t* node = nullptr;
  EXPECT_EQ(cardano_uplc_data_new_integer_small(arena, 0, &node), CARDANO_SUCCESS);

  for (size_t i = 0U; i < depth; ++i)
  {
    const cardano_uplc_data_t** field = static_cast<const cardano_uplc_data_t**>(
      cardano_uplc_arena_alloc(arena, sizeof(const cardano_uplc_data_t*), 0U));
    EXPECT_NE(field, nullptr);
    field[0] = node;

    cardano_uplc_data_t* parent = nullptr;
    EXPECT_EQ(cardano_uplc_data_new_constr(arena, 0U, field, 1U, &parent), CARDANO_SUCCESS);
    node = parent;
  }

  return node;
}

const cardano_uplc_data_t*
build_list_chain(cardano_uplc_arena_t* arena, size_t depth)
{
  cardano_uplc_data_t* node = nullptr;
  EXPECT_EQ(cardano_uplc_data_new_integer_small(arena, 0, &node), CARDANO_SUCCESS);

  for (size_t i = 0U; i < depth; ++i)
  {
    const cardano_uplc_data_t** item = static_cast<const cardano_uplc_data_t**>(
      cardano_uplc_arena_alloc(arena, sizeof(const cardano_uplc_data_t*), 0U));
    EXPECT_NE(item, nullptr);
    item[0] = node;

    cardano_uplc_data_t* parent = nullptr;
    EXPECT_EQ(cardano_uplc_data_new_list(arena, item, 1U, &parent), CARDANO_SUCCESS);
    node = parent;
  }

  return node;
}

} // namespace

TEST(cardano_uplc_data_deep, degenerateDepthDoesNotOverflowTheStack)
{
  const size_t kDepth = 1000000U;

  cardano_uplc_arena_t* arena = make_arena();

  const cardano_uplc_data_t* constr_chain = build_constr_chain(arena, kDepth);
  const cardano_uplc_data_t* list_chain   = build_list_chain(arena, kDepth);

  ASSERT_NE(constr_chain, nullptr);
  ASSERT_NE(list_chain, nullptr);

  EXPECT_TRUE(cardano_uplc_data_equals(constr_chain, constr_chain));
  EXPECT_TRUE(cardano_uplc_data_equals(list_chain, list_chain));
  EXPECT_FALSE(cardano_uplc_data_equals(constr_chain, list_chain));

  EXPECT_GT(cardano_uplc_data_node_ex_mem(constr_chain), 0);
  EXPECT_GT(cardano_uplc_data_node_count(constr_chain), 0);
  EXPECT_GT(cardano_uplc_data_node_ex_mem(list_chain), 0);
  EXPECT_GT(cardano_uplc_data_node_count(list_chain), 0);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_uplc_data_to_cbor(list_chain, writer), CARDANO_SUCCESS);
  cardano_cbor_writer_unref(&writer);

  cardano_uplc_arena_free(&arena);
}
