/**
 * \file binding_vectors.cpp
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

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

/* STATIC FUNCTIONS **********************************************************/

/**
 * Owns one native reference and releases it when the test scope exits.
 */
template<typename T>
static auto
make_scoped_object(T* ptr, void (*release)(T**))
{
  const auto deleter = [release](T* value)
  { release(&value); };

  return std::unique_ptr<T, std::function<void(T*)>>(ptr, deleter);
}

/**
 * Stops fixture processing when a native operation fails unexpectedly.
 */
static void
check_result(cardano_error_t error)
{
  if (error != CARDANO_SUCCESS)
  {
    throw std::runtime_error("Native error " + std::to_string(error));
  }
}

/**
 * Gets an owned JSON member, rejecting incomplete fixture data.
 */
static auto
get_json_member(const cardano_json_object_t* object, const char* key)
{
  cardano_json_object_t* value = nullptr;
  if (!cardano_json_object_get(object, key, std::strlen(key), &value))
  {
    throw std::runtime_error(std::string("Missing fixture field: ") + key);
  }

  return make_scoped_object(value, cardano_json_object_unref);
}

/**
 * Reads a required string from a fixture object.
 */
static std::string
get_json_string(const cardano_json_object_t* object, const char* key)
{
  auto        value  = get_json_member(object, key);
  const char* result = cardano_json_object_get_string(value.get(), nullptr);
  if (result == nullptr)
  {
    throw std::runtime_error(std::string("Expected string: ") + key);
  }

  return result;
}

/**
 * Reads a required unsigned integer from a fixture object.
 */
static uint64_t
get_json_uint(const cardano_json_object_t* object, const char* key)
{
  auto     value  = get_json_member(object, key);
  uint64_t result = 0;
  check_result(cardano_json_object_get_uint(value.get(), &result));

  return result;
}

/**
 * Decodes a fixture hex string into bytes.
 */
static std::vector<byte_t>
decode_hex(const std::string& hex)
{
  if (hex.size() % 2 != 0 || hex.find_first_not_of("0123456789abcdef") != std::string::npos)
  {
    throw std::runtime_error("Invalid fixture hex");
  }

  std::vector<byte_t> result;
  for (size_t i = 0; i < hex.size(); i += 2)
  {
    result.push_back(static_cast<byte_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
  }

  return result;
}

/**
 * Encodes bytes as lowercase hexadecimal.
 */
static std::string
encode_hex(const byte_t* data, size_t size)
{
  const char* digits = "0123456789abcdef";
  std::string result;
  for (size_t i = 0; i < size; ++i)
  {
    result += digits[data[i] >> 4];
    result += digits[data[i] & 15];
  }

  return result;
}

/**
 * Returns the CBOR writer output as a hex string.
 */
static std::string
encode_cbor(cardano_cbor_writer_t* writer)
{
  std::vector<char> output(cardano_cbor_writer_get_hex_size(writer));
  check_result(cardano_cbor_writer_encode_hex(writer, output.data(), output.size()));

  return output.data();
}

/**
 * Compares native inspection output with the fixture JSON.
 */
static void
verify_inspection(const cardano_json_object_t* expected, cardano_json_writer_t* writer)
{
  auto  wanted = get_json_member(expected, "inspection");
  char* output = encode_json(writer);
  auto  actual = make_scoped_object(cardano_json_object_parse(output, std::strlen(output)), cardano_json_object_unref);
  std::free(output);

  ASSERT_NE(actual, nullptr);
  EXPECT_STREQ(
    cardano_json_object_to_json_string(actual.get(), CARDANO_JSON_FORMAT_COMPACT, nullptr),
    cardano_json_object_to_json_string(wanted.get(), CARDANO_JSON_FORMAT_COMPACT, nullptr));
}

/**
 * Checks deterministic signing and rejects a modified message.
 */
static void
verify_signature(
  const std::string&           seed,
  const std::vector<byte_t>&   message,
  const cardano_json_object_t* expected,
  const char*                  public_field)
{
  cardano_ed25519_private_key_t* raw_key = nullptr;
  check_result(cardano_ed25519_private_key_from_normal_hex(seed.data(), seed.size(), &raw_key));

  auto                          key        = make_scoped_object(raw_key, cardano_ed25519_private_key_unref);
  cardano_ed25519_public_key_t* raw_public = nullptr;
  check_result(cardano_ed25519_private_key_get_public_key(key.get(), &raw_public));

  auto public_key     = make_scoped_object(raw_public, cardano_ed25519_public_key_unref);
  char public_hex[65] = { 0 };
  check_result(cardano_ed25519_public_key_to_hex(public_key.get(), public_hex, sizeof(public_hex)));

  EXPECT_EQ(get_json_string(expected, public_field), public_hex);

  cardano_ed25519_signature_t* raw_signature = nullptr;
  // A non-null pointer represents an empty message for APIs which validate pointers.
  const byte_t  empty = 0;
  const byte_t* data  = message.empty() ? &empty : message.data();
  check_result(cardano_ed25519_private_key_sign(key.get(), data, message.size(), &raw_signature));

  auto signature          = make_scoped_object(raw_signature, cardano_ed25519_signature_unref);
  char signature_hex[129] = { 0 };
  check_result(cardano_ed25519_signature_to_hex(signature.get(), signature_hex, sizeof(signature_hex)));

  EXPECT_EQ(get_json_string(expected, "signature"), signature_hex);
  EXPECT_TRUE(cardano_ed25519_public_verify(public_key.get(), signature.get(), data, message.size()));

  auto changed = message;
  if (changed.empty())
  {
    changed.push_back(1);
  }
  else
  {
    changed[0] ^= 1;
  }

  EXPECT_FALSE(cardano_ed25519_public_verify(public_key.get(), signature.get(), changed.data(), changed.size()));
}

/**
 * Checks decoding, re-encoding, and inspection for one native type.
 */
template<typename T>
static void
verify_cbor_case(
  const cardano_json_object_t* item,
  cardano_error_t (*decode)(cardano_cbor_reader_t*, T**),
  cardano_error_t (*encode)(const T*, cardano_cbor_writer_t*),
  void (*release)(T**),
  cardano_error_t (*json)(const T*, cardano_json_writer_t*))
{
  auto       input    = get_json_member(item, "input");
  auto       expected = get_json_member(item, "expected");
  const auto value    = get_json_string(input.get(), "cbor");
  auto       reader   = make_scoped_object(cardano_cbor_reader_from_hex(value.data(), value.size()), cardano_cbor_reader_unref);

  ASSERT_NE(reader, nullptr);

  T* raw = nullptr;

  const auto result = decode(reader.get(), &raw);
  auto       object = make_scoped_object(raw, release);
  if (cardano_json_object_has_property(expected.get(), "error", 5))
  {
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_EQ(static_cast<uint64_t>(result), get_json_uint(expected.get(), "native_error"));

    return;
  }

  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(object, nullptr);

  auto writer = make_scoped_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
  check_result(encode(object.get(), writer.get()));

  EXPECT_EQ(encode_cbor(writer.get()), get_json_string(expected.get(), "cbor"));

  if (cardano_json_object_has_property(expected.get(), "inspection", 10))
  {
    ASSERT_NE(json, nullptr);

    auto output = make_scoped_object(cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT), cardano_json_writer_unref);
    check_result(json(object.get(), output.get()));
    verify_inspection(expected.get(), output.get());
  }
}

