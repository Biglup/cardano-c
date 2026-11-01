/**
 * \file deep_clone.cpp
 *
 * \author angel.castillo
 * \date   Nov 01, 2026
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

#include "../src/allocators.h"
#include "address/cip19_test_vectors.h"
#include <cardano/cardano.h>
#include <cardano/voting_procedures/voting_procedure_list.h>

#include <cstdlib>
#include <cstring>
#include <gmock/gmock.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

/* STATIC VARIABLES **********************************************************/

static std::set<void*> clone_allocations;
static size_t          clone_allocation_count = 0U;
static size_t          clone_failure_index    = SIZE_MAX;
static bool            clone_freed_source     = false;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Tracks every native clone allocation and optionally fails exactly one call.
 */
static void*
clone_malloc(size_t size)
{
  if (clone_allocation_count++ == clone_failure_index)
  {
    return nullptr;
  }

  void* pointer = malloc(size);

  if (pointer != nullptr)
  {
    clone_allocations.insert(pointer);
  }

  return pointer;
}

/**
 * Detects attempts to free source storage and records clone cleanup.
 */
static void
clone_free(void* pointer)
{
  if (pointer == nullptr)
  {
    return;
  }

  if (clone_allocations.erase(pointer) != 1U)
  {
    clone_freed_source = true;
    return;
  }

  free(pointer);
}

/**
 * Tracks reallocations without losing the old allocation when realloc fails.
 */
static void*
clone_realloc(void* pointer, size_t size)
{
  if (clone_allocation_count++ == clone_failure_index)
  {
    return nullptr;
  }

  if ((pointer != nullptr) && (clone_allocations.count(pointer) == 0U))
  {
    clone_freed_source = true;
    return nullptr;
  }

  // Reinsert the old allocation if realloc fails; do not inspect a freed pointer.
  if (pointer != nullptr)
  {
    clone_allocations.erase(pointer);
  }

  void* replacement = realloc(pointer, size);

  if (replacement != nullptr)
  {
    clone_allocations.insert(replacement);
  }
  else if ((pointer != nullptr) && (size != 0U))
  {
    clone_allocations.insert(pointer);
  }

  return replacement;
}

/* TEST SUPPORT **************************************************************/

class clone_allocator_scope
{
  public:

    explicit clone_allocator_scope(size_t failure_index = SIZE_MAX)
    {
      EXPECT_TRUE(clone_allocations.empty());
      clone_allocation_count = 0U;
      clone_failure_index    = failure_index;
      clone_freed_source     = false;
      cardano_set_allocators(clone_malloc, clone_realloc, clone_free);
    }

    ~clone_allocator_scope()
    {
      cardano_set_allocators(malloc, realloc, free);
      EXPECT_FALSE(clone_freed_source);
      EXPECT_TRUE(clone_allocations.empty());
    }

    clone_allocator_scope(const clone_allocator_scope&)            = delete;
    clone_allocator_scope& operator=(const clone_allocator_scope&) = delete;
};

/**
 * Keeps native references scoped even when a fatal GoogleTest assertion returns.
 */
template<typename T>
static auto
own_clone_test_object(T* object, void (*release)(T**))
{
  const auto deleter = [release](T* value)
  {
    release(&value);
  };

  return std::unique_ptr<T, decltype(deleter)>(object, deleter);
}

/**
 * Serializes a test object without changing its native encoding cache.
 */
template<typename T>
static std::string
clone_test_cbor(T* object, cardano_error_t (*encode)(const T*, cardano_cbor_writer_t*), cardano_error_t expected_result = CARDANO_SUCCESS)
{
  auto writer = own_clone_test_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
  EXPECT_NE(writer, nullptr);
  EXPECT_EQ(encode(object, writer.get()), expected_result);

  std::vector<char> hex(cardano_cbor_writer_get_hex_size(writer.get()));
  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer.get(), hex.data(), hex.size()), CARDANO_SUCCESS);

  return hex.data();
}

/**
 * Exercises every native allocation failure and checks that no partial copy escapes.
 */
