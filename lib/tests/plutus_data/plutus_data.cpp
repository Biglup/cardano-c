/**
 * \file plutus_data.cpp
 *
 * \author angel.castillo
 * \date   May 14, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include <cardano/error.h>

#include <cardano/buffer.h>
#include <cardano/plutus_data/plutus_data.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PLUTUS_DATA_CBOR   = "9f01029f0102030405ff9f0102030405ff05ff";
static const char* PLUTUS_DATA_CBOR_2 = "9fc25f584037d34fac60a7dd2edba0c76fa58862c91c45ff4298e9134ba8e76be9a7513d88865bfdb9315073dc2690b0f2b59a232fbfa0a8a504df6ee9bb78e3f33fbdfef95529c9e74ff30ffe1bd1cc5795c37535899dba800000ffc25f58408d4820519e9bba2d6556c87b100709082f4c8958769899eb5d288b6f9ea9e0723df7211959860edea5829c9732422d25962e3945c68a6089f50a18b0114248b7555feea4851e9f099180600000000000000000000000ffc25f58408d4820519e9bba2d6556c87b100709082f4c8958769899eb5d288b6f9ea9e0723df7211959860edea5829c9732422d25962e3945c68a6089f50a18b0114248b7555feea4851e9f099180600000000000000000000000ffc25f584039878c5f4d4063e9a2ee75a3fbdd1492c3cad46f4ecbae977ac94b709a730e367edf9dae05acd59638d1dec25e2351c2eecb871694afae979de7085b522efe1355634138bbd920200d574cdf400324cdd1aafe10a240ffc25f584022a6282a7d960570c4c729decd677ec617061f0e501249c41f8724c89dc97dc0d24917bdb7a7ebd7c079c1c56fa21af0f119168966356ea384fb711cb766015e55bfc5bc86583f6a82ae605a93e7bf974ae74cd051c0ffc25f58404445ab8649611ee8f74a3c31e504a2f25f2f7631ef6ef828a405542904d84c997304b1b332d528ee54873b03cfb73cd3c5b35b91184f6846afccec7271bda8a05563ba46aed8c82611da47fd608d027447f8391161c0ffc25f58400258b535c4d4a22a483b22b2f5c5c65bed9e7de59266f6bbaa8997edf5bec6bb5d203641bb58d8ade1a3a5b4e5f923df502cf1e47691865fe1984eacef3be96a551ed585e070265db203a8866726bed053cb6c8aa200ffc25f5840021104310667ec434e9e2cd9fa71853593c42e1b55865ac49f80b2ea22beeec9b4a55e9545055a2bcde3a78d36836df11df0f91c1dae9a8aee58419b8650bc6c529361f9601a4005051b045d05f39a5f00ebd5ffff";
static const char* BIG_NUMBERS[]      = {
  "1093929156918367016766069563027239416446778893307251997971794948729105062347369330146869223033199554831433128491376164494134119896793625745623928731109781036903510617119765359815723399113165600284443934720",
  "2768491094397106413284351268798781278061973163918667373508176781108678876832888565950388553255499815619207549146245084281150783450096035638439655721496227482399093555200000000000000000000000000000000000000",
  "2768491094397106413284351268798781278061973163918667373508176781108678876832888565950388553255499815619207549146245084281150783450096035638439655721496227482399093555200000000000000000000000000000000000000",
  "1127320948699467529606464548687160198167487105208190997153720362564942186550892230582242980573812448057150419530802096156402677128058112319272573039196273296535693983366369964092325725072645646768416006720",
  "678966618629088994577385052394593905048788216453653741455475012343328029630393478083358655655534689789017294468365725065895808744013442165812351180871208842081615673249725577503335455257844242272891195840",
  "1337829155615373710780861189358723839738261900670472008493768766460943065914931970040774692071540815257661221428415268570880739215388841910028989315213224986535176632464067341466233795236134699058357952960",
  "45981213582240091300385870382262347274104141060516509284758089043905194449918733499912740694341485053723341097850038365519925374324306213051881991025304309829953615052414155047559800693983587151987253760",
  "2413605787847473064058493109882761763812632923885676112901376523745345875592342323079462001682936368998782686824629943810471167748859099323567551094056876663897197968204837564889906128763937156053"
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorWhenMemoryAllocationFailes)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_constr, canCreateAConstrPlutusData)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_constr, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_constr(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_constr, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  cardano_error_t error = cardano_plutus_data_new_constr(constr_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_new_constr, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_map, canCreateNewMap)
{
  // Arrange
  cardano_plutus_data_t* plutus_data     = nullptr;
  cardano_plutus_map_t*  map_plutus_data = nullptr;

  EXPECT_EQ(cardano_plutus_map_new(&map_plutus_data), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_data_new_map(map_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_map_unref(&map_plutus_data);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_map, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_map(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_map, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_plutus_map_t* map_plutus_data = nullptr;
  EXPECT_EQ(cardano_plutus_map_new(&map_plutus_data), CARDANO_SUCCESS);

  cardano_error_t error = cardano_plutus_data_new_map(map_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_plutus_map_unref(&map_plutus_data);
}

TEST(cardano_plutus_data_new_map, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_plutus_data_t* plutus_data     = nullptr;
  cardano_plutus_map_t*  map_plutus_data = nullptr;
  EXPECT_EQ(cardano_plutus_map_new(&map_plutus_data), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_plutus_data_new_map(map_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_plutus_map_unref(&map_plutus_data);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_list, canCreateANewList)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_data_new_list(list, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_list, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_list(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_list, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_plutus_list_t* list = nullptr;
  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  cardano_error_t error = cardano_plutus_data_new_list(list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_new_list, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;
  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_plutus_data_new_list(list, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_plutus_list_unref(&list);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_int, canCreateAnIntegerPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorIfPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes, canCreateABytesPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const uint8_t          bytes[]     = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  const uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfBytesIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const uint8_t          bytes[]     = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(nullptr, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  const uint8_t bytes[] = { 0x85, 0x01, 0x02, 0x03, 0x04, 0x05 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfMemoryEventuallyAllocationFails)
{
  // Arrange
  const uint8_t bytes[] = { 0x85, 0x01, 0x02, 0x03, 0x04, 0x05 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes_from_hex, canCreateABytesPlutusDataFromHex)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const char*            hex         = "850102030405";

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  const char* hex = "850102030405";

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfHexIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(nullptr, 0, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  const char* hex = "850102030405";

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfMemoryAllocationEventuallyFails)
{
  // Arrange
  const char* hex = "850102030405";

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeAnIntegerPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, canDecodeNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("24", strlen("24"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), -5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, canDecodeBigPositiveInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 72057594037927936);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryInt)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("24", strlen("24"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryInt2)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("24", strlen("24"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfInvalidBigPositiveInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c2490001000000000000", strlen("c2490001000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigPositiveInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigPositiveInteger2)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDecodeBigNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), -72057594037927936);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfInvalidBigNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c3490001000000000000", strlen("c3490001000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigNegativeInteger2)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeABytesPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("450102030405", strlen("450102030405"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_buffer_t* buffer = NULL;
  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_SUCCESS);

  const uint8_t expected_bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  EXPECT_EQ(cardano_buffer_get_size(buffer), sizeof(expected_bytes));

  const byte_t* bytes = cardano_buffer_get_data(buffer);
  for (size_t i = 0; i < sizeof(expected_bytes); ++i)
  {
    EXPECT_EQ(bytes[i], expected_bytes[i]);
  }

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryBytes)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("450102030405", strlen("450102030405"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeAListPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("9f0102030405ff", strlen("9f0102030405ff"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_plutus_list_t* list = NULL;

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&plutus_data);

  const size_t length = cardano_plutus_list_get_length(list);

  EXPECT_EQ(length, 5);

  cardano_plutus_data_t* elem1 = NULL;
  cardano_plutus_data_t* elem2 = NULL;
  cardano_plutus_data_t* elem3 = NULL;
  cardano_plutus_data_t* elem4 = NULL;
  cardano_plutus_data_t* elem5 = NULL;

  EXPECT_EQ(cardano_plutus_list_get(list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 4, &elem5), CARDANO_SUCCESS);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem3, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 3);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem4, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 4);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem5, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_list_unref(&list);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_plutus_data_unref(&elem3);
  cardano_plutus_data_unref(&elem4);
  cardano_plutus_data_unref(&elem5);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeAMapPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("a3010402050306", strlen("a3010402050306"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_plutus_map_t* map = NULL;

  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&plutus_data);

  const size_t length = cardano_plutus_map_get_length(map);

  EXPECT_EQ(length, 3);

  cardano_plutus_list_t* keys = NULL;
  cardano_plutus_data_t* key1 = NULL;
  cardano_plutus_data_t* key2 = NULL;
  cardano_plutus_data_t* key3 = NULL;

  cardano_plutus_data_t* value1 = NULL;
  cardano_plutus_data_t* value2 = NULL;
  cardano_plutus_data_t* value3 = NULL;

  EXPECT_EQ(cardano_plutus_map_get_keys(map, &keys), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_list_get(keys, 0, &key1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(keys, 1, &key2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(keys, 2, &key3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_get(map, key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get(map, key2, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get(map, key3, &value3), CARDANO_SUCCESS);

  cardano_bigint_t* key_value = NULL;
  cardano_bigint_t* value     = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(key1, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 1);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_plutus_data_to_integer(key2, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 2);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_plutus_data_to_integer(key3, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 3);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_plutus_data_to_integer(value1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 4);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(value2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 5);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(value3, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 6);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_map_unref(&map);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_list_unref(&keys);
  cardano_plutus_data_unref(&key1);
  cardano_plutus_data_unref(&key2);
  cardano_plutus_data_unref(&key3);
  cardano_plutus_data_unref(&value1);
  cardano_plutus_data_unref(&value2);
  cardano_plutus_data_unref(&value3);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryMap)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("a3010402050306", strlen("a3010402050306"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDecodeConstructorPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("d8799f0102ff", strlen("d8799f0102ff"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_constr_plutus_data_t* constr_plutus_data = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&plutus_data);

  uint64_t               alternative = 0;
  cardano_plutus_list_t* list        = NULL;

  EXPECT_EQ(cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative), CARDANO_SUCCESS);
  EXPECT_EQ(alternative, 0);

  EXPECT_EQ(cardano_constr_plutus_data_get_data(constr_plutus_data, &list), CARDANO_SUCCESS);

  const size_t length = cardano_plutus_list_get_length(list);

  EXPECT_EQ(length, 2);

  cardano_plutus_data_t* elem1 = NULL;
  cardano_plutus_data_t* elem2 = NULL;

  EXPECT_EQ(cardano_plutus_list_get(list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 1, &elem2), CARDANO_SUCCESS);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsConstrData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("d8799f0102ff", strlen("d8799f0102ff"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_to_cbor, canEncodeConstPlutusDataToCbor)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;
  cardano_plutus_data_t*        elem1              = nullptr;
  cardano_plutus_data_t*        elem2              = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &elem2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(list, elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(list, elem2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("d8799f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d8799f0102ff");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, canEncodeMapToCbor)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_map_t*  map         = nullptr;
  cardano_plutus_data_t* key1        = nullptr;
  cardano_plutus_data_t* key2        = nullptr;
  cardano_plutus_data_t* key3        = nullptr;
  cardano_plutus_data_t* value1      = nullptr;
  cardano_plutus_data_t* value2      = nullptr;
  cardano_plutus_data_t* value3      = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &key1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &key2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(3, &key3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(4, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(5, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(6, &value3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_new(&map), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(map, key1, value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_insert(map, key2, value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_insert(map, key3, value3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_map(map, &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("a3010402050306") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a3010402050306");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_map_unref(&map);
  cardano_plutus_data_unref(&key1);
  cardano_plutus_data_unref(&key2);
  cardano_plutus_data_unref(&key3);
  cardano_plutus_data_unref(&value1);
  cardano_plutus_data_unref(&value2);
  cardano_plutus_data_unref(&value3);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, canEncodeSmallByteArray)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  const uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  EXPECT_EQ(cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("450102030405") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "450102030405");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, canEncodeBigByteArray)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  /* clang-format off */
  const uint8_t bytes[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0xaa, 0xaa
  };
  /* clang-format on */

  EXPECT_EQ(cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 539);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "5f58400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070858400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070858400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070858400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070842aaaaff");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_data_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_data_to_cbor(plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("9f0102ff", strlen("9f0102ff"));
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_plutus_data_clear_cbor_cache(plutus_data);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_to_cbor(plutus_data, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("9f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "9f0102ff");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfPlutusListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_DATA_CBOR, strlen(PLUTUS_DATA_CBOR));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(PLUTUS_DATA_CBOR, strlen(PLUTUS_DATA_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_ref(plutus_data);

  // Assert
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));
  EXPECT_EQ(cardano_plutus_data_refcount(plutus_data), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_ref(nullptr);
}

TEST(cardano_plutus_data_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_unref((cardano_plutus_data_t**)nullptr);
}

TEST(cardano_plutus_data_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_ref(plutus_data);
  size_t ref_count = cardano_plutus_data_refcount(plutus_data);

  cardano_plutus_data_unref(&plutus_data);
  size_t updated_ref_count = cardano_plutus_data_refcount(plutus_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_ref(plutus_data);
  size_t ref_count = cardano_plutus_data_refcount(plutus_data);

  cardano_plutus_data_unref(&plutus_data);
  size_t updated_ref_count = cardano_plutus_data_refcount(plutus_data);

  cardano_plutus_data_unref(&plutus_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_data_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_data_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_plutus_data_set_last_error(plutus_data, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_data_get_last_error(plutus_data), "Object is NULL.");
}

TEST(cardano_plutus_data_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_data_set_last_error(plutus_data, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_data_get_last_error(plutus_data), "");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_get_kind, returnsTheKindOfPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  EXPECT_EQ(cardano_plutus_data_get_kind(plutus_data, &kind), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_get_kind, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  EXPECT_EQ(cardano_plutus_data_get_kind(plutus_data, &kind), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_get_kind, returnsErrorIfKindIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;
  EXPECT_EQ(cardano_plutus_data_get_kind(plutus_data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_CONSTR);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_integer, returnsTheIntegerValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_bigint_to_int(value), 1);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_bigint_unref(&value);
}

TEST(cardano_plutus_data_to_integer, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_integer, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_integer, returnsErrorIfPlutusDataIsNotAnInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_bytes, returnsTheBytesValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), 4);
  EXPECT_EQ(memcmp(cardano_buffer_get_data(buffer), "test", 4), 0);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_data_to_bytes, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_bytes, returnsErrorIfPlutusDataIsNotAByteArray)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_constr, returnsTheConstrValue)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data), CARDANO_SUCCESS);
  cardano_constr_plutus_data_unref(&constr_plutus_data);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_to_constr, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_constr_plutus_data_t* constr_plutus_data = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_plutus_data_to_constr, returnsErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_constr, returnsErrorIfPlutusDataIsNotAConstr)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_constr_plutus_data_t* constr_plutus_data = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_map, returnsTheMapValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_map_t*  map         = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map, &plutus_data);
  cardano_plutus_map_unref(&map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(map, testing::Not((cardano_plutus_map_t*)nullptr));

  // Cleanup
  cardano_plutus_map_unref(&map);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_map, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_map_t* map = NULL;

  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&map);
}