/**
 * Selects the native decoder for a serialization fixture.
 */
static void
verify_cbor_fixture(const cardano_json_object_t* item)
{
  const auto type = get_json_string(item, "type");
  if (type == "account_balance_interval")
  {
    return verify_cbor_case<cardano_account_balance_interval_t>(
      item,
      cardano_account_balance_interval_from_cbor,
      cardano_account_balance_interval_to_cbor,
      cardano_account_balance_interval_unref,
      cardano_account_balance_interval_to_cip116_json);
  }
  if (type == "certificate")
  {
    return verify_cbor_case<cardano_certificate_t>(
      item,
      cardano_certificate_from_cbor,
      cardano_certificate_to_cbor,
      cardano_certificate_unref,
      cardano_certificate_to_cip116_json);
  }
  if (type == "credential")
  {
    return verify_cbor_case<cardano_credential_t>(
      item,
      cardano_credential_from_cbor,
      cardano_credential_to_cbor,
      cardano_credential_unref,
      cardano_credential_to_cip116_json);
  }
  if (type == "native_script")
  {
    return verify_cbor_case<cardano_native_script_t>(
      item,
      cardano_native_script_from_cbor,
      cardano_native_script_to_cbor,
      cardano_native_script_unref,
      cardano_native_script_to_cip116_json);
  }
  if (type == "plutus_data")
  {
    return verify_cbor_case<cardano_plutus_data_t>(
      item,
      cardano_plutus_data_from_cbor,
      cardano_plutus_data_to_cbor,
      cardano_plutus_data_unref,
      cardano_plutus_data_to_cip116_json);
  }
  if (type == "proposal_procedure")
  {
    return verify_cbor_case<cardano_proposal_procedure_t>(
      item,
      cardano_proposal_procedure_from_cbor,
      cardano_proposal_procedure_to_cbor,
      cardano_proposal_procedure_unref,
      cardano_proposal_procedure_to_cip116_json);
  }
  if (type == "script")
  {
    return verify_cbor_case<cardano_script_t>(
      item, cardano_script_from_cbor, cardano_script_to_cbor, cardano_script_unref, cardano_script_to_cip116_json);
  }
  if (type == "sub_transaction")
  {
    return verify_cbor_case<cardano_sub_transaction_t>(
      item,
      cardano_sub_transaction_from_cbor,
      cardano_sub_transaction_to_cbor,
      cardano_sub_transaction_unref,
      cardano_sub_transaction_to_cip116_json);
  }
  if (type == "transaction")
  {
    return verify_cbor_case<cardano_transaction_t>(
      item,
      cardano_transaction_from_cbor,
      cardano_transaction_to_cbor,
      cardano_transaction_unref,
      cardano_transaction_to_cip116_json);
  }
  if (type == "transaction_body")
  {
    return verify_cbor_case<cardano_transaction_body_t>(
      item,
      cardano_transaction_body_from_cbor,
      cardano_transaction_body_to_cbor,
      cardano_transaction_body_unref,
      cardano_transaction_body_to_cip116_json);
  }
  if (type == "transaction_input")
  {
    return verify_cbor_case<cardano_transaction_input_t>(
      item,
      cardano_transaction_input_from_cbor,
      cardano_transaction_input_to_cbor,
      cardano_transaction_input_unref,
      cardano_transaction_input_to_cip116_json);
  }
  if (type == "transaction_output")
  {
    return verify_cbor_case<cardano_transaction_output_t>(
      item,
      cardano_transaction_output_from_cbor,
      cardano_transaction_output_to_cbor,
      cardano_transaction_output_unref,
      cardano_transaction_output_to_cip116_json);
  }
  if (type == "value")
  {
    return verify_cbor_case<cardano_value_t>(
      item, cardano_value_from_cbor, cardano_value_to_cbor, cardano_value_unref, cardano_value_to_cip116_json);
  }
  if (type == "voting_procedures")
  {
    return verify_cbor_case<cardano_voting_procedures_t>(
      item,
      cardano_voting_procedures_from_cbor,
      cardano_voting_procedures_to_cbor,
      cardano_voting_procedures_unref,
      cardano_voting_procedures_to_cip116_json);
  }
  FAIL() << "Unsupported fixture type: " << type;
}

/**
 * Checks script hashes and transaction body hashes and signatures.
 */
