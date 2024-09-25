/**
 * \file pool_params.cpp
 *
 * \author angel.castillo
 * \date   jun 28, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license");
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 *     www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

/* INCLUDES ******************************************************************/

#include <cardano/error.h>

#include <cardano/pool_params/pool_params.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                    = "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* CBOR_WITH_NULL_METADATA = "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6df6";
static const char* OPERATOR_KEY_HASH       = "1cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92582088";
static const char* VRF_VK_HASH             = "dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db00";

/* UNIT TESTS ****************************************************************/

TEST(cardano_pool_params_new, canCreatePoolParams)
{
  // Arrange
  cardano_pool_params_t* pool_params  = nullptr;
  cardano_pool_params_t* pool_params2 = nullptr;
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t*   operator_key_hash = NULL;
  cardano_blake2b_hash_t*   vrf_vk_hash       = NULL;
  uint64_t                  pledge            = 10000;
  uint64_t                  cost              = 1000;
  cardano_unit_interval_t*  margin            = NULL;
  cardano_reward_address_t* reward_account    = NULL;
  cardano_pool_owners_t*    owners            = NULL;
  cardano_relays_t*         relays            = NULL;
  cardano_pool_metadata_t*  metadata          = NULL;

  ASSERT_EQ(cardano_pool_params_get_operator_key_hash(pool_params, &operator_key_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_vrf_vk_hash(pool_params, &vrf_vk_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_pledge(pool_params, &pledge), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_cost(pool_params, &cost), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_margin(pool_params, &margin), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_reward_account(pool_params, &reward_account), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_owners(pool_params, &owners), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_relays(pool_params, &relays), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_metadata(pool_params, &metadata), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, reward_account, owners, relays, metadata, &pool_params2), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_params2, testing::Not((cardano_pool_params_t*)nullptr));

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_pool_params_to_cbor(pool_params2, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_pool_params_unref(&pool_params2);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
  cardano_blake2b_hash_unref(&operator_key_hash);
  cardano_blake2b_hash_unref(&vrf_vk_hash);
  cardano_unit_interval_unref(&margin);
  cardano_reward_address_unref(&reward_account);
  cardano_pool_owners_unref(&owners);
  cardano_relays_unref(&relays);
  cardano_pool_metadata_unref(&metadata);
}

TEST(cardano_pool_params_new, returnsErrorWhenGivenNullPointer)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t*   operator_key_hash = NULL;
  cardano_blake2b_hash_t*   vrf_vk_hash       = NULL;
  uint64_t                  pledge            = 10000;
  uint64_t                  cost              = 1000;
  cardano_unit_interval_t*  margin            = NULL;
  cardano_reward_address_t* reward_account    = NULL;
  cardano_pool_owners_t*    owners            = NULL;
  cardano_relays_t*         relays            = NULL;
  cardano_pool_metadata_t*  metadata          = NULL;

  ASSERT_EQ(cardano_pool_params_get_operator_key_hash(pool_params, &operator_key_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_vrf_vk_hash(pool_params, &vrf_vk_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_pledge(pool_params, &pledge), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_cost(pool_params, &cost), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_margin(pool_params, &margin), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_reward_account(pool_params, &reward_account), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_owners(pool_params, &owners), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_relays(pool_params, &relays), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_metadata(pool_params, &metadata), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_pool_params_new(nullptr, vrf_vk_hash, pledge, cost, margin, reward_account, owners, relays, metadata, &pool_params), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, nullptr, pledge, cost, margin, reward_account, owners, relays, metadata, &pool_params), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, nullptr, reward_account, owners, relays, metadata, &pool_params), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, nullptr, owners, relays, metadata, &pool_params), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, reward_account, nullptr, relays, metadata, &pool_params), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, reward_account, owners, nullptr, metadata, &pool_params), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, reward_account, owners, relays, metadata, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&operator_key_hash);
  cardano_blake2b_hash_unref(&vrf_vk_hash);
  cardano_unit_interval_unref(&margin);
  cardano_reward_address_unref(&reward_account);
  cardano_pool_owners_unref(&owners);
  cardano_relays_unref(&relays);
  cardano_pool_metadata_unref(&metadata);
}

