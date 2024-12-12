/**
 * \file blockfrost_pparam_parser.c
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

#include "utils.h"

#include <cardano/json/json_object.h>
#include <stdlib.h>
#include <string.h>

/* CALLBACKS PROTOTYPES ******************************************************/

/**
 * \brief Function pointer type for handling different types of protocol parameters.
 *
 * This typedef defines a function pointer type for handling various types of protocol
 * parameters during JSON parsing. The handler function is responsible for extracting
 * the relevant parameter from the JSON object and applying the appropriate setter
 * function to store the parameter in the protocol parameters structure.
 *
 * \param[in] key         The key identifying the parameter in the JSON object.
 * \param[in] parameters   A pointer to the cardano_protocol_parameters_t structure where the parameter will be set.
 * \param[in] json_obj     The JSON object containing the parameter value to be processed.
 * \param[in] setter_func  A function pointer to the appropriate setter function for the parameter.
 *
 * \return CARDANO_SUCCESS on success, or an appropriate error code on failure.
 */
typedef cardano_error_t (*parameter_handler_t)(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  void*                          setter_func);

/* STRUCTURES ****************************************************************/

/**
 * \brief Struct for mapping JSON keys to handler functions and setters.
 *
 * This structure defines a mapping between a JSON key, a handler function to process
 * the corresponding JSON value, and a setter function to store the value in the
 * protocol parameters structure.
 *
 * \var key
 * The JSON key to be mapped. It identifies the parameter in the JSON object.
 *
 * \var handler
 * The function pointer of type \ref parameter_handler_t, which handles extracting
 * and processing the value from the JSON object associated with the key.
 *
 * \var setter_func
 * A function pointer to the appropriate setter function for the specific parameter,
 * which will store the processed value in the \ref cardano_protocol_parameters_t structure.
 */
typedef struct
{
    const char*         key;
    parameter_handler_t handler;
    void*               setter_func;
} parameter_map_entry_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Processes a cost model from a JSON array for a given Plutus language version.
 *
 * This function extracts the cost model data from a JSON array, which contains the cost parameters
 * for the specified Plutus language version. The extracted values are used to create a
 * \ref cardano_cost_model_t object that is associated with the corresponding language version.
 *
 * \param[in]  json_array       The JSON array containing the cost model values.
 * \param[in]  language_version The Plutus language version for which the cost model is being processed.
 *                              This should be one of the \ref cardano_plutus_language_version_t values (e.g., V1, V2, V3).
 * \param[out] cost_model       A pointer to a pointer of the \ref cardano_cost_model_t structure,
 *                              where the processed cost model will be stored.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - `CARDANO_SUCCESS`: If the cost model was successfully processed and created.
 *         - Appropriate error code otherwise, depending on the type of failure (e.g., invalid JSON structure).
 */
static cardano_error_t
process_cost_model(
  cardano_json_object_t*                  json_array,
  const cardano_plutus_language_version_t language_version,
  cardano_cost_model_t**                  cost_model)
{
  const size_t array_len  = cardano_json_object_array_get_length(json_array);
  int64_t*     cost_array = malloc(array_len * sizeof(int64_t));

  if (cost_array == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0U; i < array_len; ++i)
  {
    cardano_json_object_t* item   = cardano_json_object_array_get_ex(json_array, i);
    cardano_error_t        result = cardano_json_object_get_signed_int(item, &cost_array[i]);

    if (result != CARDANO_SUCCESS)
    {
      free(cost_array);
      return result;
    }
  }

  cardano_error_t result = cardano_cost_model_new(language_version, cost_array, array_len, cost_model);

  free(cost_array);

  return result;
}

/**
 * \brief Handles the extraction and setting of a 64-bit unsigned integer parameter from a JSON object.
 *
 * This function is used to extract a `uint64_t` value from the specified JSON object using the provided key.
 * The extracted value is then passed to the provided setter function to update the corresponding field in
 * the `cardano_protocol_parameters_t` structure.
 *
 * \param[in]  key         The JSON key associated with the parameter to extract.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds
 *                         the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the `uint64_t` value will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the extracted `uint64_t`
 *                         value and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result.
 */