static void
verify_cbor_hash_and_signature(const cardano_json_object_t* item)
{
  const auto type     = get_json_string(item, "type");
  auto       input    = get_json_member(item, "input");
  auto       expected = get_json_member(item, "expected");
  if (!cardano_json_object_has_property(expected.get(), "hash", 4))
  {
    return;
  }

  const auto              cbor     = get_json_string(input.get(), "cbor");
  auto                    reader   = make_scoped_object(cardano_cbor_reader_from_hex(cbor.data(), cbor.size()), cardano_cbor_reader_unref);
  cardano_blake2b_hash_t* raw_hash = nullptr;
  if (type == "script")
  {
    cardano_script_t* raw = nullptr;
    check_result(cardano_script_from_cbor(reader.get(), &raw));

    auto object = make_scoped_object(raw, cardano_script_unref);
    raw_hash    = cardano_script_get_hash(object.get());
  }
  else if (type == "native_script")
  {
    cardano_native_script_t* raw = nullptr;
    check_result(cardano_native_script_from_cbor(reader.get(), &raw));

    auto object = make_scoped_object(raw, cardano_native_script_unref);
    raw_hash    = cardano_native_script_get_hash(object.get());
  }
  else if (type == "transaction")
  {
    cardano_transaction_t* raw = nullptr;
    check_result(cardano_transaction_from_cbor(reader.get(), &raw));

    auto object = make_scoped_object(raw, cardano_transaction_unref);
    raw_hash    = cardano_transaction_get_id(object.get());

    auto body   = make_scoped_object(cardano_transaction_get_body(object.get()), cardano_transaction_body_unref);
    auto writer = make_scoped_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
    check_result(cardano_transaction_body_to_cbor(body.get(), writer.get()));

    EXPECT_EQ(encode_cbor(writer.get()), get_json_string(expected.get(), "body_cbor"));
  }
  else if (type == "sub_transaction")
  {
    cardano_sub_transaction_t* raw = nullptr;
    check_result(cardano_sub_transaction_from_cbor(reader.get(), &raw));

    auto object = make_scoped_object(raw, cardano_sub_transaction_unref);
    raw_hash    = cardano_sub_transaction_get_id(object.get());

    auto body   = make_scoped_object(cardano_sub_transaction_get_body(object.get()), cardano_sub_transaction_body_unref);
    auto writer = make_scoped_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
    check_result(cardano_sub_transaction_body_to_cbor(body.get(), writer.get()));

    EXPECT_EQ(encode_cbor(writer.get()), get_json_string(expected.get(), "body_cbor"));
  }
  auto hash = make_scoped_object(raw_hash, cardano_blake2b_hash_unref);

  ASSERT_NE(hash, nullptr);
  EXPECT_EQ(
    encode_hex(cardano_blake2b_hash_get_data(hash.get()), cardano_blake2b_hash_get_bytes_size(hash.get())),
    get_json_string(expected.get(), "hash"));

  if (cardano_json_object_has_property(input.get(), "signing_seed", 12))
  {
    verify_signature(
      get_json_string(input.get(), "signing_seed"),
      decode_hex(get_json_string(expected.get(), "hash")),
      expected.get(),
      "verification_key");
  }
}

/**
 * Checks a cryptographic, address, or script-parameter fixture.
 */
static void
verify_primitive_fixture(const cardano_json_object_t* item)
{
  const auto operation = get_json_string(item, "operation");
  auto       input     = get_json_member(item, "input");
  auto       expected  = get_json_member(item, "expected");
  if (operation == "ed25519")
  {
    return verify_signature(
      get_json_string(input.get(), "seed"),
      decode_hex(get_json_string(input.get(), "message")),
      expected.get(),
      "public_key");
  }
  if (operation == "verify_ed25519")
  {
    const auto                    public_hex    = get_json_string(input.get(), "public_key");
    const auto                    signature_hex = get_json_string(input.get(), "signature");
    const auto                    message       = decode_hex(get_json_string(input.get(), "message"));
    cardano_ed25519_public_key_t* raw_public    = nullptr;
    check_result(cardano_ed25519_public_key_from_hex(public_hex.data(), public_hex.size(), &raw_public));

    auto                         public_key    = make_scoped_object(raw_public, cardano_ed25519_public_key_unref);
    cardano_ed25519_signature_t* raw_signature = nullptr;
    check_result(cardano_ed25519_signature_from_hex(signature_hex.data(), signature_hex.size(), &raw_signature));

    auto signature      = make_scoped_object(raw_signature, cardano_ed25519_signature_unref);
    auto valid          = get_json_member(expected.get(), "valid");
    bool expected_valid = false;
    check_result(cardano_json_object_get_boolean(valid.get(), &expected_valid));

    EXPECT_EQ(
      cardano_ed25519_public_verify(public_key.get(), signature.get(), message.data(), message.size()), expected_valid);

    return;
  }
  if (operation == "blake2b")
  {
    auto                    message = decode_hex(get_json_string(input.get(), "message"));
    const byte_t            empty   = 0;
    cardano_blake2b_hash_t* raw     = nullptr;
    const auto              result  = cardano_blake2b_compute_hash(
      message.empty() ? &empty : message.data(), message.size(), get_json_uint(input.get(), "size"), &raw);
    if (cardano_json_object_has_property(expected.get(), "error", 5))
    {
      auto output = make_scoped_object(raw, cardano_blake2b_hash_unref);

      EXPECT_EQ(static_cast<uint64_t>(result), get_json_uint(expected.get(), "native_error"));

      return;
    }
    check_result(result);

    auto hash = make_scoped_object(raw, cardano_blake2b_hash_unref);

    EXPECT_EQ(
      encode_hex(cardano_blake2b_hash_get_data(hash.get()), cardano_blake2b_hash_get_bytes_size(hash.get())),
      get_json_string(expected.get(), "hash"));

    return;
  }
  if (operation == "address")
  {
    auto               data = decode_hex(get_json_string(input.get(), "bytes"));
    cardano_address_t* raw  = nullptr;
    check_result(cardano_address_from_bytes(data.data(), data.size(), &raw));

    auto address = make_scoped_object(raw, cardano_address_unref);

    EXPECT_EQ(std::string(cardano_address_get_string(address.get())), get_json_string(expected.get(), "address"));

    const auto         value  = get_json_string(expected.get(), "address");
    cardano_address_t* parsed = nullptr;
    check_result(cardano_address_from_string(value.data(), value.size(), &parsed));

    auto roundtrip = make_scoped_object(parsed, cardano_address_unref);

    EXPECT_EQ(
      encode_hex(cardano_address_get_bytes(roundtrip.get()), cardano_address_get_bytes_size(roundtrip.get())),
      get_json_string(input.get(), "bytes"));

    return;
  }
  if (operation == "bip32")
  {
    auto                         entropy  = decode_hex(get_json_string(input.get(), "entropy"));
    auto                         password = decode_hex(get_json_string(input.get(), "password"));
    cardano_bip32_private_key_t* raw      = nullptr;
    check_result(cardano_bip32_private_key_from_bip39_entropy(
      password.data(), password.size(), entropy.data(), entropy.size(), &raw));

    auto                  root = make_scoped_object(raw, cardano_bip32_private_key_unref);
    auto                  path = get_json_member(input.get(), "path");
    std::vector<uint32_t> indices;
    for (size_t i = 0; i < cardano_json_object_array_get_length(path.get()); ++i)
    {
      auto     element = make_scoped_object(cardano_json_object_array_get(path.get(), i), cardano_json_object_unref);
      uint64_t index   = 0;
      check_result(cardano_json_object_get_uint(element.get(), &index));
      indices.push_back(static_cast<uint32_t>(index));
    }
    cardano_bip32_private_key_t* derived = nullptr;
    if (indices.empty())
    {
      derived = root.get();
      cardano_bip32_private_key_ref(derived);
    }
    else
    {
      check_result(cardano_bip32_private_key_derive(root.get(), indices.data(), indices.size(), &derived));
    }

    auto key = make_scoped_object(derived, cardano_bip32_private_key_unref);

    EXPECT_EQ(
      encode_hex(cardano_bip32_private_key_get_data(key.get()), cardano_bip32_private_key_get_bytes_size(key.get())),
      get_json_string(expected.get(), "private_key"));

    cardano_bip32_public_key_t* pub = nullptr;
    check_result(cardano_bip32_private_key_get_public_key(key.get(), &pub));

    auto public_key = make_scoped_object(pub, cardano_bip32_public_key_unref);

    EXPECT_EQ(
      encode_hex(
        cardano_bip32_public_key_get_data(public_key.get()), cardano_bip32_public_key_get_bytes_size(public_key.get())),
      get_json_string(expected.get(), "public_key"));

    cardano_ed25519_public_key_t* ed = nullptr;
    check_result(cardano_bip32_public_key_to_ed25519_key(public_key.get(), &ed));

    auto ed_key     = make_scoped_object(ed, cardano_ed25519_public_key_unref);
    char output[65] = { 0 };
    check_result(cardano_ed25519_public_key_to_hex(ed_key.get(), output, sizeof(output)));

    EXPECT_EQ(std::string(output), get_json_string(expected.get(), "ed25519_public_key"));

    return;
  }
  if (operation == "apply_params")
  {
    auto                   script = decode_hex(get_json_string(input.get(), "script"));
    const auto             cbor   = get_json_string(input.get(), "parameters_cbor");
    auto                   reader = make_scoped_object(cardano_cbor_reader_from_hex(cbor.data(), cbor.size()), cardano_cbor_reader_unref);
    cardano_plutus_list_t* raw    = nullptr;
    check_result(cardano_plutus_list_from_cbor(reader.get(), &raw));

    auto              params = make_scoped_object(raw, cardano_plutus_list_unref);
    cardano_buffer_t* output = nullptr;
    check_result(cardano_uplc_apply_params_to_script(params.get(), script.data(), script.size(), &output));

    auto applied = make_scoped_object(output, cardano_buffer_unref);

    EXPECT_EQ(
      encode_hex(cardano_buffer_get_data(applied.get()), cardano_buffer_get_size(applied.get())),
      get_json_string(expected.get(), "script"));

    return;
  }
  FAIL() << "Unsupported operation: " << operation;
}

