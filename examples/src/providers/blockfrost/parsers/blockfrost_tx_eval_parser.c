/**
 * \file blockfrost_tx_eval_parser.c
 *
 * \author angel.castillo
 * \date   Oct 03, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include <cardano/json/json_object.h>

#include "blockfrost_parsers.h"
#include "cardano/witness_set/redeemer_tag.h"
#include "utils.h"
#include <cardano/json/json_writer.h>

#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/script_all.h>
#include <cardano/scripts/native_scripts/script_any.h>
#include <cardano/scripts/native_scripts/script_invalid_after.h>
#include <cardano/scripts/native_scripts/script_invalid_before.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>

#include <stdlib.h>
#include <string.h>

/* FORWARD DECLARATIONS ******************************************************/

/**
 * \brief Serializes a native script clause into a JSON object.
 *
 * This function takes a native script clause, which can be a part of a multi-sig script or any other native script type,
 * and converts it into a JSON format. The serialized clause is then added to the provided JSON object.
 *
 * \param[in] writer A pointer to an initialized \ref cardano_json_writer_t object used for writing JSON data. This parameter must not be NULL.
 * \param[in] script A pointer to a \ref cardano_native_script_t representing the native script clause that is to be serialized. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the clause was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t clause_to_json(cardano_json_writer_t* writer, cardano_native_script_t* script);

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Serializes a signature clause into a JSON object.
 *
 * This function converts a signature clause, typically used in a native script to represent a required signature,
 * into a JSON format and appends it to the provided JSON object.
 *
 * \param[in] writer A pointer to an initialized JSON writer object where the serialized clause will be added. This parameter must not be NULL.
 * \param[in] from A string representing the public key or address that is required to sign the transaction. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the signature clause was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
clause_signature_to_json(cardano_json_writer_t* writer, const char* from)
{
  cardano_json_writer_write_property_name(writer, "clause", strlen("clause"));
  cardano_json_writer_write_string(writer, "signature", strlen("signature"));

  cardano_json_writer_write_property_name(writer, "from", strlen("from"));
  cardano_json_writer_write_string(writer, from, strlen(from));

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a "before" or "after" clause into a JSON object.
 *
 * This function converts a clause of type "before" or "after" (which represents time-locking conditions in a native script)
 * into a JSON format and appends it to the provided JSON object.
 *
 * \param[in] writer A pointer to an initialized JSON writer object where the serialized clause will be added. This parameter must not be NULL.
 * \param[in] clause A string representing the type of clause, either "before" or "after". This parameter must not be NULL.
 * \param[in] slot A uint64_t representing the slot number used in the "before" or "after" clause. This parameter specifies the time-locking condition.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the clause was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
clause_before_after_to_json(cardano_json_writer_t* writer, const char* clause, const uint64_t slot)
{
  cardano_json_writer_write_property_name(writer, "clause", strlen("clause"));
  cardano_json_writer_write_string(writer, clause, strlen(clause));

  cardano_json_writer_write_property_name(writer, "slot", strlen("slot"));
  cardano_json_writer_write_uint(writer, slot);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a recursive native script clause into a JSON object.
 *
 * This function converts a recursive clause (such as "all", "any", or "atLeast") from a \ref cardano_native_script_list_t
 * into a JSON format and appends it to the provided JSON object.
 *
 * \param[in] writer A pointer to an initialized JSON writer object where the serialized clause will be added. This parameter must not be NULL.
 * \param[in] clause A string representing the type of recursive clause (e.g., "all", "any", "atLeast"). This parameter must not be NULL.
 * \param[in] from A pointer to the \ref cardano_native_script_list_t containing the scripts to be serialized. This parameter must not be NULL.
 * \param[in] at_least The "atLeast" clause value. This parameter is used only if the clause is of type "atLeast".
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the clause was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
clause_recursive_to_json(
  cardano_json_writer_t*        writer,
  const char*                   clause,
  cardano_native_script_list_t* from,
  const uint64_t                at_least)
{
  cardano_json_writer_write_property_name(writer, "clause", strlen("clause"));
  cardano_json_writer_write_string(writer, clause, strlen(clause));

  if (at_least > 0U)
  {
    cardano_json_writer_write_property_name(writer, "atLeast", strlen("atLeast"));
    cardano_json_writer_write_uint(writer, at_least);
  }

  cardano_json_writer_write_property_name(writer, "from", strlen("from"));
  cardano_json_writer_write_start_array(writer);
  const size_t from_len = cardano_native_script_list_get_length(from);

  for (size_t i = 0U; i < from_len; ++i)
  {
    cardano_native_script_t* native_script = NULL;

    cardano_error_t result = cardano_native_script_list_get(from, i, &native_script);
    cardano_native_script_unref(&native_script);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = clause_to_json(writer, native_script);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  cardano_json_writer_write_end_array(writer);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a native script clause into a JSON object.
 *
 * This function converts a specific clause of a \ref cardano_native_script_t object into a JSON format
 * and appends it to the provided JSON object.
 *
 * \param[in] writer A pointer to the initialized \ref cardano_json_writer_t object used for writing JSON data. This parameter must not be NULL.
 * \param[in] script A pointer to the \ref cardano_native_script_t object containing the clause to serialize. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the clause was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
clause_to_json(cardano_json_writer_t* writer, cardano_native_script_t* script)
{
  cardano_native_script_type_t type   = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY;
  cardano_error_t              result = cardano_native_script_get_type(script, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_json_writer_write_start_object(writer);

  switch (type)
  {
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY:
    {
      cardano_script_pubkey_t* pubkey_script = NULL;
      result                                 = cardano_native_script_to_pubkey(script, &pubkey_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      cardano_script_pubkey_unref(&pubkey_script);

      cardano_blake2b_hash_t* hash = NULL;

      result = cardano_script_pubkey_get_key_hash(pubkey_script, &hash);
      cardano_blake2b_hash_unref(&hash);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t hash_size = cardano_blake2b_hash_get_hex_size(hash);
      char*        hash_str  = malloc(hash_size);

      if (hash_str == NULL)
      {
        cardano_blake2b_hash_unref(&hash);
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      result = cardano_blake2b_hash_to_hex(hash, hash_str, hash_size);

      if (result != CARDANO_SUCCESS)
      {
        free(hash_str);
        cardano_blake2b_hash_unref(&hash);
        return result;
      }

      result = clause_signature_to_json(writer, hash_str);

      free(hash_str);

      cardano_json_writer_write_end_object(writer);
      return result;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE:
    {
      cardano_script_invalid_before_t* invalid_before_script = NULL;

      result = cardano_native_script_to_invalid_before(script, &invalid_before_script);
      cardano_script_invalid_before_unref(&invalid_before_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      uint64_t slot = 0;
      result        = cardano_script_invalid_before_get_slot(invalid_before_script, &slot);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = clause_before_after_to_json(writer, "before", slot);

      cardano_json_writer_write_end_object(writer);
      return result;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER:
    {
      cardano_script_invalid_after_t* invalid_after_script = NULL;

      result = cardano_native_script_to_invalid_after(script, &invalid_after_script);
      cardano_script_invalid_after_unref(&invalid_after_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      uint64_t slot = 0;
      result        = cardano_script_invalid_after_get_slot(invalid_after_script, &slot);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = clause_before_after_to_json(writer, "after", slot);

      cardano_json_writer_write_end_object(writer);
      return result;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF:
    {
      cardano_script_any_t* any_script = NULL;

      result = cardano_native_script_to_any(script, &any_script);
      cardano_script_any_unref(&any_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      cardano_native_script_list_t* from = NULL;

      result = cardano_script_any_get_scripts(any_script, &from);
      cardano_native_script_list_unref(&from);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      return clause_recursive_to_json(writer, "any", from, 0);
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF:
    {
      cardano_script_all_t* all_script = NULL;

      result = cardano_native_script_to_all(script, &all_script);
      cardano_script_all_unref(&all_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      cardano_native_script_list_t* from = NULL;

      result = cardano_script_all_get_scripts(all_script, &from);
      cardano_native_script_list_unref(&from);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      return clause_recursive_to_json(writer, "all", from, 0);
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K:
    {
      cardano_script_n_of_k_t* n_of_k_script = NULL;

      result = cardano_native_script_to_n_of_k(script, &n_of_k_script);
      cardano_script_n_of_k_unref(&n_of_k_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      cardano_native_script_list_t* from = NULL;

      result = cardano_script_n_of_k_get_scripts(n_of_k_script, &from);
      cardano_native_script_list_unref(&from);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      uint64_t at_least = cardano_script_n_of_k_get_required(n_of_k_script);

      return clause_recursive_to_json(writer, "n_of_k", from, at_least);
    }
    default:
      return CARDANO_ERROR_INVALID_ARGUMENT;
  }
}

/**
 * \brief Serializes a native script into a JSON object.
 *
 * This function serializes the given \ref cardano_native_script_t object, which represents a native script,
 * into a JSON representation and appends it to the provided JSON object.
 *
 * \param[in] writer A pointer to the \ref cardano_json_writer_t object used for writing JSON data. This parameter must not be NULL.
 * \param[in] script A pointer to the \ref cardano_native_script_t object representing the native script. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
script_native_to_json(cardano_json_writer_t* writer, cardano_native_script_t* script)
{
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "script", strlen("script"));
  cardano_json_writer_write_property_name(writer, "language", strlen("language"));
  cardano_json_writer_write_string(writer, "native", strlen("native"));

  cardano_json_writer_write_property_name(writer, "json", strlen("json"));
  cardano_error_t result = clause_to_json(writer, script);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_json_writer_write_end_object(writer);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a transaction input into a JSON object.
 *
 * This function serializes the given \ref cardano_transaction_input_t object, which represents the input of a transaction,
 * into a JSON representation and appends it to the provided JSON object.
 *
 * \param[in] input A pointer to the \ref cardano_transaction_input_t object representing the transaction input. This parameter must not be NULL.
 * \param[out] writer A pointer to the initialized \ref cardano_json_writer_t object where the serialized transaction input will be added. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the input was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
transaction_input_to_json(
  cardano_transaction_input_t* input,
  cardano_json_writer_t*       writer)
{
  const uint64_t          index = cardano_transaction_input_get_index(input);
  cardano_blake2b_hash_t* hash  = cardano_transaction_input_get_id(input);

  const size_t hash_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hash_str  = malloc(hash_size);

  if (hash_str == NULL)
  {
    cardano_blake2b_hash_unref(&hash);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_blake2b_hash_to_hex(hash, hash_str, hash_size);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    free(hash_str);
    return result;
  }

  cardano_json_writer_write_property_name(writer, "id", strlen("id"));
  cardano_json_writer_write_string(writer, hash_str, strlen(hash_str));

  cardano_json_writer_write_property_name(writer, "index", strlen("index"));
  cardano_json_writer_write_uint(writer, index);

  free(hash_str);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a transaction value into a JSON object.
 *
 * This function serializes the given \ref cardano_value_t object, which represents the value (amount of ADA and multi-assets) in a transaction,
 * into a JSON representation and appends it to the provided JSON object.
 *
 * \param[in] value A pointer to the \ref cardano_value_t object representing the transaction value. This parameter must not be NULL.
 * \param[out] writer A pointer to the initialized \ref cardano_json_writer_t object where the serialized transaction value will be added. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the value was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
transaction_value_to_json(
  cardano_value_t*       value,
  cardano_json_writer_t* writer)
{
  const uint64_t         lovelace    = cardano_value_get_coin(value);
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);

  cardano_multi_asset_unref(&multi_asset);

  cardano_json_writer_write_property_name(writer, "value", strlen("value"));
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "ada", strlen("ada"));
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "lovelace", strlen("lovelace"));
  cardano_json_writer_write_uint(writer, lovelace);
  cardano_json_writer_write_end_object(writer);

  cardano_policy_id_list_t* policy_id_list = NULL;
  cardano_error_t           result         = cardano_multi_asset_get_keys(multi_asset, &policy_id_list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t policy_id_list_size = cardano_policy_id_list_get_length(policy_id_list);

  for (size_t i = 0; i < policy_id_list_size; i++)
  {
    cardano_blake2b_hash_t* policy_id = NULL;
    result                            = cardano_policy_id_list_get(policy_id_list, i, &policy_id);

    if (result != CARDANO_SUCCESS)
    {
      cardano_policy_id_list_unref(&policy_id_list);

      return result;
    }

    const size_t policy_id_size = cardano_blake2b_hash_get_hex_size(policy_id);
    char*        policy_id_str  = malloc(policy_id_size);

    if (policy_id_str == NULL)
    {
      cardano_blake2b_hash_unref(&policy_id);
      cardano_policy_id_list_unref(&policy_id_list);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    result = cardano_blake2b_hash_to_hex(policy_id, policy_id_str, policy_id_size);
    cardano_blake2b_hash_unref(&policy_id);

    if (result != CARDANO_SUCCESS)
    {
      free(policy_id_str);
      cardano_policy_id_list_unref(&policy_id_list);
      return result;
    }

    cardano_json_writer_write_property_name(writer, policy_id_str, strlen(policy_id_str));
    cardano_json_writer_write_start_object(writer);

    cardano_asset_name_map_t* assets = NULL;
    result                           = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);

    if (result != CARDANO_SUCCESS)
    {
      free(policy_id_str);
      cardano_policy_id_list_unref(&policy_id_list);

      return result;
    }

    cardano_asset_name_list_t* asset_names = NULL;
    result                                 = cardano_asset_name_map_get_keys(assets, &asset_names);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_name_map_unref(&assets);
      free(policy_id_str);
      cardano_policy_id_list_unref(&policy_id_list);
      return result;
    }

    const size_t asset_names_size = cardano_asset_name_list_get_length(asset_names);

    for (size_t j = 0; j < asset_names_size; j++)
    {
      cardano_asset_name_t* asset_name = NULL;
      result                           = cardano_asset_name_list_get(asset_names, j, &asset_name);

      if (result != CARDANO_SUCCESS)
      {
        cardano_asset_name_list_unref(&asset_names);
        cardano_asset_name_map_unref(&assets);
        free(policy_id_str);
        cardano_policy_id_list_unref(&policy_id_list);
        return result;
      }

      int64_t asset_quantity = 0;
      result                 = cardano_asset_name_map_get(assets, asset_name, &asset_quantity);

      if (result != CARDANO_SUCCESS)
      {
        cardano_asset_name_unref(&asset_name);
        cardano_asset_name_list_unref(&asset_names);
        cardano_asset_name_map_unref(&assets);
        free(policy_id_str);
        cardano_policy_id_list_unref(&policy_id_list);

        return result;
      }

      const char* asset_name_str = cardano_asset_name_get_hex(asset_name);

      cardano_json_writer_write_property_name(writer, asset_name_str, strlen(asset_name_str));
      cardano_json_writer_write_signed_int(writer, asset_quantity);

      cardano_asset_name_unref(&asset_name);
    }

    cardano_json_writer_write_end_object(writer);

    cardano_asset_name_list_unref(&asset_names);
    cardano_asset_name_map_unref(&assets);
    free(policy_id_str);
  }

  cardano_json_writer_write_end_object(writer);

  cardano_policy_id_list_unref(&policy_id_list);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a transaction output's address into a JSON object.
 *
 * This function serializes the given \ref cardano_address_t object into a JSON representation and appends it to the provided JSON object.
 * The address is a fundamental part of a transaction output, representing the recipient of the funds.
 *
 * \param[in] address A pointer to the \ref cardano_address_t object representing the output address. This parameter must not be NULL.
 * \param[out] writer A pointer to the initialized \ref cardano_json_writer_t object where the serialized address will be added. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the address was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
transaction_output_address_to_json(
  cardano_address_t*     address,
  cardano_json_writer_t* writer)
{
  const char* bech32 = cardano_address_get_string(address);

  if (bech32 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_writer_write_property_name(writer, "address", strlen("address"));
  cardano_json_writer_write_string(writer, bech32, strlen(bech32));

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a transaction output's datum into a JSON object.
 *
 * This function serializes the given \ref cardano_datum_t object into a JSON representation and appends it to the provided JSON object.
 * The datum is a critical component of the output in a Plutus transaction, where it can be either inline or referenced by a hash.
 *
 * \param[in] datum A pointer to the \ref cardano_datum_t object that holds the datum to be serialized. This parameter must not be NULL.
 * \param[out] json_writer A pointer to the initialized \ref cardano_json_writer_t object where the serialized datum will be added. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the datum was successfully serialized,
 * or an appropriate error code if serialization failed.
 */