TEST(cardano_pool_params_new, returnsErrorIfAllocationFails)
{
  // Arrange
  cardano_pool_params_t* pool_params  = nullptr;
  cardano_pool_params_t* pool_params2 = nullptr;
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t*   operator_key_hash = NULL;
  cardano_blake2b_hash_t*   vrf_vk_hash       = NULL;
  uint64_t                  pledge            = 10000;
  uint64_t                  cost              = 1000;
  cardano_unit_interval_t*  margin            = NULL;
  cardano_reward_address_t* reward_account    = NULL;
  cardano_pool_owners_t*    owners            = NULL;
  cardano_relays_t*         relays            = NULL;
  cardano_pool_metadata_t*  metadata          = NULL;

  ASSERT_EQ(cardano_pool_params_get_operator_key_hash(pool_params, &operator_key_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_vrf_vk_hash(pool_params, &vrf_vk_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_pledge(pool_params, &pledge), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_cost(pool_params, &cost), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_margin(pool_params, &margin), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_reward_account(pool_params, &reward_account), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_owners(pool_params, &owners), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_relays(pool_params, &relays), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_metadata(pool_params, &metadata), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  ASSERT_EQ(cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, reward_account, owners, relays, metadata, &pool_params2), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_pool_params_unref(&pool_params2);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&operator_key_hash);
  cardano_blake2b_hash_unref(&vrf_vk_hash);
  cardano_unit_interval_unref(&margin);
  cardano_reward_address_unref(&reward_account);
  cardano_pool_owners_unref(&owners);
  cardano_relays_unref(&relays);
  cardano_pool_metadata_unref(&metadata);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_pool_params_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_pool_params_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_pool_params_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_to_cbor(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, canDecodePoolParamsWithNullMetadata)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR_WITH_NULL_METADATA, strlen(CBOR_WITH_NULL_METADATA));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_params, testing::Not((cardano_pool_params_t*)nullptr));

  cardano_blake2b_hash_t*   operator_key_hash = NULL;
  cardano_blake2b_hash_t*   vrf_vk_hash       = NULL;
  uint64_t                  pledge            = 10000;
  uint64_t                  cost              = 1000;
  cardano_unit_interval_t*  margin            = NULL;
  cardano_reward_address_t* reward_account    = NULL;
  cardano_pool_owners_t*    owners            = NULL;
  cardano_relays_t*         relays            = NULL;
  cardano_pool_metadata_t*  metadata          = NULL;

  ASSERT_EQ(cardano_pool_params_get_operator_key_hash(pool_params, &operator_key_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_vrf_vk_hash(pool_params, &vrf_vk_hash), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_pledge(pool_params, &pledge), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_cost(pool_params, &cost), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_margin(pool_params, &margin), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_reward_account(pool_params, &reward_account), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_owners(pool_params, &owners), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_relays(pool_params, &relays), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_pool_params_get_metadata(pool_params, &metadata), CARDANO_SUCCESS);

  EXPECT_EQ(metadata, nullptr);

  // compare CBOR

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_pool_params_to_cbor(pool_params, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITH_NULL_METADATA) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_NULL_METADATA);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&operator_key_hash);
  cardano_blake2b_hash_unref(&vrf_vk_hash);
  cardano_unit_interval_unref(&margin);
  cardano_reward_address_unref(&reward_account);
  cardano_pool_owners_unref(&owners);
  cardano_relays_unref(&relays);
  cardano_pool_metadata_unref(&metadata);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_params_from_cbor, returnErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(nullptr, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidHash)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("ef1cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidVfrKeyHash)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92ef208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidPledge)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0ef27101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidCost)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0192710ef03e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidUnitInterval)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8ef1e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidRewardAccount)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105ef1de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidOwners)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fef010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfInvalidRelays)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fef83011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_from_cbor, returnsErrorIfMetadata)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6def7368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("89581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_params_ref(pool_params);

  // Assert
  EXPECT_THAT(pool_params, testing::Not((cardano_pool_params_t*)nullptr));
  EXPECT_EQ(cardano_pool_params_refcount(pool_params), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pool_params_unref(&pool_params);
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_params_ref(nullptr);
}

TEST(cardano_pool_params_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  // Act
  cardano_pool_params_unref(&pool_params);
}

TEST(cardano_pool_params_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_params_unref((cardano_pool_params_t**)nullptr);
}

TEST(cardano_pool_params_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_params_ref(pool_params);
  size_t ref_count = cardano_pool_params_refcount(pool_params);

  cardano_pool_params_unref(&pool_params);
  size_t updated_ref_count = cardano_pool_params_refcount(pool_params);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_params_ref(pool_params);
  size_t ref_count = cardano_pool_params_refcount(pool_params);

  cardano_pool_params_unref(&pool_params);
  size_t updated_ref_count = cardano_pool_params_refcount(pool_params);

  cardano_pool_params_unref(&pool_params);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pool_params, (cardano_pool_params_t*)nullptr);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pool_params_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pool_params_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_pool_params_set_last_error(pool_params, message);

  // Assert
  EXPECT_STREQ(cardano_pool_params_get_last_error(pool_params), "Object is NULL.");
}