/**
 * Constructs a native object and adopts its returned reference.
 */
template<typename T, typename F, typename... Args>
static auto
create_scoped_object(void (*release)(T**), F function, Args... args)
{
  T* raw = nullptr;

  const auto error  = function(args..., &raw);
  auto       result = make_scoped_object(raw, release);
  check_result(error);

  return result;
}

/**
 * Decodes a native object and adopts its returned reference.
 */
template<typename T>
static auto
read_cbor(const std::string& cbor, cardano_error_t (*decode)(cardano_cbor_reader_t*, T**), void (*release)(T**))
{
  auto reader = make_scoped_object(cardano_cbor_reader_from_hex(cbor.data(), cbor.size()), cardano_cbor_reader_unref);

  return create_scoped_object(release, decode, reader.get());
}

/**
 * Reads an exact unsigned amount stored as a decimal string.
 */
static uint64_t
get_json_amount(const cardano_json_object_t* value, const char* key)
{
  return std::stoull(get_json_string(value, key));
}

/**
 * Constructs a UTxO from a builder recipe.
 */
static auto
create_utxo(const cardano_json_object_t* value)
{
  const auto address = get_json_string(value, "address");
  const auto tx_id   = get_json_string(value, "tx_id");
  auto       addr    = create_scoped_object(cardano_address_unref, cardano_address_from_string, address.data(), address.size());
  auto
       hash  = create_scoped_object(cardano_blake2b_hash_unref, cardano_blake2b_hash_from_hex, tx_id.data(), tx_id.size());
  auto input = create_scoped_object(
    cardano_transaction_input_unref, cardano_transaction_input_new, hash.get(), get_json_uint(value, "index"));
  auto output = create_scoped_object(
    cardano_transaction_output_unref, cardano_transaction_output_new, addr.get(), get_json_amount(value, "coins"));

  return create_scoped_object(cardano_utxo_unref, cardano_utxo_new, input.get(), output.get());
}

struct callback_probe
{
    std::string kind;
    int64_t     value;

    size_t calls = 0;
};

/**
 * Records callback invocations and returns the configured redeemer result.
 */
static cardano_error_t
deferred_callback(void* context, cardano_transaction_t*, cardano_utxo_list_t*, cardano_plutus_data_t** output)
{
  auto* probe = static_cast<callback_probe*>(context);
  ++probe->calls;
  if (probe->kind == "error")
  {
    return CARDANO_ERROR_GENERIC;
  }
  if (probe->kind == "null")
  {
    return CARDANO_SUCCESS;
  }

  return cardano_plutus_data_new_integer_from_int(probe->value, output);
}

struct built_result
{
    cardano_error_t error;

    std::string cbor;
    std::string inspection;
    bool        callback_invoked;
};
template<typename T>
struct builder_api;

template<>
struct builder_api<cardano_tx_builder_t>
{
    using result_type = cardano_transaction_t;

    static constexpr auto create                            = cardano_tx_builder_new;
    static constexpr auto unref                             = cardano_tx_builder_unref;
    static constexpr auto send_lovelace_ex                  = cardano_tx_builder_send_lovelace_ex;
    static constexpr auto add_script                        = cardano_tx_builder_add_script;
    static constexpr auto add_guard_ex                      = cardano_tx_builder_add_guard_ex;
    static constexpr auto add_guard_with_redeemer           = cardano_tx_builder_add_guard_with_redeemer;
    static constexpr auto mint_token_with_id_ex             = cardano_tx_builder_mint_token_with_id_ex;
    static constexpr auto add_input_with_deferred_redeemer  = cardano_tx_builder_add_input_with_deferred_redeemer;
    static constexpr auto mint_token_with_deferred_redeemer = cardano_tx_builder_mint_token_with_deferred_redeemer;
    static constexpr auto build                             = cardano_tx_builder_build;
    static constexpr auto result_unref                      = cardano_transaction_unref;
    static constexpr auto result_to_cbor                    = cardano_transaction_to_cbor;
    static constexpr auto result_to_cip116_json             = cardano_transaction_to_cip116_json;
};