template<typename T>
static void
verify_clone_allocation_failures(T* source, cardano_error_t (*clone)(const T*, T**), void (*release)(T**))
{
  EXPECT_EQ(clone(source, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  size_t allocation_count = 0U;

  {
    clone_allocator_scope scope;
    T*                    copy   = nullptr;
    const cardano_error_t result = clone(source, &copy);
    allocation_count             = clone_allocation_count;

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_NE(copy, nullptr);
    EXPECT_NE(copy, source);

    release(&copy);
  }

  for (size_t index = 0U; index < allocation_count; ++index)
  {
    SCOPED_TRACE(index);
    clone_allocator_scope scope(index);
    T*                    copy   = nullptr;
    const cardano_error_t result = clone(source, &copy);

    EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    EXPECT_EQ(copy, nullptr);

    release(&copy);
  }
}

/**
 * Checks original encodings, independent lifetimes, and allocation rollback.
 */
template<typename T>
static void
verify_cbor_deep_clone(
  const std::vector<const char*>& fixtures,
  cardano_error_t (*decode)(cardano_cbor_reader_t*, T**),
  cardano_error_t (*encode)(const T*, cardano_cbor_writer_t*),
  cardano_error_t (*clone)(const T*, T**),
  void (*release)(T**),
  void (*clear_cache)(T*) = nullptr)
{
  for (size_t index = 0U; index < fixtures.size(); ++index)
  {
    SCOPED_TRACE(index);
    const char*           cbor          = fixtures[index];
    auto                  reader        = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
    T*                    raw_source    = nullptr;
    const cardano_error_t decode_result = decode(reader.get(), &raw_source);
    auto                  source        = own_clone_test_object(raw_source, release);

    ASSERT_EQ(decode_result, CARDANO_SUCCESS);
    ASSERT_NE(source, nullptr);

    const std::string before = clone_test_cbor(source.get(), encode);

    verify_clone_allocation_failures(source.get(), clone, release);
    EXPECT_EQ(clone_test_cbor(source.get(), encode), before);

    T*                    raw_copy     = nullptr;
    const cardano_error_t clone_result = clone(source.get(), &raw_copy);
    auto                  copy         = own_clone_test_object(raw_copy, release);

    ASSERT_EQ(clone_result, CARDANO_SUCCESS);
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(source.get(), copy.get());
    EXPECT_EQ(clone_test_cbor(copy.get(), encode), before);

    std::string     expected_after_release = before;
    cardano_error_t expected_result        = CARDANO_SUCCESS;

    if (clear_cache != nullptr)
    {
      clear_cache(source.get());
      clear_cache(copy.get());
      // Unknown redeemer variants can retain CBOR that the encoder cannot rebuild.
      // Clearing caches must preserve the same result, including that error.
      auto writer = own_clone_test_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
      ASSERT_NE(writer, nullptr);
      expected_result        = encode(source.get(), writer.get());
      expected_after_release = clone_test_cbor(source.get(), encode, expected_result);
      EXPECT_EQ(clone_test_cbor(copy.get(), encode, expected_result), expected_after_release);
    }

    source.reset();
    reader.reset();

    EXPECT_EQ(clone_test_cbor(copy.get(), encode, expected_result), expected_after_release);
  }
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_account_balance_interval_deep_clone, rejectsNullArguments)
{
  cardano_account_balance_interval_t* copy = nullptr;

  EXPECT_EQ(cardano_account_balance_interval_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_account_balance_interval_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_deep_clone, rejectsNullArguments)
{
  cardano_account_balance_intervals_map_t* copy = nullptr;

  EXPECT_EQ(cardano_account_balance_intervals_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_account_balance_intervals_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_address_deep_clone, rejectsNullArguments)
{
  cardano_address_t* copy = nullptr;

  EXPECT_EQ(cardano_address_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_address_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_deep_clone, rejectsNullArguments)
{
  cardano_anchor_t* copy = nullptr;

  EXPECT_EQ(cardano_anchor_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_anchor_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_id_deep_clone, rejectsNullArguments)
{
  cardano_asset_id_t* copy = nullptr;

  EXPECT_EQ(cardano_asset_id_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_asset_id_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_id_list_deep_clone, rejectsNullArguments)
{
  cardano_asset_id_list_t* copy = nullptr;

  EXPECT_EQ(cardano_asset_id_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_asset_id_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_id_map_deep_clone, rejectsNullArguments)
{
  cardano_asset_id_map_t* copy = nullptr;

  EXPECT_EQ(cardano_asset_id_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_asset_id_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_deep_clone, rejectsNullArguments)
{
  cardano_asset_name_t* copy = nullptr;

  EXPECT_EQ(cardano_asset_name_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_asset_name_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_list_deep_clone, rejectsNullArguments)
{
  cardano_asset_name_list_t* copy = nullptr;

  EXPECT_EQ(cardano_asset_name_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_asset_name_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_deep_clone, rejectsNullArguments)
{
  cardano_asset_name_map_t* copy = nullptr;

  EXPECT_EQ(cardano_asset_name_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_asset_name_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_auth_committee_hot_cert_deep_clone, rejectsNullArguments)
{
  cardano_auth_committee_hot_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_auth_committee_hot_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_auth_committee_hot_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_deep_clone, rejectsNullArguments)
{
  cardano_auxiliary_data_t* copy = nullptr;

  EXPECT_EQ(cardano_auxiliary_data_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_auxiliary_data_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_base_address_deep_clone, rejectsNullArguments)
{
  cardano_base_address_t* copy = nullptr;

  EXPECT_EQ(cardano_base_address_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_base_address_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_deep_clone, rejectsNullArguments)
{
  cardano_bigint_t* copy = nullptr;

  EXPECT_EQ(cardano_bigint_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_bigint_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_deep_clone, rejectsNullArguments)
{
  cardano_bip32_private_key_t* copy = nullptr;

  EXPECT_EQ(cardano_bip32_private_key_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_bip32_private_key_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_deep_clone, rejectsNullArguments)
{
  cardano_bip32_public_key_t* copy = nullptr;

  EXPECT_EQ(cardano_bip32_public_key_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_bip32_public_key_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_deep_clone, rejectsNullArguments)
{
  cardano_blake2b_hash_t* copy = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_blake2b_hash_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_set_deep_clone, rejectsNullArguments)
{
  cardano_blake2b_hash_set_t* copy = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_blake2b_hash_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bls_key_deep_clone, rejectsNullArguments)
{
  cardano_bls_key_t* copy = nullptr;

  EXPECT_EQ(cardano_bls_key_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_bls_key_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_deep_clone, rejectsNullArguments)
{
  cardano_bootstrap_witness_t* copy = nullptr;

  EXPECT_EQ(cardano_bootstrap_witness_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_bootstrap_witness_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_set_deep_clone, rejectsNullArguments)
{
  cardano_bootstrap_witness_set_t* copy = nullptr;

  EXPECT_EQ(cardano_bootstrap_witness_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_bootstrap_witness_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_buffer_deep_clone, rejectsNullArguments)
{
  cardano_buffer_t* copy = nullptr;

  EXPECT_EQ(cardano_buffer_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_buffer_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_byron_address_deep_clone, rejectsNullArguments)
{
  cardano_byron_address_t* copy = nullptr;

  EXPECT_EQ(cardano_byron_address_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_byron_address_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_cbor_reader_deep_clone, rejectsNullArguments)
{
  cardano_cbor_reader_t* copy = nullptr;

  EXPECT_EQ(cardano_cbor_reader_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_cbor_reader_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_deep_clone, rejectsNullArguments)
{
  cardano_cbor_writer_t* copy = nullptr;

  EXPECT_EQ(cardano_cbor_writer_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_cbor_writer_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_deep_clone, rejectsNullArguments)
{
  cardano_certificate_t* copy = nullptr;

  EXPECT_EQ(cardano_certificate_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_certificate_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_set_deep_clone, rejectsNullArguments)
{
  cardano_certificate_set_t* copy = nullptr;

  EXPECT_EQ(cardano_certificate_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_certificate_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_deep_clone, rejectsNullArguments)
{
  cardano_committee_t* copy = nullptr;

  EXPECT_EQ(cardano_committee_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_committee_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_deep_clone, rejectsNullArguments)
{
  cardano_committee_members_map_t* copy = nullptr;

  EXPECT_EQ(cardano_committee_members_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_committee_members_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_constitution_deep_clone, rejectsNullArguments)
{
  cardano_constitution_t* copy = nullptr;

  EXPECT_EQ(cardano_constitution_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_constitution_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_constr_plutus_data_deep_clone, rejectsNullArguments)
{
  cardano_constr_plutus_data_t* copy = nullptr;

  EXPECT_EQ(cardano_constr_plutus_data_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_constr_plutus_data_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_cost_model_deep_clone, rejectsNullArguments)
{
  cardano_cost_model_t* copy = nullptr;

  EXPECT_EQ(cardano_cost_model_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_cost_model_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_costmdls_deep_clone, rejectsNullArguments)
{
  cardano_costmdls_t* copy = nullptr;

  EXPECT_EQ(cardano_costmdls_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_costmdls_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_credential_deep_clone, rejectsNullArguments)
{
  cardano_credential_t* copy = nullptr;

  EXPECT_EQ(cardano_credential_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_credential_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_credential_set_deep_clone, rejectsNullArguments)
{
  cardano_credential_set_t* copy = nullptr;

  EXPECT_EQ(cardano_credential_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_credential_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_datum_deep_clone, rejectsNullArguments)
{
  cardano_datum_t* copy = nullptr;

  EXPECT_EQ(cardano_datum_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_datum_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_deep_clone, rejectsNullArguments)
{
  cardano_direct_deposit_map_t* copy = nullptr;

  EXPECT_EQ(cardano_direct_deposit_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_direct_deposit_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_deep_clone, rejectsNullArguments)
{
  cardano_drep_t* copy = nullptr;

  EXPECT_EQ(cardano_drep_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_drep_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_deep_clone, rejectsNullArguments)
{
  cardano_drep_voting_thresholds_t* copy = nullptr;

  EXPECT_EQ(cardano_drep_voting_thresholds_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_drep_voting_thresholds_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ed25519_private_key_deep_clone, rejectsNullArguments)
{
  cardano_ed25519_private_key_t* copy = nullptr;

  EXPECT_EQ(cardano_ed25519_private_key_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ed25519_private_key_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ed25519_public_key_deep_clone, rejectsNullArguments)
{
  cardano_ed25519_public_key_t* copy = nullptr;

  EXPECT_EQ(cardano_ed25519_public_key_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ed25519_public_key_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ed25519_signature_deep_clone, rejectsNullArguments)
{
  cardano_ed25519_signature_t* copy = nullptr;

  EXPECT_EQ(cardano_ed25519_signature_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ed25519_signature_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_enterprise_address_deep_clone, rejectsNullArguments)
{
  cardano_enterprise_address_t* copy = nullptr;

  EXPECT_EQ(cardano_enterprise_address_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_enterprise_address_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_deep_clone, rejectsNullArguments)
{
  cardano_ex_unit_prices_t* copy = nullptr;

  EXPECT_EQ(cardano_ex_unit_prices_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ex_unit_prices_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ex_units_deep_clone, rejectsNullArguments)
{
  cardano_ex_units_t* copy = nullptr;

  EXPECT_EQ(cardano_ex_units_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ex_units_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_genesis_key_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_genesis_key_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_genesis_key_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_genesis_key_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_deep_clone, rejectsNullArguments)
{
  cardano_governance_action_id_t* copy = nullptr;

  EXPECT_EQ(cardano_governance_action_id_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_governance_action_id_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_list_deep_clone, rejectsNullArguments)
{
  cardano_governance_action_id_list_t* copy = nullptr;

  EXPECT_EQ(cardano_governance_action_id_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_governance_action_id_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_guard_set_deep_clone, rejectsNullArguments)
{
  cardano_guard_set_t* copy = nullptr;

  EXPECT_EQ(cardano_guard_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_guard_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_hard_fork_initiation_action_deep_clone, rejectsNullArguments)
{
  cardano_hard_fork_initiation_action_t* copy = nullptr;

  EXPECT_EQ(cardano_hard_fork_initiation_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_hard_fork_initiation_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_info_action_deep_clone, rejectsNullArguments)
{
  cardano_info_action_t* copy = nullptr;

  EXPECT_EQ(cardano_info_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_info_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ipv4_deep_clone, rejectsNullArguments)
{
  cardano_ipv4_t* copy = nullptr;

  EXPECT_EQ(cardano_ipv4_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ipv4_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ipv6_deep_clone, rejectsNullArguments)
{
  cardano_ipv6_t* copy = nullptr;

  EXPECT_EQ(cardano_ipv6_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_ipv6_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_json_object_deep_clone, rejectsNullArguments)
{
  cardano_json_object_t* copy = nullptr;

  EXPECT_EQ(cardano_json_object_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_json_object_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_json_writer_deep_clone, rejectsNullArguments)
{
  cardano_json_writer_t* copy = nullptr;

  EXPECT_EQ(cardano_json_writer_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_json_writer_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_deep_clone, rejectsNullArguments)
{
  cardano_metadatum_t* copy = nullptr;

  EXPECT_EQ(cardano_metadatum_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_metadatum_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_label_list_deep_clone, rejectsNullArguments)
{
  cardano_metadatum_label_list_t* copy = nullptr;

  EXPECT_EQ(cardano_metadatum_label_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_metadatum_label_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_list_deep_clone, rejectsNullArguments)
{
  cardano_metadatum_list_t* copy = nullptr;

  EXPECT_EQ(cardano_metadatum_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_metadatum_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_deep_clone, rejectsNullArguments)
{
  cardano_metadatum_map_t* copy = nullptr;

  EXPECT_EQ(cardano_metadatum_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_metadatum_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_deep_clone, rejectsNullArguments)
{
  cardano_mir_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_mir_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_mir_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_deep_clone, rejectsNullArguments)
{
  cardano_mir_to_pot_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_mir_to_pot_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_mir_to_pot_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_deep_clone, rejectsNullArguments)
{
  cardano_mir_to_stake_creds_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_mir_to_stake_creds_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_mir_to_stake_creds_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_deep_clone, rejectsNullArguments)
{
  cardano_multi_asset_t* copy = nullptr;

  EXPECT_EQ(cardano_multi_asset_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_multi_asset_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_multi_host_name_relay_deep_clone, rejectsNullArguments)
{
  cardano_multi_host_name_relay_t* copy = nullptr;

  EXPECT_EQ(cardano_multi_host_name_relay_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_multi_host_name_relay_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_deep_clone, rejectsNullArguments)
{
  cardano_native_script_t* copy = nullptr;

  EXPECT_EQ(cardano_native_script_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_native_script_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_deep_clone, rejectsNullArguments)
{
  cardano_native_script_list_t* copy = nullptr;

  EXPECT_EQ(cardano_native_script_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_native_script_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_set_deep_clone, rejectsNullArguments)
{
  cardano_native_script_set_t* copy = nullptr;

  EXPECT_EQ(cardano_native_script_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_native_script_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_new_constitution_action_deep_clone, rejectsNullArguments)
{
  cardano_new_constitution_action_t* copy = nullptr;

  EXPECT_EQ(cardano_new_constitution_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_new_constitution_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_no_confidence_action_deep_clone, rejectsNullArguments)
{
  cardano_no_confidence_action_t* copy = nullptr;

  EXPECT_EQ(cardano_no_confidence_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_no_confidence_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_parameter_change_action_deep_clone, rejectsNullArguments)
{
  cardano_parameter_change_action_t* copy = nullptr;

  EXPECT_EQ(cardano_parameter_change_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_parameter_change_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_deep_clone, rejectsNullArguments)
{
  cardano_plutus_data_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_data_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_data_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_set_deep_clone, rejectsNullArguments)
{
  cardano_plutus_data_set_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_data_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_data_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_list_deep_clone, rejectsNullArguments)
{
  cardano_plutus_list_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_map_deep_clone, rejectsNullArguments)
{
  cardano_plutus_map_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v1_script_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v1_script_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v1_script_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v1_script_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v1_script_list_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v1_script_list_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v1_script_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v1_script_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v1_script_set_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v1_script_set_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v1_script_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v1_script_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v2_script_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v2_script_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v2_script_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_list_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v2_script_list_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v2_script_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v2_script_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_set_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v2_script_set_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v2_script_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v2_script_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v3_script_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v3_script_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v3_script_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v3_script_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v3_script_list_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v3_script_list_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v3_script_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v3_script_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v3_script_set_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v3_script_set_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v3_script_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v3_script_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v4_script_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v4_script_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v4_script_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_list_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v4_script_list_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v4_script_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v4_script_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_set_deep_clone, rejectsNullArguments)
{
  cardano_plutus_v4_script_set_t* copy = nullptr;

  EXPECT_EQ(cardano_plutus_v4_script_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_plutus_v4_script_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pointer_address_deep_clone, rejectsNullArguments)
{
  cardano_pointer_address_t* copy = nullptr;

  EXPECT_EQ(cardano_pointer_address_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pointer_address_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_policy_id_list_deep_clone, rejectsNullArguments)
{
  cardano_policy_id_list_t* copy = nullptr;

  EXPECT_EQ(cardano_policy_id_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_policy_id_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_metadata_deep_clone, rejectsNullArguments)
{
  cardano_pool_metadata_t* copy = nullptr;

  EXPECT_EQ(cardano_pool_metadata_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pool_metadata_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_owners_deep_clone, rejectsNullArguments)
{
  cardano_pool_owners_t* copy = nullptr;

  EXPECT_EQ(cardano_pool_owners_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pool_owners_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_params_deep_clone, rejectsNullArguments)
{
  cardano_pool_params_t* copy = nullptr;

  EXPECT_EQ(cardano_pool_params_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pool_params_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_registration_cert_deep_clone, rejectsNullArguments)
{
  cardano_pool_registration_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_pool_registration_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pool_registration_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_retirement_cert_deep_clone, rejectsNullArguments)
{
  cardano_pool_retirement_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_pool_retirement_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pool_retirement_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_deep_clone, rejectsNullArguments)
{
  cardano_pool_voting_thresholds_t* copy = nullptr;

  EXPECT_EQ(cardano_pool_voting_thresholds_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_pool_voting_thresholds_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_deep_clone, rejectsNullArguments)
{
  cardano_proposal_procedure_t* copy = nullptr;

  EXPECT_EQ(cardano_proposal_procedure_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_proposal_procedure_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_set_deep_clone, rejectsNullArguments)
{
  cardano_proposal_procedure_set_t* copy = nullptr;

  EXPECT_EQ(cardano_proposal_procedure_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_proposal_procedure_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_deep_clone, rejectsNullArguments)
{
  cardano_proposed_param_updates_t* copy = nullptr;

  EXPECT_EQ(cardano_proposed_param_updates_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_proposed_param_updates_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_deep_clone, rejectsNullArguments)
{
  cardano_protocol_param_update_t* copy = nullptr;

  EXPECT_EQ(cardano_protocol_param_update_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_protocol_param_update_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_deep_clone, rejectsNullArguments)
{
  cardano_protocol_parameters_t* copy = nullptr;

  EXPECT_EQ(cardano_protocol_parameters_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_protocol_parameters_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_version_deep_clone, rejectsNullArguments)
{
  cardano_protocol_version_t* copy = nullptr;

  EXPECT_EQ(cardano_protocol_version_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_protocol_version_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_deep_clone, rejectsNullArguments)
{
  cardano_redeemer_t* copy = nullptr;

  EXPECT_EQ(cardano_redeemer_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_redeemer_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_list_deep_clone, rejectsNullArguments)
{
  cardano_redeemer_list_t* copy = nullptr;

  EXPECT_EQ(cardano_redeemer_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_redeemer_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_register_drep_cert_deep_clone, rejectsNullArguments)
{
  cardano_register_drep_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_register_drep_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_register_drep_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_registration_cert_deep_clone, rejectsNullArguments)
{
  cardano_registration_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_registration_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_registration_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_relay_deep_clone, rejectsNullArguments)
{
  cardano_relay_t* copy = nullptr;

  EXPECT_EQ(cardano_relay_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_relay_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_relays_deep_clone, rejectsNullArguments)
{
  cardano_relays_t* copy = nullptr;

  EXPECT_EQ(cardano_relays_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_relays_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_required_guards_map_deep_clone, rejectsNullArguments)
{
  cardano_required_guards_map_t* copy = nullptr;

  EXPECT_EQ(cardano_required_guards_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_required_guards_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_resign_committee_cold_cert_deep_clone, rejectsNullArguments)
{
  cardano_resign_committee_cold_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_resign_committee_cold_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_resign_committee_cold_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_deep_clone, rejectsNullArguments)
{
  cardano_reward_address_t* copy = nullptr;

  EXPECT_EQ(cardano_reward_address_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_reward_address_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_list_deep_clone, rejectsNullArguments)
{
  cardano_reward_address_list_t* copy = nullptr;

  EXPECT_EQ(cardano_reward_address_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_reward_address_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_deep_clone, rejectsNullArguments)
{
  cardano_script_t* copy = nullptr;

  EXPECT_EQ(cardano_script_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_all_deep_clone, rejectsNullArguments)
{
  cardano_script_all_t* copy = nullptr;

  EXPECT_EQ(cardano_script_all_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_all_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_any_deep_clone, rejectsNullArguments)
{
  cardano_script_any_t* copy = nullptr;

  EXPECT_EQ(cardano_script_any_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_any_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_deep_clone, rejectsNullArguments)
{
  cardano_script_invalid_after_t* copy = nullptr;

  EXPECT_EQ(cardano_script_invalid_after_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_invalid_after_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_deep_clone, rejectsNullArguments)
{
  cardano_script_invalid_before_t* copy = nullptr;

  EXPECT_EQ(cardano_script_invalid_before_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_invalid_before_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_n_of_k_deep_clone, rejectsNullArguments)
{
  cardano_script_n_of_k_t* copy = nullptr;

  EXPECT_EQ(cardano_script_n_of_k_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_n_of_k_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_deep_clone, rejectsNullArguments)
{
  cardano_script_pubkey_t* copy = nullptr;

  EXPECT_EQ(cardano_script_pubkey_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_pubkey_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_deep_clone, rejectsNullArguments)
{
  cardano_script_require_guard_t* copy = nullptr;

  EXPECT_EQ(cardano_script_require_guard_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_script_require_guard_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_deep_clone, rejectsNullArguments)
{
  cardano_single_host_addr_relay_t* copy = nullptr;

  EXPECT_EQ(cardano_single_host_addr_relay_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_single_host_addr_relay_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_name_relay_deep_clone, rejectsNullArguments)
{
  cardano_single_host_name_relay_t* copy = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_single_host_name_relay_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_stake_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_stake_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_stake_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_deregistration_cert_deep_clone, rejectsNullArguments)
{
  cardano_stake_deregistration_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_stake_deregistration_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_stake_deregistration_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_registration_cert_deep_clone, rejectsNullArguments)
{
  cardano_stake_registration_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_stake_registration_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_stake_registration_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_registration_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_stake_registration_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_stake_registration_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_stake_registration_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_stake_vote_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_stake_vote_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_stake_vote_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_registration_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_stake_vote_registration_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_stake_vote_registration_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_stake_vote_registration_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_deep_clone, rejectsNullArguments)
{
  cardano_sub_transaction_t* copy = nullptr;

  EXPECT_EQ(cardano_sub_transaction_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_sub_transaction_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_deep_clone, rejectsNullArguments)
{
  cardano_sub_transaction_body_t* copy = nullptr;

  EXPECT_EQ(cardano_sub_transaction_body_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_set_deep_clone, rejectsNullArguments)
{
  cardano_sub_transaction_set_t* copy = nullptr;

  EXPECT_EQ(cardano_sub_transaction_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_sub_transaction_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_deep_clone, rejectsNullArguments)
{
  cardano_transaction_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_body_deep_clone, rejectsNullArguments)
{
  cardano_transaction_body_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_body_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_body_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_deep_clone, rejectsNullArguments)
{
  cardano_transaction_input_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_input_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_input_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_set_deep_clone, rejectsNullArguments)
{
  cardano_transaction_input_set_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_input_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_input_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_deep_clone, rejectsNullArguments)
{
  cardano_transaction_metadata_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_metadata_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_metadata_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_deep_clone, rejectsNullArguments)
{
  cardano_transaction_output_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_output_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_output_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_list_deep_clone, rejectsNullArguments)
{
  cardano_transaction_output_list_t* copy = nullptr;

  EXPECT_EQ(cardano_transaction_output_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_transaction_output_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_treasury_withdrawals_action_deep_clone, rejectsNullArguments)
{
  cardano_treasury_withdrawals_action_t* copy = nullptr;

  EXPECT_EQ(cardano_treasury_withdrawals_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_treasury_withdrawals_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_unit_interval_deep_clone, rejectsNullArguments)
{
  cardano_unit_interval_t* copy = nullptr;

  EXPECT_EQ(cardano_unit_interval_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_unit_interval_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_unregister_drep_cert_deep_clone, rejectsNullArguments)
{
  cardano_unregister_drep_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_unregister_drep_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_unregister_drep_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_unregistration_cert_deep_clone, rejectsNullArguments)
{
  cardano_unregistration_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_unregistration_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_unregistration_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_update_deep_clone, rejectsNullArguments)
{
  cardano_update_t* copy = nullptr;

  EXPECT_EQ(cardano_update_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_update_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_update_committee_action_deep_clone, rejectsNullArguments)
{
  cardano_update_committee_action_t* copy = nullptr;

  EXPECT_EQ(cardano_update_committee_action_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_update_committee_action_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_update_drep_cert_deep_clone, rejectsNullArguments)
{
  cardano_update_drep_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_update_drep_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_update_drep_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_deep_clone, rejectsNullArguments)
{
  cardano_utxo_t* copy = nullptr;

  EXPECT_EQ(cardano_utxo_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_utxo_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_list_deep_clone, rejectsNullArguments)
{
  cardano_utxo_list_t* copy = nullptr;

  EXPECT_EQ(cardano_utxo_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_utxo_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_deep_clone, rejectsNullArguments)
{
  cardano_value_t* copy = nullptr;

  EXPECT_EQ(cardano_value_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_value_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_deep_clone, rejectsNullArguments)
{
  cardano_vkey_witness_t* copy = nullptr;

  EXPECT_EQ(cardano_vkey_witness_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_vkey_witness_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_set_deep_clone, rejectsNullArguments)
{
  cardano_vkey_witness_set_t* copy = nullptr;

  EXPECT_EQ(cardano_vkey_witness_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_vkey_witness_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vote_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_vote_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_vote_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_vote_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vote_registration_delegation_cert_deep_clone, rejectsNullArguments)
{
  cardano_vote_registration_delegation_cert_t* copy = nullptr;

  EXPECT_EQ(cardano_vote_registration_delegation_cert_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_vote_registration_delegation_cert_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_deep_clone, rejectsNullArguments)
{
  cardano_voter_t* copy = nullptr;

  EXPECT_EQ(cardano_voter_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_voter_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_list_deep_clone, rejectsNullArguments)
{
  cardano_voter_list_t* copy = nullptr;

  EXPECT_EQ(cardano_voter_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_voter_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_deep_clone, rejectsNullArguments)
{
  cardano_voting_procedure_t* copy = nullptr;

  EXPECT_EQ(cardano_voting_procedure_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_voting_procedure_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_list_deep_clone, rejectsNullArguments)
{
  cardano_voting_procedure_list_t* copy = nullptr;

  EXPECT_EQ(cardano_voting_procedure_list_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_voting_procedure_list_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_deep_clone, rejectsNullArguments)
{
  cardano_voting_procedures_t* copy = nullptr;

  EXPECT_EQ(cardano_voting_procedures_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_voting_procedures_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_deep_clone, rejectsNullArguments)
{
  cardano_withdrawal_map_t* copy = nullptr;

  EXPECT_EQ(cardano_withdrawal_map_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_withdrawal_map_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_witness_set_deep_clone, rejectsNullArguments)
{
  cardano_witness_set_t* copy = nullptr;

  EXPECT_EQ(cardano_witness_set_deep_clone(nullptr, &copy), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(copy, nullptr);
  EXPECT_EQ(cardano_witness_set_deep_clone(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d9010284581c00000000000000000000000000000000000000000000000000000001581c00000000000000000000000000000000000000000000000000000002581c00000000000000000000000000000000000000000000000000000003581c00000000000000000000000000000000000000000000000000000004",
      "84581c00000000000000000000000000000000000000000000000000000001581c00000000000000000000000000000000000000000000000000000002581c00000000000000000000000000000000000000000000000000000003581c00000000000000000000000000000000000000000000000000000004",
    },
    cardano_blake2b_hash_set_from_cbor,
    cardano_blake2b_hash_set_to_cbor,
    cardano_blake2b_hash_set_deep_clone,
    cardano_blake2b_hash_set_unref);
}

TEST(cardano_blake2b_hash_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_blake2b_hash_from_cbor,
    cardano_blake2b_hash_to_cbor,
    cardano_blake2b_hash_deep_clone,
    cardano_blake2b_hash_unref);
}

TEST(cardano_transaction_metadata_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65",
    },
    cardano_transaction_metadata_from_cbor,
    cardano_transaction_metadata_to_cbor,
    cardano_transaction_metadata_deep_clone,
    cardano_transaction_metadata_unref);
}

TEST(cardano_metadatum_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "9f01029f0102030405ff9f0102030405ff05ff",
    },
    cardano_metadatum_from_cbor,
    cardano_metadatum_to_cbor,
    cardano_metadatum_deep_clone,
    cardano_metadatum_unref);
}

TEST(cardano_plutus_v4_script_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011",
    },
    cardano_plutus_v4_script_list_from_cbor,
    cardano_plutus_v4_script_list_to_cbor,
    cardano_plutus_v4_script_list_deep_clone,
    cardano_plutus_v4_script_list_unref);
}

TEST(cardano_auxiliary_data_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012",
      "d90103a200a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501828202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54",
      "d90103a100a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65",
      "82a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65828202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54",
      "a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65",
      "a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65",
      "d90103a600a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54028447460100002200104746010000220011474601000022001247460100002200130384474601000022001047460100002200114746010000220012474601000022001304834746010000220010474601000022001147460100002200120583474601000022001047460100002200114746010000220012",
    },
    cardano_auxiliary_data_from_cbor,
    cardano_auxiliary_data_to_cbor,
    cardano_auxiliary_data_deep_clone,
    cardano_auxiliary_data_unref,
    cardano_auxiliary_data_clear_cbor_cache);
}

TEST(cardano_metadatum_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "9f01029f0102030405ff9f0102030405ff05ff",
    },
    cardano_metadatum_list_from_cbor,
    cardano_metadatum_list_to_cbor,
    cardano_metadatum_list_deep_clone,
    cardano_metadatum_list_unref);
}

TEST(cardano_plutus_v3_script_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011",
    },
    cardano_plutus_v3_script_list_from_cbor,
    cardano_plutus_v3_script_list_to_cbor,
    cardano_plutus_v3_script_list_deep_clone,
    cardano_plutus_v3_script_list_unref);
}

TEST(cardano_plutus_v1_script_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011",
    },
    cardano_plutus_v1_script_list_from_cbor,
    cardano_plutus_v1_script_list_to_cbor,
    cardano_plutus_v1_script_list_deep_clone,
    cardano_plutus_v1_script_list_unref);
}

TEST(cardano_plutus_v2_script_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011",
    },
    cardano_plutus_v2_script_list_from_cbor,
    cardano_plutus_v2_script_list_to_cbor,
    cardano_plutus_v2_script_list_deep_clone,
    cardano_plutus_v2_script_list_unref);
}

TEST(cardano_metadatum_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a10102",
    },
    cardano_metadatum_map_from_cbor,
    cardano_metadatum_map_to_cbor,
    cardano_metadatum_map_deep_clone,
    cardano_metadatum_map_unref);
}

TEST(cardano_vote_registration_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "840c8200581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000",
    },
    cardano_vote_registration_delegation_cert_from_cbor,
    cardano_vote_registration_delegation_cert_to_cbor,
    cardano_vote_registration_delegation_cert_deep_clone,
    cardano_vote_registration_delegation_cert_unref);
}

TEST(cardano_unregistration_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83088200581c0000000000000000000000000000000000000000000000000000000000",
      "83088200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a001e8480",
    },
    cardano_unregistration_cert_from_cbor,
    cardano_unregistration_cert_to_cbor,
    cardano_unregistration_cert_deep_clone,
    cardano_unregistration_cert_unref);
}

TEST(cardano_certificate_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d901028483078200581c000000000000000000000000000000000000000000000000000000000083088200581c0000000000000000000000000000000000000000000000000000000000830f8200581c00000000000000000000000000000000000000000000000000000000f683028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92",
      "8483078200581c000000000000000000000000000000000000000000000000000000000083088200581c0000000000000000000000000000000000000000000000000000000000830f8200581c00000000000000000000000000000000000000000000000000000000f683028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92",
    },
    cardano_certificate_set_from_cbor,
    cardano_certificate_set_to_cbor,
    cardano_certificate_set_deep_clone,
    cardano_certificate_set_unref);
}

TEST(cardano_vote_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83098200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_vote_delegation_cert_from_cbor,
    cardano_vote_delegation_cert_to_cbor,
    cardano_vote_delegation_cert_deep_clone,
    cardano_vote_delegation_cert_unref);
}

TEST(cardano_genesis_key_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003",
    },
    cardano_genesis_key_delegation_cert_from_cbor,
    cardano_genesis_key_delegation_cert_to_cbor,
    cardano_genesis_key_delegation_cert_deep_clone,
    cardano_genesis_key_delegation_cert_unref);
}

TEST(cardano_stake_vote_registration_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "850d8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000",
    },
    cardano_stake_vote_registration_delegation_cert_from_cbor,
    cardano_stake_vote_registration_delegation_cert_to_cbor,
    cardano_stake_vote_registration_delegation_cert_deep_clone,
    cardano_stake_vote_registration_delegation_cert_unref);
}

TEST(cardano_registration_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83078200581c0000000000000000000000000000000000000000000000000000000000",
      "83078200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a001e8480",
    },
    cardano_registration_cert_from_cbor,
    cardano_registration_cert_to_cbor,
    cardano_registration_cert_deep_clone,
    cardano_registration_cert_unref);
}

TEST(cardano_mir_to_stake_creds_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8200a18200581c0101010101010101010101010101010101010101010101010101010100",
      "8201a18200581c0101010101010101010101010101010101010101010101010101010100",
    },
    cardano_mir_to_stake_creds_cert_from_cbor,
    cardano_mir_to_stake_creds_cert_to_cbor,
    cardano_mir_to_stake_creds_cert_deep_clone,
    cardano_mir_to_stake_creds_cert_unref);
}

TEST(cardano_stake_registration_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82008200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f",
    },
    cardano_stake_registration_cert_from_cbor,
    cardano_stake_registration_cert_to_cbor,
    cardano_stake_registration_cert_deep_clone,
    cardano_stake_registration_cert_unref);
}

TEST(cardano_register_drep_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "84108200581c0000000000000000000000000000000000000000000000000000000000f6",
      "84108200581c0000000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_register_drep_cert_from_cbor,
    cardano_register_drep_cert_to_cbor,
    cardano_register_drep_cert_deep_clone,
    cardano_register_drep_cert_unref);
}

TEST(cardano_certificate_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000",
      "8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003",
      "820682001a000f4240",
      "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
      "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8",
      "84108200581c0000000000000000000000000000000000000000000000000000000000f6",
      "83078200581c0000000000000000000000000000000000000000000000000000000000",
      "830f8200581c00000000000000000000000000000000000000000000000000000000f6",
      "83028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92",
      "82018200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f",
      "82008200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f",
      "840b8200581c00000000000000000000000000000000000000000000000000000000581c0000000000000000000000000000000000000000000000000000000000",
      "840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000",
      "850d8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000",
      "83118200581c0000000000000000000000000000000000000000000000000000000000",
      "83088200581c0000000000000000000000000000000000000000000000000000000000",
      "83128200581c00000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "83098200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000",
      "840c8200581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000",
    },
    cardano_certificate_from_cbor,
    cardano_certificate_to_cbor,
    cardano_certificate_deep_clone,
    cardano_certificate_unref);
}

TEST(cardano_stake_registration_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "840b8200581c00000000000000000000000000000000000000000000000000000000581c0000000000000000000000000000000000000000000000000000000000",
    },
    cardano_stake_registration_delegation_cert_from_cbor,
    cardano_stake_registration_delegation_cert_to_cbor,
    cardano_stake_registration_delegation_cert_deep_clone,
    cardano_stake_registration_delegation_cert_unref);
}

TEST(cardano_pool_registration_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
      "8b03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0825860000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f5830606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f1927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
      "8b03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0f61927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
    },
    cardano_pool_registration_cert_from_cbor,
    cardano_pool_registration_cert_to_cbor,
    cardano_pool_registration_cert_deep_clone,
    cardano_pool_registration_cert_unref);
}

TEST(cardano_stake_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92",
    },
    cardano_stake_delegation_cert_from_cbor,
    cardano_stake_delegation_cert_to_cbor,
    cardano_stake_delegation_cert_deep_clone,
    cardano_stake_delegation_cert_unref);
}

TEST(cardano_stake_deregistration_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82018200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f",
    },
    cardano_stake_deregistration_cert_from_cbor,
    cardano_stake_deregistration_cert_to_cbor,
    cardano_stake_deregistration_cert_deep_clone,
    cardano_stake_deregistration_cert_unref);
}

TEST(cardano_update_drep_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83128200581c00000000000000000000000000000000000000000000000000000000f6",
      "83128200581c00000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_update_drep_cert_from_cbor,
    cardano_update_drep_cert_to_cbor,
    cardano_update_drep_cert_deep_clone,
    cardano_update_drep_cert_unref);
}

TEST(cardano_resign_committee_cold_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "830f8200581c00000000000000000000000000000000000000000000000000000000f6",
      "830f8200581c00000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_resign_committee_cold_cert_from_cbor,
    cardano_resign_committee_cold_cert_to_cbor,
    cardano_resign_committee_cold_cert_deep_clone,
    cardano_resign_committee_cold_cert_unref);
}

TEST(cardano_mir_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "820682001a000f4240",
      "820682011a000f4240",
      "82068200a18200581c0101010101010101010101010101010101010101010101010101010100",
      "82068201a18200581c0101010101010101010101010101010101010101010101010101010100",
    },
    cardano_mir_cert_from_cbor,
    cardano_mir_cert_to_cbor,
    cardano_mir_cert_deep_clone,
    cardano_mir_cert_unref);
}

TEST(cardano_auth_committee_hot_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_auth_committee_hot_cert_from_cbor,
    cardano_auth_committee_hot_cert_to_cbor,
    cardano_auth_committee_hot_cert_deep_clone,
    cardano_auth_committee_hot_cert_unref);
}

TEST(cardano_unregister_drep_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83118200581c0000000000000000000000000000000000000000000000000000000000",
    },
    cardano_unregister_drep_cert_from_cbor,
    cardano_unregister_drep_cert_to_cbor,
    cardano_unregister_drep_cert_deep_clone,
    cardano_unregister_drep_cert_unref);
}

TEST(cardano_stake_vote_delegation_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_stake_vote_delegation_cert_from_cbor,
    cardano_stake_vote_delegation_cert_to_cbor,
    cardano_stake_vote_delegation_cert_deep_clone,
    cardano_stake_vote_delegation_cert_unref);
}

TEST(cardano_pool_retirement_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8",
    },
    cardano_pool_retirement_cert_from_cbor,
    cardano_pool_retirement_cert_to_cbor,
    cardano_pool_retirement_cert_deep_clone,
    cardano_pool_retirement_cert_unref);
}

TEST(cardano_mir_to_pot_cert_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82001a000f4240",
      "82011a000f4240",
    },
    cardano_mir_to_pot_cert_from_cbor,
    cardano_mir_to_pot_cert_to_cbor,
    cardano_mir_to_pot_cert_deep_clone,
    cardano_mir_to_pot_cert_unref);
}

TEST(cardano_voting_procedures_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_voting_procedures_from_cbor,
    cardano_voting_procedures_to_cbor,
    cardano_voting_procedures_deep_clone,
    cardano_voting_procedures_unref);
}

TEST(cardano_voter_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8200581c00000000000000000000000000000000000000000000000000000000",
      "8200581c00000000000000000000000000000000000000000000000000000001",
      "8200581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_voter_from_cbor,
    cardano_voter_to_cbor,
    cardano_voter_deep_clone,
    cardano_voter_unref);
}

TEST(cardano_voting_procedure_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8200f6",
      "8201f6",
      "8202f6",
      "8200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "8201827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "8202827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_voting_procedure_from_cbor,
    cardano_voting_procedure_to_cbor,
    cardano_voting_procedure_deep_clone,
    cardano_voting_procedure_unref);
}

TEST(cardano_plutus_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a10102",
    },
    cardano_plutus_map_from_cbor,
    cardano_plutus_map_to_cbor,
    cardano_plutus_map_deep_clone,
    cardano_plutus_map_unref,
    cardano_plutus_map_clear_cbor_cache);
}

TEST(cardano_plutus_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "9f01029f0102030405ff9f0102030405ff05ff",
    },
    cardano_plutus_list_from_cbor,
    cardano_plutus_list_to_cbor,
    cardano_plutus_list_deep_clone,
    cardano_plutus_list_unref,
    cardano_plutus_list_clear_cbor_cache);
}

TEST(cardano_constr_plutus_data_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d8799f0102030405ff",
    },
    cardano_constr_plutus_data_from_cbor,
    cardano_constr_plutus_data_to_cbor,
    cardano_constr_plutus_data_deep_clone,
    cardano_constr_plutus_data_unref,
    cardano_constr_plutus_data_clear_cbor_cache);
}

TEST(cardano_plutus_data_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "9f01029f0102030405ff9f0102030405ff05ff",
      "9fc25f584037d34fac60a7dd2edba0c76fa58862c91c45ff4298e9134ba8e76be9a7513d88865bfdb9315073dc2690b0f2b59a232fbfa0a8a504df6ee9bb78e3f33fbdfef95529c9e74ff30ffe1bd1cc5795c37535899dba800000ffc25f58408d4820519e9bba2d6556c87b100709082f4c8958769899eb5d288b6f9ea9e0723df7211959860edea5829c9732422d25962e3945c68a6089f50a18b0114248b7555feea4851e9f099180600000000000000000000000ffc25f58408d4820519e9bba2d6556c87b100709082f4c8958769899eb5d288b6f9ea9e0723df7211959860edea5829c9732422d25962e3945c68a6089f50a18b0114248b7555feea4851e9f099180600000000000000000000000ffc25f584039878c5f4d4063e9a2ee75a3fbdd1492c3cad46f4ecbae977ac94b709a730e367edf9dae05acd59638d1dec25e2351c2eecb871694afae979de7085b522efe1355634138bbd920200d574cdf400324cdd1aafe10a240ffc25f584022a6282a7d960570c4c729decd677ec617061f0e501249c41f8724c89dc97dc0d24917bdb7a7ebd7c079c1c56fa21af0f119168966356ea384fb711cb766015e55bfc5bc86583f6a82ae605a93e7bf974ae74cd051c0ffc25f58404445ab8649611ee8f74a3c31e504a2f25f2f7631ef6ef828a405542904d84c997304b1b332d528ee54873b03cfb73cd3c5b35b91184f6846afccec7271bda8a05563ba46aed8c82611da47fd608d027447f8391161c0ffc25f58400258b535c4d4a22a483b22b2f5c5c65bed9e7de59266f6bbaa8997edf5bec6bb5d203641bb58d8ade1a3a5b4e5f923df502cf1e47691865fe1984eacef3be96a551ed585e070265db203a8866726bed053cb6c8aa200ffc25f5840021104310667ec434e9e2cd9fa71853593c42e1b55865ac49f80b2ea22beeec9b4a55e9545055a2bcde3a78d36836df11df0f91c1dae9a8aee58419b8650bc6c529361f9601a4005051b045d05f39a5f00ebd5ffff",
    },
    cardano_plutus_data_from_cbor,
    cardano_plutus_data_to_cbor,
    cardano_plutus_data_deep_clone,
    cardano_plutus_data_unref,
    cardano_plutus_data_clear_cbor_cache);
}

TEST(cardano_sub_transaction_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83a200d90102800180a0f6",
      "83a200d90102800180a0a1016474657374",
      "83a200d90102800180a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40af6",
      "83a200d90102800180a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40aa1016474657374",
      "83a201800081825820000000000000000000000000000000000000000000000000000000000000000000a0f6",
      "83a201800081825820000000000000000000000000000000000000000000000000000000000000000000a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40af6",
    },
    cardano_sub_transaction_from_cbor,
    cardano_sub_transaction_to_cbor,
    cardano_sub_transaction_deep_clone,
    cardano_sub_transaction_unref,
    cardano_sub_transaction_clear_cbor_cache);
}

TEST(cardano_transaction_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "84af00d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804d90102828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0050758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0dd90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010ed9010281581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500a700d90102818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501d90102868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402d9010281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003d90102815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45004d9010281187b05a282010082d87a9f187bff82190bb8191b5882020182d87a9f187bff821913881907d006d90102815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5d90103a100a6011904d20263737472039f1904d263737472ff0445627974657305a2667374726b6579187b9f676c6973746b6579ff6873747276616c75650626",
      "84a600d9010281825820260aed6e7a24044b1254a87a509468a649f522a4e54e830ac10f27ea7b5ec61f010183a300581d70b429738bd6cc58b5c7932d001aa2bd05cfea47020a556c8c753d4436011a004c4b40028200582007845f8f3841996e3d8157954e2f5e2fb90465f27112fc5fe9056d916fae245ba200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba011a04636769a200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5447742544319271044774554481a0031f9194577444f47451a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada931c72a600a6e3305bd22c7aeb9ada7c3f6823b155f4db85de36a69aa200d9010281825820e686ade5bc97372f271fd2abc06cfd96c24b3d9170f9459de1d8e3dd8fd385575840653324a9dddad004f05a8ac99fa2d1811af5f00543591407fb5206cfe9ac91bb1412404323fa517e0e189684cd3592e7f74862e3f16afbc262519abec958180c04d9010281d8799fd8799fd8799fd8799f581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68ffd8799fd8799fd8799f581c042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339baffffffff581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c681b000001863784a12ed8799fd8799f4040ffd8799f581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff1984577444f4745ffffffd8799fd87980190c8efffff5f6",
      "84a40081825820f6dd880fb30480aa43117c73bfd09442ba30de5644c3ec1a91d9232fbe715aab000182a20058390071213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2cad9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13011b0000000253c8e4f6a300581d702ed2631dbb277c84334453c5c437b86325d371f0835a28b910a91a6e011a001e848002820058209d7fee57d1dbb9b000b2a133256af0f2c83ffe638df523b2d1c13d405356d8ae021a0002fb050b582088e4779d217d10398a705530f9fb2af53ffac20aef6e75e85c26e93a00877556a10481d8799fd8799f40ffd8799fa1d8799fd8799fd87980d8799fd8799f581c71213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2caffd8799fd8799fd8799f581cd9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13ffffffffffd8799f4040ffff1a001e8480a0a000ffd87c9f9fd8799fd8799fd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799f4040ffd87a9f1a00989680ffffd87c9f9fd8799fd87a9fd8799f4752656c65617365d8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffff9fd8799f0101ffffffd87c9f9fd8799fd87b9fd9050280ffd87980ffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980fffff5f6",
      "83a50081825820bf30608a974d09c56dd62ca10199ec11746ea2d90dbd83649d4f37c629b1ba840001818258390117d237fb8f952c995cd28f73c555adc2307322d819b7f565196ce754348144bff68f23c1386b85dea0f8425ca574b1a11e188ffaba67537c1a0048f96f021a000351d1031a019732f30682a7581c162f94554ac8c225383a2248c245659eda870eaa82d0ef25fc7dcd82a10d8100581c2075a095b3c844a29c24317a94a643ab8e22d54a3a3a72a420260af6a10d8100581c268cfc0b89e910ead22e0ade91493d8212f53f3e2164b2e4bef0819ba10d8100581c60baee25cbc90047e83fd01e1e57dc0b06d3d0cb150d0ab40bbfead1a10d8100581cad5463153dc3d24b9ff133e46136028bdc1edbb897f5a7cf1b37950ca10d8100581cb9547b8a57656539a8d9bc42c008e38d9c8bd9c8adbb1e73ad529497a10d8100581cf7b341c14cd58fca4195a9b278cce1ef402dc0e06deb77e543cd1757a10d8100190103a1008882582061261a95b7613ee6bf2067dad77b70349729b0c50d57bc1cf30de0db4a1e73a858407d72721e7504e12d50204f7d9e9d9fe60d9c6a4fd18ad629604729df4f7f3867199b62885623fab68a02863e7877955ca4a56c867157a559722b7b350b668a0b8258209180d818e69cd997e34663c418a648c076f2e19cd4194e486e159d8580bc6cda5840af668e57c98f0c3d9b47c66eb9271213c39b4ea1b4d543b0892f03985edcef4216d1f98f7b731eedc260a2154124b5cab015bfeaf694d58966d124ad2ff60f0382582089c29f8c4af27b7accbe589747820134ebbaa1caf3ce949270a3d0c7dcfd541b58401ad69342385ba6c3bef937a79456d7280c0d539128072db15db120b1579c46ba95d18c1fa073d7dbffb4d975b1e02ebb7372936940cff0a96fce950616d2f504825820f14f712dc600d793052d4842d50cefa4e65884ea6cf83707079eb8ce302efc855840638f7410929e7eab565b1451effdfbeea2a8839f7cfcc4c4483c4931d489547a2e94b73e4b15f8494de7f42ea31e573c459a9a7e5269af17b0978e70567de80e8258208b53207629f9a30e4b2015044f337c01735abe67243c19470c9dae8c7b73279858400c4ed03254c33a19256b7a3859079a9b75215cad83871a9b74eb51d8bcab52911c37ea5c43bdd212d006d1e6670220ff1d03714addf94f490e482edacbb08f068258205fddeedade2714d6db2f9e1104743d2d8d818ecddc306e176108db14caadd4415840bf48f5dd577b5cb920bfe60e13c8b1b889366c23e2f2e28d51814ed23def3a0ff4a1964f806829d40180d83b5230728409c1f18ddb5a61c44e614b823bd43f01825820cbc6b506e94fbefe442eecee376f3b3ebaf89415ef5cd2efb666e06ddae48393584089bff8f81a20b22f2c3f8a2288b15f1798b51f3363e0437a46c0a2e4e283b7c1018eba0b2b192d6d522ac8df2f2e95b4c8941b387cda89857ab0ae77db14780c825820e8c03a03c0b2ddbea4195caf39f41e669f7d251ecf221fbb2f275c0a5d7e05d158402643ac53dd4da4f6e80fb192b2bf7d1dd9a333bbacea8f07531ba450dd8fb93e481589d370a6ef33a97e03b2f5816e4b2c6a8abf606a859108ba6f416e530d07f6",
      "83a400d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000180020a0e828201581caabbccddeeff00112233445566778899aabbccddeeff0011223344558201581cbaf8f17908592241efd1f36ec3a12fd5e8dc907c83da9d9400210aeea101d901028182068201581caabbccddeeff00112233445566778899aabbccddeeff001122334455f6",
      "84b700d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000182825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc1a00989680a3005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc011a001e848003d8184a82044746010000220013021a00030d400319271004d901028183028200581c00112233445566778899aabbccddeeff00112233445566778899aabb581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9205a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0050758209e2e4ea189a571107fddaa2459ad14241a080c0142c02c89e4070cf71e87825108186409a2581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c4138310b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0dd90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010ed90102828200581c00112233445566778899aabbccddeeff00112233445566778899aabb8201581caabbccddeeff00112233445566778899aabbccddeeff0011223344550f0010825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc1a003d0900111a004c4b4012d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50313a18202581c00112233445566778899aabbccddeeff00112233445566778899aabba1825820ee155ace9c40292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e25008201827368747470733a2f2f6578616d706c652e636f6d5820000000000000000000000000000000000000000000000000000000000000000014d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820ee155ace9c40292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e2500a700186412a103831a0003236119032c011821d81e8218590218221a004c4b4018231a000f424018241964001825d81e820302f6827368747470733a2f2f6578616d706c652e636f6d58200000000000000000000000000000000000000000000000000000000000000000151a00989680161903e817d901028283a600d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5020181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc1a002dc6c004d901028183078200581c00112233445566778899aabbccddeeff00112233445566778899aabb1a001e848005a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00509a1581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a1401903e81818a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d87980a0f683a200d90102800180a0a10164746573741819a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01a001e8480581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a000f4240181aa3581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821901f4f6581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570821864191388581df0aabbccddeeff00112233445566778899aabbccddeeff00112233445582f6192710a800d90102818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501d901028182068200581c00112233445566778899aabbccddeeff00112233445566778899aabb02d9010281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003d9010281474601000022001004d9010281187b05a282000082d87a9f187bff82190bb8191b5882060082d87a9f187bff821913881907d006d9010281474601000022001107d90102814746010000220012f5d90103a600a11902d17064696a6b737472612d6d6178696d616c01818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54028147460100002200100381474601000022001104814746010000220012058247460100002200134747010000222200",
    },
    cardano_transaction_from_cbor,
    cardano_transaction_to_cbor,
    cardano_transaction_deep_clone,
    cardano_transaction_unref,
    cardano_transaction_clear_cbor_cache);
}

TEST(cardano_required_guards_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d87980",
      "a28201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d879808200581c00112233445566778899aabbccddeeff00112233445566778899aabbf6",
      "a18200581c00112233445566778899aabbccddeeff00112233445566778899aabbf6",
      "a18201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d87980",
    },
    cardano_required_guards_map_from_cbor,
    cardano_required_guards_map_to_cbor,
    cardano_required_guards_map_deep_clone,
    cardano_required_guards_map_unref);
}

TEST(cardano_transaction_input_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90102848258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001021058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001022058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102305",
      "848258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001021058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001022058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102305",
    },
    cardano_transaction_input_set_from_cbor,
    cardano_transaction_input_set_to_cbor,
    cardano_transaction_input_set_deep_clone,
    cardano_transaction_input_set_unref);
}

TEST(cardano_sub_transaction_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d901028283a300d90102800180031864a0a101647465737483a200d90102800180a0f6",
      "8283a300d90102800180031864a0a101647465737483a200d90102800180a0f6",
      "d901028283a200d90102800180a0f683a300d90102800180031864a0a1016474657374",
    },
    cardano_sub_transaction_set_from_cbor,
    cardano_sub_transaction_set_to_cbor,
    cardano_sub_transaction_set_deep_clone,
    cardano_sub_transaction_set_unref);
}

TEST(cardano_direct_deposit_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01903e8581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce725701907d0",
      "a2581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce725701907d0581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01903e8",
      "a1581de100112233445566778899aabbccddeeff00112233445566778899aabb1a000f4240",
    },
    cardano_direct_deposit_map_from_cbor,
    cardano_direct_deposit_map_to_cbor,
    cardano_direct_deposit_map_deep_clone,
    cardano_direct_deposit_map_unref);
}

TEST(cardano_account_balance_intervals_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a3581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce7257082f6192710581df0aabbccddeeff00112233445566778899aabbccddeeff001122334455182a",
      "a3581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570821901f4f6581df0aabbccddeeff00112233445566778899aabbccddeeff00112233445582f6192710",
      "a2581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce7257082f6192710581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388",
    },
    cardano_account_balance_intervals_map_from_cbor,
    cardano_account_balance_intervals_map_to_cbor,
    cardano_account_balance_intervals_map_deep_clone,
    cardano_account_balance_intervals_map_unref);
}

TEST(cardano_sub_transaction_body_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a200d90102800180",
      "b300d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e031903e804d90102818304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f405a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f050758200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d508186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50ed90102828200581c00112233445566778899aabbccddeeff00112233445566778899aabb8201581caabbccddeeff00112233445566778899aabbccddeeff0011223344550f0112d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50013a18202581c00112233445566778899aabbccddeeff00112233445566778899aabba18258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5038201f614d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000151907d0161903e81818a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d879801819a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a000f4240181aa1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388",
    },
    cardano_sub_transaction_body_from_cbor,
    cardano_sub_transaction_body_to_cbor,
    cardano_sub_transaction_body_deep_clone,
    cardano_sub_transaction_body_unref,
    cardano_sub_transaction_body_clear_cbor_cache);
}

TEST(cardano_value_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a",
      "821a000f4240a2581c00000000000000000000000000001100000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a",
      "821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218c8443334353618c6444041424214581c11111111111111111111111111111111111111111111111111111111a3443031323218c8443334353618c6444041424214",
    },
    cardano_value_from_cbor,
    cardano_value_to_cbor,
    cardano_value_deep_clone,
    cardano_value_unref);
}

TEST(cardano_account_balance_interval_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "821864191388",
      "821864192710",
      "821901f4f6",
      "82f6192710",
      "8200f6",
      "8200192710",
      "1903e8",
      "1b0000000100000000",
      "82011864",
      "82f61864",
      "8201f6",
    },
    cardano_account_balance_interval_from_cbor,
    cardano_account_balance_interval_to_cbor,
    cardano_account_balance_interval_deep_clone,
    cardano_account_balance_interval_unref);
}

TEST(cardano_transaction_body_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "b100818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570030682a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba19020b0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d390f0110825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500",
      "b500818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570030682a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba19020b0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d390f0110825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50013a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000001481841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000151907d0161903e8",
      "b500d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804d90102828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570030682a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba19020b0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0dd90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010ed9010281581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d390f0110a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50013a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f5820000000000000000000000000000000000000000000000000000000000000000014d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000151907d0161903e8",
      "a400d90102848258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001021058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001022058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001023050184a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02820058200000000000000000000000000000000000000000000000000000000000000000a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ffa2005826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51b8cd9819561bddc6614011a02faf0800218640319fde8",
      "a300d90102848258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001021058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001022058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001023050184a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02820058200000000000000000000000000000000000000000000000000000000000000000a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ffa2005826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51b8cd9819561bddc6614011a02faf080021864",
      "b500d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804d90102828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570030682a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba19020b0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0dd90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d390f0110a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50013a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f5820000000000000000000000000000000000000000000000000000000000000000014d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000151907d0161903e8",
    },
    cardano_transaction_body_from_cbor,
    cardano_transaction_body_to_cbor,
    cardano_transaction_body_deep_clone,
    cardano_transaction_body_unref,
    cardano_transaction_body_clear_cbor_cache);
}

TEST(cardano_transaction_output_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "84a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02820058200000000000000000000000000000000000000000000000000000000000000000a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ffa2005826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51b8cd9819561bddc6614011a02faf080",
    },
    cardano_transaction_output_list_from_cbor,
    cardano_transaction_output_list_to_cbor,
    cardano_transaction_output_list_deep_clone,
    cardano_transaction_output_list_unref);
}

TEST(cardano_transaction_output_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011",
      "a400583900537ba48a023f0a3c66e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011",
      "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4340a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011",
      "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200122211",
      "83583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a58200000000000000000000000000000000000000000000000000000000000000000",
      "82583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a",
      "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff",
      "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a0282005820000000000000000000000000000000000000000000000000000000000000000003d8185182014e4d01000033222220051200120011",
      "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011",
      "82583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a",
      "825826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51bbcd9819561bddc66141a02faf080",
    },
    cardano_transaction_output_from_cbor,
    cardano_transaction_output_to_cbor,
    cardano_transaction_output_deep_clone,
    cardano_transaction_output_unref);
}

TEST(cardano_transaction_input_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102005",
    },
    cardano_transaction_input_from_cbor,
    cardano_transaction_input_to_cbor,
    cardano_transaction_input_deep_clone,
    cardano_transaction_input_unref);
}

TEST(cardano_asset_name_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "41aa",
    },
    cardano_asset_name_from_cbor,
    cardano_asset_name_to_cbor,
    cardano_asset_name_deep_clone,
    cardano_asset_name_unref);
}

TEST(cardano_asset_name_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303",
    },
    cardano_asset_name_map_from_cbor,
    cardano_asset_name_map_to_cbor,
    cardano_asset_name_map_deep_clone,
    cardano_asset_name_map_unref);
}

TEST(cardano_multi_asset_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a",
      "a2581c00000000000000000000000000000000000000002200000000000000a34430313232186444333435361863444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435361863444041424229",
      "a2581c00000000000000000000000000000000000000000000000000000000a34430313232186444333435361863444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435361863444041424229",
    },
    cardano_multi_asset_from_cbor,
    cardano_multi_asset_to_cbor,
    cardano_multi_asset_deep_clone,
    cardano_multi_asset_unref);
}

TEST(cardano_script_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82014e4d01000033222220051200120011",
      "82025908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041",
      "82035908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041",
      "82044e4d01000033222220051200120011",
    },
    cardano_script_from_cbor,
    cardano_script_to_cbor,
    cardano_script_deep_clone,
    cardano_script_unref);
}

TEST(cardano_script_invalid_before_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82041b0000000000000005",
      "820405",
    },
    cardano_script_invalid_before_from_cbor,
    cardano_script_invalid_before_to_cbor,
    cardano_script_invalid_before_deep_clone,
    cardano_script_invalid_before_unref,
    cardano_script_invalid_before_clear_cbor_cache);
}

TEST(cardano_script_invalid_after_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82051b0000000000000007",
      "820507",
    },
    cardano_script_invalid_after_from_cbor,
    cardano_script_invalid_after_to_cbor,
    cardano_script_invalid_after_deep_clone,
    cardano_script_invalid_after_unref,
    cardano_script_invalid_after_clear_cbor_cache);
}

TEST(cardano_script_require_guard_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82068200581c00112233445566778899aabbccddeeff00112233445566778899aabb",
      "82068201581caabbccddeeff00112233445566778899aabbccddeeff001122334455",
      "8206820059001c00112233445566778899aabbccddeeff00112233445566778899aabb",
    },
    cardano_script_require_guard_from_cbor,
    cardano_script_require_guard_to_cbor,
    cardano_script_require_guard_deep_clone,
    cardano_script_require_guard_unref,
    cardano_script_require_guard_clear_cbor_cache);
}

TEST(cardano_script_n_of_k_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83031b00000000000000018282041b000000000000000582051b0000000000000007",
      "83030182820405820507",
    },
    cardano_script_n_of_k_from_cbor,
    cardano_script_n_of_k_to_cbor,
    cardano_script_n_of_k_deep_clone,
    cardano_script_n_of_k_unref,
    cardano_script_n_of_k_clear_cbor_cache);
}

TEST(cardano_native_script_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82041b0000000000000005",
      "820405",
      "820409",
      "82018282041b000000000000000582051b0000000000000007",
      "820182820405820507",
    },
    cardano_native_script_from_cbor,
    cardano_native_script_to_cbor,
    cardano_native_script_deep_clone,
    cardano_native_script_unref,
    cardano_native_script_clear_cbor_cache);
}