TEST(cardano_plutus_data_to_map, returnsErrorIfMapIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_map_t*  map         = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_map_unref(&map);
}

TEST(cardano_plutus_data_to_map, returnsErrorIfPlutusDataIsNotAMap)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_map_t* map = NULL;

  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_list, returnsTheListValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list, &plutus_data);
  cardano_plutus_list_unref(&list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_plutus_list_t*)nullptr));

  // Cleanup
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_list, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_list_t* list = NULL;

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_to_list, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_to_list, returnsErrorIfPlutusDataIsNotAList)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_list_t* list = NULL;

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothPlutusDataAreEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_integer_from_int(1, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsFalseIfPlutusDataAreDifferent)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_integer_from_int(2, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsFalseIfPlutusDataAreDifferentTypes)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsFalseIfPlutusDataAreNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);
}

TEST(cardano_plutus_data_equals, returnsFalseIfOnePlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
}

TEST(cardano_plutus_data_equals, returnsFalseIfBothPlutusDataAreNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothAreConstrPlutusDataAndEqual)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data1        = nullptr;
  cardano_plutus_data_t*        plutus_data2        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data1 = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data2 = nullptr;
  cardano_plutus_list_t*        list1               = nullptr;
  cardano_plutus_list_t*        list2               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_new(&list2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_constr_plutus_data_new(0, list1, &constr_plutus_data1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list2, &constr_plutus_data2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data1, &plutus_data1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data2, &plutus_data2), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data1);
  cardano_constr_plutus_data_unref(&constr_plutus_data2);
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
  cardano_plutus_list_unref(&list1);
  cardano_plutus_list_unref(&list2);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothArePlutusMapAndEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;
  cardano_plutus_map_t*  map1         = nullptr;
  cardano_plutus_map_t*  map2         = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&map1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_map_new(&map2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map2, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_map_unref(&map1);
  cardano_plutus_map_unref(&map2);
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothArePlutusListAndEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;
  cardano_plutus_list_t* list1        = nullptr;
  cardano_plutus_list_t* list2        = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&list2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list2, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_list_unref(&list1);
  cardano_plutus_list_unref(&list2);
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothAreBytesAndEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_new_integer, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer(nullptr, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer, returnsErrorIfIntegerIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer((const cardano_bigint_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bigint_t* integer = NULL;
  EXPECT_EQ(cardano_bigint_from_int(1, &integer), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer(integer, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_uint, returnsErrorIfPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_uint, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_uint, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_uint, canReturnUint)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* integer = nullptr;
  error                     = cardano_plutus_data_to_integer(data, &integer);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(integer), 0);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("0", 1, 10, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfStringIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string(nullptr, 0, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfEmptyString)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("", 0, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfInvalidString)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("a", 1, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_CONVERSION_FAILED);

  // Cleanup
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsPlutusDataWithCorrectNumber)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("123", 3, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* integer = nullptr;
  error                     = cardano_plutus_data_to_integer(data, &integer);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(integer), 123);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMemoryAllocationFailsWhileReadingUint)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", strlen("00"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_from_cbor(reader, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_plutus_data_unref(&data);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMemoryAllocationFailsWhileReadingUint2)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", strlen("00"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_from_cbor(reader, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_plutus_data_unref(&data);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_to_cbor, canSerializeMaxUint64AsUnsignedInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_uint(UINT64_MAX, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "1bffffffffffffffff");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeSmallUint64AsUnsignedInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_uint(1U, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "01");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeMinInt64AsInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_int(INT64_MIN, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "3b7fffffffffffffff");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeSmallIntAsInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_int(-1, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "20");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeBigInteger)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_string("340199290171201906221318119490500689920", strlen("340199290171201906221318119490500689920"), 10, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "c250fff00000000000000000000000000000");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_clear_cbor_cache, doesNothingIfGivenNull)
{
  // Act
  cardano_plutus_data_clear_cbor_cache(nullptr);
}

TEST(cardano_plutus_data_from_cbor, canReadListOfBigNums)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(PLUTUS_DATA_CBOR_2, strlen(PLUTUS_DATA_CBOR_2));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_list_t* list = nullptr;

  error = cardano_plutus_data_to_list(plutus_data, &list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t list_size = cardano_plutus_list_get_length(list);

  for (size_t i = 0; i < list_size; i++)
  {
    cardano_plutus_data_t* element = nullptr;
    error                          = cardano_plutus_list_get(list, i, &element);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    cardano_bigint_t* integer = nullptr;
    error                     = cardano_plutus_data_to_integer(element, &integer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t string_size = cardano_bigint_get_string_size(integer, 10);

    char* string = (char*)malloc(string_size);
    error        = cardano_bigint_to_string(integer, string, string_size, 10);

    EXPECT_EQ(error, CARDANO_SUCCESS);
    EXPECT_STREQ(string, BIG_NUMBERS[i]);

    cardano_plutus_data_unref(&element);
    cardano_bigint_unref(&integer);
    free(string);
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_plutus_data_clear_cbor_cache(plutus_data);

  error = cardano_plutus_data_to_cbor(plutus_data, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, PLUTUS_DATA_CBOR_2);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_writer_unref(&writer);
  free(cbor_hex);
}