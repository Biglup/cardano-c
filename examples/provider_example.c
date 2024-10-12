/**
 * \file provider_example.c
 *
 * \author angel.castillo
 * \date   Sep 28, 2024
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

#include "providers/provider_factory.h"

#include "utils/console.h"
#include "utils/utils.h"
#include <cardano/cardano.h>

#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const uint64_t API_KEY_MAX_LENGTH = 39U;

/* DECLARATIONS **************************************************************/

/**
 * \brief Creates a Cardano address object from a string representation.
 *
 * This function creates a \ref cardano_address_t object from the provided address string. The address string can represent a
 * Bech32 or hexadecimal Cardano address. The function validates the address format and initializes the corresponding
 * address object.
 *
 * \param[in] address A pointer to the address string. This parameter must not be NULL.
 * \param[in] address_length The length of the address string in bytes.
 *
 * \return A pointer to the newly created \ref cardano_address_t object if the address is valid. Returns NULL if the
 * address creation fails due to an invalid address format or any internal error.
 */
static cardano_address_t*
create_address(const char* address, size_t address_length);

/**
 * \brief Creates a Cardano reward address object from a string representation.
 *
 * This function creates a \ref cardano_reward_address_t object from the provided address string. The address string can represent
 * a valid Bech32-encoded Cardano reward address. The function validates the address format and initializes the corresponding
 * reward address object.
 *
 * \param[in] address_str A pointer to the reward address string. This parameter must not be NULL.
 * \param[in] address_str_length The length of the reward address string in bytes.
 *
 * \return A pointer to the newly created \ref cardano_reward_address_t object if the address is valid. Returns NULL if the
 * address creation fails due to an invalid address format or any internal error.
 */
static cardano_reward_address_t*
create_reward_address(const char* address_str, size_t address_str_length);

/**
 * \brief Displays a summary of the total balance, including UTXO and reward balances.
 *
 * This function aggregates the ADA from all UTXOs in the provided \ref cardano_utxo_list_t and combines it with the reward balance.
 * The resulting balances are displayed, including:
 * - The total amount of ADA in the UTXOs.
 * - The reward balance.
 * - The total of UTXO balance + reward balance.
 *
 * \param[in] utxos A pointer to the UTXO list (\ref cardano_utxo_list_t) from which the ADA amounts will be summed.
 *                  This parameter must not be NULL.
 * \param[in] reward_balance The total amount of rewards as a \c uint64_t.
 *
 * \return \ref cardano_error_t indicating success or an error if the UTXO list cannot be processed.
 */
static cardano_error_t
display_balance(cardano_utxo_list_t* utxos, uint64_t reward_balance);

/* MAIN **********************************************************************/

/**
 * \brief Entry point of the program.
 *
 * \return Returns `0` on successful execution, or a non-zero value if there is an error.
 */