TEST(cardano_script_all_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82018282041b000000000000000582051b0000000000000007",
      "820182820405820507",
    },
    cardano_script_all_from_cbor,
    cardano_script_all_to_cbor,
    cardano_script_all_deep_clone,
    cardano_script_all_unref,
    cardano_script_all_clear_cbor_cache);
}

TEST(cardano_native_script_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "818200581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_native_script_list_from_cbor,
    cardano_native_script_list_to_cbor,
    cardano_native_script_list_deep_clone,
    cardano_native_script_list_unref,
    cardano_native_script_list_clear_cbor_cache);
}

TEST(cardano_script_any_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82028282041b000000000000000582051b0000000000000007",
      "820282820405820507",
    },
    cardano_script_any_from_cbor,
    cardano_script_any_to_cbor,
    cardano_script_any_deep_clone,
    cardano_script_any_unref,
    cardano_script_any_clear_cbor_cache);
}

TEST(cardano_script_pubkey_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "820059001c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37",
      "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37",
    },
    cardano_script_pubkey_from_cbor,
    cardano_script_pubkey_to_cbor,
    cardano_script_pubkey_deep_clone,
    cardano_script_pubkey_unref,
    cardano_script_pubkey_clear_cbor_cache);
}

TEST(cardano_plutus_v1_script_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "4e4d01000033222220051200120011",
    },
    cardano_plutus_v1_script_from_cbor,
    cardano_plutus_v1_script_to_cbor,
    cardano_plutus_v1_script_deep_clone,
    cardano_plutus_v1_script_unref);
}

