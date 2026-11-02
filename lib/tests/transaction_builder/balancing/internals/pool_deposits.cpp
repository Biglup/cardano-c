/**
 * \file pool_deposits.cpp
 *
 * \author angel.castillo
 * \date   Nov 02, 2026
 *
 * \section LICENSE
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

#include "../../../allocators_helpers.h"

extern "C" {
#include "../../../../src/transaction_builder/balancing/internals/pool_deposits.h"
}

#include <allocators.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <gmock/gmock.h>

#include <vector>

/* CONSTANTS *****************************************************************/

static const char* POOL_REGISTRATION_CBOR  = "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* POOL_UPDATE_CBOR        = "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db0194e201903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* POOL_REGISTRATION2_CBOR = "8a03581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f58208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* POOL_RETIREMENT_CBOR    = "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8";
static const char* POOL_OPERATOR_HEX       = "d85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";
static const char* SUB_TX_CBOR             = "83a400d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0098968004d901028482008200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d083078200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a83088200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0148304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f405a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f05a0f6";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a certificate from its CBOR hex string.
 * \param cbor the CBOR hex string of the certificate.
 * \return A new instance of the certificate.
 */
static cardano_certificate_t*
new_certificate(const char* cbor)
{
  cardano_certificate_t* certificate = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_certificate_from_cbor(reader, &certificate), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return certificate;
}

/**
 * Creates a certificate set from the CBOR hex strings of its certificates.
 * \param cbors the CBOR hex strings of the certificates, in order.
 * \return A new instance of the certificate set.
 */
static cardano_certificate_set_t*
new_certificate_set(const std::vector<const char*>& cbors)
{
  cardano_certificate_set_t* certificates = nullptr;

  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);

  for (const char* cbor: cbors)
  {
    cardano_certificate_t* certificate = new_certificate(cbor);

    EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

    cardano_certificate_unref(&certificate);
  }

  return certificates;
}

/**
 * Creates a set of sub transactions, each carrying the given certificates.
 * \param bodies the CBOR hex strings of the certificates of each sub transaction.
 * \return A new instance of the sub transaction set.
 */
static cardano_sub_transaction_set_t*
new_sub_transaction_set(const std::vector<std::vector<const char*>>& bodies)
{
  cardano_sub_transaction_set_t* sub_transactions = nullptr;

  EXPECT_EQ(cardano_sub_transaction_set_new(&sub_transactions), CARDANO_SUCCESS);

  for (const std::vector<const char*>& cbors: bodies)
  {
    cardano_sub_transaction_t* sub_tx = nullptr;
    cardano_cbor_reader_t*     reader = cardano_cbor_reader_from_hex(SUB_TX_CBOR, strlen(SUB_TX_CBOR));

    EXPECT_EQ(cardano_sub_transaction_from_cbor(reader, &sub_tx), CARDANO_SUCCESS);

    cardano_sub_transaction_body_t* body         = cardano_sub_transaction_get_body(sub_tx);
    cardano_certificate_set_t*      certificates = new_certificate_set(cbors);

    EXPECT_EQ(cardano_sub_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);

    cardano_sub_transaction_clear_cbor_cache(sub_tx);

    EXPECT_EQ(cardano_sub_transaction_set_add(sub_transactions, sub_tx), CARDANO_SUCCESS);

    cardano_certificate_set_unref(&certificates);
    cardano_sub_transaction_body_unref(&body);
    cardano_sub_transaction_unref(&sub_tx);
    cardano_cbor_reader_unref(&reader);
  }

  return sub_transactions;
}

/**
 * Counts the pool deposits a batch counts more than once when each body is charged on its own.
 * \param top the CBOR hex strings of the certificates of the top level body.
 * \param bodies the CBOR hex strings of the certificates of each sub transaction.
 * \return The number of pool deposits counted more than once.
 */