static cardano_error_t
handle_uint64(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, uint64_t))
{
  CARDANO_UNUSED(key);

  uint64_t value = 0U;

  cardano_error_t result = cardano_json_object_get_uint(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return setter_func(parameters, value);
}

/**
 * \brief Handles the extraction and setting of a unit interval parameter from a JSON object.
 *
 * This function is used to extract a `double` value from the specified JSON object using the provided key,
 * convert it into a `cardano_unit_interval_t`, and then pass it to the provided setter function to update
 * the corresponding field in the `cardano_protocol_parameters_t` structure.
 *
 * \param[in]  key         The JSON key associated with the unit interval parameter to extract.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds
 *                         the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the unit interval will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the extracted
 *                         `cardano_unit_interval_t` value and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or the conversion process.
 */
static cardano_error_t
handle_unit_interval(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_unit_interval_t*))
{
  CARDANO_UNUSED(key);

  double          value  = 0.0;
  cardano_error_t result = cardano_json_object_get_double(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_unit_interval_t* interval = NULL;
  result                            = cardano_unit_interval_from_double(value, &interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&interval);
    return result;
  }

  result = setter_func(parameters, interval);
  cardano_unit_interval_unref(&interval);

  return result;
}

/**
 * \brief Handles the extraction and setting of the protocol version from a JSON object.
 *
 * This function is used to extract version-related information (major and minor) from the specified JSON object using the provided key,
 * construct a `cardano_protocol_version_t` structure, and pass it to the provided setter function to update the corresponding field
 * in the \ref cardano_protocol_parameters_t structure.
 *
 * \param[in]  key         The JSON key associated with the version parameter to extract.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds
 *                         the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the version information will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the constructed
 *                         \ref cardano_protocol_version_t and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or the version construction process.
 */