template<>
struct builder_api<cardano_sub_tx_builder_t>
{
    using result_type = cardano_sub_transaction_t;

    static constexpr auto create                            = cardano_sub_tx_builder_new;
    static constexpr auto unref                             = cardano_sub_tx_builder_unref;
    static constexpr auto send_lovelace_ex                  = cardano_sub_tx_builder_send_lovelace_ex;
    static constexpr auto add_script                        = cardano_sub_tx_builder_add_script;
    static constexpr auto add_guard_ex                      = cardano_sub_tx_builder_add_guard_ex;
    static constexpr auto add_guard_with_redeemer           = cardano_sub_tx_builder_add_guard_with_redeemer;
    static constexpr auto mint_token_with_id_ex             = cardano_sub_tx_builder_mint_token_with_id_ex;
    static constexpr auto add_input_with_deferred_redeemer  = cardano_sub_tx_builder_add_input_with_deferred_redeemer;
    static constexpr auto mint_token_with_deferred_redeemer = cardano_sub_tx_builder_mint_token_with_deferred_redeemer;
    static constexpr auto build                             = cardano_sub_tx_builder_build;
    static constexpr auto result_unref                      = cardano_sub_transaction_unref;
    static constexpr auto result_to_cbor                    = cardano_sub_transaction_to_cbor;
    static constexpr auto result_to_cip116_json             = cardano_sub_transaction_to_cip116_json;
};

static built_result build_fixture(
  const cardano_json_object_t* config,
  const cardano_json_object_t* cases,
  const cardano_json_object_t* recipe,
  size_t                       depth);

/**
 * Executes an ordered builder recipe using only fixture inputs.
 */
