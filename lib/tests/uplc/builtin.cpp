/**
 * \file builtin.cpp
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

#include "../../src/uplc/builtins/uplc_builtin.h"
#include <cardano/error.h>

#include <gmock/gmock.h>

/* STATIC HELPERS ************************************************************/

static bool
is_valid_arity(const size_t arity)
{
  return (arity == 1U) || (arity == 2U) || (arity == 3U) || (arity == 4U) || (arity == 6U);
}

static bool
is_valid_force_count(const size_t force_count)
{
  return (force_count == 0U) || (force_count == 1U) || (force_count == 2U);
}

static bool
is_valid_version(const cardano_uplc_lang_version_t version)
{
  return (version == CARDANO_UPLC_LANG_VERSION_V1) || (version == CARDANO_UPLC_LANG_VERSION_V2) ||
    (version == CARDANO_UPLC_LANG_VERSION_V3) || (version == CARDANO_UPLC_LANG_VERSION_V4);
}

/**
 * \brief One row of the independent expected-metadata table.
 *
 * This restates the builtin metadata so the test can disagree with uplc_builtin.c when
 * an entry there is omitted (and therefore default-fills to 0). The values are taken
 * from the Plutus builtin specification.
 */
struct expected_builtin_metadata
{
    cardano_uplc_builtin_t      tag;
    size_t                      arity;
    size_t                      force_count;
    cardano_uplc_lang_version_t first_version;
};

static const cardano_uplc_lang_version_t V1 = CARDANO_UPLC_LANG_VERSION_V1;
static const cardano_uplc_lang_version_t V2 = CARDANO_UPLC_LANG_VERSION_V2;
static const cardano_uplc_lang_version_t V3 = CARDANO_UPLC_LANG_VERSION_V3;
static const cardano_uplc_lang_version_t V4 = CARDANO_UPLC_LANG_VERSION_V4;

/**
 * \brief Independent expected arity, force count and first version for every tag.
 *
 * The array is indexed implicitly by position and the position must equal the tag,
 * which the test asserts. Any omission or typo in uplc_builtin.c (whose tables default
 * unset entries to 0, a valid force count and a valid version) disagrees with this
 * table and fails the EveryTagMatchesTheExpectedMetadata test.
 */
static const expected_builtin_metadata EXPECTED_METADATA[] = {
  { CARDANO_UPLC_BUILTIN_ADD_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_MOD_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_EQUALS_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING, 3U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_SHA2_256, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_SHA3_256, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_BLAKE2B_256, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE, 3U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_APPEND_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_EQUALS_STRING, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_ENCODE_UTF8, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_DECODE_UTF8, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_IF_THEN_ELSE, 3U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_CHOOSE_UNIT, 2U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_TRACE, 2U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_FST_PAIR, 1U, 2U, V1 },
  { CARDANO_UPLC_BUILTIN_SND_PAIR, 1U, 2U, V1 },
  { CARDANO_UPLC_BUILTIN_CHOOSE_LIST, 3U, 2U, V1 },
  { CARDANO_UPLC_BUILTIN_MK_CONS, 2U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_HEAD_LIST, 1U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_TAIL_LIST, 1U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_NULL_LIST, 1U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_CHOOSE_DATA, 6U, 1U, V1 },
  { CARDANO_UPLC_BUILTIN_CONSTR_DATA, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_MAP_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_LIST_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_I_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_B_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_UN_MAP_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_UN_LIST_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_UN_I_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_UN_B_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_EQUALS_DATA, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_MK_PAIR_DATA, 2U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_MK_NIL_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA, 1U, 0U, V1 },
  { CARDANO_UPLC_BUILTIN_SERIALISE_DATA, 1U, 0U, V2 },
  { CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE, 3U, 0U, V2 },
  { CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE, 3U, 0U, V2 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_KECCAK_256, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BLAKE2B_224, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING, 3U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_AND_BYTE_STRING, 3U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_OR_BYTE_STRING, 3U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING, 3U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_READ_BIT, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_WRITE_BITS, 3U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_REPLICATE_BYTE, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING, 2U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_COUNT_SET_BITS, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_RIPEMD_160, 1U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER, 3U, 0U, V3 },
  { CARDANO_UPLC_BUILTIN_DROP_LIST, 2U, 1U, V4 },
  { CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY, 1U, 1U, V4 },
  { CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY, 1U, 1U, V4 },
  { CARDANO_UPLC_BUILTIN_INDEX_ARRAY, 2U, 1U, V4 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL, 2U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL, 2U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_INSERT_COIN, 4U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_LOOKUP_COIN, 3U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_UNION_VALUE, 2U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_VALUE_CONTAINS, 2U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_VALUE_DATA, 1U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_UN_VALUE_DATA, 1U, 0U, V4 },
  { CARDANO_UPLC_BUILTIN_SCALE_VALUE, 2U, 0U, V4 }
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_builtin, everyTagMatchesTheExpectedMetadata)
{
  // Arrange
  const size_t expected_count = sizeof(EXPECTED_METADATA) / sizeof(EXPECTED_METADATA[0]);

  // Act / Assert
  EXPECT_EQ(expected_count, (size_t)CARDANO_UPLC_BUILTIN_COUNT)
    << "expected metadata table does not cover every tag";

  for (size_t tag = 0U; tag < expected_count; ++tag)
  {
    const expected_builtin_metadata& row = EXPECTED_METADATA[tag];

    EXPECT_EQ((size_t)row.tag, tag) << "expected metadata row out of order at tag " << tag;

    size_t          arity = 0U;
    cardano_error_t error = cardano_uplc_builtin_arity(row.tag, &arity);
    EXPECT_EQ(error, CARDANO_SUCCESS) << "missing arity entry for tag " << tag;
    EXPECT_EQ(arity, row.arity) << "wrong arity for tag " << tag;

    size_t force_count = 0U;
    error              = cardano_uplc_builtin_force_count(row.tag, &force_count);
    EXPECT_EQ(error, CARDANO_SUCCESS) << "missing force-count entry for tag " << tag;
    EXPECT_EQ(force_count, row.force_count) << "wrong force count for tag " << tag;

    cardano_uplc_lang_version_t version = CARDANO_UPLC_LANG_VERSION_V1;
    error                               = cardano_uplc_builtin_first_version(row.tag, &version);
    EXPECT_EQ(error, CARDANO_SUCCESS) << "missing version entry for tag " << tag;
    EXPECT_EQ(version, row.first_version) << "wrong first version for tag " << tag;
  }
}

