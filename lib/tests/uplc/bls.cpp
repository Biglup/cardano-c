/**
 * \file bls.cpp
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

#include "../../src/uplc/ast/uplc_term.h"
#include <cardano/error.h>
#include <cardano/typedefs.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/builtins/bls.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <array>
#include <blst.h>
#include <cstdint>
#include <cstring>
#include <gmock/gmock.h>

/* STATIC HELPERS ************************************************************/

namespace
{

/* The BLS12-381 G1 generator in compressed serialization (48 bytes). */
constexpr std::array<uint8_t, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE> G1_GENERATOR_COMPRESSED = {
  0x97,
  0xf1,
  0xd3,
  0xa7,
  0x31,
  0x97,
  0xd7,
  0x94,
  0x26,
  0x95,
  0x63,
  0x8c,
  0x4f,
  0xa9,
  0xac,
  0x0f,
  0xc3,
  0x68,
  0x8c,
  0x4f,
  0x97,
  0x74,
  0xb9,
  0x05,
  0xa1,
  0x4e,
  0x3a,
  0x3f,
  0x17,
  0x1b,
  0xac,
  0x58,
  0x6c,
  0x55,
  0xe8,
  0x3f,
  0xf9,
  0x7a,
  0x1a,
  0xef,
  0xfb,
  0x3a,
  0xf0,
  0x0a,
  0xdb,
  0x22,
  0xc6,
  0xbb
};

/* The BLS12-381 G2 generator in compressed serialization (96 bytes). */
constexpr std::array<uint8_t, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE> G2_GENERATOR_COMPRESSED = {
  0x93,
  0xe0,
  0x2b,
  0x60,
  0x52,
  0x71,
  0x9f,
  0x60,
  0x7d,
  0xac,
  0xd3,
  0xa0,
  0x88,
  0x27,
  0x4f,
  0x65,
  0x59,
  0x6b,
  0xd0,
  0xd0,
  0x99,
  0x20,
  0xb6,
  0x1a,
  0xb5,
  0xda,
  0x61,
  0xbb,
  0xdc,
  0x7f,
  0x50,
  0x49,
  0x33,
  0x4c,
  0xf1,
  0x12,
  0x13,
  0x94,
  0x5d,
  0x57,
  0xe5,
  0xac,
  0x7d,
  0x05,
  0x5d,
  0x04,
  0x2b,
  0x7e,
  0x02,
  0x4a,
  0xa2,
  0xb2,
  0xf0,
  0x8f,
  0x0a,
  0x91,
  0x26,
  0x08,
  0x05,
  0x27,
  0x2d,
  0xc5,
  0x10,
  0x51,
  0xc6,
  0xe4,
  0x7a,
  0xd4,
  0xfa,
  0x40,
  0x3b,
  0x02,
  0xb4,
  0x51,
  0x0b,
  0x64,
  0x7a,
  0xe3,
  0xd1,
  0x77,
  0x0b,
  0xac,
  0x03,
  0x26,
  0xa8,
  0x05,
  0xbb,
  0xef,
  0xd4,
  0x80,
  0x56,
  0xc8,
  0xc1,
  0x21,
  0xbd,
  0xb8
};

cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);
  return arena;
}

/* An arena whose tiny block size forces every allocation onto its own block,
 * so the blob allocation that follows the constant-node allocation is served
 * by a fresh backing malloc (and can therefore be made to fail on the Nth
 * call). */
cardano_uplc_arena_t*
new_arena_one_object_per_block()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(8U, &arena), CARDANO_SUCCESS);
  return arena;
}

} // namespace

/* UNIT TESTS - cardano_uplc_constant_new_bls *******************************/