TEST(cardano_plutus_v2_script_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041",
    },
    cardano_plutus_v2_script_from_cbor,
    cardano_plutus_v2_script_to_cbor,
    cardano_plutus_v2_script_deep_clone,
    cardano_plutus_v2_script_unref);
}

TEST(cardano_plutus_v4_script_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "4e4d01000033222220051200120011",
    },
    cardano_plutus_v4_script_from_cbor,
    cardano_plutus_v4_script_to_cbor,
    cardano_plutus_v4_script_deep_clone,
    cardano_plutus_v4_script_unref);
}

TEST(cardano_plutus_v3_script_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041",
    },
    cardano_plutus_v3_script_from_cbor,
    cardano_plutus_v3_script_to_cbor,
    cardano_plutus_v3_script_deep_clone,
    cardano_plutus_v3_script_unref);
}

TEST(cardano_pool_metadata_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
      "827368747470733a2f2f6578616d706c652e636f6d40",
    },
    cardano_pool_metadata_from_cbor,
    cardano_pool_metadata_to_cbor,
    cardano_pool_metadata_deep_clone,
    cardano_pool_metadata_unref);
}

TEST(cardano_single_host_addr_relay_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "84000a440a03020a5001020304010203040102030401020304",
    },
    cardano_single_host_addr_relay_from_cbor,
    cardano_single_host_addr_relay_to_cbor,
    cardano_single_host_addr_relay_deep_clone,
    cardano_single_host_addr_relay_unref);
}

TEST(cardano_pool_owners_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d9010285581c00000000000000000000000000000000000000000000000000000000581c11111111111111111111111111111111111111111111111111111111581c22222222222222222222222222222222222222222222222222222222581c33333333333333333333333333333333333333333333333333333333581c44444444444444444444444444444444444444444444444444444444",
      "85581c00000000000000000000000000000000000000000000000000000000581c11111111111111111111111111111111111111111111111111111111581c22222222222222222222222222222222222222222222222222222222581c33333333333333333333333333333333333333333333333333333333581c44444444444444444444444444444444444444444444444444444444",
    },
    cardano_pool_owners_from_cbor,
    cardano_pool_owners_to_cbor,
    cardano_pool_owners_deep_clone,
    cardano_pool_owners_unref);
}

TEST(cardano_ipv4_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "440a03020a",
    },
    cardano_ipv4_from_cbor,
    cardano_ipv4_to_cbor,
    cardano_ipv4_deep_clone,
    cardano_ipv4_unref);
}

TEST(cardano_pool_params_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
      "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6df6",
      "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0825860000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f5830606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f1927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
      "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0f61927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5",
    },
    cardano_pool_params_from_cbor,
    cardano_pool_params_to_cbor,
    cardano_pool_params_deep_clone,
    cardano_pool_params_unref);
}

TEST(cardano_single_host_name_relay_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8301f66b6578616d706c652e636f6d",
      "83010a6b6578616d706c652e636f6d",
    },
    cardano_single_host_name_relay_from_cbor,
    cardano_single_host_name_relay_to_cbor,
    cardano_single_host_name_relay_deep_clone,
    cardano_single_host_name_relay_unref);
}

TEST(cardano_relay_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "83010a6b6578616d706c652e636f6d",
      "8301f66b6578616d706c652e636f6d",
      "82026b6578616d706c652e636f6d",
      "84000a440a03020a5001020304010203040102030401020304",
      "84000a440a03020a5000000000000000000000ffff0a03020a",
    },
    cardano_relay_from_cbor,
    cardano_relay_to_cbor,
    cardano_relay_deep_clone,
    cardano_relay_unref);
}

TEST(cardano_bls_key_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "825860000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f5830606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f",
    },
    cardano_bls_key_from_cbor,
    cardano_bls_key_to_cbor,
    cardano_bls_key_deep_clone,
    cardano_bls_key_unref);
}

TEST(cardano_ipv6_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "5001020304010203040102030401020304",
    },
    cardano_ipv6_from_cbor,
    cardano_ipv6_to_cbor,
    cardano_ipv6_deep_clone,
    cardano_ipv6_unref);
}

TEST(cardano_multi_host_name_relay_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82026b6578616d706c652e636f6d",
    },
    cardano_multi_host_name_relay_from_cbor,
    cardano_multi_host_name_relay_to_cbor,
    cardano_multi_host_name_relay_deep_clone,
    cardano_multi_host_name_relay_unref);
}

TEST(cardano_relays_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "858301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d",
    },
    cardano_relays_from_cbor,
    cardano_relays_to_cbor,
    cardano_relays_deep_clone,
    cardano_relays_unref);
}

TEST(cardano_ex_unit_prices_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82d81e820102d81e820103",
    },
    cardano_ex_unit_prices_from_cbor,
    cardano_ex_unit_prices_to_cbor,
    cardano_ex_unit_prices_deep_clone,
    cardano_ex_unit_prices_unref);
}

TEST(cardano_pool_voting_thresholds_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "85d81e820000d81e820101d81e820202d81e820303d81e820404",
    },
    cardano_pool_voting_thresholds_from_cbor,
    cardano_pool_voting_thresholds_to_cbor,
    cardano_pool_voting_thresholds_deep_clone,
    cardano_pool_voting_thresholds_unref);
}

TEST(cardano_proposed_param_updates_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba",
    },
    cardano_proposed_param_updates_from_cbor,
    cardano_proposed_param_updates_to_cbor,
    cardano_proposed_param_updates_deep_clone,
    cardano_proposed_param_updates_unref);
}

TEST(cardano_protocol_param_update_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "b8210018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d018201913881821d81e82185902",
      "a418221affffffff18231a0140000018241964001825d81e820f0a",
      "ab1826d81e8203021827d81e82011418281903e818291907d0182a190bb8182b1901f4182cd81e820305182d1a00010000182e1a000186a0182f821a00d59f801b00000002540be40018301a00032000",
      "a81831181e1832d81e82050218330f18341902581835071836d81e82010a18371903e81838182c",
      "a81831181e1832d81e82050218330f18341902581835f61836d81e82010a18371903e81838182c",
      "ab1826f61827d81e82011418281903e818291907d0182a190bb8182b1901f4182cd81e820305182d1a00010000182e1a000186a0182f821a00d59f801b00000002540be40018301a00032000",
    },
    cardano_protocol_param_update_from_cbor,
    cardano_protocol_param_update_to_cbor,
    cardano_protocol_param_update_deep_clone,
    cardano_protocol_param_update_unref);
}

TEST(cardano_drep_voting_thresholds_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909",
    },
    cardano_drep_voting_thresholds_from_cbor,
    cardano_drep_voting_thresholds_to_cbor,
    cardano_drep_voting_thresholds_deep_clone,
    cardano_drep_voting_thresholds_unref);
}

TEST(cardano_costmdls_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a118fe820102",
    },
    cardano_costmdls_from_cbor,
    cardano_costmdls_to_cbor,
    cardano_costmdls_deep_clone,
    cardano_costmdls_unref);
}

TEST(cardano_cost_model_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "0098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a",
      "0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a",
      "0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304",
      "0398b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304",
      "0083010203",
      "009f010203ff",
      "0483010203",
      "1818820a0b",
      "18ff8105",
    },
    cardano_cost_model_from_cbor,
    cardano_cost_model_to_cbor,
    cardano_cost_model_deep_clone,
    cardano_cost_model_unref);
}

TEST(cardano_update_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba19020b",
    },
    cardano_update_from_cbor,
    cardano_update_to_cbor,
    cardano_update_deep_clone,
    cardano_update_unref);
}

TEST(cardano_unit_interval_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d81e820105",
    },
    cardano_unit_interval_from_cbor,
    cardano_unit_interval_to_cbor,
    cardano_unit_interval_deep_clone,
    cardano_unit_interval_unref);
}

TEST(cardano_guard_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39",
      "d9010281581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39",
      "828200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d398201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37",
      "d90102828200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d398201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37",
      "828201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39",
      "818200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39",
    },
    cardano_guard_set_from_cbor,
    cardano_guard_set_to_cbor,
    cardano_guard_set_deep_clone,
    cardano_guard_set_unref);
}

TEST(cardano_anchor_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_anchor_from_cbor,
    cardano_anchor_to_cbor,
    cardano_anchor_deep_clone,
    cardano_anchor_unref);
}

TEST(cardano_governance_action_id_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "825820000000000000000000000000000000000000000000000000000000000000000003",
    },
    cardano_governance_action_id_from_cbor,
    cardano_governance_action_id_to_cbor,
    cardano_governance_action_id_deep_clone,
    cardano_governance_action_id_unref);
}

TEST(cardano_datum_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8201d81849d8799f0102030405ff",
      "820058200000000000000000000000000000000000000000000000000000000000000000",
      "8201d818421865",
      "8201d818449f0102ff",
    },
    cardano_datum_from_cbor,
    cardano_datum_to_cbor,
    cardano_datum_deep_clone,
    cardano_datum_unref);
}

TEST(cardano_credential_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8200581c00000000000000000000000000000000000000000000000000000000",
    },
    cardano_credential_from_cbor,
    cardano_credential_to_cbor,
    cardano_credential_deep_clone,
    cardano_credential_unref);
}

TEST(cardano_ex_units_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "821b000086788ffc4e831b00015060e9e46451",
    },
    cardano_ex_units_from_cbor,
    cardano_ex_units_to_cbor,
    cardano_ex_units_deep_clone,
    cardano_ex_units_unref);
}

TEST(cardano_utxo_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a",
      "82825820bb217abaca60fc0ca78c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a",
      "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a",
    },
    cardano_utxo_from_cbor,
    cardano_utxo_to_cbor,
    cardano_utxo_deep_clone,
    cardano_utxo_unref);
}

TEST(cardano_protocol_version_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "820103",
    },
    cardano_protocol_version_from_cbor,
    cardano_protocol_version_to_cbor,
    cardano_protocol_version_deep_clone,
    cardano_protocol_version_unref);
}

TEST(cardano_drep_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8200581c00000000000000000000000000000000000000000000000000000000",
      "8201581c00000000000000000000000000000000000000000000000000000000",
      "8102",
      "8103",
    },
    cardano_drep_from_cbor,
    cardano_drep_to_cbor,
    cardano_drep_deep_clone,
    cardano_drep_unref);
}

TEST(cardano_withdrawal_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005",
    },
    cardano_withdrawal_map_from_cbor,
    cardano_withdrawal_map_to_cbor,
    cardano_withdrawal_map_deep_clone,
    cardano_withdrawal_map_unref);
}

TEST(cardano_bootstrap_witness_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0",
    },
    cardano_bootstrap_witness_from_cbor,
    cardano_bootstrap_witness_to_cbor,
    cardano_bootstrap_witness_deep_clone,
    cardano_bootstrap_witness_unref);
}

TEST(cardano_plutus_data_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90102849f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff",
      "849f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff",
      "9f01029f0102030405ff9f0102030405ff05ff",
      "9f01029f0102030405ff9f0102030405ff05ff",
      "9f01029f0102030405ff9f0102030405ff05ff",
      "9f01029f0102030405ff9f0102030405ff05ff",
    },
    cardano_plutus_data_set_from_cbor,
    cardano_plutus_data_set_to_cbor,
    cardano_plutus_data_set_deep_clone,
    cardano_plutus_data_set_unref,
    cardano_plutus_data_set_clear_cbor_cache);
}

TEST(cardano_plutus_v3_script_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "814746010000220010",
    },
    cardano_plutus_v3_script_set_from_cbor,
    cardano_plutus_v3_script_set_to_cbor,
    cardano_plutus_v3_script_set_deep_clone,
    cardano_plutus_v3_script_set_unref);
}

