/**
 * \file binding_conformance.cpp
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

#include "json_helpers.h"
#include <cardano/cardano.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

/* TEST FIXTURE **************************************************************/

class binding_conformance : public testing::Test
{
  protected:

    cardano_json_object_t*         fixture           = nullptr;
    cardano_json_object_t*         inspection        = nullptr;
    cardano_json_object_t*         actual_inspection = nullptr;
    cardano_cbor_reader_t*         reader            = nullptr;
    cardano_cbor_writer_t*         writer            = nullptr;
    cardano_json_writer_t*         json_writer       = nullptr;
    cardano_transaction_t*         transaction       = nullptr;
    cardano_transaction_body_t*    body              = nullptr;
    cardano_blake2b_hash_t*        id                = nullptr;
    cardano_ed25519_private_key_t* key               = nullptr;
    cardano_ed25519_public_key_t*  public_key        = nullptr;
    cardano_ed25519_signature_t*   signature         = nullptr;

    void
    TearDown() override
    {
      cardano_ed25519_signature_unref(&signature);
      cardano_ed25519_public_key_unref(&public_key);
      cardano_ed25519_private_key_unref(&key);
      cardano_blake2b_hash_unref(&id);
      cardano_transaction_body_unref(&body);
      cardano_transaction_unref(&transaction);
      cardano_json_writer_unref(&json_writer);
      cardano_cbor_writer_unref(&writer);
      cardano_cbor_reader_unref(&reader);
      cardano_json_object_unref(&actual_inspection);
      cardano_json_object_unref(&inspection);
      cardano_json_object_unref(&fixture);
    }

    /**
     * Reads a required string from the transaction fixture.
     */
    std::string
    get_fixture_string(const char* name)
    {
      cardano_json_object_t* value = nullptr;

      EXPECT_TRUE(cardano_json_object_get(fixture, name, std::strlen(name), &value));

      size_t      length = 0;
      const char* text   = cardano_json_object_get_string(value, &length);

      EXPECT_NE(text, nullptr);

      std::string result = text == nullptr ? "" : std::string(text);
      cardano_json_object_unref(&value);

      return result;
    }