int
main(void)
{
  // Preprod addresses.
  static const char* address       = "addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk";
  static const char* stake_address = "stake_test1urk0s687df3s2g0tdtnj7qpefa36gnjdv68upkpdfxkgkqq8kq6ly";

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  cardano_provider_t* provider = NULL;

  cardano_error_t result = create_blockfrost_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key, cardano_utils_safe_strlen(api_key, API_KEY_MAX_LENGTH), &provider);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create blockfrost provider: %s\n", cardano_provider_get_last_error(provider));
    cardano_provider_unref(&provider);

    return EXIT_FAILURE;
  }

  console_info("Cardano Provider Example");
  console_debug("libcardano-c:  V-%s", cardano_get_lib_version());
  console_debug("Provider name: %s\n", cardano_provider_get_name(provider));

  cardano_address_t* payment_address = create_address(address, cardano_utils_safe_strlen(address, 128));

  if (payment_address == NULL)
  {
    cardano_provider_unref(&provider);

    return EXIT_FAILURE;
  }

  cardano_reward_address_t* reward_address = create_reward_address(stake_address, cardano_utils_safe_strlen(stake_address, 128));

  if (reward_address == NULL)
  {
    cardano_address_unref(&payment_address);
    cardano_provider_unref(&provider);

    return EXIT_FAILURE;
  }

  uint64_t rewards_available = 0U;
  result                     = cardano_provider_get_rewards_available(provider, reward_address, &rewards_available);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get rewards available: %s", cardano_provider_get_last_error(provider));

    cardano_provider_unref(&provider);
    cardano_address_unref(&payment_address);
    cardano_reward_address_unref(&reward_address);

    return EXIT_FAILURE;
  }

  cardano_utxo_list_t* utxo_list = NULL;
  result                         = cardano_provider_get_unspent_outputs(provider, payment_address, &utxo_list);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get unspent outputs: %s", cardano_provider_get_last_error(provider));

    cardano_provider_unref(&provider);
    cardano_address_unref(&payment_address);
    cardano_reward_address_unref(&reward_address);

    return EXIT_FAILURE;
  }

  result = display_balance(utxo_list, rewards_available);

  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_reward_address_unref(&reward_address);
  cardano_utxo_list_unref(&utxo_list);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to display balance: %s", cardano_provider_get_last_error(provider));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* IMPLEMENTATIONS **********************************************************/

static cardano_address_t*
create_address(const char* address, const size_t address_length)
{
  cardano_address_t* payment_address = NULL;

  cardano_error_t result = cardano_address_from_string(address, address_length, &payment_address);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create payment address: %s", cardano_error_to_string(result));
    return NULL;
  }

  return payment_address;
}

static cardano_reward_address_t*
create_reward_address(const char* address_str, const size_t address_str_length)
{
  cardano_reward_address_t* reward_address = NULL;
  cardano_error_t           result         = cardano_reward_address_from_bech32(address_str, address_str_length, &reward_address);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create reward address: %s", cardano_error_to_string(result));
    return NULL;
  }

  return reward_address;
}

static cardano_error_t
display_balance(cardano_utxo_list_t* utxos, const uint64_t reward_balance)
{
  cardano_value_t* total = NULL;

  cardano_error_t result = cardano_value_new(0, NULL, &total);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create value");
    return result;
  }

  for (size_t i = 0U; i < cardano_utxo_list_get_length(utxos); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(utxos, i, &utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&total);

      console_error("Failed to get utxo");
      return result;
    }

    cardano_transaction_output_t* output       = cardano_utxo_get_output(utxo);
    cardano_value_t*              output_value = cardano_transaction_output_get_value(output);

    if (output_value == NULL)
    {
      // *_unref functions are safe to use with NULL pointers.
      cardano_utxo_unref(&utxo);
      cardano_value_unref(&output_value);
      cardano_transaction_output_unref(&output);
      cardano_value_unref(&total);

      console_error("Failed to get output value");

      return result;
    }

    cardano_value_t* tmp = NULL;

    result = cardano_value_add(total, output_value, &tmp);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);
      cardano_value_unref(&output_value);
      cardano_transaction_output_unref(&output);
      cardano_value_unref(&total);

      console_error("Failed to add value");

      return result;
    }

    cardano_value_unref(&total);
    total = tmp;

    cardano_utxo_unref(&utxo);
    cardano_value_unref(&output_value);
    cardano_transaction_output_unref(&output);
  }

  uint64_t total_coin     = cardano_value_get_coin(total);
  uint64_t total_lovelace = total_coin + reward_balance;

  console_info("Balance Summary");
  console_info("===================================");

  console_write("Available lovelace:   ");
  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("%lu\n", total_coin);
  console_reset_color();

  console_write("Withdrawable rewards: ");
  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("%lu\n", reward_balance);
  console_reset_color();

  console_write("Total lovelace:       ");
  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("%lu\n", total_lovelace);
  console_reset_color();

  cardano_value_unref(&total);

  return result;
}