TEST(cardano_vkey_witness_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
    },
    cardano_vkey_witness_from_cbor,
    cardano_vkey_witness_to_cbor,
    cardano_vkey_witness_deep_clone,
    cardano_vkey_witness_unref);
}

TEST(cardano_plutus_v2_script_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "814746010000220010",
    },
    cardano_plutus_v2_script_set_from_cbor,
    cardano_plutus_v2_script_set_to_cbor,
    cardano_plutus_v2_script_set_deep_clone,
    cardano_plutus_v2_script_set_unref);
}

TEST(cardano_native_script_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90102848200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37",
      "848200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37",
    },
    cardano_native_script_set_from_cbor,
    cardano_native_script_set_to_cbor,
    cardano_native_script_set_deep_clone,
    cardano_native_script_set_unref);
}

TEST(cardano_plutus_v4_script_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "814746010000220010",
    },
    cardano_plutus_v4_script_set_from_cbor,
    cardano_plutus_v4_script_set_to_cbor,
    cardano_plutus_v4_script_set_deep_clone,
    cardano_plutus_v4_script_set_unref);
}

TEST(cardano_bootstrap_witness_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90102848458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0",
      "848458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0",
      "d90102818458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0",
      "d90102818458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c5840111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111115820000000000000000000000000000000000000000000000000000000000000000041a0",
      "d90102838458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c5840111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111115820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a0",
      "d90102828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a0",
    },
    cardano_bootstrap_witness_set_from_cbor,
    cardano_bootstrap_witness_set_to_cbor,
    cardano_bootstrap_witness_set_deep_clone,
    cardano_bootstrap_witness_set_unref,
    cardano_bootstrap_witness_set_clear_cbor_cache);
}

TEST(cardano_vkey_witness_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90102848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
      "848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
      "d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
      "d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c584011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
      "d90102838258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c5840111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111118258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
      "d90102828258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c5840111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111118258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
    },
    cardano_vkey_witness_set_from_cbor,
    cardano_vkey_witness_set_to_cbor,
    cardano_vkey_witness_set_deep_clone,
    cardano_vkey_witness_set_unref,
    cardano_vkey_witness_set_clear_cbor_cache);
}

TEST(cardano_plutus_v1_script_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "814746010000220010",
    },
    cardano_plutus_v1_script_set_from_cbor,
    cardano_plutus_v1_script_set_to_cbor,
    cardano_plutus_v1_script_set_deep_clone,
    cardano_plutus_v1_script_set_unref);
}

TEST(cardano_redeemer_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "840000d8799f0102030405ff821821182c",
      "840600d8799f0102030405ff8218371842",
      "84001b0000000100000000d8799f0102030405ff821821182c",
    },
    cardano_redeemer_from_cbor,
    cardano_redeemer_to_cbor,
    cardano_redeemer_deep_clone,
    cardano_redeemer_unref,
    cardano_redeemer_clear_cbor_cache);
}

TEST(cardano_redeemer_list_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a482000082d8799f0102030405ff821821182c82010182d8799f0102030405ff821821182c82030382d8799f0102030405ff821821182c82040482d8799f0102030405ff821821182c",
      "a482000182d8799f0102030405ff821821182c82000082d8799f0102030405ff821821182c82000182d8799f0102030405ff821821182c82000382d8799f0102030405ff821821182c82000482d8799f0102030405ff821821182c",
      "84840000d8799f0102030405ff821821182c840101d8799f0102030405ff821821182c840303d8799f0102030405ff821821182c840404d8799f0102030405ff821821182c",
      "a282000082d8799f0102030405ff821821182c82060082d8799f0102030405ff8218371842",
      "82840000d8799f0102030405ff821821182c840400d8799f0102030405ff8218371842",
      "a282000082d8799f0102030405ff821821182c82040082d8799f0102030405ff8218371842",
      "a282000082d8799f0102030405ff821821182c82000082d8799f0102030405ff8218371842",
    },
    cardano_redeemer_list_from_cbor,
    cardano_redeemer_list_to_cbor,
    cardano_redeemer_list_deep_clone,
    cardano_redeemer_list_unref,
    cardano_redeemer_list_clear_cbor_cache);
}

TEST(cardano_witness_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a100838258204a352f53eb4311d552aa9e1c6f0125846a3b607011d691f0e774d893d940b8525840c4f13cc397a50193061ce899b3eda906ad1adf3f3d515b52248ea5aa142781cd9c2ccc52ac62b2e1b5226de890104ec530bda4c38a19b691946da9addb3213f5825820290c08454c58a8c7fad6351e65a652460bd4f80f485f1ccfc350ff6a4d5bd4de5840026f47bab2f24da9690746bdb0e55d53a5eef45a969e3dd2873a3e6bb8ef3316d9f80489bacfd2f543108e284a40847ae7ce33fa358fcfe439a37990ad3107e98258204d953d6a9d556da3f3e26622c725923130f5733d1a3c4013ef8c34d15a070fd75840f9218e5a569c5ace38b1bb81e1f1c0b2d7fea2fe7fb913fdd06d79906436103345347a81494b83f83bf43466b0cebdbbdcef15384f67c255e826c249336ce2c7",
      "a700838258204a352f53eb4311d552aa9e1c6f0125846a3b607011d691f0e774d893d940b8525840c4f13cc397a50193061ce899b3eda906ad1adf3f3d515b52248ea5aa142781cd9c2ccc52ac62b2e1b5226de890104ec530bda4c38a19b691946da9addb3213f5825820290c08454c58a8c7fad6351e65a652460bd4f80f485f1ccfc350ff6a4d5bd4de5840026f47bab2f24da9690746bdb0e55d53a5eef45a969e3dd2873a3e6bb8ef3316d9f80489bacfd2f543108e284a40847ae7ce33fa358fcfe439a37990ad3107e98258204d953d6a9d556da3f3e26622c725923130f5733d1a3c4013ef8c34d15a070fd75840f9218e5a569c5ace38b1bb81e1f1c0b2d7fea2fe7fb913fdd06d79906436103345347a81494b83f83bf43466b0cebdbbdcef15384f67c255e826c249336ce2c701848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e863895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a00384474601000022001047460100002200114746010000220012474601000022001304833bffffffffffffffff3bffffffffffffffff3bffffffffffffffff0584840000d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840100d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840200d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840300d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645106844746010000220010474601000022001147460100002200124746010000220013",
      "a800d90102838258204a352f53eb4311d552aa9e1c6f0125846a3b607011d691f0e774d893d940b8525840c4f13cc397a50193061ce899b3eda906ad1adf3f3d515b52248ea5aa142781cd9c2ccc52ac62b2e1b5226de890104ec530bda4c38a19b691946da9addb3213f5825820290c08454c58a8c7fad6351e65a652460bd4f80f485f1ccfc350ff6a4d5bd4de5840026f47bab2f24da9690746bdb0e55d53a5eef45a969e3dd2873a3e6bb8ef3316d9f80489bacfd2f543108e284a40847ae7ce33fa358fcfe439a37990ad3107e98258204d953d6a9d556da3f3e26622c725923130f5733d1a3c4013ef8c34d15a070fd75840f9218e5a569c5ace38b1bb81e1f1c0b2d7fea2fe7fb913fdd06d79906436103345347a81494b83f83bf43466b0cebdbbdcef15384f67c255e826c249336ce2c701d90102848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402d90102828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e863895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a003d9010284474601000022001047460100002200114746010000220012474601000022001304d90102833bffffffffffffffff3bffffffffffffffff3bffffffffffffffff05a482000082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645182010082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645182020082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645182030082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645106d9010284474601000022001047460100002200114746010000220012474601000022001307d90102844746010000220010474601000022001147460100002200124746010000220013",
      "a108d90102814e4d01000033222220051200120011",
      "a108814e4d01000033222220051200120011",
      "a208d90102814e4d0100003322222005120012001107d90102814e4d01000033222220051200120011",
      "a207d90102814e4d0100003322222005120012001108d90102814e4d01000033222220051200120011",
      "a702828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e863895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a000838258204a352f53eb4311d552aa9e1c6f0125846a3b607011d691f0e774d893d940b8525840c4f13cc397a50193061ce899b3eda906ad1adf3f3d515b52248ea5aa142781cd9c2ccc52ac62b2e1b5226de890104ec530bda4c38a19b691946da9addb3213f5825820290c08454c58a8c7fad6351e65a652460bd4f80f485f1ccfc350ff6a4d5bd4de5840026f47bab2f24da9690746bdb0e55d53a5eef45a969e3dd2873a3e6bb8ef3316d9f80489bacfd2f543108e284a40847ae7ce33fa358fcfe439a37990ad3107e98258204d953d6a9d556da3f3e26622c725923130f5733d1a3c4013ef8c34d15a070fd75840f9218e5a569c5ace38b1bb81e1f1c0b2d7fea2fe7fb913fdd06d79906436103345347a81494b83f83bf43466b0cebdbbdcef15384f67c255e826c249336ce2c70584840000d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840100d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840200d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840300d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645101848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540384474601000022001047460100002200114746010000220012474601000022001304833bffffffffffffffff3bffffffffffffffff3bffffffffffffffff06844746010000220010474601000022001147460100002200124746010000220013",
      "a700d90102838258204a352f53eb4311d552aa9e1c6f0125846a3b607011d691f0e774d893d940b8525840c4f13cc397a50193061ce899b3eda906ad1adf3f3d515b52248ea5aa142781cd9c2ccc52ac62b2e1b5226de890104ec530bda4c38a19b691946da9addb3213f5825820290c08454c58a8c7fad6351e65a652460bd4f80f485f1ccfc350ff6a4d5bd4de5840026f47bab2f24da9690746bdb0e55d53a5eef45a969e3dd2873a3e6bb8ef3316d9f80489bacfd2f543108e284a40847ae7ce33fa358fcfe439a37990ad3107e98258204d953d6a9d556da3f3e26622c725923130f5733d1a3c4013ef8c34d15a070fd75840f9218e5a569c5ace38b1bb81e1f1c0b2d7fea2fe7fb913fdd06d79906436103345347a81494b83f83bf43466b0cebdbbdcef15384f67c255e826c249336ce2c701d90102848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402d90102828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e863895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a003d9010284474601000022001047460100002200114746010000220012474601000022001304d90102833bffffffffffffffff3bffffffffffffffff3bffffffffffffffff05a482000082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645182010082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645182020082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645182030082d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645106d90102844746010000220010474601000022001147460100002200124746010000220013",
      "a601848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e863895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a00384474601000022001047460100002200114746010000220012474601000022001304833bffffffffffffffff3bffffffffffffffff3bffffffffffffffff0584840000d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840100d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840200d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840300d8799f0102030405ff821b000086788ffc4e831b00015060e9e4645106844746010000220010474601000022001147460100002200124746010000220013",
      "a701848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402828458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e863895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff41a00384474601000022001047460100002200114746010000220012474601000022001304833bffffffffffffffff3bffffffffffffffff3bffffffffffffffff0584840000d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840100d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840200d8799f0102030405ff821b000086788ffc4e831b00015060e9e46451840300d8799f0102030405ff821b000086788ffc4e831b00015060e9e464510684474601000022001047460100002200114746010000220012474601000022001300d90102848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
      "a200d90102848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a02d90102848458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0",
      "a200d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a02d90102818458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0",
    },
    cardano_witness_set_from_cbor,
    cardano_witness_set_to_cbor,
    cardano_witness_set_deep_clone,
    cardano_witness_set_unref,
    cardano_witness_set_clear_cbor_cache);
}

TEST(cardano_update_committee_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105",
      "8504f6d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105",
    },
    cardano_update_committee_action_from_cbor,
    cardano_update_committee_action_to_cbor,
    cardano_update_committee_action_deep_clone,
    cardano_update_committee_action_unref);
}

TEST(cardano_committee_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003d81e820502",
    },
    cardano_committee_from_cbor,
    cardano_committee_to_cbor,
    cardano_committee_deep_clone,
    cardano_committee_unref);
}

TEST(cardano_parameter_change_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d",
      "8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d",
      "8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388f6",
    },
    cardano_parameter_change_action_from_cbor,
    cardano_parameter_change_action_to_cbor,
    cardano_parameter_change_action_deep_clone,
    cardano_parameter_change_action_unref);
}

TEST(cardano_hard_fork_initiation_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8301825820000000000000000000000000000000000000000000000000000000000000000003820103",
      "8301f6820103",
    },
    cardano_hard_fork_initiation_action_from_cbor,
    cardano_hard_fork_initiation_action_to_cbor,
    cardano_hard_fork_initiation_action_deep_clone,
    cardano_hard_fork_initiation_action_unref);
}

TEST(cardano_no_confidence_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8203825820000000000000000000000000000000000000000000000000000000000000000003",
      "8203f6",
    },
    cardano_no_confidence_action_from_cbor,
    cardano_no_confidence_action_to_cbor,
    cardano_no_confidence_action_deep_clone,
    cardano_no_confidence_action_unref);
}

TEST(cardano_new_constitution_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6",
      "8305f682827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6",
    },
    cardano_new_constitution_action_from_cbor,
    cardano_new_constitution_action_to_cbor,
    cardano_new_constitution_action_deep_clone,
    cardano_new_constitution_action_unref);
}

TEST(cardano_treasury_withdrawals_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d",
      "8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01f6",
    },
    cardano_treasury_withdrawals_action_from_cbor,
    cardano_treasury_withdrawals_action_to_cbor,
    cardano_treasury_withdrawals_action_deep_clone,
    cardano_treasury_withdrawals_action_unref);
}

TEST(cardano_info_action_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "8106",
    },
    cardano_info_action_from_cbor,
    cardano_info_action_to_cbor,
    cardano_info_action_deep_clone,
    cardano_info_action_unref);
}

TEST(cardano_constitution_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6",
      "82827668747470733a2f2f7777772e736f6d6575726c2e696f5820000000000000000000000000000000000000000000000000000000000000000058200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_constitution_from_cbor,
    cardano_constitution_to_cbor,
    cardano_constitution_deep_clone,
    cardano_constitution_unref);
}

TEST(cardano_proposal_procedure_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_proposal_procedure_from_cbor,
    cardano_proposal_procedure_to_cbor,
    cardano_proposal_procedure_deep_clone,
    cardano_proposal_procedure_unref);
}

TEST(cardano_committee_members_map_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003",
    },
    cardano_committee_members_map_from_cbor,
    cardano_committee_members_map_to_cbor,
    cardano_committee_members_map_deep_clone,
    cardano_committee_members_map_unref);
}

TEST(cardano_proposal_procedure_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d9010284841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
      "84841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000",
    },
    cardano_proposal_procedure_set_from_cbor,
    cardano_proposal_procedure_set_to_cbor,
    cardano_proposal_procedure_set_deep_clone,
    cardano_proposal_procedure_set_unref);
}

TEST(cardano_credential_set_deep_clone, preservesEncodingAndRollsBackAllocationFailures)
{
  // Fixtures from the existing native serialization tests.
  verify_cbor_deep_clone(
    {
      "d90102848200581c000000000000000000000000000000000000000000000000000000008200581c100000000000000000000000000000000000000000000000000000008200581c200000000000000000000000000000000000000000000000000000008200581c30000000000000000000000000000000000000000000000000000000",
      "848200581c000000000000000000000000000000000000000000000000000000008200581c100000000000000000000000000000000000000000000000000000008200581c200000000000000000000000000000000000000000000000000000008200581c30000000000000000000000000000000000000000000000000000000",
    },
    cardano_credential_set_from_cbor,
    cardano_credential_set_to_cbor,
    cardano_credential_set_deep_clone,
    cardano_credential_set_unref);
}

TEST(cardano_ed25519_public_key_deep_clone, ownsIndependentKeyBytes)
{
  // Arrange
  const std::string             hex(32U * 2U, '0');
  cardano_ed25519_public_key_t* raw_source = nullptr;
  ASSERT_EQ(cardano_ed25519_public_key_from_hex(hex.data(), hex.size(), &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_ed25519_public_key_unref);

  verify_clone_allocation_failures(source.get(), cardano_ed25519_public_key_deep_clone, cardano_ed25519_public_key_unref);

  // Act
  cardano_ed25519_public_key_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_ed25519_public_key_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_ed25519_public_key_unref);

  // Assert
  EXPECT_NE(cardano_ed25519_public_key_get_data(source.get()), cardano_ed25519_public_key_get_data(copy.get()));
  EXPECT_EQ(memcmp(cardano_ed25519_public_key_get_data(source.get()), cardano_ed25519_public_key_get_data(copy.get()), 32U), 0);
  source.reset();

  const std::vector<byte_t> expected(32U, 0U);
  EXPECT_EQ(memcmp(cardano_ed25519_public_key_get_data(copy.get()), expected.data(), expected.size()), 0);
}

TEST(cardano_ed25519_private_key_deep_clone, ownsIndependentKeyBytes)
{
  // Arrange
  const std::string              hex(32U * 2U, '0');
  cardano_ed25519_private_key_t* raw_source = nullptr;
  ASSERT_EQ(cardano_ed25519_private_key_from_normal_hex(hex.data(), hex.size(), &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_ed25519_private_key_unref);

  verify_clone_allocation_failures(source.get(), cardano_ed25519_private_key_deep_clone, cardano_ed25519_private_key_unref);

  // Act
  cardano_ed25519_private_key_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_ed25519_private_key_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_ed25519_private_key_unref);

  // Assert
  EXPECT_NE(cardano_ed25519_private_key_get_data(source.get()), cardano_ed25519_private_key_get_data(copy.get()));
  EXPECT_EQ(memcmp(cardano_ed25519_private_key_get_data(source.get()), cardano_ed25519_private_key_get_data(copy.get()), 32U), 0);
  source.reset();

  const std::vector<byte_t> expected(32U, 0U);
  EXPECT_EQ(memcmp(cardano_ed25519_private_key_get_data(copy.get()), expected.data(), expected.size()), 0);
}

TEST(cardano_bip32_public_key_deep_clone, ownsIndependentKeyBytes)
{
  // Arrange
  const std::string           hex(64U * 2U, '0');
  cardano_bip32_public_key_t* raw_source = nullptr;
  ASSERT_EQ(cardano_bip32_public_key_from_hex(hex.data(), hex.size(), &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_bip32_public_key_unref);

  verify_clone_allocation_failures(source.get(), cardano_bip32_public_key_deep_clone, cardano_bip32_public_key_unref);

  // Act
  cardano_bip32_public_key_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_bip32_public_key_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_bip32_public_key_unref);

  // Assert
  EXPECT_NE(cardano_bip32_public_key_get_data(source.get()), cardano_bip32_public_key_get_data(copy.get()));
  EXPECT_EQ(memcmp(cardano_bip32_public_key_get_data(source.get()), cardano_bip32_public_key_get_data(copy.get()), 64U), 0);
  source.reset();

  const std::vector<byte_t> expected(64U, 0U);
  EXPECT_EQ(memcmp(cardano_bip32_public_key_get_data(copy.get()), expected.data(), expected.size()), 0);
}

TEST(cardano_bip32_private_key_deep_clone, ownsIndependentKeyBytes)
{
  // Arrange
  const std::string            hex(96U * 2U, '0');
  cardano_bip32_private_key_t* raw_source = nullptr;
  ASSERT_EQ(cardano_bip32_private_key_from_hex(hex.data(), hex.size(), &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_bip32_private_key_unref);

  verify_clone_allocation_failures(source.get(), cardano_bip32_private_key_deep_clone, cardano_bip32_private_key_unref);

  // Act
  cardano_bip32_private_key_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_bip32_private_key_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_bip32_private_key_unref);

  // Assert
  EXPECT_NE(cardano_bip32_private_key_get_data(source.get()), cardano_bip32_private_key_get_data(copy.get()));
  EXPECT_EQ(memcmp(cardano_bip32_private_key_get_data(source.get()), cardano_bip32_private_key_get_data(copy.get()), 96U), 0);
  source.reset();

  const std::vector<byte_t> expected(96U, 0U);
  EXPECT_EQ(memcmp(cardano_bip32_private_key_get_data(copy.get()), expected.data(), expected.size()), 0);
}