TEST(cardano_pool_params_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_pool_params_set_last_error(pool_params, message);

  // Assert
  EXPECT_STREQ(cardano_pool_params_get_last_error(pool_params), "");

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_operator_key_hash, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* operator_key_hash = nullptr;

  // Act
  cardano_error_t error = cardano_pool_params_get_operator_key_hash(nullptr, &operator_key_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_operator_key_hash, returnsErrorIfOperatorKeyHashIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_operator_key_hash(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_vrf_vk_hash, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* vrf_vk_hash = nullptr;

  // Act
  cardano_error_t error = cardano_pool_params_get_vrf_vk_hash(nullptr, &vrf_vk_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_vrf_vk_hash, returnsErrorIfVrfVkHashIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_vrf_vk_hash(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_operator_key_hash, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* operator_key_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(OPERATOR_KEY_HASH, strlen(OPERATOR_KEY_HASH), &operator_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_operator_key_hash(nullptr, operator_key_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&operator_key_hash);
}

TEST(cardano_pool_params_set_operator_key_hash, returnsErrorIfOperatorKeyHashIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* operator_key_hash = nullptr;

  // Act
  error = cardano_pool_params_set_operator_key_hash(pool_params, operator_key_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_operator_key_hash, canSetTheHash)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* operator_key_hash = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(OPERATOR_KEY_HASH, strlen(OPERATOR_KEY_HASH), &operator_key_hash), CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_operator_key_hash(pool_params, operator_key_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&operator_key_hash);
}

TEST(cardano_pool_params_set_vrf_vk_hash, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* vrf_vk_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_VK_HASH, strlen(VRF_VK_HASH), &vrf_vk_hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_vrf_vk_hash(nullptr, vrf_vk_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&vrf_vk_hash);
}

TEST(cardano_pool_params_set_vrf_vk_hash, returnsErrorIfVrfVkHashIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* vrf_vk_hash = nullptr;

  // Act
  error = cardano_pool_params_set_vrf_vk_hash(pool_params, vrf_vk_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_vrf_vk_hash, canSetTheHash)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* vrf_vk_hash = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_VK_HASH, strlen(VRF_VK_HASH), &vrf_vk_hash), CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_vrf_vk_hash(pool_params, vrf_vk_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&vrf_vk_hash);
}

TEST(cardano_pool_params_get_pledge, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  uint64_t pledge = 0;

  // Act
  cardano_error_t error = cardano_pool_params_get_pledge(nullptr, &pledge);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_pledge, returnsErrorIfPledgeIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_pledge(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_pledge, canGetThePledge)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t pledge = 0;

  // Act
  error = cardano_pool_params_get_pledge(pool_params, &pledge);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(pledge, 10000);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_pledge, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  uint64_t pledge = 500000000;

  // Act
  cardano_error_t error = cardano_pool_params_set_pledge(nullptr, pledge);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_set_pledge, canSetThePledge)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t pledge = 500000000;

  // Act
  error = cardano_pool_params_set_pledge(pool_params, pledge);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_cost, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  uint64_t cost = 0;

  // Act
  cardano_error_t error = cardano_pool_params_get_cost(nullptr, &cost);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_cost, returnsErrorIfCostIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_cost(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_cost, canGetTheCost)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t cost = 0;

  // Act
  error = cardano_pool_params_get_cost(pool_params, &cost);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cost, 1000);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_cost, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  uint64_t cost = 340000000;

  // Act
  cardano_error_t error = cardano_pool_params_set_cost(nullptr, cost);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_set_cost, canSetTheCost)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t cost = 340000000;

  // Act
  error = cardano_pool_params_set_cost(pool_params, cost);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_margin, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_unit_interval_t* margin = NULL;

  // Act
  cardano_error_t error = cardano_pool_params_get_margin(nullptr, &margin);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_margin, returnsErrorIfMarginIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* margin = NULL;

  // Act
  error = cardano_pool_params_get_margin(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_margin, canGetTheMargin)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* margin = NULL;

  // Act
  error = cardano_pool_params_get_margin(pool_params, &margin);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_get_numerator(margin), 1);
  EXPECT_EQ(cardano_unit_interval_get_denominator(margin), 5);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_unit_interval_unref(&margin);
}