template<typename T>
static built_result
build_recipe(
  const cardano_json_object_t* config,
  const cardano_json_object_t* cases,
  const cardano_json_object_t* recipe,
  size_t                       depth)
{
  using api = builder_api<T>;
  if (depth > 8)
  {
    throw std::runtime_error("Cyclic or excessive recipe nesting");
  }

  auto input      = get_json_member(recipe, "input");
  auto parameters = create_scoped_object(cardano_protocol_parameters_unref, cardano_protocol_parameters_new);
  auto numeric    = get_json_member(config, "parameters");
  check_result(
    cardano_protocol_parameters_set_min_fee_a(parameters.get(), get_json_amount(numeric.get(), "min_fee_a")));
  check_result(
    cardano_protocol_parameters_set_min_fee_b(parameters.get(), get_json_amount(numeric.get(), "min_fee_b")));
  check_result(
    cardano_protocol_parameters_set_max_tx_size(parameters.get(), get_json_amount(numeric.get(), "max_tx_size")));
  check_result(
    cardano_protocol_parameters_set_max_value_size(parameters.get(), get_json_amount(numeric.get(), "max_value_size")));
  check_result(cardano_protocol_parameters_set_ada_per_utxo_byte(
    parameters.get(), get_json_amount(numeric.get(), "ada_per_utxo_byte")));
  check_result(cardano_protocol_parameters_set_collateral_percentage(
    parameters.get(), get_json_amount(numeric.get(), "collateral_percentage")));
  check_result(cardano_protocol_parameters_set_max_collateral_inputs(
    parameters.get(), get_json_amount(numeric.get(), "max_collateral_inputs")));
  check_result(
    cardano_protocol_parameters_set_key_deposit(parameters.get(), get_json_amount(numeric.get(), "key_deposit")));
  check_result(
    cardano_protocol_parameters_set_pool_deposit(parameters.get(), get_json_amount(numeric.get(), "pool_deposit")));

  auto pv = get_json_member(
    cardano_json_object_has_property(input.get(), "protocol_version", 16) ? input.get() : config, "protocol_version");
  auto     major         = make_scoped_object(cardano_json_object_array_get(pv.get(), 0), cardano_json_object_unref);
  auto     minor         = make_scoped_object(cardano_json_object_array_get(pv.get(), 1), cardano_json_object_unref);
  uint64_t major_version = 0;
  uint64_t minor_version = 0;
  check_result(cardano_json_object_get_uint(major.get(), &major_version));
  check_result(cardano_json_object_get_uint(minor.get(), &minor_version));

  auto version =
    create_scoped_object(cardano_protocol_version_unref, cardano_protocol_version_new, major_version, minor_version);
  check_result(cardano_protocol_parameters_set_protocol_version(parameters.get(), version.get()));

  auto limits = get_json_member(config, "max_tx_ex_units");
  auto units  = create_scoped_object(
    cardano_ex_units_unref,
    cardano_ex_units_new,
    get_json_amount(limits.get(), "memory"),
    get_json_amount(limits.get(), "steps"));
  check_result(cardano_protocol_parameters_set_max_tx_ex_units(parameters.get(), units.get()));

  auto prices = get_json_member(config, "execution_prices");
  auto price  = [&](const char* kind)
  {
    auto pair = get_json_member(prices.get(), kind);
    auto n    = make_scoped_object(cardano_json_object_array_get(pair.get(), 0), cardano_json_object_unref);
    auto d    = make_scoped_object(cardano_json_object_array_get(pair.get(), 1), cardano_json_object_unref);

    return create_scoped_object(
      cardano_unit_interval_unref,
      cardano_unit_interval_new,
      std::stoull(cardano_json_object_get_string(n.get(), nullptr)),
      std::stoull(cardano_json_object_get_string(d.get(), nullptr)));
  };

  auto memory = price("memory");
  auto steps  = price("steps");
  auto execution =
    create_scoped_object(cardano_ex_unit_prices_unref, cardano_ex_unit_prices_new, memory.get(), steps.get());
  check_result(cardano_protocol_parameters_set_execution_costs(parameters.get(), execution.get()));

  auto                 values = get_json_member(config, "plutus_v4_costs");
  std::vector<int64_t> costs;
  for (size_t i = 0; i < cardano_json_object_array_get_length(values.get()); ++i)
  {
    auto    v    = make_scoped_object(cardano_json_object_array_get(values.get(), i), cardano_json_object_unref);
    int64_t cost = 0;
    check_result(cardano_json_object_get_signed_int(v.get(), &cost));
    costs.push_back(cost);
  }
  auto model = create_scoped_object(
    cardano_cost_model_unref, cardano_cost_model_new, CARDANO_PLUTUS_LANGUAGE_VERSION_V4, costs.data(), costs.size());
  auto models = create_scoped_object(cardano_costmdls_unref, cardano_costmdls_new);
  check_result(cardano_costmdls_insert(models.get(), model.get()));
  check_result(cardano_protocol_parameters_set_cost_models(parameters.get(), models.get()));

  auto                  slot        = get_json_member(config, "slot_config");
  cardano_slot_config_t slot_config = {
    get_json_amount(slot.get(), "zero_time"),
    get_json_amount(slot.get(), "zero_slot"),
    get_json_amount(slot.get(), "slot_length")
  };

  std::vector<std::unique_ptr<callback_probe>> callbacks;
  auto                                         builder = make_scoped_object(api::create(parameters.get(), &slot_config), api::unref);
  if (!builder)
  {
    throw std::runtime_error("Builder allocation failed");
  }
  if constexpr (std::is_same_v<T, cardano_tx_builder_t>)
  {
    auto utxos = get_json_member(input.get(), "utxos");
    auto list  = create_scoped_object(cardano_utxo_list_unref, cardano_utxo_list_new);
    for (size_t i = 0; i < cardano_json_object_array_get_length(utxos.get()); ++i)
    {
      auto v    = make_scoped_object(cardano_json_object_array_get(utxos.get(), i), cardano_json_object_unref);
      auto utxo = create_utxo(v.get());
      check_result(cardano_utxo_list_add(list.get(), utxo.get()));
    }
    cardano_tx_builder_set_utxos(builder.get(), list.get());

    const auto address = get_json_string(input.get(), "change_address");
    cardano_tx_builder_set_change_address_ex(builder.get(), address.data(), address.size());
  }
  auto        operations = get_json_member(input.get(), "steps");
  std::string native_hash;
  for (size_t i = 0; i < cardano_json_object_array_get_length(operations.get()); ++i)
  {
    auto       step = make_scoped_object(cardano_json_object_array_get(operations.get(), i), cardano_json_object_unref);
    const auto op   = get_json_string(step.get(), "op");
    if (op == "send")
    {
      const auto address = get_json_string(step.get(), "address");
      api::send_lovelace_ex(builder.get(), address.data(), address.size(), get_json_amount(step.get(), "amount"));
    }
    else if (op == "script")
    {
      if (get_json_string(step.get(), "language") != "plutus_v4")
      {
        throw std::runtime_error("Unsupported recipe language");
      }

      const auto value  = get_json_string(step.get(), "bytes");
      auto       script = create_scoped_object(
        cardano_plutus_v4_script_unref, cardano_plutus_v4_script_new_bytes_from_hex, value.data(), value.size());
      auto wrapper = create_scoped_object(cardano_script_unref, cardano_script_new_plutus_v4, script.get());
      api::add_script(builder.get(), wrapper.get());
    }
    else if (op == "native_script")
    {
      auto script =
        read_cbor(get_json_string(step.get(), "cbor"), cardano_native_script_from_cbor, cardano_native_script_unref);
      auto hash = make_scoped_object(cardano_native_script_get_hash(script.get()), cardano_blake2b_hash_unref);
      native_hash =
        encode_hex(cardano_blake2b_hash_get_data(hash.get()), cardano_blake2b_hash_get_bytes_size(hash.get()));

      auto wrapper = create_scoped_object(cardano_script_unref, cardano_script_new_native, script.get());
      api::add_script(builder.get(), wrapper.get());
    }
    else if (op == "native_mint")
    {
      const auto asset = native_hash + get_json_string(step.get(), "asset_name");
      api::mint_token_with_id_ex(
        builder.get(), asset.data(), asset.size(), std::stoll(get_json_string(step.get(), "amount")), nullptr);
    }
    else if (op == "guard" || op == "require_guard")
    {
      const auto hash = get_json_string(step.get(), "hash");
      const auto kind = get_json_string(step.get(), "kind");
      if (kind != "key" && kind != "script")
      {
        throw std::runtime_error("Unknown guard kind");
      }

      const auto type       = kind == "key" ? CARDANO_CREDENTIAL_TYPE_KEY_HASH : CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
      auto       credential = read_cbor(
        std::string(type == CARDANO_CREDENTIAL_TYPE_KEY_HASH ? "8200581c" : "8201581c") + hash,
        cardano_credential_from_cbor,
        cardano_credential_unref);
      auto data = make_scoped_object(static_cast<cardano_plutus_data_t*>(nullptr), cardano_plutus_data_unref);
      if (cardano_json_object_has_property(step.get(), "redeemer", 8))
      {
        data = create_scoped_object(
          cardano_plutus_data_unref,
          cardano_plutus_data_new_integer_from_int,
          std::stoll(get_json_string(step.get(), "redeemer")));
      }
      if (op == "require_guard")
      {
        if constexpr (std::is_same_v<T, cardano_sub_tx_builder_t>)
        {
          cardano_sub_tx_builder_require_top_level_guard_ex(builder.get(), hash.data(), hash.size(), type, data.get());
        }
        else
        {
          throw std::runtime_error("Top-level guard requirement belongs to a subtransaction");
        }
      }
      else if (kind == "key")
      {
        api::add_guard_ex(builder.get(), hash.data(), hash.size(), type);
      }
      else
      {
        api::add_guard_with_redeemer(builder.get(), credential.get(), data.get());
      }
    }
    else if (op == "input" || op == "mint")
    {
      auto       cb   = get_json_member(step.get(), "callback");
      const auto kind = get_json_string(cb.get(), "kind");
      if (kind != "constant_integer" && kind != "error" && kind != "null")
      {
        throw std::runtime_error("Unknown callback kind");
      }

      auto probe   = std::make_unique<callback_probe>();
      probe->kind  = kind;
      probe->value = kind == "constant_integer" ? std::stoll(get_json_string(cb.get(), "value")) : 0;

      auto* context = probe.get();
      callbacks.push_back(std::move(probe));
      if (op == "input")
      {
        auto v    = get_json_member(step.get(), "utxo");
        auto utxo = create_utxo(v.get());
        api::add_input_with_deferred_redeemer(builder.get(), utxo.get(), deferred_callback, context, nullptr);
      }
      else
      {
        const auto asset = get_json_string(step.get(), "asset_id");
        auto       hash =
          create_scoped_object(cardano_blake2b_hash_unref, cardano_blake2b_hash_from_hex, asset.data(), size_t(56));
        auto name = create_scoped_object(
          cardano_asset_name_unref, cardano_asset_name_from_hex, asset.data() + 56, asset.size() - 56);
        api::mint_token_with_deferred_redeemer(
          builder.get(),
          hash.get(),
          name.get(),
          std::stoll(get_json_string(step.get(), "amount")),
          deferred_callback,
          context);
      }
    }
    else if constexpr (std::is_same_v<T, cardano_tx_builder_t>)
    {
      if (op == "deposit" || op == "withdraw")
      {
        const auto address = get_json_string(step.get(), "address");
        if (op == "deposit")
        {
          cardano_tx_builder_add_direct_deposit_ex(
            builder.get(), address.data(), address.size(), get_json_amount(step.get(), "amount"));
        }
        else
        {
          cardano_tx_builder_withdraw_rewards_ex(
            builder.get(), address.data(), address.size(), std::stoll(get_json_string(step.get(), "amount")), nullptr);
        }
      }
      else if (op == "balance" || op == "starting_balance")
      {
        const auto address  = get_json_string(step.get(), "address");
        auto       interval = make_scoped_object(
          static_cast<cardano_account_balance_interval_t*>(nullptr), cardano_account_balance_interval_unref);
        if (cardano_json_object_has_property(step.get(), "exact", 5))
        {
          interval = create_scoped_object(
            cardano_account_balance_interval_unref,
            cardano_account_balance_interval_new_exact,
            get_json_amount(step.get(), "exact"));
        }
        else
        {
          uint64_t   lower     = 0;
          uint64_t   upper     = 0;
          const bool has_lower = cardano_json_object_has_property(step.get(), "lower", 5);
          const bool has_upper = cardano_json_object_has_property(step.get(), "upper", 5);
          if (has_lower)
          {
            lower = get_json_amount(step.get(), "lower");
          }
          if (has_upper)
          {
            upper = get_json_amount(step.get(), "upper");
          }
          interval = create_scoped_object(
            cardano_account_balance_interval_unref,
            cardano_account_balance_interval_new,
            has_lower ? &lower : nullptr,
            has_upper ? &upper : nullptr);
        }
        if (op == "balance")
        {
          cardano_tx_builder_add_account_balance_interval_ex(
            builder.get(), address.data(), address.size(), interval.get());
        }
        else
        {
          cardano_tx_builder_add_starting_account_balance_interval_ex(
            builder.get(), address.data(), address.size(), interval.get());
        }
      }
      else if (op == "sub_transaction")
      {
        const auto name  = get_json_string(step.get(), "recipe");
        bool       found = false;
        for (size_t c = 0; c < cardano_json_object_array_get_length(cases); ++c)
        {
          auto child = make_scoped_object(cardano_json_object_array_get(cases, c), cardano_json_object_unref);
          if (get_json_string(child.get(), "id") != name)
          {
            continue;
          }
          if (get_json_string(child.get(), "type") != "sub_transaction")
          {
            throw std::runtime_error("Batch child must be a subtransaction recipe");
          }

          const auto result = build_fixture(config, cases, child.get(), depth + 1);
          check_result(result.error);

          auto sub      = read_cbor(result.cbor, cardano_sub_transaction_from_cbor, cardano_sub_transaction_unref);
          auto resolved = create_scoped_object(cardano_utxo_list_unref, cardano_utxo_list_new);
          cardano_tx_builder_add_sub_transaction(builder.get(), sub.get(), resolved.get());
          found = true;
          break;
        }
        if (!found)
        {
          throw std::runtime_error("Missing subtransaction recipe");
        }
      }
      else
      {
        throw std::runtime_error("Unsupported builder step: " + op);
      }
    }
    else
    {
      throw std::runtime_error("Unsupported subtransaction step: " + op);
    }
  }
  typename api::result_type* raw = nullptr;

  const auto error   = api::build(builder.get(), &raw);
  auto       result  = make_scoped_object(raw, api::result_unref);
  bool       invoked = false;
  for (const auto& cb: callbacks)
  {
    invoked = invoked || cb->calls > 0;
  }
  if (error != CARDANO_SUCCESS)
  {
    return { error, "", "", invoked };
  }
  if constexpr (std::is_same_v<T, cardano_sub_tx_builder_t>)
  {
    if (cardano_json_object_has_property(input.get(), "signing_seed", 12))
    {
      const auto seed = get_json_string(input.get(), "signing_seed");
      auto       key  = create_scoped_object(
        cardano_ed25519_private_key_unref, cardano_ed25519_private_key_from_normal_hex, seed.data(), seed.size());
      auto hash      = make_scoped_object(cardano_sub_transaction_get_id(result.get()), cardano_blake2b_hash_unref);
      auto signature = create_scoped_object(
        cardano_ed25519_signature_unref,
        cardano_ed25519_private_key_sign,
        key.get(),
        cardano_blake2b_hash_get_data(hash.get()),
        cardano_blake2b_hash_get_bytes_size(hash.get()));
      auto pub =
        create_scoped_object(cardano_ed25519_public_key_unref, cardano_ed25519_private_key_get_public_key, key.get());
      auto witness =
        create_scoped_object(cardano_vkey_witness_unref, cardano_vkey_witness_new, pub.get(), signature.get());
      auto witnesses = create_scoped_object(cardano_vkey_witness_set_unref, cardano_vkey_witness_set_new);
      check_result(cardano_vkey_witness_set_add(witnesses.get(), witness.get()));
      check_result(cardano_sub_transaction_apply_vkey_witnesses(result.get(), witnesses.get()));
    }
  }
  auto writer = make_scoped_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
  check_result(api::result_to_cbor(result.get(), writer.get()));

  auto json = make_scoped_object(cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT), cardano_json_writer_unref);
  check_result(api::result_to_cip116_json(result.get(), json.get()));

  char*             output    = encode_json(json.get());
  const std::string inspected = output;
  std::free(output);

  return { CARDANO_SUCCESS, encode_cbor(writer.get()), inspected, invoked };
}