TEST(cardano_ed25519_signature_deep_clone, ownsIndependentKeyBytes)
{
  // Arrange
  const std::string            hex(64U * 2U, '0');
  cardano_ed25519_signature_t* raw_source = nullptr;
  ASSERT_EQ(cardano_ed25519_signature_from_hex(hex.data(), hex.size(), &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_ed25519_signature_unref);

  verify_clone_allocation_failures(source.get(), cardano_ed25519_signature_deep_clone, cardano_ed25519_signature_unref);

  // Act
  cardano_ed25519_signature_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_ed25519_signature_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_ed25519_signature_unref);

  // Assert
  EXPECT_NE(cardano_ed25519_signature_get_data(source.get()), cardano_ed25519_signature_get_data(copy.get()));
  EXPECT_EQ(memcmp(cardano_ed25519_signature_get_data(source.get()), cardano_ed25519_signature_get_data(copy.get()), 64U), 0);
  source.reset();

  const std::vector<byte_t> expected(64U, 0U);
  EXPECT_EQ(memcmp(cardano_ed25519_signature_get_data(copy.get()), expected.data(), expected.size()), 0);
}

TEST(cardano_buffer_deep_clone, preservesCapacityAndReadPositionWithoutSharingBytes)
{
  const byte_t bytes[] = { 1U, 2U, 3U, 4U };
  auto         source  = own_clone_test_object(cardano_buffer_new_from(bytes, sizeof(bytes)), cardano_buffer_unref);
  byte_t       first   = 0U;
  ASSERT_EQ(cardano_buffer_read(source.get(), &first, 1U), CARDANO_SUCCESS);
  verify_clone_allocation_failures(source.get(), cardano_buffer_deep_clone, cardano_buffer_unref);

  cardano_buffer_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_buffer_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_buffer_unref);

  EXPECT_EQ(cardano_buffer_get_capacity(copy.get()), cardano_buffer_get_capacity(source.get()));
  EXPECT_NE(cardano_buffer_get_data(copy.get()), cardano_buffer_get_data(source.get()));
  cardano_buffer_get_data(copy.get())[1] = 99U;
  EXPECT_EQ(cardano_buffer_get_data(source.get())[1], 2U);
  ASSERT_EQ(cardano_buffer_read(copy.get(), &first, 1U), CARDANO_SUCCESS);
  EXPECT_EQ(first, 99U);
  ASSERT_EQ(cardano_buffer_read(source.get(), &first, 1U), CARDANO_SUCCESS);
  EXPECT_EQ(first, 2U);
}

TEST(cardano_transaction_deep_clone, mutationsDoNotCrossTransactionBoundaries)
{
  const char*            cbor       = "84a3008001800200a0f5f6";
  auto                   reader     = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_transaction_t* raw_source = nullptr;
  ASSERT_EQ(cardano_transaction_from_cbor(reader.get(), &raw_source), CARDANO_SUCCESS);
  auto                   source   = own_clone_test_object(raw_source, cardano_transaction_unref);
  cardano_transaction_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_transaction_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy        = own_clone_test_object(raw_copy, cardano_transaction_unref);
  auto source_body = own_clone_test_object(cardano_transaction_get_body(source.get()), cardano_transaction_body_unref);
  auto copy_body   = own_clone_test_object(cardano_transaction_get_body(copy.get()), cardano_transaction_body_unref);

  auto original_hash = own_clone_test_object(cardano_transaction_get_id(source.get()), cardano_blake2b_hash_unref);
  auto cloned_hash   = own_clone_test_object(cardano_transaction_get_id(copy.get()), cardano_blake2b_hash_unref);
  ASSERT_NE(original_hash, nullptr);
  ASSERT_NE(cloned_hash, nullptr);
  EXPECT_EQ(cardano_blake2b_hash_compare(original_hash.get(), cloned_hash.get()), 0);

  EXPECT_NE(source_body.get(), copy_body.get());
  EXPECT_EQ(cardano_transaction_body_set_fee(copy_body.get(), 42U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_get_fee(source_body.get()), 0U);
  EXPECT_EQ(cardano_transaction_body_get_fee(copy_body.get()), 42U);
  EXPECT_EQ(cardano_transaction_body_set_fee(source_body.get(), 7U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_get_fee(copy_body.get()), 42U);

  cardano_transaction_clear_cbor_cache(source.get());
  cardano_transaction_clear_cbor_cache(copy.get());
  auto source_hash = own_clone_test_object(cardano_transaction_get_id(source.get()), cardano_blake2b_hash_unref);
  auto copy_hash   = own_clone_test_object(cardano_transaction_get_id(copy.get()), cardano_blake2b_hash_unref);
  EXPECT_NE(cardano_blake2b_hash_compare(source_hash.get(), copy_hash.get()), 0);
}

TEST(cardano_plutus_data_deep_clone, nestedListsDoNotShareMutableChildren)
{
  const char*            cbor       = "818101";
  auto                   reader     = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_data_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_data_from_cbor(reader.get(), &raw_source), CARDANO_SUCCESS);
  auto                   source   = own_clone_test_object(raw_source, cardano_plutus_data_unref);
  cardano_plutus_data_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_data_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_plutus_data_unref);

  cardano_plutus_list_t* raw_source_list = nullptr;
  cardano_plutus_list_t* raw_copy_list   = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_list(source.get(), &raw_source_list), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_data_to_list(copy.get(), &raw_copy_list), CARDANO_SUCCESS);
  auto source_list = own_clone_test_object(raw_source_list, cardano_plutus_list_unref);
  auto copy_list   = own_clone_test_object(raw_copy_list, cardano_plutus_list_unref);
  EXPECT_NE(source_list.get(), copy_list.get());

  cardano_plutus_data_t* raw_source_child = nullptr;
  cardano_plutus_data_t* raw_copy_child   = nullptr;
  ASSERT_EQ(cardano_plutus_list_get(source_list.get(), 0U, &raw_source_child), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_get(copy_list.get(), 0U, &raw_copy_child), CARDANO_SUCCESS);
  auto source_child = own_clone_test_object(raw_source_child, cardano_plutus_data_unref);
  auto copy_child   = own_clone_test_object(raw_copy_child, cardano_plutus_data_unref);
  EXPECT_NE(source_child.get(), copy_child.get());

  cardano_plutus_list_t* raw_nested = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_list(copy_child.get(), &raw_nested), CARDANO_SUCCESS);
  auto                   nested     = own_clone_test_object(raw_nested, cardano_plutus_list_unref);
  cardano_plutus_data_t* raw_number = nullptr;
  ASSERT_EQ(cardano_plutus_data_new_integer_from_int(9, &raw_number), CARDANO_SUCCESS);
  auto number = own_clone_test_object(raw_number, cardano_plutus_data_unref);
  ASSERT_EQ(cardano_plutus_list_add(nested.get(), number.get()), CARDANO_SUCCESS);
  cardano_plutus_data_clear_cbor_cache(copy.get());

  EXPECT_EQ(clone_test_cbor(source.get(), cardano_plutus_data_to_cbor), "818101");
  EXPECT_EQ(clone_test_cbor(copy.get(), cardano_plutus_data_to_cbor), "9f9f0109ffff");
}

TEST(cardano_protocol_parameters_deep_clone, copiesDefaultParametersAndOwnedValues)
{
  cardano_protocol_parameters_t* raw_source = nullptr;
  ASSERT_EQ(cardano_protocol_parameters_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_protocol_parameters_unref);
  verify_clone_allocation_failures(source.get(), cardano_protocol_parameters_deep_clone, cardano_protocol_parameters_unref);
  cardano_protocol_parameters_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_protocol_parameters_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy             = own_clone_test_object(raw_copy, cardano_protocol_parameters_unref);
  auto source_influence = own_clone_test_object(cardano_protocol_parameters_get_pool_pledge_influence(source.get()), cardano_unit_interval_unref);
  auto copy_influence   = own_clone_test_object(cardano_protocol_parameters_get_pool_pledge_influence(copy.get()), cardano_unit_interval_unref);
  auto source_models    = own_clone_test_object(cardano_protocol_parameters_get_cost_models(source.get()), cardano_costmdls_unref);
  auto copy_models      = own_clone_test_object(cardano_protocol_parameters_get_cost_models(copy.get()), cardano_costmdls_unref);
  EXPECT_NE(source_influence.get(), copy_influence.get());
  EXPECT_NE(source_models.get(), copy_models.get());
  ASSERT_EQ(cardano_protocol_parameters_set_min_fee_a(copy.get(), 42U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_min_fee_a(source.get()), 0U);
  EXPECT_EQ(cardano_protocol_parameters_get_min_fee_a(copy.get()), 42U);
}

TEST(cardano_json_object_deep_clone, preservesNestedValuesAndCachedText)
{
  const char* json   = "{\"amount\":18446744073709551615,\"values\":[null,true,-3,1.25,\"text\"]}";
  auto        source = own_clone_test_object(cardano_json_object_parse(json, strlen(json)), cardano_json_object_unref);
  ASSERT_NE(source, nullptr);
  const std::string before = cardano_json_object_to_json_string(source.get(), CARDANO_JSON_FORMAT_COMPACT, nullptr);
  verify_clone_allocation_failures(source.get(), cardano_json_object_deep_clone, cardano_json_object_unref);
  cardano_json_object_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_json_object_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy         = own_clone_test_object(raw_copy, cardano_json_object_unref);
  auto source_child = own_clone_test_object(cardano_json_object_get_value_at(source.get(), 1U), cardano_json_object_unref);
  auto copy_child   = own_clone_test_object(cardano_json_object_get_value_at(copy.get(), 1U), cardano_json_object_unref);
  EXPECT_NE(source_child.get(), copy_child.get());
  source.reset();
  EXPECT_EQ(std::string(cardano_json_object_to_json_string(copy.get(), CARDANO_JSON_FORMAT_COMPACT, nullptr)), before);
}

TEST(cardano_bigint_deep_clone, ownsIndependentIntegerStorage)
{
  const char*       decimal    = "184467440737095516160000000000";
  cardano_bigint_t* raw_source = nullptr;
  ASSERT_EQ(cardano_bigint_from_string(decimal, strlen(decimal), 10, &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_bigint_unref);
  verify_clone_allocation_failures(source.get(), cardano_bigint_deep_clone, cardano_bigint_unref);
  cardano_bigint_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_bigint_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_bigint_unref);
  EXPECT_EQ(cardano_bigint_compare(source.get(), copy.get()), 0);
  cardano_bigint_add(source.get(), source.get(), source.get());
  EXPECT_NE(cardano_bigint_compare(source.get(), copy.get()), 0);
}

TEST(cardano_blake2b_hash_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*             cbor        = "581c00000000000000000000000000000000000000000000000000000000";
  auto                    reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_blake2b_hash_t* raw_element = nullptr;
  ASSERT_EQ(cardano_blake2b_hash_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                        element    = own_clone_test_object(raw_element, cardano_blake2b_hash_unref);
  cardano_blake2b_hash_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_blake2b_hash_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_blake2b_hash_set_unref);
  ASSERT_EQ(cardano_blake2b_hash_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_blake2b_hash_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_blake2b_hash_set_deep_clone, cardano_blake2b_hash_set_unref);

  cardano_blake2b_hash_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_blake2b_hash_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                    copy      = own_clone_test_object(raw_copy, cardano_blake2b_hash_set_unref);
  cardano_blake2b_hash_t* raw_child = nullptr;
  ASSERT_EQ(cardano_blake2b_hash_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_blake2b_hash_unref);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_blake2b_hash_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_blake2b_hash_to_cbor), clone_test_cbor(element.get(), cardano_blake2b_hash_to_cbor));
}

TEST(cardano_plutus_v4_script_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "4e4d01000033222220051200120011";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v4_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_plutus_v4_script_unref);
  cardano_plutus_v4_script_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v4_script_list_unref);
  ASSERT_EQ(cardano_plutus_v4_script_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v4_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v4_script_list_deep_clone, cardano_plutus_v4_script_list_unref);

  cardano_plutus_v4_script_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v4_script_list_unref);
  cardano_plutus_v4_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v4_script_unref);

  EXPECT_EQ(cardano_plutus_v4_script_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v4_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v4_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v4_script_to_cbor));
}

TEST(cardano_metadatum_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*          cbor        = "9f01029f0102030405ff9f0102030405ff05ff";
  auto                 reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_metadatum_t* raw_element = nullptr;
  ASSERT_EQ(cardano_metadatum_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                      element    = own_clone_test_object(raw_element, cardano_metadatum_unref);
  cardano_metadatum_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_metadatum_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_metadatum_list_unref);
  ASSERT_EQ(cardano_metadatum_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_metadatum_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_metadatum_list_deep_clone, cardano_metadatum_list_unref);

  cardano_metadatum_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_metadatum_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                 copy      = own_clone_test_object(raw_copy, cardano_metadatum_list_unref);
  cardano_metadatum_t* raw_child = nullptr;
  ASSERT_EQ(cardano_metadatum_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_metadatum_unref);

  EXPECT_EQ(cardano_metadatum_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_metadatum_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_metadatum_to_cbor), clone_test_cbor(element.get(), cardano_metadatum_to_cbor));
}

TEST(cardano_plutus_v3_script_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v3_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_plutus_v3_script_unref);
  cardano_plutus_v3_script_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v3_script_list_unref);
  ASSERT_EQ(cardano_plutus_v3_script_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v3_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v3_script_list_deep_clone, cardano_plutus_v3_script_list_unref);

  cardano_plutus_v3_script_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v3_script_list_unref);
  cardano_plutus_v3_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v3_script_unref);

  EXPECT_EQ(cardano_plutus_v3_script_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v3_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v3_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v3_script_to_cbor));
}

TEST(cardano_plutus_v1_script_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "4e4d01000033222220051200120011";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v1_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_plutus_v1_script_unref);
  cardano_plutus_v1_script_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v1_script_list_unref);
  ASSERT_EQ(cardano_plutus_v1_script_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v1_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v1_script_list_deep_clone, cardano_plutus_v1_script_list_unref);

  cardano_plutus_v1_script_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v1_script_list_unref);
  cardano_plutus_v1_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v1_script_unref);

  EXPECT_EQ(cardano_plutus_v1_script_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v1_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v1_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v1_script_to_cbor));
}

TEST(cardano_plutus_v2_script_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v2_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_plutus_v2_script_unref);
  cardano_plutus_v2_script_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v2_script_list_unref);
  ASSERT_EQ(cardano_plutus_v2_script_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v2_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v2_script_list_deep_clone, cardano_plutus_v2_script_list_unref);

  cardano_plutus_v2_script_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v2_script_list_unref);
  cardano_plutus_v2_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v2_script_unref);

  EXPECT_EQ(cardano_plutus_v2_script_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v2_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v2_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v2_script_to_cbor));
}

TEST(cardano_certificate_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*            cbor        = "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
  auto                   reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_certificate_t* raw_element = nullptr;
  ASSERT_EQ(cardano_certificate_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                       element    = own_clone_test_object(raw_element, cardano_certificate_unref);
  cardano_certificate_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_certificate_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_certificate_set_unref);
  ASSERT_EQ(cardano_certificate_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_certificate_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_certificate_set_deep_clone, cardano_certificate_set_unref);

  cardano_certificate_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_certificate_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                   copy      = own_clone_test_object(raw_copy, cardano_certificate_set_unref);
  cardano_certificate_t* raw_child = nullptr;
  ASSERT_EQ(cardano_certificate_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_certificate_unref);

  EXPECT_EQ(cardano_certificate_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_certificate_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_certificate_to_cbor), clone_test_cbor(element.get(), cardano_certificate_to_cbor));
}

TEST(cardano_voter_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*      cbor        = "8200581c00000000000000000000000000000000000000000000000000000000";
  auto             reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_voter_t* raw_element = nullptr;
  ASSERT_EQ(cardano_voter_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                  element    = own_clone_test_object(raw_element, cardano_voter_unref);
  cardano_voter_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_voter_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_voter_list_unref);
  ASSERT_EQ(cardano_voter_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_voter_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_voter_list_deep_clone, cardano_voter_list_unref);

  cardano_voter_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_voter_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto             copy      = own_clone_test_object(raw_copy, cardano_voter_list_unref);
  cardano_voter_t* raw_child = nullptr;
  ASSERT_EQ(cardano_voter_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_voter_unref);

  EXPECT_EQ(cardano_voter_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_voter_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_voter_to_cbor), clone_test_cbor(element.get(), cardano_voter_to_cbor));
}

TEST(cardano_governance_action_id_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                     cbor        = "825820000000000000000000000000000000000000000000000000000000000000000003";
  auto                            reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_governance_action_id_t* raw_element = nullptr;
  ASSERT_EQ(cardano_governance_action_id_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                                 element    = own_clone_test_object(raw_element, cardano_governance_action_id_unref);
  cardano_governance_action_id_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_governance_action_id_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_governance_action_id_list_unref);
  ASSERT_EQ(cardano_governance_action_id_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_governance_action_id_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_governance_action_id_list_deep_clone, cardano_governance_action_id_list_unref);

  cardano_governance_action_id_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_governance_action_id_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                            copy      = own_clone_test_object(raw_copy, cardano_governance_action_id_list_unref);
  cardano_governance_action_id_t* raw_child = nullptr;
  ASSERT_EQ(cardano_governance_action_id_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_governance_action_id_unref);

  EXPECT_EQ(cardano_governance_action_id_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_governance_action_id_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_governance_action_id_to_cbor), clone_test_cbor(element.get(), cardano_governance_action_id_to_cbor));
}

TEST(cardano_voting_procedure_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "8200f6";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_voting_procedure_t* raw_element = nullptr;
  ASSERT_EQ(cardano_voting_procedure_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_voting_procedure_unref);
  cardano_voting_procedure_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_voting_procedure_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_voting_procedure_list_unref);
  ASSERT_EQ(cardano_voting_procedure_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_voting_procedure_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_voting_procedure_list_deep_clone, cardano_voting_procedure_list_unref);

  cardano_voting_procedure_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_voting_procedure_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_voting_procedure_list_unref);
  cardano_voting_procedure_t* raw_child = nullptr;
  ASSERT_EQ(cardano_voting_procedure_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_voting_procedure_unref);

  EXPECT_EQ(cardano_voting_procedure_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_voting_procedure_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_voting_procedure_to_cbor), clone_test_cbor(element.get(), cardano_voting_procedure_to_cbor));
}

TEST(cardano_plutus_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*            cbor        = "9f01029f0102030405ff9f0102030405ff05ff";
  auto                   reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_data_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_data_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                   element    = own_clone_test_object(raw_element, cardano_plutus_data_unref);
  cardano_plutus_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_list_unref);
  ASSERT_EQ(cardano_plutus_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_data_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_list_deep_clone, cardano_plutus_list_unref);

  cardano_plutus_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                   copy      = own_clone_test_object(raw_copy, cardano_plutus_list_unref);
  cardano_plutus_data_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_data_unref);

  EXPECT_EQ(cardano_plutus_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_data_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_data_to_cbor), clone_test_cbor(element.get(), cardano_plutus_data_to_cbor));
}

TEST(cardano_transaction_input_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                  cbor        = "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102005";
  auto                         reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_transaction_input_t* raw_element = nullptr;
  ASSERT_EQ(cardano_transaction_input_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_transaction_input_unref);
  cardano_transaction_input_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_transaction_input_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_transaction_input_set_unref);
  ASSERT_EQ(cardano_transaction_input_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_transaction_input_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_transaction_input_set_deep_clone, cardano_transaction_input_set_unref);

  cardano_transaction_input_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_transaction_input_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                         copy      = own_clone_test_object(raw_copy, cardano_transaction_input_set_unref);
  cardano_transaction_input_t* raw_child = nullptr;
  ASSERT_EQ(cardano_transaction_input_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_transaction_input_unref);

  EXPECT_EQ(cardano_transaction_input_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_transaction_input_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_transaction_input_to_cbor), clone_test_cbor(element.get(), cardano_transaction_input_to_cbor));
}