static uint64_t
count_repeated_pool_deposits(const std::vector<const char*>& top, const std::vector<std::vector<const char*>>& bodies)
{
  cardano_certificate_set_t*     certificates     = new_certificate_set(top);
  cardano_sub_transaction_set_t* sub_transactions = new_sub_transaction_set(bodies);
  uint64_t                       count            = 0U;

  EXPECT_EQ(_cardano_count_repeated_pool_deposits(certificates, sub_transactions, &count), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_sub_transaction_set_unref(&sub_transactions);

  return count;
}

/* UNIT TESTS ****************************************************************/

TEST(_cardano_get_pool_operator_key_hash, returnsTheOperatorKeyHashOfAPoolRegistration)
{
  cardano_certificate_t*  certificate       = new_certificate(POOL_REGISTRATION_CBOR);
  cardano_blake2b_hash_t* operator_key_hash = nullptr;
  cardano_blake2b_hash_t* expected          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(POOL_OPERATOR_HEX, strlen(POOL_OPERATOR_HEX), &expected), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_get_pool_operator_key_hash(certificate, &operator_key_hash), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_blake2b_hash_equals(operator_key_hash, expected));

  cardano_blake2b_hash_unref(&operator_key_hash);
  cardano_blake2b_hash_unref(&expected);
  cardano_certificate_unref(&certificate);
}

TEST(_cardano_get_pool_operator_key_hash, returnsErrorIfTheCertificateIsNotAPoolRegistration)
{
  cardano_certificate_t*  certificate       = new_certificate(POOL_RETIREMENT_CBOR);
  cardano_blake2b_hash_t* operator_key_hash = nullptr;

  EXPECT_EQ(_cardano_get_pool_operator_key_hash(certificate, &operator_key_hash), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);
  EXPECT_EQ(operator_key_hash, nullptr);

  cardano_certificate_unref(&certificate);
}

TEST(_cardano_get_pool_operator_key_hash, returnsErrorIfGivenNull)
{
  cardano_blake2b_hash_t* operator_key_hash = nullptr;

  EXPECT_EQ(_cardano_get_pool_operator_key_hash(nullptr, &operator_key_hash), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_count_repeated_pool_deposits, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_cardano_count_repeated_pool_deposits(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_count_repeated_pool_deposits, returnsZeroIfThereAreNoCertificates)
{
  uint64_t count = 1U;

  EXPECT_EQ(_cardano_count_repeated_pool_deposits(nullptr, nullptr, &count), CARDANO_SUCCESS);
  EXPECT_EQ(count, 0U);
}

TEST(_cardano_count_repeated_pool_deposits, ignoresRepeatedRegistrationsInsideABody)
{
  EXPECT_EQ(count_repeated_pool_deposits({ POOL_REGISTRATION_CBOR, POOL_UPDATE_CBOR, POOL_RETIREMENT_CBOR }, { { POOL_REGISTRATION2_CBOR, POOL_REGISTRATION2_CBOR } }), 0U);
}

TEST(_cardano_count_repeated_pool_deposits, countsAPoolRegisteredByTheTopLevelBodyAndASubTransaction)
{
  EXPECT_EQ(count_repeated_pool_deposits({ POOL_REGISTRATION_CBOR }, { { POOL_UPDATE_CBOR } }), 1U);
}

TEST(_cardano_count_repeated_pool_deposits, countsAPoolRegisteredByTwoSubTransactions)
{
  EXPECT_EQ(count_repeated_pool_deposits({}, { { POOL_REGISTRATION_CBOR }, { POOL_REGISTRATION_CBOR, POOL_UPDATE_CBOR } }), 1U);
}

TEST(_cardano_count_repeated_pool_deposits, countsEachExtraBodyOfEachPool)
{
  EXPECT_EQ(count_repeated_pool_deposits({ POOL_REGISTRATION_CBOR, POOL_REGISTRATION2_CBOR }, { { POOL_REGISTRATION2_CBOR, POOL_REGISTRATION_CBOR }, { POOL_REGISTRATION_CBOR } }), 3U);
}

TEST(_cardano_count_repeated_pool_deposits, returnsErrorOnMemoryAllocationFailure)
{
  cardano_certificate_set_t*     certificates     = new_certificate_set({ POOL_REGISTRATION_CBOR });
  cardano_sub_transaction_set_t* sub_transactions = new_sub_transaction_set({ { POOL_REGISTRATION_CBOR } });

  for (int i = 0; i < 3; ++i)
  {
    uint64_t count = 7U;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    EXPECT_EQ(_cardano_count_repeated_pool_deposits(certificates, sub_transactions, &count), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    EXPECT_EQ(count, 7U);
  }

  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_certificate_set_unref(&certificates);
  cardano_sub_transaction_set_unref(&sub_transactions);
}