/**
 * Selects the transaction or subtransaction builder for a recipe.
 */
static built_result
build_fixture(
  const cardano_json_object_t* config,
  const cardano_json_object_t* cases,
  const cardano_json_object_t* recipe,
  size_t                       depth)
{
  if (get_json_string(recipe, "type") == "transaction")
  {
    return build_recipe<cardano_tx_builder_t>(config, cases, recipe, depth);
  }
  if (get_json_string(recipe, "type") == "sub_transaction")
  {
    return build_recipe<cardano_sub_tx_builder_t>(config, cases, recipe, depth);
  }
  throw std::runtime_error("Unsupported builder type");
}

/**
 * Compares a built transaction or failure with the expected fixture result.
 */
static void
verify_builder_fixture(
  const cardano_json_object_t* config,
  const cardano_json_object_t* cases,
  const cardano_json_object_t* recipe)
{
  auto       expected      = get_json_member(recipe, "expected");
  const auto actual        = build_fixture(config, cases, recipe, 0);
  auto       invoked       = get_json_member(expected.get(), "callback_invoked");
  bool       should_invoke = false;
  check_result(cardano_json_object_get_boolean(invoked.get(), &should_invoke));

  EXPECT_EQ(actual.callback_invoked, should_invoke);

  if (cardano_json_object_has_property(expected.get(), "error", 5))
  {
    EXPECT_NE(actual.error, CARDANO_SUCCESS);
    EXPECT_EQ(static_cast<uint64_t>(actual.error), get_json_uint(expected.get(), "native_error"));

    return;
  }

  ASSERT_EQ(actual.error, CARDANO_SUCCESS);
  EXPECT_EQ(actual.cbor, get_json_string(expected.get(), "cbor"));

  auto inspection = get_json_member(expected.get(), "inspection");
  auto parsed     = make_scoped_object(
    cardano_json_object_parse(actual.inspection.data(), actual.inspection.size()), cardano_json_object_unref);

  EXPECT_STREQ(
    cardano_json_object_to_json_string(parsed.get(), CARDANO_JSON_FORMAT_COMPACT, nullptr),
    cardano_json_object_to_json_string(inspection.get(), CARDANO_JSON_FORMAT_COMPACT, nullptr));
}