TEST(cardano_sub_transaction_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                cbor        = "83a200d90102800180a0f6";
  auto                       reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_sub_transaction_t* raw_element = nullptr;
  ASSERT_EQ(cardano_sub_transaction_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                           element    = own_clone_test_object(raw_element, cardano_sub_transaction_unref);
  cardano_sub_transaction_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_sub_transaction_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_sub_transaction_set_unref);
  ASSERT_EQ(cardano_sub_transaction_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_sub_transaction_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_sub_transaction_set_deep_clone, cardano_sub_transaction_set_unref);

  cardano_sub_transaction_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_sub_transaction_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                       copy      = own_clone_test_object(raw_copy, cardano_sub_transaction_set_unref);
  cardano_sub_transaction_t* raw_child = nullptr;
  ASSERT_EQ(cardano_sub_transaction_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_sub_transaction_unref);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_sub_transaction_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_sub_transaction_to_cbor), clone_test_cbor(element.get(), cardano_sub_transaction_to_cbor));
}

TEST(cardano_transaction_output_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                   cbor        = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  auto                          reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_transaction_output_t* raw_element = nullptr;
  ASSERT_EQ(cardano_transaction_output_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                               element    = own_clone_test_object(raw_element, cardano_transaction_output_unref);
  cardano_transaction_output_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_transaction_output_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_transaction_output_list_unref);
  ASSERT_EQ(cardano_transaction_output_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_transaction_output_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_transaction_output_list_deep_clone, cardano_transaction_output_list_unref);

  cardano_transaction_output_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_transaction_output_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                          copy      = own_clone_test_object(raw_copy, cardano_transaction_output_list_unref);
  cardano_transaction_output_t* raw_child = nullptr;
  ASSERT_EQ(cardano_transaction_output_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_transaction_output_unref);

  EXPECT_EQ(cardano_transaction_output_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_transaction_output_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_transaction_output_to_cbor), clone_test_cbor(element.get(), cardano_transaction_output_to_cbor));
}

TEST(cardano_asset_name_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*           cbor        = "41aa";
  auto                  reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_asset_name_t* raw_element = nullptr;
  ASSERT_EQ(cardano_asset_name_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                       element    = own_clone_test_object(raw_element, cardano_asset_name_unref);
  cardano_asset_name_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_asset_name_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_asset_name_list_unref);
  ASSERT_EQ(cardano_asset_name_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_asset_name_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_asset_name_list_deep_clone, cardano_asset_name_list_unref);

  cardano_asset_name_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_asset_name_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                  copy      = own_clone_test_object(raw_copy, cardano_asset_name_list_unref);
  cardano_asset_name_t* raw_child = nullptr;
  ASSERT_EQ(cardano_asset_name_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_asset_name_unref);

  EXPECT_EQ(cardano_asset_name_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_asset_name_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_asset_name_to_cbor), clone_test_cbor(element.get(), cardano_asset_name_to_cbor));
}

TEST(cardano_policy_id_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*             cbor        = "581c00000000000000000000000000000000000000000000000000000000";
  auto                    reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_blake2b_hash_t* raw_element = nullptr;
  ASSERT_EQ(cardano_blake2b_hash_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                      element    = own_clone_test_object(raw_element, cardano_blake2b_hash_unref);
  cardano_policy_id_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_policy_id_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_policy_id_list_unref);
  ASSERT_EQ(cardano_policy_id_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_blake2b_hash_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_policy_id_list_deep_clone, cardano_policy_id_list_unref);

  cardano_policy_id_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_policy_id_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                    copy      = own_clone_test_object(raw_copy, cardano_policy_id_list_unref);
  cardano_blake2b_hash_t* raw_child = nullptr;
  ASSERT_EQ(cardano_policy_id_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_blake2b_hash_unref);

  EXPECT_EQ(cardano_policy_id_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_blake2b_hash_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_blake2b_hash_to_cbor), clone_test_cbor(element.get(), cardano_blake2b_hash_to_cbor));
}

TEST(cardano_native_script_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*              cbor        = "82041b0000000000000005";
  auto                     reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_native_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_native_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                          element    = own_clone_test_object(raw_element, cardano_native_script_unref);
  cardano_native_script_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_native_script_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_native_script_list_unref);
  ASSERT_EQ(cardano_native_script_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_native_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_native_script_list_deep_clone, cardano_native_script_list_unref);

  cardano_native_script_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_native_script_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                     copy      = own_clone_test_object(raw_copy, cardano_native_script_list_unref);
  cardano_native_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_native_script_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_native_script_unref);

  EXPECT_EQ(cardano_native_script_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_native_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_native_script_to_cbor), clone_test_cbor(element.get(), cardano_native_script_to_cbor));
}

TEST(cardano_pool_owners_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*             cbor        = "581c00000000000000000000000000000000000000000000000000000000";
  auto                    reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_blake2b_hash_t* raw_element = nullptr;
  ASSERT_EQ(cardano_blake2b_hash_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                   element    = own_clone_test_object(raw_element, cardano_blake2b_hash_unref);
  cardano_pool_owners_t* raw_source = nullptr;
  ASSERT_EQ(cardano_pool_owners_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_pool_owners_unref);
  ASSERT_EQ(cardano_pool_owners_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_blake2b_hash_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_pool_owners_deep_clone, cardano_pool_owners_unref);

  cardano_pool_owners_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_pool_owners_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                    copy      = own_clone_test_object(raw_copy, cardano_pool_owners_unref);
  cardano_blake2b_hash_t* raw_child = nullptr;
  ASSERT_EQ(cardano_pool_owners_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_blake2b_hash_unref);

  EXPECT_EQ(cardano_pool_owners_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_blake2b_hash_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_blake2b_hash_to_cbor), clone_test_cbor(element.get(), cardano_blake2b_hash_to_cbor));
}

TEST(cardano_relays_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*      cbor        = "83010a6b6578616d706c652e636f6d";
  auto             reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_relay_t* raw_element = nullptr;
  ASSERT_EQ(cardano_relay_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto              element    = own_clone_test_object(raw_element, cardano_relay_unref);
  cardano_relays_t* raw_source = nullptr;
  ASSERT_EQ(cardano_relays_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_relays_unref);
  ASSERT_EQ(cardano_relays_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_relay_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_relays_deep_clone, cardano_relays_unref);

  cardano_relays_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_relays_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto             copy      = own_clone_test_object(raw_copy, cardano_relays_unref);
  cardano_relay_t* raw_child = nullptr;
  ASSERT_EQ(cardano_relays_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_relay_unref);

  EXPECT_EQ(cardano_relays_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_relay_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_relay_to_cbor), clone_test_cbor(element.get(), cardano_relay_to_cbor));
}

TEST(cardano_guard_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*           cbor        = "8200581c00000000000000000000000000000000000000000000000000000000";
  auto                  reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_credential_t* raw_element = nullptr;
  ASSERT_EQ(cardano_credential_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                 element    = own_clone_test_object(raw_element, cardano_credential_unref);
  cardano_guard_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_guard_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_guard_set_unref);
  ASSERT_EQ(cardano_guard_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_credential_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_guard_set_deep_clone, cardano_guard_set_unref);

  cardano_guard_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_guard_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                  copy      = own_clone_test_object(raw_copy, cardano_guard_set_unref);
  cardano_credential_t* raw_child = nullptr;
  ASSERT_EQ(cardano_guard_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_credential_unref);

  EXPECT_EQ(cardano_guard_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_credential_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_credential_to_cbor), clone_test_cbor(element.get(), cardano_credential_to_cbor));
}

TEST(cardano_utxo_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*     cbor        = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
  auto            reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_utxo_t* raw_element = nullptr;
  ASSERT_EQ(cardano_utxo_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                 element    = own_clone_test_object(raw_element, cardano_utxo_unref);
  cardano_utxo_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_utxo_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_utxo_list_unref);
  ASSERT_EQ(cardano_utxo_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_utxo_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_utxo_list_deep_clone, cardano_utxo_list_unref);

  cardano_utxo_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_utxo_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto            copy      = own_clone_test_object(raw_copy, cardano_utxo_list_unref);
  cardano_utxo_t* raw_child = nullptr;
  ASSERT_EQ(cardano_utxo_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_utxo_unref);

  EXPECT_EQ(cardano_utxo_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_utxo_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_utxo_to_cbor), clone_test_cbor(element.get(), cardano_utxo_to_cbor));
}

TEST(cardano_plutus_data_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*            cbor        = "9f01029f0102030405ff9f0102030405ff05ff";
  auto                   reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_data_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_data_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                       element    = own_clone_test_object(raw_element, cardano_plutus_data_unref);
  cardano_plutus_data_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_data_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_data_set_unref);
  ASSERT_EQ(cardano_plutus_data_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_data_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_data_set_deep_clone, cardano_plutus_data_set_unref);

  cardano_plutus_data_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_data_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                   copy      = own_clone_test_object(raw_copy, cardano_plutus_data_set_unref);
  cardano_plutus_data_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_data_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_data_unref);

  EXPECT_EQ(cardano_plutus_data_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_data_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_data_to_cbor), clone_test_cbor(element.get(), cardano_plutus_data_to_cbor));
}

TEST(cardano_plutus_v3_script_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v3_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                            element    = own_clone_test_object(raw_element, cardano_plutus_v3_script_unref);
  cardano_plutus_v3_script_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v3_script_set_unref);
  ASSERT_EQ(cardano_plutus_v3_script_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v3_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v3_script_set_deep_clone, cardano_plutus_v3_script_set_unref);

  cardano_plutus_v3_script_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v3_script_set_unref);
  cardano_plutus_v3_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v3_script_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v3_script_unref);

  EXPECT_EQ(cardano_plutus_v3_script_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v3_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v3_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v3_script_to_cbor));
}

TEST(cardano_plutus_v2_script_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v2_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                            element    = own_clone_test_object(raw_element, cardano_plutus_v2_script_unref);
  cardano_plutus_v2_script_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v2_script_set_unref);
  ASSERT_EQ(cardano_plutus_v2_script_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v2_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v2_script_set_deep_clone, cardano_plutus_v2_script_set_unref);

  cardano_plutus_v2_script_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v2_script_set_unref);
  cardano_plutus_v2_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v2_script_unref);

  EXPECT_EQ(cardano_plutus_v2_script_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v2_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v2_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v2_script_to_cbor));
}

TEST(cardano_native_script_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*              cbor        = "82041b0000000000000005";
  auto                     reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_native_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_native_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                         element    = own_clone_test_object(raw_element, cardano_native_script_unref);
  cardano_native_script_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_native_script_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_native_script_set_unref);
  ASSERT_EQ(cardano_native_script_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_native_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_native_script_set_deep_clone, cardano_native_script_set_unref);

  cardano_native_script_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_native_script_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                     copy      = own_clone_test_object(raw_copy, cardano_native_script_set_unref);
  cardano_native_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_native_script_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_native_script_unref);

  EXPECT_EQ(cardano_native_script_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_native_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_native_script_to_cbor), clone_test_cbor(element.get(), cardano_native_script_to_cbor));
}

TEST(cardano_plutus_v4_script_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "4e4d01000033222220051200120011";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v4_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                            element    = own_clone_test_object(raw_element, cardano_plutus_v4_script_unref);
  cardano_plutus_v4_script_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v4_script_set_unref);
  ASSERT_EQ(cardano_plutus_v4_script_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v4_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v4_script_set_deep_clone, cardano_plutus_v4_script_set_unref);

  cardano_plutus_v4_script_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v4_script_set_unref);
  cardano_plutus_v4_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v4_script_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v4_script_unref);

  EXPECT_EQ(cardano_plutus_v4_script_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v4_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v4_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v4_script_to_cbor));
}

TEST(cardano_bootstrap_witness_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                  cbor        = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
  auto                         reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_bootstrap_witness_t* raw_element = nullptr;
  ASSERT_EQ(cardano_bootstrap_witness_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                             element    = own_clone_test_object(raw_element, cardano_bootstrap_witness_unref);
  cardano_bootstrap_witness_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_bootstrap_witness_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_bootstrap_witness_set_unref);
  ASSERT_EQ(cardano_bootstrap_witness_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_bootstrap_witness_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_bootstrap_witness_set_deep_clone, cardano_bootstrap_witness_set_unref);

  cardano_bootstrap_witness_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_bootstrap_witness_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                         copy      = own_clone_test_object(raw_copy, cardano_bootstrap_witness_set_unref);
  cardano_bootstrap_witness_t* raw_child = nullptr;
  ASSERT_EQ(cardano_bootstrap_witness_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_bootstrap_witness_unref);

  EXPECT_EQ(cardano_bootstrap_witness_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_bootstrap_witness_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_bootstrap_witness_to_cbor), clone_test_cbor(element.get(), cardano_bootstrap_witness_to_cbor));
}

TEST(cardano_vkey_witness_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*             cbor        = "8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";
  auto                    reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_vkey_witness_t* raw_element = nullptr;
  ASSERT_EQ(cardano_vkey_witness_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                        element    = own_clone_test_object(raw_element, cardano_vkey_witness_unref);
  cardano_vkey_witness_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_vkey_witness_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_vkey_witness_set_unref);
  ASSERT_EQ(cardano_vkey_witness_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_vkey_witness_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_vkey_witness_set_deep_clone, cardano_vkey_witness_set_unref);

  cardano_vkey_witness_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_vkey_witness_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                    copy      = own_clone_test_object(raw_copy, cardano_vkey_witness_set_unref);
  cardano_vkey_witness_t* raw_child = nullptr;
  ASSERT_EQ(cardano_vkey_witness_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_vkey_witness_unref);

  EXPECT_EQ(cardano_vkey_witness_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_vkey_witness_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_vkey_witness_to_cbor), clone_test_cbor(element.get(), cardano_vkey_witness_to_cbor));
}

TEST(cardano_plutus_v1_script_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                 cbor        = "4e4d01000033222220051200120011";
  auto                        reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_plutus_v1_script_t* raw_element = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                            element    = own_clone_test_object(raw_element, cardano_plutus_v1_script_unref);
  cardano_plutus_v1_script_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_plutus_v1_script_set_unref);
  ASSERT_EQ(cardano_plutus_v1_script_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_plutus_v1_script_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_plutus_v1_script_set_deep_clone, cardano_plutus_v1_script_set_unref);

  cardano_plutus_v1_script_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                        copy      = own_clone_test_object(raw_copy, cardano_plutus_v1_script_set_unref);
  cardano_plutus_v1_script_t* raw_child = nullptr;
  ASSERT_EQ(cardano_plutus_v1_script_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_plutus_v1_script_unref);

  EXPECT_EQ(cardano_plutus_v1_script_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_plutus_v1_script_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_plutus_v1_script_to_cbor), clone_test_cbor(element.get(), cardano_plutus_v1_script_to_cbor));
}

TEST(cardano_redeemer_list_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*         cbor        = "840000d8799f0102030405ff821821182c";
  auto                reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_redeemer_t* raw_element = nullptr;
  ASSERT_EQ(cardano_redeemer_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                     element    = own_clone_test_object(raw_element, cardano_redeemer_unref);
  cardano_redeemer_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_redeemer_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_redeemer_list_unref);
  ASSERT_EQ(cardano_redeemer_list_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_redeemer_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_redeemer_list_deep_clone, cardano_redeemer_list_unref);

  cardano_redeemer_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_redeemer_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                copy      = own_clone_test_object(raw_copy, cardano_redeemer_list_unref);
  cardano_redeemer_t* raw_child = nullptr;
  ASSERT_EQ(cardano_redeemer_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_redeemer_unref);

  EXPECT_EQ(cardano_redeemer_list_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_redeemer_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_redeemer_to_cbor), clone_test_cbor(element.get(), cardano_redeemer_to_cbor));
}

TEST(cardano_proposal_procedure_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*                   cbor        = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
  auto                          reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_proposal_procedure_t* raw_element = nullptr;
  ASSERT_EQ(cardano_proposal_procedure_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                              element    = own_clone_test_object(raw_element, cardano_proposal_procedure_unref);
  cardano_proposal_procedure_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_proposal_procedure_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_proposal_procedure_set_unref);
  ASSERT_EQ(cardano_proposal_procedure_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_proposal_procedure_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_proposal_procedure_set_deep_clone, cardano_proposal_procedure_set_unref);

  cardano_proposal_procedure_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_proposal_procedure_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                          copy      = own_clone_test_object(raw_copy, cardano_proposal_procedure_set_unref);
  cardano_proposal_procedure_t* raw_child = nullptr;
  ASSERT_EQ(cardano_proposal_procedure_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_proposal_procedure_unref);

  EXPECT_EQ(cardano_proposal_procedure_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_proposal_procedure_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_proposal_procedure_to_cbor), clone_test_cbor(element.get(), cardano_proposal_procedure_to_cbor));
}

TEST(cardano_credential_set_deep_clone, copiesCollectionElementsWithoutRetainingTheSource)
{
  const char*           cbor        = "8200581c00000000000000000000000000000000000000000000000000000000";
  auto                  reader      = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  cardano_credential_t* raw_element = nullptr;
  ASSERT_EQ(cardano_credential_from_cbor(reader.get(), &raw_element), CARDANO_SUCCESS);
  auto                      element    = own_clone_test_object(raw_element, cardano_credential_unref);
  cardano_credential_set_t* raw_source = nullptr;
  ASSERT_EQ(cardano_credential_set_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_credential_set_unref);
  ASSERT_EQ(cardano_credential_set_add(source.get(), element.get()), CARDANO_SUCCESS);
  const size_t references = cardano_credential_refcount(element.get());
  verify_clone_allocation_failures(source.get(), cardano_credential_set_deep_clone, cardano_credential_set_unref);

  cardano_credential_set_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_credential_set_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                  copy      = own_clone_test_object(raw_copy, cardano_credential_set_unref);
  cardano_credential_t* raw_child = nullptr;
  ASSERT_EQ(cardano_credential_set_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_credential_unref);

  EXPECT_EQ(cardano_credential_set_get_length(copy.get()), 1U);
  EXPECT_NE(child.get(), element.get());
  EXPECT_EQ(cardano_credential_refcount(element.get()), references);
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_credential_to_cbor), clone_test_cbor(element.get(), cardano_credential_to_cbor));
}

TEST(cardano_base_address_deep_clone, preservesAddressAndOwnsItsStorage)
{
  const std::string& text        = Cip19TestVectors::basePaymentKeyStakeKey;
  cardano_address_t* raw_address = nullptr;
  ASSERT_EQ(cardano_address_from_string(text.data(), text.size(), &raw_address), CARDANO_SUCCESS);
  auto address = own_clone_test_object(raw_address, cardano_address_unref);
  verify_clone_allocation_failures(address.get(), cardano_address_deep_clone, cardano_address_unref);
  cardano_address_t* raw_address_copy = nullptr;
  ASSERT_EQ(cardano_address_deep_clone(address.get(), &raw_address_copy), CARDANO_SUCCESS);
  auto address_copy = own_clone_test_object(raw_address_copy, cardano_address_unref);
  EXPECT_TRUE(cardano_address_equals(address.get(), address_copy.get()));
  EXPECT_NE(cardano_address_get_bytes(address.get()), cardano_address_get_bytes(address_copy.get()));

  auto source = own_clone_test_object(cardano_address_to_base_address(address.get()), cardano_base_address_unref);
  ASSERT_NE(source, nullptr);
  verify_clone_allocation_failures(source.get(), cardano_base_address_deep_clone, cardano_base_address_unref);
  cardano_base_address_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_base_address_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_base_address_unref);
  source.reset();
  address.reset();
  auto generic_copy = own_clone_test_object(cardano_base_address_to_address(copy.get()), cardano_address_unref);
  ASSERT_NE(generic_copy, nullptr);
  EXPECT_EQ(std::string(cardano_address_get_string(generic_copy.get())), text);
}