static cardano_error_t
transaction_output_datum_to_json(
  cardano_datum_t*       datum,
  cardano_json_writer_t* json_writer)
{
  cardano_datum_type_t type   = CARDANO_DATUM_TYPE_DATA_HASH;
  cardano_error_t      result = cardano_datum_get_type(datum, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_DATUM_TYPE_DATA_HASH:
    {
      const char* hash_str = cardano_datum_get_data_hash_hex(datum);

      cardano_json_writer_write_property_name(json_writer, "datumHash", strlen("datumHash"));
      cardano_json_writer_write_string(json_writer, hash_str, strlen(hash_str));

      break;
    }
    case CARDANO_DATUM_TYPE_INLINE_DATA:
    {
      cardano_plutus_data_t* data = cardano_datum_get_inline_data(datum);

      if (data == NULL)
      {
        return CARDANO_ERROR_POINTER_IS_NULL;
      }

      cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

      if (writer == NULL)
      {
        cardano_plutus_data_unref(&data);
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      result = cardano_plutus_data_to_cbor(data, writer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_cbor_writer_unref(&writer);
        cardano_plutus_data_unref(&data);

        return result;
      }

      const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
      char*        cbor_str  = malloc(cbor_size);

      if (cbor_str == NULL)
      {
        cardano_cbor_writer_unref(&writer);
        cardano_plutus_data_unref(&data);

        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      result = cardano_cbor_writer_encode_hex(writer, cbor_str, cbor_size);

      if (result != CARDANO_SUCCESS)
      {
        free(cbor_str);
        cardano_cbor_writer_unref(&writer);
        cardano_plutus_data_unref(&data);

        return result;
      }

      cardano_json_writer_write_property_name(json_writer, "datum", strlen("datum"));
      cardano_json_writer_write_string(json_writer, cbor_str, strlen(cbor_str));

      free(cbor_str);
      cardano_cbor_writer_unref(&writer);
      cardano_plutus_data_unref(&data);
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Returns the string representation of a Plutus script language.
 *
 * This function returns a constant string that represents the given Plutus script language, based on the \ref cardano_script_language_t enum value.
 * It helps in translating the language enum into a human-readable string for use in JSON serialization or logging.
 *
 * \param[in] language The \ref cardano_script_language_t representing the Plutus script language. It can be one of the supported languages, such as PlutusV1 or PlutusV2.
 *
 * \return A constant character pointer to the string representation of the language. If the language is unknown, it returns "Unknown".
 */
static const char*
get_plutus_script_string(cardano_script_language_t language)
{
  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      return "plutus:v1";
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      return "plutus:v2";
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      return "plutus:v3";
    default:
      return "native";
  }
}

/**
 * \brief Serializes a Plutus script within a transaction output into a JSON object.
 *
 * This function serializes the given Plutus script, represented by \ref cardano_script_t, into a JSON object, adding the result to the provided \c json_output object.
 * The script is associated with the specified \c language.
 *
 * \param[in] language The \ref cardano_script_language_t representing the language of the Plutus script.
 *                     It determines the type of Plutus script (e.g., PlutusV1 or PlutusV2).
 * \param[in] script A pointer to the \ref cardano_script_t representing the Plutus script to be serialized. This must not be NULL.
 * \param[out] json_writer A pointer to an initialized \ref cardano_json_writer_t where the serialized script data will be added. This must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script was successfully serialized,
 * or an appropriate error code if an error occurred.
 */
static cardano_error_t
transaction_output_plutus_script_to_json(
  cardano_script_language_t language,
  cardano_script_t*         script,
  cardano_json_writer_t*    json_writer)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_script_to_cbor(script, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_str  = malloc(cbor_size);

  if (cbor_str == NULL)
  {
    cardano_cbor_writer_unref(&writer);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_cbor_writer_encode_hex(writer, cbor_str, cbor_size);

  if (result != CARDANO_SUCCESS)
  {
    free(cbor_str);
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  cardano_json_writer_write_property_name(json_writer, "script", strlen("script"));
  cardano_json_writer_write_start_object(json_writer);

  cardano_json_writer_write_property_name(json_writer, "language", strlen("language"));
  cardano_json_writer_write_string(json_writer, get_plutus_script_string(language), strlen(get_plutus_script_string(language)));

  cardano_json_writer_write_property_name(json_writer, "cbor", strlen("cbor"));
  cardano_json_writer_write_string(json_writer, cbor_str, strlen(cbor_str));

  cardano_json_writer_write_end_object(json_writer);

  free(cbor_str);
  cardano_cbor_writer_unref(&writer);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a transaction output script into a JSON object.
 *
 * This function serializes the given \ref cardano_script_t into a JSON object representation, adding the result to the provided \c json_output object.
 *
 * \param[in] script A pointer to the \ref cardano_script_t representing the script to be serialized. This must not be NULL.
 * \param[out] json_writer A pointer to an initialized \ref cardano_json_writer_t where the serialized script data will be added. This must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script was successfully serialized,
 * or an appropriate error code if an error occurred.
 */
static cardano_error_t
transaction_output_script_to_json(
  cardano_script_t*      script,
  cardano_json_writer_t* json_writer)
{
  cardano_script_language_t language = CARDANO_SCRIPT_LANGUAGE_NATIVE;
  cardano_error_t           result   = cardano_script_get_language(script, &language);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
    {
      cardano_native_script_t* native_script = NULL;

      result = cardano_script_to_native(script, &native_script);
      cardano_native_script_unref(&native_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      return script_native_to_json(json_writer, native_script);
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      return transaction_output_plutus_script_to_json(language, script, json_writer);
    }
    default:
    {
      return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
    }
  }
}

/**
 * \brief Serializes a transaction output into a JSON object.
 *
 * This function serializes the given \ref cardano_transaction_output_t into a JSON object representation.
 * The serialized transaction output will be added to the provided \c main_obj.
 *
 * \param[in] output A pointer to the \ref cardano_transaction_output_t representing the transaction output to be serialized. This must not be NULL.
 * \param[out] writer A pointer to an initialized \ref cardano_json_writer_t where the serialized transaction output will be added. This must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the transaction output was successfully serialized,
 * or an appropriate error code if an error occurred.
 */
static cardano_error_t
transaction_output_to_json(
  cardano_transaction_output_t* output,
  cardano_json_writer_t*        writer)
{
  cardano_address_t* address = cardano_transaction_output_get_address(output);
  cardano_address_unref(&address);

  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = transaction_output_address_to_json(address, writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = transaction_value_to_json(value, writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_datum_t* datum = cardano_transaction_output_get_datum(output);
  cardano_datum_unref(&datum);

  if (datum != NULL)
  {
    result = transaction_output_datum_to_json(datum, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  cardano_script_t* script = cardano_transaction_output_get_script_ref(output);
  cardano_script_unref(&script);

  if (script != NULL)
  {
    result = transaction_output_script_to_json(script, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a list of UTXOs into a JSON object.
 *
 * This function serializes the given \ref cardano_utxo_list_t into a JSON object representation.
 * The serialized UTXOs will be added to the provided \c json_main_obj.
 *
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t representing the UTXOs to be serialized. This must not be NULL.
 * \param[out] writer A pointer to an initialized \ref cardano_json_writer_t where the serialized UTXOs will be added. This must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXOs were successfully serialized,
 * or an appropriate error code if an error occurred.
 */
static cardano_error_t
additional_utxos_to_json(
  cardano_utxo_list_t*   utxos,
  cardano_json_writer_t* writer)
{
  const size_t utxos_len = cardano_utxo_list_get_length(utxos);

  cardano_json_writer_write_property_name(writer, "additionalUtxo", strlen("additionalUtxo"));
  cardano_json_writer_write_start_array(writer);

  if (utxos == NULL)
  {
    cardano_json_writer_write_end_array(writer);
    return CARDANO_SUCCESS;
  }

  for (size_t i = 0U; i < utxos_len; ++i)
  {
    cardano_json_writer_write_start_object(writer);

    cardano_utxo_t* utxo = NULL;

    cardano_error_t result = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    if (input == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    result = transaction_input_to_json(input, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_json_writer_write_end_object(writer);

    cardano_json_writer_write_start_object(writer);

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    if (output == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    result = transaction_output_to_json(output, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_json_writer_write_end_object(writer);
  }

  cardano_json_writer_write_end_array(writer);

  return CARDANO_SUCCESS;
}

/**
 * \brief Serializes a Cardano transaction into a JSON object.
 *
 * This function serializes the given \ref cardano_transaction_t into a JSON object representation.
 * The output \c out_obj must be an initialized \ref json_object where the serialized transaction will be stored.
 *
 * \param[in] transaction A pointer to the \ref cardano_transaction_t to be serialized. This must not be NULL.
 * \param[out] out_obj A pointer to an initialized \ref json_object where the serialized transaction data will be added. This must not be NULL.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the transaction was successfully serialized,
 * or an appropriate error code if an error occurred.
 */
static cardano_error_t
cardano_transaction_to_json(
  cardano_transaction_t* transaction,
  cardano_json_writer_t* writer)
{
  cardano_cbor_writer_t* cbor_writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_transaction_to_cbor(transaction, cbor_writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&cbor_writer);
    return result;
  }

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(cbor_writer);
  char*        cbor_str  = malloc(cbor_size);

  if (cbor_str == NULL)
  {
    cardano_cbor_writer_unref(&cbor_writer);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_cbor_writer_encode_hex(cbor_writer, cbor_str, cbor_size);

  if (result != CARDANO_SUCCESS)
  {
    free(cbor_str);
    cardano_cbor_writer_unref(&cbor_writer);
    return result;
  }

  cardano_json_writer_write_property_name(writer, "cbor", strlen("cbor"));
  cardano_json_writer_write_string(writer, cbor_str, strlen(cbor_str));

  free(cbor_str);
  cardano_cbor_writer_unref(&cbor_writer);

  return CARDANO_SUCCESS;
}

/**
 * \brief Converts a redeemer tag string to its corresponding enum value.
 *
 * This function takes a string representing a redeemer tag (e.g., "spend", "mint", etc.) and converts it into the corresponding
 * \ref cardano_redeemer_tag_t enum value.
 *
 * \param[in] tag_str A pointer to the string representing the redeemer tag. This must not be NULL.
 * \param[out] tag_enum A pointer to a \ref cardano_redeemer_tag_t enum where the result will be stored if the conversion is successful. This must not be NULL.
 *
 * \return <tt>true</tt> if the conversion is successful, otherwise <tt>false</tt>.
 */
static bool
redeemer_tag_string_to_enum(const char* tag_str, cardano_redeemer_tag_t* tag_enum)
{
  if (strcmp(tag_str, "spend") == 0)
  {
    *tag_enum = CARDANO_REDEEMER_TAG_SPEND;
  }
  else if (strcmp(tag_str, "mint") == 0)
  {
    *tag_enum = CARDANO_REDEEMER_TAG_MINT;
  }
  else if (strcmp(tag_str, "certificate") == 0)
  {
    *tag_enum = CARDANO_REDEEMER_TAG_CERTIFYING;
  }
  else if (strcmp(tag_str, "withdrawal") == 0)
  {
    *tag_enum = CARDANO_REDEEMER_TAG_REWARD;
  }
  else if (strcmp(tag_str, "vote") == 0)
  {
    *tag_enum = CARDANO_REDEEMER_TAG_VOTING;
  }
  else if (strcmp(tag_str, "propose") == 0)
  {
    *tag_enum = CARDANO_REDEEMER_TAG_PROPOSING;
  }
  else
  {
    return false;
  }

  return true;
}

/* PUBLIC FUNCTIONS **********************************************************/

cardano_error_t
cardano_evaluate_params_to_json(
  cardano_transaction_t* transaction,
  cardano_utxo_list_t*   utxos,
  char**                 json_main_obj_str,
  size_t*                json_main_obj_size)
{
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  cardano_json_writer_write_start_object(writer);
  cardano_error_t result = cardano_transaction_to_json(transaction, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_writer_unref(&writer);
    return result;
  }

  result = additional_utxos_to_json(utxos, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_writer_unref(&writer);
    return result;
  }

  cardano_json_writer_write_end_object(writer);

  *json_main_obj_size = cardano_json_writer_get_encoded_size(writer);

  if ((*json_main_obj_size) == 0)
  {
    cardano_json_writer_unref(&writer);
    return CARDANO_ERROR_INVALID_JSON;
  }

  *json_main_obj_str  = (char*)malloc(*json_main_obj_size);
  result              = cardano_json_writer_encode(writer, *json_main_obj_str, *json_main_obj_size);
  *json_main_obj_size = (*json_main_obj_size) - 1; // Remove the null terminator

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_writer_unref(&writer);
    free((void*)json_main_obj_str);
    return result;
  }

  cardano_json_writer_unref(&writer);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_blockfrost_parse_tx_eval_response(
  cardano_provider_impl_t*  provider,
  const char*               json,
  const size_t              size,
  cardano_redeemer_list_t*  original_redeemers,
  cardano_redeemer_list_t** redeemers)
{
  cardano_error_t result = cardano_redeemer_list_clone(original_redeemers, redeemers);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_json_object_t* parsed_json = cardano_json_object_parse(json, size);

  if (parsed_json == NULL)
  {
    cardano_redeemer_list_unref(redeemers);
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* result_obj;
  cardano_json_object_t* evaluation_result_obj;

  if (!cardano_json_object_get_ex(parsed_json, "result", 6, &result_obj))
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");
    cardano_json_object_unref(&parsed_json);
    cardano_redeemer_list_unref(redeemers);

    return CARDANO_ERROR_INVALID_JSON;
  }

  if (cardano_json_object_get_ex(result_obj, "EvaluationFailure", 17, &evaluation_result_obj))
  {
    cardano_utils_set_error_message(provider, "Failed evaluate scripts");
    cardano_json_object_unref(&parsed_json);
    cardano_redeemer_list_unref(redeemers);

    return CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE;
  }

  if (!cardano_json_object_get_ex(result_obj, "EvaluationResult", 16, &evaluation_result_obj))
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");
    cardano_json_object_unref(&parsed_json);

    cardano_redeemer_list_unref(redeemers);

    return CARDANO_ERROR_INVALID_JSON;
  }

  const size_t evaluation_result_size = cardano_json_object_get_property_count(evaluation_result_obj);

  for (size_t i = 0; i < evaluation_result_size; i++)
  {
    size_t                 key_size;
    const char*            key = cardano_json_object_get_key_at(evaluation_result_obj, i, &key_size);
    cardano_json_object_t* val = cardano_json_object_get_value_at_ex(evaluation_result_obj, i);

    const char* colon_ptr = strchr(key, ':');

    if (colon_ptr == NULL)
    {
      continue;
    }

    const size_t tag_length   = colon_ptr - key;
    char         tag_str[128] = { 0 };

    if (tag_length >= sizeof(tag_str))
    {
      continue;
    }

    cardano_utils_safe_memcpy(tag_str, 128, key, tag_length);
    tag_str[tag_length] = '\0';

    const char* index_str = colon_ptr + 1;

    if (*index_str == '\0')
    {
      continue;
    }

    char*      endptr = NULL;
    const long index  = strtol(index_str, &endptr, 10);

    if ((endptr == NULL) || (*endptr != '\0'))
    {
      continue;
    }

    cardano_redeemer_tag_t tag_enum;

    if (!redeemer_tag_string_to_enum(tag_str, &tag_enum))
    {
      continue;
    }

    cardano_json_object_t* memory_obj = NULL;
    cardano_json_object_t* steps_obj  = NULL;

    if (cardano_json_object_get_ex(val, "memory", 6, &memory_obj) && cardano_json_object_get_ex(val, "steps", 5, &steps_obj))
    {
      uint64_t memory = 0U;
      uint64_t steps  = 0U;

      result = cardano_json_object_get_uint(memory_obj, &memory);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utils_set_error_message(provider, "Failed to parse JSON response");
        cardano_json_object_unref(&parsed_json);
        cardano_redeemer_list_unref(redeemers);

        return result;
      }

      result = cardano_json_object_get_uint(steps_obj, &steps);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utils_set_error_message(provider, "Failed to parse JSON response");
        cardano_json_object_unref(&parsed_json);
        cardano_redeemer_list_unref(redeemers);

        return result;
      }

      result = cardano_redeemer_list_set_ex_units(*redeemers, tag_enum, index, memory, steps);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utils_set_error_message(provider, "Failed to parse JSON response");
        cardano_json_object_unref(&parsed_json);
        cardano_redeemer_list_unref(redeemers);

        return result;
      }
    }
    else
    {
      continue;
    }
  }

  cardano_json_object_unref(&parsed_json);

  return CARDANO_SUCCESS;
}