static cardano_error_t
handle_version(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_protocol_version_t*))
{
  uint64_t        value  = 0U;
  cardano_error_t result = cardano_json_object_get_uint(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_protocol_version_t* version = cardano_protocol_parameters_get_protocol_version(parameters);

  if (strcmp(key, "protocol_major_ver") == 0)
  {
    result = cardano_protocol_version_set_major(version, value);
  }
  else if (strcmp(key, "protocol_minor_ver") == 0)
  {
    result = cardano_protocol_version_set_minor(version, value);
  }
  else
  {
    cardano_protocol_version_unref(&version);

    return CARDANO_ERROR_INVALID_JSON;
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_protocol_version_unref(&version);
    return result;
  }

  result = setter_func(parameters, version);
  cardano_protocol_version_unref(&version);

  return result;
}

/**
 * \brief Handles the extraction and setting of execution unit prices from a JSON object.
 *
 * This function is responsible for extracting execution unit pricing information (both memory and step prices)
 * from the specified JSON object, constructing a \ref cardano_ex_unit_prices_t structure, and passing it to
 * the provided setter function to update the corresponding field in the \ref cardano_protocol_parameters_t structure.
 *
 * \param[in]  key         The JSON key associated with the price parameter to extract.
 *                         This is expected to differentiate between the "price_mem" and "price_step" fields.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure
 *                         that holds the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the price information
 *                         (for both memory and steps) will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the constructed
 *                         \ref cardano_ex_unit_prices_t structure and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or the price construction process.
 */
static cardano_error_t
handle_prices(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_ex_unit_prices_t*))
{
  double value = 0.0;

  cardano_error_t result = cardano_json_object_get_double(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_unit_interval_t* interval = NULL;
  result                            = cardano_unit_interval_from_double(value, &interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&interval);
    return result;
  }

  cardano_ex_unit_prices_t* prices = cardano_protocol_parameters_get_execution_costs(parameters);

  if (strcmp(key, "price_mem") == 0)
  {
    result = cardano_ex_unit_prices_set_memory_prices(prices, interval);
  }
  else if (strcmp(key, "price_step") == 0)
  {
    result = cardano_ex_unit_prices_set_steps_prices(prices, interval);
  }
  else
  {
    cardano_unit_interval_unref(&interval);
    cardano_ex_unit_prices_unref(&prices);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_unit_interval_unref(&interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ex_unit_prices_unref(&prices);
    return result;
  }

  result = setter_func(parameters, prices);
  cardano_ex_unit_prices_unref(&prices);

  return result;
}

/**
 * \brief Handles the extraction and setting of maximum execution units (memory and steps) from a JSON object.
 *
 * This function extracts the maximum execution units (memory and step values) from the specified JSON object.
 * It constructs a \ref cardano_ex_units_t structure and passes it to the provided setter function, which updates
 * the corresponding field in the \ref cardano_protocol_parameters_t structure.
 *
 * \param[in]  key         The JSON key associated with the execution units to extract (e.g., "max_tx_ex_mem", "max_tx_ex_steps").
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the maximum execution units will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the constructed
 *                         \ref cardano_ex_units_t structure and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or the execution unit construction process.
 */
static cardano_error_t
handle_max_ex(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_ex_units_t*))
{
  uint64_t value = 0U;

  cardano_error_t result = cardano_json_object_get_uint(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_ex_units_t* units = NULL;

  if ((strcmp(key, "max_tx_ex_mem") == 0) || (strcmp(key, "max_tx_ex_steps") == 0))
  {
    units = cardano_protocol_parameters_get_max_tx_ex_units(parameters);
  }
  else
  {
    units = cardano_protocol_parameters_get_max_block_ex_units(parameters);
  }

  if ((strcmp(key, "max_tx_ex_mem") == 0) || (strcmp(key, "max_block_ex_mem") == 0))
  {
    result = cardano_ex_units_set_memory(units, value);
  }
  else
  {
    result = cardano_ex_units_set_cpu_steps(units, value);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_ex_units_unref(&units);
    return result;
  }

  result = setter_func(parameters, units);
  cardano_ex_units_unref(&units);

  return result;
}

/**
 * \brief Handles the extraction and setting of pool voting thresholds (PVT) from a JSON object.
 *
 * This function extracts the pool voting thresholds associated with governance operations
 * (e.g., motion of no confidence, committee thresholds) from the specified JSON object.
 * It constructs a \ref cardano_pool_voting_thresholds_t structure and passes it to the provided setter function,
 * which updates the corresponding field in the \ref cardano_protocol_parameters_t structure.
 *
 * \param[in]  key         The JSON key associated with the specific pool voting threshold to extract.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the pool voting threshold will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the constructed
 *                         \ref cardano_pool_voting_thresholds_t structure and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or the threshold construction process.
 */
static cardano_error_t
handle_pvt(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_pool_voting_thresholds_t*))
{
  double value = 0.0;

  cardano_error_t result = cardano_json_object_get_double(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_unit_interval_t* interval = NULL;
  result                            = cardano_unit_interval_from_double(value, &interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&interval);
    return result;
  }

  cardano_pool_voting_thresholds_t* threshold = cardano_protocol_parameters_get_pool_voting_thresholds(parameters);

  if (strcmp(key, "pvt_motion_no_confidence") == 0)
  {
    result = cardano_pool_voting_thresholds_set_motion_no_confidence(threshold, interval);
  }
  else if (strcmp(key, "pvt_committee_normal") == 0)
  {
    result = cardano_pool_voting_thresholds_set_committee_normal(threshold, interval);
  }
  else if (strcmp(key, "pvt_committee_no_confidence") == 0)
  {
    result = cardano_pool_voting_thresholds_set_committee_no_confidence(threshold, interval);
  }
  else if (strcmp(key, "pvt_hard_fork_initiation") == 0)
  {
    result = cardano_pool_voting_thresholds_set_hard_fork_initiation(threshold, interval);
  }
  else if (strcmp(key, "pvt_p_p_security_group") == 0)
  {
    result = cardano_pool_voting_thresholds_set_security_relevant_param(threshold, interval);
  }
  else
  {
    cardano_unit_interval_unref(&interval);
    cardano_pool_voting_thresholds_unref(&threshold);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_unit_interval_unref(&interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_pool_voting_thresholds_unref(&threshold);
    return result;
  }

  result = setter_func(parameters, threshold);
  cardano_pool_voting_thresholds_unref(&threshold);

  return result;
}

/**
 * \brief Handles the extraction and setting of decentralized representative (DRep) voting thresholds (DVT) from a JSON object.
 *
 * This function extracts the decentralized representative (DRep) voting thresholds, which are related to governance actions,
 * from the specified JSON object. It constructs a \ref cardano_drep_voting_thresholds_t structure and passes it to the provided setter function,
 * which updates the corresponding field in the \ref cardano_protocol_parameters_t structure.
 *
 * \param[in]  key         The JSON key associated with the specific DRep voting threshold to extract.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the DRep voting threshold will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the constructed
 *                         \ref cardano_drep_voting_thresholds_t structure and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or the threshold construction process.
 */
static cardano_error_t
handle_dvt(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_drep_voting_thresholds_t*))
{
  double value = 0.0;

  cardano_error_t result = cardano_json_object_get_double(json_obj, &value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_unit_interval_t* interval = NULL;
  result                            = cardano_unit_interval_from_double(value, &interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&interval);
    return result;
  }

  cardano_drep_voting_thresholds_t* threshold = cardano_protocol_parameters_get_drep_voting_thresholds(parameters);

  if (strcmp(key, "dvt_motion_no_confidence") == 0)
  {
    result = cardano_drep_voting_thresholds_set_motion_no_confidence(threshold, interval);
  }
  else if (strcmp(key, "dvt_committee_normal") == 0)
  {
    result = cardano_drep_voting_thresholds_set_committee_normal(threshold, interval);
  }
  else if (strcmp(key, "dvt_committee_no_confidence") == 0)
  {
    result = cardano_drep_voting_thresholds_set_committee_no_confidence(threshold, interval);
  }
  else if (strcmp(key, "dvt_update_to_constitution") == 0)
  {
    result = cardano_drep_voting_thresholds_set_update_constitution(threshold, interval);
  }
  else if (strcmp(key, "dvt_hard_fork_initiation") == 0)
  {
    result = cardano_drep_voting_thresholds_set_hard_fork_initiation(threshold, interval);
  }
  else if (strcmp(key, "dvt_p_p_network_group") == 0)
  {
    result = cardano_drep_voting_thresholds_set_pp_network_group(threshold, interval);
  }
  else if (strcmp(key, "dvt_p_p_economic_group") == 0)
  {
    result = cardano_drep_voting_thresholds_set_pp_economic_group(threshold, interval);
  }
  else if (strcmp(key, "dvt_p_p_technical_group") == 0)
  {
    result = cardano_drep_voting_thresholds_set_pp_technical_group(threshold, interval);
  }
  else if (strcmp(key, "dvt_p_p_gov_group") == 0)
  {
    result = cardano_drep_voting_thresholds_set_pp_governance_group(threshold, interval);
  }
  else if (strcmp(key, "dvt_treasury_withdrawal") == 0)
  {
    result = cardano_drep_voting_thresholds_set_treasury_withdrawal(threshold, interval);
  }
  else
  {
    cardano_unit_interval_unref(&interval);
    cardano_drep_voting_thresholds_unref(&threshold);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_unit_interval_unref(&interval);

  if (result != CARDANO_SUCCESS)
  {
    cardano_drep_voting_thresholds_unref(&threshold);
    return result;
  }

  result = setter_func(parameters, threshold);
  cardano_drep_voting_thresholds_unref(&threshold);

  return result;
}

/**
 * \brief Handles the extraction and setting of a buffer field from a JSON object.
 *
 * This function extracts a buffer (e.g., byte array) from the specified JSON object and converts it
 * into a \ref cardano_buffer_t structure, which is then passed to the provided setter function.
 * The setter function updates the corresponding field in the \ref cardano_protocol_parameters_t structure.
 *
 * \param[in]  key         The JSON key associated with the buffer to extract.
 * \param[in]  parameters  A pointer to the \ref cardano_protocol_parameters_t structure that holds the protocol parameters to be updated.
 * \param[in]  json_obj    A pointer to the JSON object from which the buffer will be extracted.
 * \param[in]  setter_func A function pointer to the setter function that takes the constructed
 *                         \ref cardano_buffer_t structure and applies it to the \ref cardano_protocol_parameters_t structure.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the setter function’s result or buffer construction process.
 */
static cardano_error_t
handle_buffer(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_buffer_t*))
{
  CARDANO_UNUSED(key);

  size_t      value_len = 0U;
  const char* value     = cardano_json_object_get_string(json_obj, &value_len);

  cardano_buffer_t* entropy = cardano_buffer_from_hex(value, value_len - 1U);

  if (entropy == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_error_t result = setter_func(parameters, entropy);
  cardano_buffer_unref(&entropy);

  return result;
}

/**
 * \brief Processes a cost model from a JSON object and inserts it into the cost models container.
 *
 * This function extracts the cost model for a specific Plutus language version from the given JSON object,
 * processes the cost model, and inserts it into the provided cost models container.
 *
 * \param[in]  json_obj         The JSON object that contains the cost model array associated with the Plutus version.
 * \param[in]  version_key      The JSON key used to identify the cost model in the JSON object (e.g., "PlutusV1", "PlutusV2").
 * \param[in]  language_version The Plutus language version identifier (e.g., `CARDANO_PLUTUS_LANGUAGE_VERSION_V1`).
 * \param[out] cost_models      The container to which the processed cost model will be inserted.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the cost model processing and insertion logic.
 */
static cardano_error_t
process_and_insert_cost_model(
  cardano_json_object_t* json_obj,
  const char*            version_key,
  int                    language_version,
  cardano_costmdls_t*    cost_models)
{
  cardano_json_object_t* json_version = NULL;

  if (cardano_json_object_get_ex(json_obj, version_key, strlen(version_key), &json_version))
  {
    cardano_cost_model_t* cost_model = NULL;
    cardano_error_t       result     = process_cost_model(json_version, language_version, &cost_model);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_costmdls_insert(cost_models, cost_model);
    cardano_cost_model_unref(&cost_model);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Handles the parsing, processing, and setting of cost models from a JSON object.
 *
 * This function extracts and processes the cost models (e.g., Plutus V1, V2, V3) from the provided JSON object,
 * creates the corresponding \ref cardano_costmdls_t structure, and uses the provided setter function to
 * assign the cost models to the protocol parameters.
 *
 * \param[in]  key               The JSON key associated with the cost models (e.g., "cost_models_raw").
 * \param[in]  parameters        A pointer to the \ref cardano_protocol_parameters_t structure where the cost models will be set.
 * \param[in]  json_obj          The JSON object containing the cost models.
 * \param[in]  setter_func       A function pointer that sets the processed cost models in the protocol parameters.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 *         - Other error codes depending on the cost model processing and setting logic.
 */
static cardano_error_t
handle_cost_models(
  const char*                    key,
  cardano_protocol_parameters_t* parameters,
  cardano_json_object_t*         json_obj,
  cardano_error_t (*setter_func)(cardano_protocol_parameters_t*, cardano_costmdls_t*))
{
  CARDANO_UNUSED(key);

  cardano_costmdls_t* cost_models = NULL;
  cardano_error_t     result      = cardano_costmdls_new(&cost_models);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = process_and_insert_cost_model(json_obj, "PlutusV1", CARDANO_PLUTUS_LANGUAGE_VERSION_V1, cost_models);

  if (result != CARDANO_SUCCESS)
  {
    cardano_costmdls_unref(&cost_models);
    return result;
  }

  result = process_and_insert_cost_model(json_obj, "PlutusV2", CARDANO_PLUTUS_LANGUAGE_VERSION_V2, cost_models);

  if (result != CARDANO_SUCCESS)
  {
    cardano_costmdls_unref(&cost_models);
    return result;
  }

  result = process_and_insert_cost_model(json_obj, "PlutusV3", CARDANO_PLUTUS_LANGUAGE_VERSION_V3, cost_models);

  if (result != CARDANO_SUCCESS)
  {
    cardano_costmdls_unref(&cost_models);
    return result;
  }

  result = setter_func(parameters, cost_models);
  cardano_costmdls_unref(&cost_models);

  return result;
}

/* STATIC VARIABLES **********************************************************/

/**
 * \brief Mapping between JSON keys and handler functions for protocol parameters.
 */
static parameter_map_entry_t parameter_map[] = {
  { "min_fee_a", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_min_fee_a },
  { "min_fee_b", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_min_fee_b },
  { "max_block_size", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_max_block_body_size },
  { "max_tx_size", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_max_tx_size },
  { "max_block_header_size", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_max_block_header_size },
  { "key_deposit", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_key_deposit },
  { "pool_deposit", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_pool_deposit },
  { "e_max", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_max_epoch },
  { "n_opt", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_n_opt },
  { "a0", (void*)handle_unit_interval, (void*)cardano_protocol_parameters_set_pool_pledge_influence },
  { "rho", (void*)handle_unit_interval, (void*)cardano_protocol_parameters_set_expansion_rate },
  { "tau", (void*)handle_unit_interval, (void*)cardano_protocol_parameters_set_treasury_growth_rate },
  { "decentralisation_param", (void*)handle_unit_interval, (void*)cardano_protocol_parameters_set_d },
  { "extra_entropy", (void*)handle_buffer, (void*)cardano_protocol_parameters_set_extra_entropy },
  { "protocol_major_ver", (void*)handle_version, (void*)cardano_protocol_parameters_set_protocol_version },
  { "protocol_minor_ver", (void*)handle_version, (void*)cardano_protocol_parameters_set_protocol_version },
  { "coins_per_utxo_word", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_ada_per_utxo_byte },
  { "min_pool_cost", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_min_pool_cost },
  { "cost_models_raw", (void*)handle_cost_models, (void*)cardano_protocol_parameters_set_cost_models },
  { "price_mem", (void*)handle_prices, (void*)cardano_protocol_parameters_set_execution_costs },
  { "price_step", (void*)handle_prices, (void*)cardano_protocol_parameters_set_execution_costs },
  { "max_tx_ex_mem", (void*)handle_max_ex, (void*)cardano_protocol_parameters_set_max_tx_ex_units },
  { "max_tx_ex_steps", (void*)handle_max_ex, (void*)cardano_protocol_parameters_set_max_tx_ex_units },
  { "max_block_ex_mem", (void*)handle_max_ex, (void*)cardano_protocol_parameters_set_max_block_ex_units },
  { "max_block_ex_steps", (void*)handle_max_ex, (void*)cardano_protocol_parameters_set_max_block_ex_units },
  { "max_val_size", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_max_value_size },
  { "collateral_percent", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_collateral_percentage },
  { "max_collateral_inputs", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_max_collateral_inputs },
  { "pvt_motion_no_confidence", (void*)handle_pvt, (void*)cardano_protocol_parameters_set_pool_voting_thresholds },
  { "pvt_committee_normal", (void*)handle_pvt, (void*)cardano_protocol_parameters_set_pool_voting_thresholds },
  { "pvt_committee_no_confidence", (void*)handle_pvt, (void*)cardano_protocol_parameters_set_pool_voting_thresholds },
  { "pvt_hard_fork_initiation", (void*)handle_pvt, (void*)cardano_protocol_parameters_set_pool_voting_thresholds },
  { "pvt_p_p_security_group", (void*)handle_pvt, (void*)cardano_protocol_parameters_set_pool_voting_thresholds },
  { "dvt_motion_no_confidence", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_committee_normal", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_committee_no_confidence", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_update_to_constitution", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_hard_fork_initiation", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_p_p_network_group", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_p_p_economic_group", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_p_p_technical_group", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_p_p_gov_group", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "dvt_treasury_withdrawal", (void*)handle_dvt, (void*)cardano_protocol_parameters_set_drep_voting_thresholds },
  { "committee_min_size", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_min_committee_size },
  { "committee_max_term_length", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_committee_term_limit },
  { "gov_action_lifetime", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_governance_action_validity_period },
  { "gov_action_deposit", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_governance_action_deposit },
  { "drep_deposit", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_drep_deposit },
  { "drep_activity", (void*)handle_uint64, (void*)cardano_protocol_parameters_set_drep_inactivity_period },
  { "min_fee_ref_script_cost_per_byte", (void*)handle_unit_interval, (void*)cardano_protocol_parameters_set_ref_script_cost_per_byte },
  { NULL, NULL, NULL }
};

/* IMPLEMENTATION *************************************************************/

cardano_error_t
cardano_blockfrost_parse_protocol_parameters(
  cardano_provider_impl_t*        provider,
  const char*                     json,
  const size_t                    size,
  cardano_protocol_parameters_t** parameters)
{
  cardano_json_object_t* parsed_json = cardano_json_object_parse(json, size);

  if (parsed_json == NULL)
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_error_t result = cardano_protocol_parameters_new(parameters);
  if (result != CARDANO_SUCCESS)
  {
    cardano_json_object_unref(&parsed_json);

    return result;
  }

  for (parameter_map_entry_t* entry = parameter_map; entry->key != NULL; ++entry)
  {
    cardano_json_object_t* json_obj = NULL;

    if (!cardano_json_object_get_ex(parsed_json, entry->key, strlen(entry->key), &json_obj))
    {
      continue;
    }

    result = entry->handler(entry->key, *parameters, json_obj, entry->setter_func);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&parsed_json);
      return result;
    }
  }

  cardano_json_object_unref(&parsed_json);

  return CARDANO_SUCCESS;
}