TEST(cardano_enterprise_address_deep_clone, preservesAddressAndOwnsItsStorage)
{
  const std::string& text        = Cip19TestVectors::enterpriseKey;
  cardano_address_t* raw_address = nullptr;
  ASSERT_EQ(cardano_address_from_string(text.data(), text.size(), &raw_address), CARDANO_SUCCESS);
  auto address = own_clone_test_object(raw_address, cardano_address_unref);
  verify_clone_allocation_failures(address.get(), cardano_address_deep_clone, cardano_address_unref);
  cardano_address_t* raw_address_copy = nullptr;
  ASSERT_EQ(cardano_address_deep_clone(address.get(), &raw_address_copy), CARDANO_SUCCESS);
  auto address_copy = own_clone_test_object(raw_address_copy, cardano_address_unref);
  EXPECT_TRUE(cardano_address_equals(address.get(), address_copy.get()));
  EXPECT_NE(cardano_address_get_bytes(address.get()), cardano_address_get_bytes(address_copy.get()));

  auto source = own_clone_test_object(cardano_address_to_enterprise_address(address.get()), cardano_enterprise_address_unref);
  ASSERT_NE(source, nullptr);
  verify_clone_allocation_failures(source.get(), cardano_enterprise_address_deep_clone, cardano_enterprise_address_unref);
  cardano_enterprise_address_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_enterprise_address_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_enterprise_address_unref);
  source.reset();
  address.reset();
  auto generic_copy = own_clone_test_object(cardano_enterprise_address_to_address(copy.get()), cardano_address_unref);
  ASSERT_NE(generic_copy, nullptr);
  EXPECT_EQ(std::string(cardano_address_get_string(generic_copy.get())), text);
}

TEST(cardano_pointer_address_deep_clone, preservesAddressAndOwnsItsStorage)
{
  const std::string& text        = Cip19TestVectors::testnetPointerKey;
  cardano_address_t* raw_address = nullptr;
  ASSERT_EQ(cardano_address_from_string(text.data(), text.size(), &raw_address), CARDANO_SUCCESS);
  auto address = own_clone_test_object(raw_address, cardano_address_unref);
  verify_clone_allocation_failures(address.get(), cardano_address_deep_clone, cardano_address_unref);
  cardano_address_t* raw_address_copy = nullptr;
  ASSERT_EQ(cardano_address_deep_clone(address.get(), &raw_address_copy), CARDANO_SUCCESS);
  auto address_copy = own_clone_test_object(raw_address_copy, cardano_address_unref);
  EXPECT_TRUE(cardano_address_equals(address.get(), address_copy.get()));
  EXPECT_NE(cardano_address_get_bytes(address.get()), cardano_address_get_bytes(address_copy.get()));

  auto source = own_clone_test_object(cardano_address_to_pointer_address(address.get()), cardano_pointer_address_unref);
  ASSERT_NE(source, nullptr);
  verify_clone_allocation_failures(source.get(), cardano_pointer_address_deep_clone, cardano_pointer_address_unref);
  cardano_pointer_address_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_pointer_address_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_pointer_address_unref);
  source.reset();
  address.reset();
  auto generic_copy = own_clone_test_object(cardano_pointer_address_to_address(copy.get()), cardano_address_unref);
  ASSERT_NE(generic_copy, nullptr);
  EXPECT_EQ(std::string(cardano_address_get_string(generic_copy.get())), text);
}

TEST(cardano_reward_address_deep_clone, preservesAddressAndOwnsItsStorage)
{
  const std::string& text        = Cip19TestVectors::testnetRewardKey;
  cardano_address_t* raw_address = nullptr;
  ASSERT_EQ(cardano_address_from_string(text.data(), text.size(), &raw_address), CARDANO_SUCCESS);
  auto address = own_clone_test_object(raw_address, cardano_address_unref);
  verify_clone_allocation_failures(address.get(), cardano_address_deep_clone, cardano_address_unref);
  cardano_address_t* raw_address_copy = nullptr;
  ASSERT_EQ(cardano_address_deep_clone(address.get(), &raw_address_copy), CARDANO_SUCCESS);
  auto address_copy = own_clone_test_object(raw_address_copy, cardano_address_unref);
  EXPECT_TRUE(cardano_address_equals(address.get(), address_copy.get()));
  EXPECT_NE(cardano_address_get_bytes(address.get()), cardano_address_get_bytes(address_copy.get()));

  auto source = own_clone_test_object(cardano_address_to_reward_address(address.get()), cardano_reward_address_unref);
  ASSERT_NE(source, nullptr);
  verify_clone_allocation_failures(source.get(), cardano_reward_address_deep_clone, cardano_reward_address_unref);
  cardano_reward_address_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_reward_address_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_reward_address_unref);
  source.reset();
  address.reset();
  auto generic_copy = own_clone_test_object(cardano_reward_address_to_address(copy.get()), cardano_address_unref);
  ASSERT_NE(generic_copy, nullptr);
  EXPECT_EQ(std::string(cardano_address_get_string(generic_copy.get())), text);
}

TEST(cardano_byron_address_deep_clone, preservesAddressAndOwnsItsStorage)
{
  const std::string& text        = Cip19TestVectors::byronTestnetDaedalus;
  cardano_address_t* raw_address = nullptr;
  ASSERT_EQ(cardano_address_from_string(text.data(), text.size(), &raw_address), CARDANO_SUCCESS);
  auto address = own_clone_test_object(raw_address, cardano_address_unref);
  verify_clone_allocation_failures(address.get(), cardano_address_deep_clone, cardano_address_unref);
  cardano_address_t* raw_address_copy = nullptr;
  ASSERT_EQ(cardano_address_deep_clone(address.get(), &raw_address_copy), CARDANO_SUCCESS);
  auto address_copy = own_clone_test_object(raw_address_copy, cardano_address_unref);
  EXPECT_TRUE(cardano_address_equals(address.get(), address_copy.get()));
  EXPECT_NE(cardano_address_get_bytes(address.get()), cardano_address_get_bytes(address_copy.get()));

  auto source = own_clone_test_object(cardano_address_to_byron_address(address.get()), cardano_byron_address_unref);
  ASSERT_NE(source, nullptr);
  verify_clone_allocation_failures(source.get(), cardano_byron_address_deep_clone, cardano_byron_address_unref);
  cardano_byron_address_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_byron_address_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_byron_address_unref);
  source.reset();
  address.reset();
  auto generic_copy = own_clone_test_object(cardano_byron_address_to_address(copy.get()), cardano_address_unref);
  ASSERT_NE(generic_copy, nullptr);
  EXPECT_EQ(std::string(cardano_address_get_string(generic_copy.get())), text);
}

TEST(cardano_cbor_reader_deep_clone, preservesNestedParsingState)
{
  const char* cbor   = "9f9f0102ff03ff";
  auto        source = own_clone_test_object(cardano_cbor_reader_from_hex(cbor, strlen(cbor)), cardano_cbor_reader_unref);
  int64_t     size   = 0;
  uint64_t    value  = 0U;
  ASSERT_EQ(cardano_cbor_reader_read_start_array(source.get(), &size), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_reader_read_start_array(source.get(), &size), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_reader_read_uint(source.get(), &value), CARDANO_SUCCESS);
  EXPECT_EQ(value, 1U);
  verify_clone_allocation_failures(source.get(), cardano_cbor_reader_deep_clone, cardano_cbor_reader_unref);

  cardano_cbor_reader_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_cbor_reader_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_cbor_reader_unref);
  ASSERT_EQ(cardano_cbor_reader_read_uint(copy.get(), &value), CARDANO_SUCCESS);
  EXPECT_EQ(value, 2U);
  ASSERT_EQ(cardano_cbor_reader_read_uint(source.get(), &value), CARDANO_SUCCESS);
  EXPECT_EQ(value, 2U);
  source.reset();
  ASSERT_EQ(cardano_cbor_reader_read_end_array(copy.get()), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_reader_read_uint(copy.get(), &value), CARDANO_SUCCESS);
  EXPECT_EQ(value, 3U);
  ASSERT_EQ(cardano_cbor_reader_read_end_array(copy.get()), CARDANO_SUCCESS);
}

TEST(cardano_cbor_writer_deep_clone, continuesWritingIndependently)
{
  auto source = own_clone_test_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
  ASSERT_EQ(cardano_cbor_writer_write_start_array(source.get(), 2), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_writer_write_uint(source.get(), 1U), CARDANO_SUCCESS);
  verify_clone_allocation_failures(source.get(), cardano_cbor_writer_deep_clone, cardano_cbor_writer_unref);
  cardano_cbor_writer_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_cbor_writer_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_cbor_writer_unref);
  ASSERT_EQ(cardano_cbor_writer_write_uint(source.get(), 2U), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_writer_write_uint(copy.get(), 3U), CARDANO_SUCCESS);
  char source_hex[7] = { 0 };
  char copy_hex[7]   = { 0 };
  ASSERT_EQ(cardano_cbor_writer_encode_hex(source.get(), source_hex, sizeof(source_hex)), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cbor_writer_encode_hex(copy.get(), copy_hex, sizeof(copy_hex)), CARDANO_SUCCESS);
  EXPECT_STREQ(source_hex, "820102");
  EXPECT_STREQ(copy_hex, "820103");
}

TEST(cardano_json_writer_deep_clone, continuesNestedWritingIndependently)
{
  auto source = own_clone_test_object(cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT), cardano_json_writer_unref);
  cardano_json_writer_write_start_array(source.get());
  cardano_json_writer_write_start_array(source.get());
  cardano_json_writer_write_uint(source.get(), 1U);
  verify_clone_allocation_failures(source.get(), cardano_json_writer_deep_clone, cardano_json_writer_unref);
  cardano_json_writer_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_json_writer_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_json_writer_unref);
  cardano_json_writer_write_uint(source.get(), 2U);
  cardano_json_writer_write_uint(copy.get(), 3U);
  cardano_json_writer_write_end_array(source.get());
  cardano_json_writer_write_end_array(source.get());
  cardano_json_writer_write_end_array(copy.get());
  cardano_json_writer_write_end_array(copy.get());
  char source_json[8] = { 0 };
  char copy_json[8]   = { 0 };
  ASSERT_EQ(cardano_json_writer_encode(source.get(), source_json, sizeof(source_json)), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_json_writer_encode(copy.get(), copy_json, sizeof(copy_json)), CARDANO_SUCCESS);
  EXPECT_STREQ(source_json, "[[1,2]]");
  EXPECT_STREQ(copy_json, "[[1,3]]");
}

TEST(cardano_asset_id_deep_clone, copiesNestedIdentifiersAndCollections)
{
  const std::string   hex        = std::string(56U, '0') + "abcd";
  cardano_asset_id_t* raw_source = nullptr;
  ASSERT_EQ(cardano_asset_id_from_hex(hex.data(), hex.size(), &raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_asset_id_unref);
  verify_clone_allocation_failures(source.get(), cardano_asset_id_deep_clone, cardano_asset_id_unref);
  cardano_asset_id_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_asset_id_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy          = own_clone_test_object(raw_copy, cardano_asset_id_unref);
  auto source_policy = own_clone_test_object(cardano_asset_id_get_policy_id(source.get()), cardano_blake2b_hash_unref);
  auto copy_policy   = own_clone_test_object(cardano_asset_id_get_policy_id(copy.get()), cardano_blake2b_hash_unref);
  auto source_name   = own_clone_test_object(cardano_asset_id_get_asset_name(source.get()), cardano_asset_name_unref);
  auto copy_name     = own_clone_test_object(cardano_asset_id_get_asset_name(copy.get()), cardano_asset_name_unref);
  EXPECT_NE(source_policy.get(), copy_policy.get());
  EXPECT_NE(source_name.get(), copy_name.get());
  EXPECT_STREQ(cardano_asset_id_get_hex(copy.get()), hex.c_str());

  cardano_asset_id_list_t* raw_list = nullptr;
  ASSERT_EQ(cardano_asset_id_list_new(&raw_list), CARDANO_SUCCESS);
  auto list = own_clone_test_object(raw_list, cardano_asset_id_list_unref);
  ASSERT_EQ(cardano_asset_id_list_add(list.get(), source.get()), CARDANO_SUCCESS);
  verify_clone_allocation_failures(list.get(), cardano_asset_id_list_deep_clone, cardano_asset_id_list_unref);
  cardano_asset_id_list_t* raw_list_copy = nullptr;
  ASSERT_EQ(cardano_asset_id_list_deep_clone(list.get(), &raw_list_copy), CARDANO_SUCCESS);
  auto                list_copy = own_clone_test_object(raw_list_copy, cardano_asset_id_list_unref);
  cardano_asset_id_t* raw_child = nullptr;
  ASSERT_EQ(cardano_asset_id_list_get(list_copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_asset_id_unref);
  EXPECT_NE(child.get(), source.get());
  EXPECT_STREQ(cardano_asset_id_get_hex(child.get()), hex.c_str());

  cardano_asset_id_map_t* raw_map = nullptr;
  ASSERT_EQ(cardano_asset_id_map_new(&raw_map), CARDANO_SUCCESS);
  auto map = own_clone_test_object(raw_map, cardano_asset_id_map_unref);
  ASSERT_EQ(cardano_asset_id_map_insert(map.get(), source.get(), 42), CARDANO_SUCCESS);
  verify_clone_allocation_failures(map.get(), cardano_asset_id_map_deep_clone, cardano_asset_id_map_unref);
  cardano_asset_id_map_t* raw_map_copy = nullptr;
  ASSERT_EQ(cardano_asset_id_map_deep_clone(map.get(), &raw_map_copy), CARDANO_SUCCESS);
  auto map_copy = own_clone_test_object(raw_map_copy, cardano_asset_id_map_unref);
  ASSERT_EQ(cardano_asset_id_map_insert(map.get(), source.get(), 99), CARDANO_SUCCESS);
  int64_t amount = 0;
  ASSERT_EQ(cardano_asset_id_map_get(map_copy.get(), source.get(), &amount), CARDANO_SUCCESS);
  EXPECT_EQ(amount, 42);
}

TEST(cardano_metadatum_label_list_deep_clone, copiesLabelsAndCollectionState)
{
  cardano_metadatum_label_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_metadatum_label_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_metadatum_label_list_unref);
  ASSERT_EQ(cardano_metadatum_label_list_add(source.get(), UINT64_MAX), CARDANO_SUCCESS);
  verify_clone_allocation_failures(source.get(), cardano_metadatum_label_list_deep_clone, cardano_metadatum_label_list_unref);
  cardano_metadatum_label_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_metadatum_label_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto copy = own_clone_test_object(raw_copy, cardano_metadatum_label_list_unref);
  ASSERT_EQ(cardano_metadatum_label_list_add(source.get(), 0U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_label_list_get_length(copy.get()), 1U);
  uint64_t label = 0U;
  ASSERT_EQ(cardano_metadatum_label_list_get(copy.get(), 0U, &label), CARDANO_SUCCESS);
  EXPECT_EQ(label, UINT64_MAX);
}

TEST(cardano_reward_address_list_deep_clone, copiesAddressesAndCredentials)
{
  const std::string&        text        = Cip19TestVectors::testnetRewardKey;
  cardano_reward_address_t* raw_address = nullptr;
  ASSERT_EQ(cardano_reward_address_from_bech32(text.data(), text.size(), &raw_address), CARDANO_SUCCESS);
  auto                           address    = own_clone_test_object(raw_address, cardano_reward_address_unref);
  cardano_reward_address_list_t* raw_source = nullptr;
  ASSERT_EQ(cardano_reward_address_list_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_reward_address_list_unref);
  ASSERT_EQ(cardano_reward_address_list_add(source.get(), address.get()), CARDANO_SUCCESS);
  verify_clone_allocation_failures(source.get(), cardano_reward_address_list_deep_clone, cardano_reward_address_list_unref);
  cardano_reward_address_list_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_reward_address_list_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                      copy      = own_clone_test_object(raw_copy, cardano_reward_address_list_unref);
  cardano_reward_address_t* raw_child = nullptr;
  ASSERT_EQ(cardano_reward_address_list_get(copy.get(), 0U, &raw_child), CARDANO_SUCCESS);
  auto child             = own_clone_test_object(raw_child, cardano_reward_address_unref);
  auto source_credential = own_clone_test_object(cardano_reward_address_get_credential(address.get()), cardano_credential_unref);
  auto copy_credential   = own_clone_test_object(cardano_reward_address_get_credential(child.get()), cardano_credential_unref);
  EXPECT_NE(child.get(), address.get());
  EXPECT_NE(source_credential.get(), copy_credential.get());
  EXPECT_STREQ(cardano_reward_address_get_string(child.get()), text.c_str());
}

TEST(cardano_input_to_redeemer_map_deep_clone, copiesKeysAndRedeemerValues)
{
  const char*                  input_cbor      = "825820000000000000000000000000000000000000000000000000000000000000000000";
  const char*                  redeemer_cbor   = "84000001820000";
  auto                         input_reader    = own_clone_test_object(cardano_cbor_reader_from_hex(input_cbor, strlen(input_cbor)), cardano_cbor_reader_unref);
  auto                         redeemer_reader = own_clone_test_object(cardano_cbor_reader_from_hex(redeemer_cbor, strlen(redeemer_cbor)), cardano_cbor_reader_unref);
  cardano_transaction_input_t* raw_input       = nullptr;
  cardano_redeemer_t*          raw_redeemer    = nullptr;
  ASSERT_EQ(cardano_transaction_input_from_cbor(input_reader.get(), &raw_input), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_redeemer_from_cbor(redeemer_reader.get(), &raw_redeemer), CARDANO_SUCCESS);
  auto                             input      = own_clone_test_object(raw_input, cardano_transaction_input_unref);
  auto                             redeemer   = own_clone_test_object(raw_redeemer, cardano_redeemer_unref);
  cardano_input_to_redeemer_map_t* raw_source = nullptr;
  ASSERT_EQ(cardano_input_to_redeemer_map_new(&raw_source), CARDANO_SUCCESS);
  auto source = own_clone_test_object(raw_source, cardano_input_to_redeemer_map_unref);
  ASSERT_EQ(cardano_input_to_redeemer_map_insert(source.get(), input.get(), redeemer.get()), CARDANO_SUCCESS);
  verify_clone_allocation_failures(source.get(), cardano_input_to_redeemer_map_deep_clone, cardano_input_to_redeemer_map_unref);
  cardano_input_to_redeemer_map_t* raw_copy = nullptr;
  ASSERT_EQ(cardano_input_to_redeemer_map_deep_clone(source.get(), &raw_copy), CARDANO_SUCCESS);
  auto                copy      = own_clone_test_object(raw_copy, cardano_input_to_redeemer_map_unref);
  cardano_redeemer_t* raw_child = nullptr;
  ASSERT_EQ(cardano_input_to_redeemer_map_get(copy.get(), input.get(), &raw_child), CARDANO_SUCCESS);
  auto child = own_clone_test_object(raw_child, cardano_redeemer_unref);
  EXPECT_NE(child.get(), redeemer.get());
  EXPECT_EQ(clone_test_cbor(child.get(), cardano_redeemer_to_cbor), clone_test_cbor(redeemer.get(), cardano_redeemer_to_cbor));
}

TEST(cardano_protocol_param_update_deep_clone, preservesAbsentAndExplicitZeroFields)
{
  cardano_protocol_param_update_t* raw_source = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&raw_source), CARDANO_SUCCESS);
  auto                             source     = own_clone_test_object(raw_source, cardano_protocol_param_update_unref);
  cardano_protocol_param_update_t* raw_absent = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_deep_clone(source.get(), &raw_absent), CARDANO_SUCCESS);
  auto absent = own_clone_test_object(raw_absent, cardano_protocol_param_update_unref);
  EXPECT_EQ(clone_test_cbor(absent.get(), cardano_protocol_param_update_to_cbor), "a0");

  const uint64_t zero = 0U;
  ASSERT_EQ(cardano_protocol_param_update_set_min_fee_a(source.get(), &zero), CARDANO_SUCCESS);
  cardano_protocol_param_update_t* raw_present = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_deep_clone(source.get(), &raw_present), CARDANO_SUCCESS);
  auto present = own_clone_test_object(raw_present, cardano_protocol_param_update_unref);
  source.reset();
  uint64_t fee = UINT64_MAX;
  ASSERT_EQ(cardano_protocol_param_update_get_min_fee_a(present.get(), &fee), CARDANO_SUCCESS);
  EXPECT_EQ(fee, 0U);
  EXPECT_EQ(clone_test_cbor(present.get(), cardano_protocol_param_update_to_cbor), "a10000");
  EXPECT_EQ(clone_test_cbor(absent.get(), cardano_protocol_param_update_to_cbor), "a0");
}
