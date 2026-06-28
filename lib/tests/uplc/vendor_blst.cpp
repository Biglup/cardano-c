/**
 * \file vendor_blst.cpp
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

#include <blst.h>

#include <array>
#include <cstdint>
#include <gtest/gtest.h>

/* STATIC HELPERS ************************************************************/

namespace
{

/* The BLS12-381 G1 generator in compressed serialization (48 bytes). */
constexpr std::array<uint8_t, 48U> G1_GENERATOR_COMPRESSED = {
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
constexpr std::array<uint8_t, 96U> G2_GENERATOR_COMPRESSED = {
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

} // namespace

/* UNIT TESTS ****************************************************************/

TEST(vendor_blst, g1_generator_compresses_to_the_known_value)
{
  // Arrange
  std::array<uint8_t, 48U> out {};

  // Act
  blst_p1_compress(out.data(), blst_p1_generator());

  // Assert
  EXPECT_EQ(out, G1_GENERATOR_COMPRESSED);
}

TEST(vendor_blst, g2_generator_compresses_to_the_known_value)
{
  // Arrange
  std::array<uint8_t, 96U> out {};

  // Act
  blst_p2_compress(out.data(), blst_p2_generator());

  // Assert
  EXPECT_EQ(out, G2_GENERATOR_COMPRESSED);
}

TEST(vendor_blst, g1_compress_then_uncompress_round_trips_in_the_subgroup)
{
  // Arrange
  std::array<uint8_t, 48U> out {};
  blst_p1_affine           decoded;

  blst_p1_compress(out.data(), blst_p1_generator());

  // Act
  BLST_ERROR err = blst_p1_uncompress(&decoded, out.data());

  // Assert
  ASSERT_EQ(err, BLST_SUCCESS);
  EXPECT_TRUE(blst_p1_affine_in_g1(&decoded));
  EXPECT_TRUE(blst_p1_affine_is_equal(&decoded, blst_p1_affine_generator()));
}

TEST(vendor_blst, g1_scalar_multiply_by_two_equals_point_doubling)
{
  // Arrange
  blst_p1    doubled;
  blst_p1    scaled;
  const byte two[1] = { 0x02 };

  // Act
  blst_p1_double(&doubled, blst_p1_generator());
  blst_p1_mult(&scaled, blst_p1_generator(), two, 2U);

  // Assert
  EXPECT_TRUE(blst_p1_is_equal(&doubled, &scaled));
}

TEST(vendor_blst, pairing_of_the_generators_is_a_nontrivial_gt_element)
{
  // Arrange
  blst_fp12 result;

  // Act
  blst_miller_loop(&result, blst_p2_affine_generator(), blst_p1_affine_generator());
  blst_final_exp(&result, &result);

  // Assert
  EXPECT_FALSE(blst_fp12_is_one(&result));
  EXPECT_TRUE(blst_fp12_in_group(&result));
}

TEST(vendor_blst, pairing_is_bilinear_for_a_small_scalar)
{
  // Arrange: check e(2*P, Q) == e(P, 2*Q).
  blst_p1        p2;
  blst_p2        q2;
  blst_p1_affine p2_aff;
  blst_p2_affine q2_aff;
  blst_fp12      lhs;
  blst_fp12      rhs;
  const byte     two[1] = { 0x02 };

  blst_p1_mult(&p2, blst_p1_generator(), two, 2U);
  blst_p2_mult(&q2, blst_p2_generator(), two, 2U);

  blst_p1_to_affine(&p2_aff, &p2);
  blst_p2_to_affine(&q2_aff, &q2);

  // Act
  blst_miller_loop(&lhs, blst_p2_affine_generator(), &p2_aff);
  blst_final_exp(&lhs, &lhs);

  blst_miller_loop(&rhs, &q2_aff, blst_p1_affine_generator());
  blst_final_exp(&rhs, &rhs);

  // Assert
  EXPECT_TRUE(blst_fp12_is_equal(&lhs, &rhs));
}