    /**
     * Returns the current CBOR writer output as a hex string.
     */
    std::string
    encode_cbor()
    {
      std::vector<char> hex(cardano_cbor_writer_get_hex_size(writer));

      EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex.data(), hex.size()), CARDANO_SUCCESS);

      return std::string(hex.data());
    }

    /**
     * Checks transaction encoding, inspection, hashes, and signatures.
     */
    void
    verify_fixture(const char* name)
    {
      // Arrange
      const auto path = std::filesystem::path(__FILE__).parent_path() / "vectors" / "bindings" / "v1" /
        (std::string(name) + ".json");
      std::ifstream stream(path);

      ASSERT_TRUE(stream.is_open());

      const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
      fixture = cardano_json_object_parse(text.data(), text.size());

      ASSERT_NE(fixture, nullptr);
      EXPECT_EQ(get_fixture_string("id"), name);

      cardano_json_object_t* schema = nullptr;

      ASSERT_TRUE(cardano_json_object_get(fixture, "schema_version", 14, &schema));

      uint64_t   schema_version = 0;
      const auto schema_result  = cardano_json_object_get_uint(schema, &schema_version);
      cardano_json_object_unref(&schema);

      ASSERT_EQ(schema_result, CARDANO_SUCCESS);
      ASSERT_EQ(schema_version, 1U);

      // Act and assert: transaction and body serialization.
      const auto cbor = get_fixture_string("transaction_cbor");
      reader          = cardano_cbor_reader_from_hex(cbor.data(), cbor.size());

      ASSERT_NE(reader, nullptr);
      ASSERT_EQ(cardano_transaction_from_cbor(reader, &transaction), CARDANO_SUCCESS);

      writer = cardano_cbor_writer_new();

      ASSERT_NE(writer, nullptr);
      ASSERT_EQ(cardano_transaction_to_cbor(transaction, writer), CARDANO_SUCCESS);
      EXPECT_EQ(encode_cbor(), cbor);

      body = cardano_transaction_get_body(transaction);

      ASSERT_NE(body, nullptr);

      cardano_cbor_writer_unref(&writer);
      writer = cardano_cbor_writer_new();

      ASSERT_NE(writer, nullptr);
      ASSERT_EQ(cardano_transaction_body_to_cbor(body, writer), CARDANO_SUCCESS);
      EXPECT_EQ(encode_cbor(), get_fixture_string("body_cbor"));

      // Act and assert: transaction ID and deterministic signing.
      id = cardano_transaction_get_id(transaction);

      ASSERT_NE(id, nullptr);

      std::array<char, 65> hash_hex {};

      ASSERT_EQ(cardano_blake2b_hash_to_hex(id, hash_hex.data(), hash_hex.size()), CARDANO_SUCCESS);
      EXPECT_EQ(hash_hex.data(), get_fixture_string("transaction_id"));

      const auto seed = get_fixture_string("signing_seed");

      ASSERT_EQ(cardano_ed25519_private_key_from_normal_hex(seed.data(), seed.size(), &key), CARDANO_SUCCESS);
      ASSERT_EQ(cardano_ed25519_private_key_get_public_key(key, &public_key), CARDANO_SUCCESS);

      std::array<char, 65> key_hex {};

      ASSERT_EQ(cardano_ed25519_public_key_to_hex(public_key, key_hex.data(), key_hex.size()), CARDANO_SUCCESS);
      EXPECT_EQ(key_hex.data(), get_fixture_string("verification_key"));

      const byte_t* message = cardano_blake2b_hash_get_data(id);
      const size_t  size    = cardano_blake2b_hash_get_bytes_size(id);

      ASSERT_EQ(cardano_ed25519_private_key_sign(key, message, size, &signature), CARDANO_SUCCESS);

      std::array<char, 129> signature_hex {};

      ASSERT_EQ(
        cardano_ed25519_signature_to_hex(signature, signature_hex.data(), signature_hex.size()), CARDANO_SUCCESS);
      EXPECT_EQ(signature_hex.data(), get_fixture_string("signature"));
      EXPECT_TRUE(cardano_ed25519_public_verify(public_key, signature, message, size));

      std::vector<byte_t> modified(message, message + size);
      modified[0] ^= 1;

      EXPECT_FALSE(cardano_ed25519_public_verify(public_key, signature, modified.data(), modified.size()));

      // Act and assert: transaction inspection.
      json_writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

      ASSERT_NE(json_writer, nullptr);
      ASSERT_EQ(cardano_transaction_to_cip116_json(transaction, json_writer), CARDANO_SUCCESS);

      char* json = encode_json(json_writer);

      ASSERT_NE(json, nullptr);

      actual_inspection = cardano_json_object_parse(json, std::strlen(json));

      std::free(json);

      ASSERT_NE(actual_inspection, nullptr);
      ASSERT_TRUE(cardano_json_object_get(fixture, "inspection", 10, &inspection));

      // The native writer has stable key order; binding consumers compare JSON structurally.
      EXPECT_STREQ(
        cardano_json_object_to_json_string(actual_inspection, CARDANO_JSON_FORMAT_COMPACT, nullptr),
        cardano_json_object_to_json_string(inspection, CARDANO_JSON_FORMAT_COMPACT, nullptr));
    }
};

/* UNIT TESTS ****************************************************************/

TEST_F(binding_conformance, conwayKeyGuard)
{
  verify_fixture("conway-key-guard");
}

TEST_F(binding_conformance, dijkstraAccountBalances)
{
  verify_fixture("dijkstra-account-balances");
}

TEST_F(binding_conformance, dijkstraSignedSubtransaction)
{
  verify_fixture("dijkstra-signed-subtransaction");
}