TEST(cardano_pool_params_set_margin, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_unit_interval_t* margin = NULL;

  EXPECT_EQ(cardano_unit_interval_new(1, 5, &margin), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_margin(nullptr, margin);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&margin);
}

TEST(cardano_pool_params_set_margin, returnsErrorIfMarginIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* margin = NULL;

  // Act
  error = cardano_pool_params_set_margin(pool_params, margin);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_margin, canSetTheMargin)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_unit_interval_t* margin = NULL;

  EXPECT_EQ(cardano_unit_interval_new(1, 5, &margin), CARDANO_SUCCESS);

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_margin(pool_params, margin);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_unit_interval_unref(&margin);
}

TEST(cardano_pool_params_get_reward_account, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_account = NULL;

  // Act
  cardano_error_t error = cardano_pool_params_get_reward_account(nullptr, &reward_account);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_reward_account, returnsErrorIfRewardAccountIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_reward_address_t* reward_account = NULL;

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_reward_account(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_reward_account, canGetTheRewardAccount)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_reward_address_t* reward_account = NULL;

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_reward_account(pool_params, &reward_account);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(reward_account), "stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr");

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_reward_address_unref(&reward_account);
}

TEST(cardano_pool_params_set_reward_account, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_account = NULL;

  EXPECT_EQ(cardano_reward_address_from_bech32("stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr", strlen("stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr"), &reward_account), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_reward_account(nullptr, reward_account);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_account);
}