/**
 * Loads and checks every case in a conformance suite.
 */
static void
run_fixture_suite(const char* file)
{
  const auto    path = std::filesystem::path(__FILE__).parent_path() / "vectors" / "bindings" / "v1" / file;
  std::ifstream stream(path);

  ASSERT_TRUE(stream.is_open());

  const std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  auto              root = make_scoped_object(cardano_json_object_parse(content.data(), content.size()), cardano_json_object_unref);

  ASSERT_NE(root, nullptr);
  ASSERT_EQ(get_json_uint(root.get(), "schema_version"), 1U);

  auto cases = get_json_member(root.get(), "cases");

  ASSERT_GT(cardano_json_object_array_get_length(cases.get()), 0U);

  for (size_t i = 0; i < cardano_json_object_array_get_length(cases.get()); ++i)
  {
    auto item = make_scoped_object(cardano_json_object_array_get(cases.get(), i), cardano_json_object_unref);
    SCOPED_TRACE(get_json_string(item.get(), "id"));
    if (get_json_string(item.get(), "operation") == "cbor")
    {
      ASSERT_NO_THROW(verify_cbor_fixture(item.get()));

      if (!testing::Test::HasFatalFailure())
      {
        ASSERT_NO_THROW(verify_cbor_hash_and_signature(item.get()));
      }
    }
    else if (get_json_string(item.get(), "operation") == "build")
    {
      auto config = get_json_member(root.get(), "config");

      ASSERT_NO_THROW(verify_builder_fixture(config.get(), cases.get(), item.get()));
    }
    else
    {
      ASSERT_NO_THROW(verify_primitive_fixture(item.get()));
    }
  }
}

/* UNIT TESTS ****************************************************************/

TEST(binding_vectors, serialization)
{
  run_fixture_suite("serialization.json");
}

TEST(binding_vectors, invalidInputs)
{
  run_fixture_suite("invalid.json");
}

TEST(binding_vectors, primitives)
{
  run_fixture_suite("primitives.json");
}

TEST(binding_vectors, builders)
{
  run_fixture_suite("builders.json");
}

TEST(binding_memory, releasingOneReferenceDoesNotConsumeAnotherOwner)
{
  auto owner = create_scoped_object(cardano_plutus_data_unref, cardano_plutus_data_new_integer_from_int, int64_t(42));

  EXPECT_EQ(cardano_plutus_data_refcount(owner.get()), 1U);

  cardano_plutus_data_t* alias = owner.get();
  cardano_plutus_data_ref(alias);

  EXPECT_EQ(cardano_plutus_data_refcount(owner.get()), 2U);

  cardano_plutus_data_unref(&alias);

  EXPECT_EQ(alias, owner.get());
  EXPECT_EQ(cardano_plutus_data_refcount(owner.get()), 1U);

  // The binding has released its reference even though another owner keeps it alive.
  alias = nullptr;
  cardano_plutus_data_unref(&alias);

  EXPECT_EQ(cardano_plutus_data_refcount(owner.get()), 1U);
}

TEST(binding_memory, ownedGetterSurvivesParentRelease)
{
  auto transaction = read_cbor("84a3008001800200a0f5f6", cardano_transaction_from_cbor, cardano_transaction_unref);
  auto body        = make_scoped_object(cardano_transaction_get_body(transaction.get()), cardano_transaction_body_unref);

  ASSERT_NE(body, nullptr);
  EXPECT_EQ(cardano_transaction_body_refcount(body.get()), 2U);

  transaction.reset();

  EXPECT_EQ(cardano_transaction_body_refcount(body.get()), 1U);

  auto writer = make_scoped_object(cardano_cbor_writer_new(), cardano_cbor_writer_unref);
  check_result(cardano_transaction_body_to_cbor(body.get(), writer.get()));

  EXPECT_EQ(encode_cbor(writer.get()), "a3008001800200");
}

TEST(binding_memory, builderScopesReleaseAfterSuccessAndCallbackFailure)
{
  // Run under Valgrind/LSan: includes successful callbacks, failures, null output,
  // and recursively built signed children. Each invocation owns a separate scope.
  for (size_t iteration = 0; iteration < 16; ++iteration)
  {
    SCOPED_TRACE(iteration);
    run_fixture_suite("builders.json");
  }
}