TEST(cardano_uplc_constant_new_bls, failsOnNullArgs)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  blst_p1                  point;
  cardano_uplc_constant_t* constant = nullptr;

  blst_p1_from_affine(&point, blst_p1_affine_generator());

  // Act / Assert
  EXPECT_EQ(cardano_uplc_constant_new_bls(nullptr, CARDANO_UPLC_TYPE_BLS_G1, &point, sizeof(point), &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, nullptr, sizeof(point), &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &point, sizeof(point), nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bls, rejectsNonBlsKind)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  blst_p1                  point;
  cardano_uplc_constant_t* constant = nullptr;

  blst_p1_from_affine(&point, blst_p1_affine_generator());

  // Act / Assert
  EXPECT_EQ(cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_INTEGER, &point, sizeof(point), &constant), CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bls, rejectsZeroSize)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  blst_p1                  point;
  cardano_uplc_constant_t* constant = nullptr;

  blst_p1_from_affine(&point, blst_p1_affine_generator());

  // Act / Assert
  EXPECT_EQ(cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &point, 0U, &constant), CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bls, buildsAG1ConstantFromAPoint)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  blst_p1                  point;
  cardano_uplc_constant_t* constant = nullptr;

  blst_p1_from_affine(&point, blst_p1_affine_generator());

  // Act
  cardano_error_t error = cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &point, sizeof(point), &constant);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BLS_G1);
  EXPECT_NE(constant->as.bls, nullptr);

  /* The blob is an independent copy of the source bytes. */
  EXPECT_NE(constant->as.bls, static_cast<const void*>(&point));
  EXPECT_EQ(std::memcmp(constant->as.bls, &point, sizeof(point)), 0);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bls, failsWhenConstantNodeAllocationFails)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  blst_p1                  point;
  cardano_uplc_constant_t* constant = nullptr;

  blst_p1_from_affine(&point, blst_p1_affine_generator());

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &point, sizeof(point), &constant);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(constant, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bls, failsWhenBlobAllocationFails)
{
  // Arrange: a tiny-block arena forces the constant node and the blob onto
  // separate backing blocks, so the blob allocation is the second malloc.
  cardano_uplc_arena_t*    arena = new_arena_one_object_per_block();
  blst_p1                  point;
  cardano_uplc_constant_t* constant = nullptr;

  blst_p1_from_affine(&point, blst_p1_affine_generator());

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &point, sizeof(point), &constant);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(constant, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

/* UNIT TESTS - cardano_uplc_int_bls_g1_from_compressed *********************/

TEST(cardano_uplc_int_bls_g1_from_compressed, failsOnNullArena)
{
  // Arrange
  cardano_uplc_constant_t* constant = nullptr;

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g1_from_compressed(nullptr, G1_GENERATOR_COMPRESSED.data(), G1_GENERATOR_COMPRESSED.size(), &constant),
    CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_int_bls_g1_from_compressed, failsOnNullConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), G1_GENERATOR_COMPRESSED.size(), nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g1_from_compressed, rejectsWrongLength)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act / Assert: 47 and 49 bytes are both rejected.
  EXPECT_EQ(
    cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE - 1U, &constant),
    CARDANO_ERROR_DECODING);
  EXPECT_EQ(
    cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE + 1U, &constant),
    CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g1_from_compressed, rejectsBadEncoding)
{
  // Arrange: 48 bytes that are not a valid compressed point.
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE> garbage {};
  garbage.fill(0xFFU);

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g1_from_compressed(arena, garbage.data(), garbage.size(), &constant),
    CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g1_from_compressed, decodesTheGenerator)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), G1_GENERATOR_COMPRESSED.size(), &constant);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BLS_G1);
  EXPECT_NE(constant->as.bls, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

/* UNIT TESTS - cardano_uplc_int_bls_g2_from_compressed *********************/

TEST(cardano_uplc_int_bls_g2_from_compressed, failsOnNullArena)
{
  // Arrange
  cardano_uplc_constant_t* constant = nullptr;

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g2_from_compressed(nullptr, G2_GENERATOR_COMPRESSED.data(), G2_GENERATOR_COMPRESSED.size(), &constant),
    CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_int_bls_g2_from_compressed, failsOnNullConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), G2_GENERATOR_COMPRESSED.size(), nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g2_from_compressed, rejectsWrongLength)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE - 1U, &constant),
    CARDANO_ERROR_DECODING);
  EXPECT_EQ(
    cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE + 1U, &constant),
    CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g2_from_compressed, rejectsBadEncoding)
{
  // Arrange
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE> garbage {};
  garbage.fill(0xFFU);

  // Act / Assert
  EXPECT_EQ(
    cardano_uplc_int_bls_g2_from_compressed(arena, garbage.data(), garbage.size(), &constant),
    CARDANO_ERROR_DECODING);
  EXPECT_EQ(constant, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g2_from_compressed, decodesTheGenerator)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), G2_GENERATOR_COMPRESSED.size(), &constant);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(constant, nullptr);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BLS_G2);
  EXPECT_NE(constant->as.bls, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

/* UNIT TESTS - cardano_uplc_int_bls_g1_compress ***************************/

TEST(cardano_uplc_int_bls_g1_compress, failsOnNullArgs)
{
  // Arrange
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE> out {};

  ASSERT_EQ(cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), G1_GENERATOR_COMPRESSED.size(), &constant), CARDANO_SUCCESS);

  // Act / Assert
  EXPECT_EQ(cardano_uplc_int_bls_g1_compress(nullptr, out.data()), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_bls_g1_compress(constant, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g1_compress, rejectsAWrongKindConstant)
{
  // Arrange: a G2 constant passed to the G1 compressor.
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE> out {};

  ASSERT_EQ(cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), G2_GENERATOR_COMPRESSED.size(), &constant), CARDANO_SUCCESS);

  // Act / Assert
  EXPECT_EQ(cardano_uplc_int_bls_g1_compress(constant, out.data()), CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g1_compress, roundTripsTheGenerator)
{
  // Arrange
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE> out {};

  ASSERT_EQ(cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), G1_GENERATOR_COMPRESSED.size(), &constant), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_int_bls_g1_compress(constant, out.data());

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(out, G1_GENERATOR_COMPRESSED);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

/* UNIT TESTS - cardano_uplc_int_bls_g2_compress ***************************/

TEST(cardano_uplc_int_bls_g2_compress, failsOnNullArgs)
{
  // Arrange
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE> out {};

  ASSERT_EQ(cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), G2_GENERATOR_COMPRESSED.size(), &constant), CARDANO_SUCCESS);

  // Act / Assert
  EXPECT_EQ(cardano_uplc_int_bls_g2_compress(nullptr, out.data()), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_bls_g2_compress(constant, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g2_compress, rejectsAWrongKindConstant)
{
  // Arrange: a G1 constant passed to the G2 compressor.
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE> out {};

  ASSERT_EQ(cardano_uplc_int_bls_g1_from_compressed(arena, G1_GENERATOR_COMPRESSED.data(), G1_GENERATOR_COMPRESSED.size(), &constant), CARDANO_SUCCESS);

  // Act / Assert
  EXPECT_EQ(cardano_uplc_int_bls_g2_compress(constant, out.data()), CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_int_bls_g2_compress, roundTripsTheGenerator)
{
  // Arrange
  cardano_uplc_arena_t*                                    arena    = new_arena();
  cardano_uplc_constant_t*                                 constant = nullptr;
  std::array<uint8_t, CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE> out {};

  ASSERT_EQ(cardano_uplc_int_bls_g2_from_compressed(arena, G2_GENERATOR_COMPRESSED.data(), G2_GENERATOR_COMPRESSED.size(), &constant), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_int_bls_g2_compress(constant, out.data());

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(out, G2_GENERATOR_COMPRESSED);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}