TEST(cardano_pool_params_set_reward_account, returnsErrorIfRewardAccountIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_reward_address_t* reward_account = NULL;

  EXPECT_EQ(cardano_reward_address_from_bech32("stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr", strlen("stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr"), &reward_account), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_reward_account(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_reward_address_unref(&reward_account);
}

TEST(cardano_pool_params_set_reward_account, canSetTheRewardAccount)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_reward_address_t* reward_account = NULL;

  EXPECT_EQ(cardano_reward_address_from_bech32("stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr", strlen("stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr"), &reward_account), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_reward_account(pool_params, reward_account);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_reward_address_unref(&reward_account);
}

TEST(cardano_pool_params_get_owners, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_pool_owners_t* owners = NULL;

  // Act
  cardano_error_t error = cardano_pool_params_get_owners(nullptr, &owners);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_owners, returnsErrorIfOwnersIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_pool_owners_t* owners = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_owners(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_owners, canGetTheOwners)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_pool_owners_t* owners = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_owners(pool_params, &owners);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_pool_owners_get_length(owners), 1);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_owners_unref(&owners);
}

TEST(cardano_pool_params_set_owners, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_pool_owners_t* owners = NULL;

  EXPECT_EQ(cardano_pool_owners_new(&owners), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_owners(nullptr, owners);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_owners_unref(&owners);
}

TEST(cardano_pool_params_set_owners, returnsErrorIfOwnersIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_pool_owners_t* owners = NULL;

  EXPECT_EQ(cardano_pool_owners_new(&owners), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_owners(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_owners_unref(&owners);
}

TEST(cardano_pool_params_set_owners, canSetTheOwners)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_pool_owners_t* owners = NULL;

  EXPECT_EQ(cardano_pool_owners_new(&owners), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_owners(pool_params, owners);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_owners_unref(&owners);
}

TEST(cardano_pool_params_get_relays, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_relays_t* relays = NULL;

  // Act
  cardano_error_t error = cardano_pool_params_get_relays(nullptr, &relays);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_relays, returnsErrorIfRelaysIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_relays_t* relays = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_relays(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_relays, canGetTheRelays)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_relays_t* relays = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_relays(pool_params, &relays);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_relays_get_length(relays), 3);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_relays_unref(&relays);
}

TEST(cardano_pool_params_set_relays, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_relays_t* relays = NULL;

  EXPECT_EQ(cardano_relays_new(&relays), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_relays(nullptr, relays);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_pool_params_set_relays, returnsErrorIfRelaysIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_relays_t* relays = NULL;

  EXPECT_EQ(cardano_relays_new(&relays), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_relays(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_relays_unref(&relays);
}

TEST(cardano_pool_params_set_relays, canSetTheRelays)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_relays_t* relays = NULL;

  EXPECT_EQ(cardano_relays_new(&relays), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_relays(pool_params, relays);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_relays_unref(&relays);
}

TEST(cardano_pool_params_get_metadata, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_pool_metadata_t* metadata = NULL;

  // Act
  cardano_error_t error = cardano_pool_params_get_metadata(nullptr, &metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_get_metadata, returnsErrorIfMetadataIsNull)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_metadata(pool_params, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_get_metadata, canGetTheMetadata)
{
  // Arrange
  cardano_pool_params_t* pool_params = nullptr;

  cardano_pool_metadata_t* metadata = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_get_metadata(pool_params, &metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_metadata_unref(&metadata);
}

TEST(cardano_pool_params_set_metadata, returnsErrorIfPoolParamsIsNull)
{
  // Arrange
  cardano_pool_metadata_t* metadata = NULL;
  cardano_cbor_reader_t*   reader   = cardano_cbor_reader_from_hex("827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  EXPECT_EQ(cardano_pool_metadata_from_cbor(reader, &metadata), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_params_set_metadata(nullptr, metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_metadata_unref(&metadata);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_metadata, canSetNullMetadata)
{
  // Arrange
  cardano_pool_params_t*   pool_params = nullptr;
  cardano_pool_metadata_t* metadata    = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_params_from_cbor(reader, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_metadata(pool_params, metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_params_set_metadata, canSetTheMetadata)
{
  // Arrange
  cardano_pool_params_t*   pool_params = nullptr;
  cardano_pool_metadata_t* metadata    = NULL;

  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex("827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));
  cardano_cbor_reader_t* reader2 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  EXPECT_EQ(cardano_pool_metadata_from_cbor(reader, &metadata), CARDANO_SUCCESS);

  cardano_error_t error = cardano_pool_params_from_cbor(reader2, &pool_params);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_params_set_metadata(pool_params, metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&pool_params);
  cardano_pool_metadata_unref(&metadata);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_reader_unref(&reader2);
}