TEST(cardano_uplc_builtin_arity, everyTagHasAPopulatedInRangeArity)
{
  // Act / Assert
  for (size_t tag = 0U; tag < (size_t)CARDANO_UPLC_BUILTIN_COUNT; ++tag)
  {
    size_t          arity = 0U;
    cardano_error_t error = cardano_uplc_builtin_arity((cardano_uplc_builtin_t)tag, &arity);

    EXPECT_EQ(error, CARDANO_SUCCESS) << "missing arity entry for tag " << tag;
    EXPECT_TRUE(is_valid_arity(arity)) << "out-of-range arity for tag " << tag;
  }
}

TEST(cardano_uplc_builtin_force_count, everyTagHasAPopulatedInRangeForceCount)
{
  // Act / Assert
  for (size_t tag = 0U; tag < (size_t)CARDANO_UPLC_BUILTIN_COUNT; ++tag)
  {
    size_t          force_count = 0U;
    cardano_error_t error       = cardano_uplc_builtin_force_count((cardano_uplc_builtin_t)tag, &force_count);

    EXPECT_EQ(error, CARDANO_SUCCESS) << "missing force-count entry for tag " << tag;
    EXPECT_TRUE(is_valid_force_count(force_count)) << "out-of-range force count for tag " << tag;
  }
}

TEST(cardano_uplc_builtin_first_version, everyTagHasAPopulatedInRangeVersion)
{
  // Act / Assert
  for (size_t tag = 0U; tag < (size_t)CARDANO_UPLC_BUILTIN_COUNT; ++tag)
  {
    cardano_uplc_lang_version_t version = CARDANO_UPLC_LANG_VERSION_V1;
    cardano_error_t             error   = cardano_uplc_builtin_first_version((cardano_uplc_builtin_t)tag, &version);

    EXPECT_EQ(error, CARDANO_SUCCESS) << "missing version entry for tag " << tag;
    EXPECT_TRUE(is_valid_version(version)) << "out-of-range version for tag " << tag;
  }
}

TEST(cardano_uplc_builtin_arity, spotChecksMatchTheReferences)
{
  // Arrange
  size_t arity = 0U;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_ADD_INTEGER, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 2U);

  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_IF_THEN_ELSE, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 3U);

  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_CHOOSE_DATA, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 6U);

  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_SHA2_256, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 1U);

  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_FST_PAIR, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 1U);

  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 3U);

  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_INSERT_COIN, &arity), CARDANO_SUCCESS);
  EXPECT_EQ(arity, 4U);
}

TEST(cardano_uplc_builtin_force_count, spotChecksMatchTheReferences)
{
  // Arrange
  size_t force_count = 0U;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_force_count(CARDANO_UPLC_BUILTIN_ADD_INTEGER, &force_count), CARDANO_SUCCESS);
  EXPECT_EQ(force_count, 0U);

  EXPECT_EQ(cardano_uplc_builtin_force_count(CARDANO_UPLC_BUILTIN_IF_THEN_ELSE, &force_count), CARDANO_SUCCESS);
  EXPECT_EQ(force_count, 1U);

  EXPECT_EQ(cardano_uplc_builtin_force_count(CARDANO_UPLC_BUILTIN_FST_PAIR, &force_count), CARDANO_SUCCESS);
  EXPECT_EQ(force_count, 2U);

  EXPECT_EQ(cardano_uplc_builtin_force_count(CARDANO_UPLC_BUILTIN_CHOOSE_LIST, &force_count), CARDANO_SUCCESS);
  EXPECT_EQ(force_count, 2U);

  EXPECT_EQ(cardano_uplc_builtin_force_count(CARDANO_UPLC_BUILTIN_CHOOSE_DATA, &force_count), CARDANO_SUCCESS);
  EXPECT_EQ(force_count, 1U);
}

TEST(cardano_uplc_builtin_first_version, spotChecksMatchTheReferences)
{
  // Arrange
  cardano_uplc_lang_version_t version = CARDANO_UPLC_LANG_VERSION_V1;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_ADD_INTEGER, &version), CARDANO_SUCCESS);
  EXPECT_EQ(version, CARDANO_UPLC_LANG_VERSION_V1);

  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_SERIALISE_DATA, &version), CARDANO_SUCCESS);
  EXPECT_EQ(version, CARDANO_UPLC_LANG_VERSION_V2);

  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE, &version), CARDANO_SUCCESS);
  EXPECT_EQ(version, CARDANO_UPLC_LANG_VERSION_V2);

  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD, &version), CARDANO_SUCCESS);
  EXPECT_EQ(version, CARDANO_UPLC_LANG_VERSION_V3);

  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER, &version), CARDANO_SUCCESS);
  EXPECT_EQ(version, CARDANO_UPLC_LANG_VERSION_V3);

  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_SCALE_VALUE, &version), CARDANO_SUCCESS);
  EXPECT_EQ(version, CARDANO_UPLC_LANG_VERSION_V4);
}

TEST(cardano_uplc_builtin_t, tagValuesMatchTheFlatEncoding)
{
  // Act / Assert
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_ADD_INTEGER, 0);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_SHA2_256, 18);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE, 21);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_SERIALISE_DATA, 51);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE, 52);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE, 53);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_KECCAK_256, 71);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_BLAKE2B_224, 72);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER, 87);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_SCALE_VALUE, 100);
  EXPECT_EQ((int)CARDANO_UPLC_BUILTIN_COUNT, 101);
}

TEST(cardano_uplc_builtin_arity, returnsErrorOnNullOutput)
{
  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_arity(CARDANO_UPLC_BUILTIN_ADD_INTEGER, NULL), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_builtin_arity, returnsErrorOnInvalidTag)
{
  // Arrange
  size_t arity = 0U;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_arity((cardano_uplc_builtin_t)CARDANO_UPLC_BUILTIN_COUNT, &arity), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_uplc_builtin_arity((cardano_uplc_builtin_t)-1, &arity), CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_uplc_builtin_force_count, returnsErrorOnNullOutput)
{
  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_force_count(CARDANO_UPLC_BUILTIN_ADD_INTEGER, NULL), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_builtin_force_count, returnsErrorOnInvalidTag)
{
  // Arrange
  size_t force_count = 0U;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_force_count((cardano_uplc_builtin_t)CARDANO_UPLC_BUILTIN_COUNT, &force_count), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_uplc_builtin_force_count((cardano_uplc_builtin_t)-1, &force_count), CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_uplc_builtin_first_version, returnsErrorOnNullOutput)
{
  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_first_version(CARDANO_UPLC_BUILTIN_ADD_INTEGER, NULL), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_builtin_first_version, returnsErrorOnInvalidTag)
{
  // Arrange
  cardano_uplc_lang_version_t version = CARDANO_UPLC_LANG_VERSION_V1;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_builtin_first_version((cardano_uplc_builtin_t)CARDANO_UPLC_BUILTIN_COUNT, &version), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_uplc_builtin_first_version((cardano_uplc_builtin_t)-1, &version), CARDANO_ERROR_INVALID_ARGUMENT);
}
