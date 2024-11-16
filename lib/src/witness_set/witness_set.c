/**
 * \file witness_set.c
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/object.h>
#include <cardano/witness_set/witness_set.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <cardano/scripts/script.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A witness is a piece of information that allows you to efficiently verify the
 * authenticity of the transaction (also known as proof).
 *
 * In Cardano, transactions have multiple types of authentication proofs, these can range
 * from signatures for spending UTxOs, to scripts (with its arguments, datums and redeemers) for
 * smart contract execution.
 */
typedef struct cardano_witness_set_t
{
    cardano_object_t                 base;
    cardano_vkey_witness_set_t*      vkey_witnesses;
    cardano_native_script_set_t*     native_scripts;
    cardano_bootstrap_witness_set_t* bootstrap_witnesses;
    cardano_plutus_v1_script_set_t*  plutus_v1_scripts;
    cardano_plutus_data_set_t*       plutus_data;
    cardano_redeemer_list_t*         redeemer;
    cardano_plutus_v2_script_set_t*  plutus_v2_scripts;
    cardano_plutus_v3_script_set_t*  plutus_v3_scripts;
} cardano_witness_set_t;

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Type definition for a parameter handler function.
 *
 * This type defines a function pointer used for handling witness set fields
 * when reading from a CBOR reader. Each handler function reads a specific type of
 * field from the CBOR reader and stores the result in the specified field pointer.
 *
 * The function pointed to by `param_handler_t` is responsible for:
 * - Reading the field value from the provided CBOR reader.
 * - Storing the field value in the provided field pointer.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the field value.
 * \param[out] field_ptr A pointer to the field where the read value should be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the field was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
typedef cardano_error_t (*param_handler_t)(cardano_cbor_reader_t*, void*);

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a witness set object.
 *
 * This function is responsible for properly deallocating a witness set object (`cardano_witness_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the witness_set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_witness_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the witness_set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_witness_set_deallocate(void* object)
{
  assert(object != NULL);

  cardano_witness_set_t* data = (cardano_witness_set_t*)object;

  cardano_vkey_witness_set_unref(&data->vkey_witnesses);
  cardano_native_script_set_unref(&data->native_scripts);
  cardano_bootstrap_witness_set_unref(&data->bootstrap_witnesses);
  cardano_plutus_v1_script_set_unref(&data->plutus_v1_scripts);
  cardano_plutus_data_set_unref(&data->plutus_data);
  cardano_redeemer_list_unref(&data->redeemer);
  cardano_plutus_v2_script_set_unref(&data->plutus_v2_scripts);
  cardano_plutus_v3_script_set_unref(&data->plutus_v3_scripts);

  _cardano_free(object);
}

/**
 * \brief Computes the map size of this witness set object.
 *
 * \param witness The witness set object to compute the map size for.
 *
 * \return The map size of the witness set object.
 */
static size_t
get_map_size(const cardano_witness_set_t* witness)
{
  assert(witness != NULL);

  size_t map_size = 0U;

  if (cardano_vkey_witness_set_get_length(witness->vkey_witnesses) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_native_script_set_get_length(witness->native_scripts) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_bootstrap_witness_set_get_length(witness->bootstrap_witnesses) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_plutus_v1_script_set_get_length(witness->plutus_v1_scripts) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_plutus_data_set_get_length(witness->plutus_data) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_redeemer_list_get_length(witness->redeemer) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_plutus_v2_script_set_get_length(witness->plutus_v2_scripts) > 0U)
  {
    map_size += 1U;
  }

  if (cardano_plutus_v3_script_set_get_length(witness->plutus_v3_scripts) > 0U)
  {
    map_size += 1U;
  }

  return map_size;
}

/**
 * \brief Retrieves a pointer to the specified field in the witness set witness object.
 *
 * This function returns a pointer to the field specified by the key in the given protocol
 * parameter witness object. The key corresponds to different fields within the witness object.
 * If the key does not correspond to any field, NULL is returned.
 *
 * \param[in] witness Pointer to the witness set witness object.
 *
 * \return void* A pointer to the specified field, or NULL if the key does not correspond to any field.
 *
 * \note The caller must ensure that the `cardano_witness_set_t` pointer is not NULL before calling this function.
 */
static void*
get_field_ptr(cardano_witness_set_t* witness, size_t key)
{
  assert(witness != NULL);

  switch (key)
  {
    case 0:
      return (void*)&witness->vkey_witnesses;
    case 1:
      return (void*)&witness->native_scripts;
    case 2:
      return (void*)&witness->bootstrap_witnesses;
    case 3:
      return (void*)&witness->plutus_v1_scripts;
    case 4:
      return (void*)&witness->plutus_data;
    case 5:
      return (void*)&witness->redeemer;
    case 6:
      return (void*)&witness->plutus_v2_scripts;
    case 7:
      return (void*)&witness->plutus_v3_scripts;

    default:
      return NULL;
  }
}

/**
 * \brief Reads a cardano_vk_witness_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_vk_witness_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_vk_witness_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_vk_witness_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_vk_witness_set_t value should be stored.
 *                       The field pointer should be of type cardano_vk_witness_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_vk_witness_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_vk_witness_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_vkey_witness_set_t** field = (cardano_vkey_witness_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_vkey_witness_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_native_script_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_native_script_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_native_script_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_native_script_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_native_script_set_t value should be stored.
 *                       The field pointer should be of type cardano_native_script_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_native_script_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_native_script_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_native_script_set_t** field = (cardano_native_script_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_native_script_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_bootstrap_witness_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_bootstrap_witness_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_bootstrap_witness_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_bootstrap_witness_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_bootstrap_witness_set_t value should be stored.
 *                       The field pointer should be of type cardano_bootstrap_witness_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_bootstrap_witness_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_bootstrap_witness_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_bootstrap_witness_set_t** field = (cardano_bootstrap_witness_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_bootstrap_witness_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_plutus_v1_script_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_plutus_v1_script_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_plutus_v1_script_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_plutus_v1_script_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_plutus_v1_script_set_t value should be stored.
 *                       The field pointer should be of type cardano_plutus_v1_script_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_plutus_v1_script_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_plutus_v1_script_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_plutus_v1_script_set_t** field = (cardano_plutus_v1_script_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_plutus_v1_script_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_plutus_data_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_plutus_data_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_plutus_data_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_plutus_data_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_plutus_data_set_t value should be stored.
 *                       The field pointer should be of type cardano_plutus_data_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_plutus_data_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_plutus_data(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_plutus_data_set_t** field = (cardano_plutus_data_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_plutus_data_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_redeemer_list_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_redeemer_list_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_redeemer_list_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_redeemer_list_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_redeemer_list_t value should be stored.
 *                       The field pointer should be of type cardano_redeemer_list_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_redeemer_list_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_redeemer_list(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_redeemer_list_t** field = (cardano_redeemer_list_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_redeemer_list_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_plutus_v2_script_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_plutus_v2_script_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_plutus_v2_script_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_plutus_v2_script_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_plutus_v2_script_set_t value should be stored.
 *                       The field pointer should be of type cardano_plutus_v2_script_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_plutus_v2_script_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_plutus_v2_script_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_plutus_v2_script_set_t** field = (cardano_plutus_v2_script_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_plutus_v2_script_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_plutus_v3_script_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_plutus_v3_script_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_plutus_v3_script_set_t in the witness set witness.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_plutus_v3_script_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_plutus_v3_script_set_t value should be stored.
 *                       The field pointer should be of type cardano_plutus_v3_script_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_plutus_v3_script_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_plutus_v3_script_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_plutus_v3_script_set_t** field = (cardano_plutus_v3_script_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_plutus_v3_script_set_from_cbor(reader, field);
}

/**
 * \brief Handles an invalid key in the CBOR map during deserialization.
 *
 * This function is used as a handler for invalid keys encountered in the CBOR map.
 * It does nothing with the provided reader and field pointer, and always returns
 * \ref CARDANO_ERROR_INVALID_CBOR_MAP_KEY to indicate the error.
 *
 * \param[in] reader A pointer to the CBOR reader. This parameter is unused.
 * \param[in] field_ptr A pointer to the field where the value should be stored. This parameter is unused.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - Always returns \ref CARDANO_ERROR_INVALID_CBOR_MAP_KEY.
 */
static cardano_error_t
handle_invalid_key(cardano_cbor_reader_t* reader, void* field_ptr)
{
  CARDANO_UNUSED(reader);
  CARDANO_UNUSED(field_ptr);

  return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated vkey witness set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the vkey witness set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_vkey_witness_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_vkey_witness_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_vkey_witness_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_vkey_witness_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated native script value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the native script value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_native_script_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_native_script_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_native_script_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_native_script_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated bootstrap witness set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the bootstrap witness set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_bootstrap_witness_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_bootstrap_witness_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_bootstrap_witness_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_bootstrap_witness_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated plutus v1 script set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the plutus v1 script set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_plutus_v1_script_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_plutus_v1_script_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_plutus_v1_script_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_v1_script_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated plutus data value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the plutus data value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_plutus_data_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_plutus_data_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_plutus_data_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_data_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated redeemer list value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the redeemer list value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_redeemer_list_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_redeemer_list_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_redeemer_list_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_redeemer_list_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated plutus v2 script set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the plutus v2 script set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_plutus_v2_script_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_plutus_v2_script_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_plutus_v2_script_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_v2_script_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated plutus v3 script set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the plutus v3 script set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_plutus_v3_script_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_plutus_v3_script_set_t* value)
{
  assert(writer != NULL);

  if ((value != NULL) && (cardano_plutus_v3_script_set_get_length(value) > 0U))
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_v3_script_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Allocates and initializes a new cardano_witness_set_t structure.
 *
 * \param witness_set A pointer to the location where the new cardano_witness_set_t structure should be stored.
 * \return The pointer to the newly allocated cardano_witness_set_t structure.
 */
static cardano_witness_set_t*
create_witness_set_new(void)
{
  cardano_witness_set_t* witness_set = (cardano_witness_set_t*)_cardano_malloc(sizeof(cardano_witness_set_t));

  if (witness_set == NULL)
  {
    return NULL;
  }

  witness_set->base.deallocator    = cardano_witness_set_deallocate;
  witness_set->base.ref_count      = 1;
  witness_set->base.last_error[0]  = '\0';
  witness_set->vkey_witnesses      = NULL;
  witness_set->native_scripts      = NULL;
  witness_set->bootstrap_witnesses = NULL;
  witness_set->plutus_v1_scripts   = NULL;
  witness_set->plutus_data         = NULL;
  witness_set->redeemer            = NULL;
  witness_set->plutus_v2_scripts   = NULL;
  witness_set->plutus_v3_scripts   = NULL;

  return witness_set;
}

/* STATIC CONSTANTS ***********************************************************/

// cppcheck-suppress misra-c2012-8.9; Reason: Is more readable to define the map here
static const param_handler_t param_handlers[] = {
  handle_vk_witness_set,
  handle_native_script_set,
  handle_bootstrap_witness_set,
  handle_plutus_v1_script_set,
  handle_plutus_data,
  handle_redeemer_list,
  handle_plutus_v2_script_set,
  handle_plutus_v3_script_set
};

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_witness_set_new(cardano_witness_set_t** witness_set)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *witness_set = create_witness_set_new();

  if (*witness_set == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_witness_set_from_cbor(cardano_cbor_reader_t* reader, cardano_witness_set_t** witness_set)
{
  if (!reader || !witness_set)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *witness_set                   = NULL;
  cardano_witness_set_t* witness = create_witness_set_new();

  if (witness == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  int64_t         map_size = 0;
  cardano_error_t result   = cardano_cbor_reader_read_start_map(reader, &map_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_witness_set_unref(&witness);

    return result;
  }

  for (size_t i = 0U; i < (size_t)map_size; ++i)
  {
    uint64_t key = 0;
    result       = cardano_cbor_reader_read_uint(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_witness_set_unref(&witness);

      return result;
    }

    if (key >= (sizeof(param_handlers) / sizeof(param_handlers[0])))
    {
      cardano_witness_set_unref(&witness);

      return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
    }

    void* field_ptr = get_field_ptr(witness, key);

    if (field_ptr == NULL)
    {
      cardano_witness_set_unref(&witness);

      return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
    }

    result = param_handlers[key](reader, field_ptr);

    if (result != CARDANO_SUCCESS)
    {
      cardano_witness_set_unref(&witness);

      return result;
    }
  }

  *witness_set = witness;

  return cardano_cbor_validate_end_map("witness_set", reader);
}

cardano_error_t
cardano_witness_set_to_cbor(const cardano_witness_set_t* witness_set, cardano_cbor_writer_t* writer)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t map_size = get_map_size(witness_set);

  cardano_error_t result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_vkey_witness_set_if_present(writer, 0U, witness_set->vkey_witnesses);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_native_script_if_present(writer, 1U, witness_set->native_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_bootstrap_witness_set_if_present(writer, 2U, witness_set->bootstrap_witnesses);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_plutus_v1_script_set_if_present(writer, 3U, witness_set->plutus_v1_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_plutus_data_if_present(writer, 4U, witness_set->plutus_data);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_redeemer_list_if_present(writer, 5U, witness_set->redeemer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_plutus_v2_script_set_if_present(writer, 6U, witness_set->plutus_v2_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_plutus_v3_script_set_if_present(writer, 7U, witness_set->plutus_v3_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_vkey_witness_set_t*
cardano_witness_set_get_vkeys(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_vkey_witness_set_ref(witness_set->vkey_witnesses);

  return witness_set->vkey_witnesses;
}

cardano_error_t
cardano_witness_set_set_vkeys(
  cardano_witness_set_t*      witness_set,
  cardano_vkey_witness_set_t* vkeys)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vkeys == NULL)
  {
    cardano_vkey_witness_set_unref(&witness_set->vkey_witnesses);
    witness_set->vkey_witnesses = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_vkey_witness_set_ref(vkeys);
  cardano_vkey_witness_set_unref(&witness_set->vkey_witnesses);
  witness_set->vkey_witnesses = vkeys;

  return CARDANO_SUCCESS;
}

cardano_bootstrap_witness_set_t*
cardano_witness_set_get_bootstrap(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_bootstrap_witness_set_ref(witness_set->bootstrap_witnesses);

  return witness_set->bootstrap_witnesses;
}

cardano_error_t
cardano_witness_set_set_bootstrap(
  cardano_witness_set_t*           witness_set,
  cardano_bootstrap_witness_set_t* bootstraps)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bootstraps == NULL)
  {
    cardano_bootstrap_witness_set_unref(&witness_set->bootstrap_witnesses);
    witness_set->bootstrap_witnesses = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_bootstrap_witness_set_ref(bootstraps);
  cardano_bootstrap_witness_set_unref(&witness_set->bootstrap_witnesses);
  witness_set->bootstrap_witnesses = bootstraps;

  return CARDANO_SUCCESS;
}

cardano_native_script_set_t*
cardano_witness_set_get_native_scripts(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_native_script_set_ref(witness_set->native_scripts);

  return witness_set->native_scripts;
}

cardano_error_t
cardano_witness_set_set_native_scripts(
  cardano_witness_set_t*       witness_set,
  cardano_native_script_set_t* native_scripts)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_scripts == NULL)
  {
    cardano_native_script_set_unref(&witness_set->native_scripts);
    witness_set->native_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_native_script_set_ref(native_scripts);
  cardano_native_script_set_unref(&witness_set->native_scripts);
  witness_set->native_scripts = native_scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_v1_script_set_t*
cardano_witness_set_get_plutus_v1_scripts(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_plutus_v1_script_set_ref(witness_set->plutus_v1_scripts);

  return witness_set->plutus_v1_scripts;
}

cardano_error_t
cardano_witness_set_set_plutus_v1_scripts(
  cardano_witness_set_t*          witness_set,
  cardano_plutus_v1_script_set_t* plutus_scripts)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_scripts == NULL)
  {
    cardano_plutus_v1_script_set_unref(&witness_set->plutus_v1_scripts);
    witness_set->plutus_v1_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_v1_script_set_ref(plutus_scripts);
  cardano_plutus_v1_script_set_unref(&witness_set->plutus_v1_scripts);
  witness_set->plutus_v1_scripts = plutus_scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_v2_script_set_t*
cardano_witness_set_get_plutus_v2_scripts(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_plutus_v2_script_set_ref(witness_set->plutus_v2_scripts);

  return witness_set->plutus_v2_scripts;
}

cardano_error_t
cardano_witness_set_set_plutus_v2_scripts(
  cardano_witness_set_t*          witness_set,
  cardano_plutus_v2_script_set_t* plutus_scripts)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_scripts == NULL)
  {
    cardano_plutus_v2_script_set_unref(&witness_set->plutus_v2_scripts);
    witness_set->plutus_v2_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_v2_script_set_ref(plutus_scripts);
  cardano_plutus_v2_script_set_unref(&witness_set->plutus_v2_scripts);
  witness_set->plutus_v2_scripts = plutus_scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_v3_script_set_t*
cardano_witness_set_get_plutus_v3_scripts(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_plutus_v3_script_set_ref(witness_set->plutus_v3_scripts);

  return witness_set->plutus_v3_scripts;
}

cardano_error_t
cardano_witness_set_set_plutus_v3_scripts(
  cardano_witness_set_t*          witness_set,
  cardano_plutus_v3_script_set_t* plutus_scripts)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_scripts == NULL)
  {
    cardano_plutus_v3_script_set_unref(&witness_set->plutus_v3_scripts);
    witness_set->plutus_v3_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_v3_script_set_ref(plutus_scripts);
  cardano_plutus_v3_script_set_unref(&witness_set->plutus_v3_scripts);
  witness_set->plutus_v3_scripts = plutus_scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_data_set_t*
cardano_witness_set_get_plutus_data(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_plutus_data_set_ref(witness_set->plutus_data);

  return witness_set->plutus_data;
}

cardano_error_t
cardano_witness_set_set_plutus_data(
  cardano_witness_set_t*     witness_set,
  cardano_plutus_data_set_t* plutus_data)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data == NULL)
  {
    cardano_plutus_data_set_unref(&witness_set->plutus_data);
    witness_set->plutus_data = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_data_set_ref(plutus_data);
  cardano_plutus_data_set_unref(&witness_set->plutus_data);
  witness_set->plutus_data = plutus_data;

  return CARDANO_SUCCESS;
}

cardano_redeemer_list_t*
cardano_witness_set_get_redeemers(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return NULL;
  }

  cardano_redeemer_list_ref(witness_set->redeemer);

  return witness_set->redeemer;
}

cardano_error_t
cardano_witness_set_set_redeemers(
  cardano_witness_set_t*   witness_set,
  cardano_redeemer_list_t* redeemers)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemers == NULL)
  {
    cardano_redeemer_list_unref(&witness_set->redeemer);
    witness_set->redeemer = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_redeemer_list_ref(redeemers);
  cardano_redeemer_list_unref(&witness_set->redeemer);
  witness_set->redeemer = redeemers;

  return CARDANO_SUCCESS;
}

void
cardano_witness_set_clear_cbor_cache(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return;
  }

  cardano_plutus_data_set_clear_cbor_cache(witness_set->plutus_data);
  cardano_redeemer_list_clear_cbor_cache(witness_set->redeemer);

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_plutus_data_set_set_use_tag(witness_set->plutus_data, true);
  CARDANO_UNUSED(result);

  result = cardano_vkey_witness_set_set_use_tag(witness_set->vkey_witnesses, true);
  CARDANO_UNUSED(result);

  result = cardano_bootstrap_witness_set_set_use_tag(witness_set->bootstrap_witnesses, true);
  CARDANO_UNUSED(result);

  result = cardano_native_script_set_set_use_tag(witness_set->native_scripts, true);
  CARDANO_UNUSED(result);

  result = cardano_plutus_v1_script_set_set_use_tag(witness_set->plutus_v1_scripts, true);
  CARDANO_UNUSED(result);

  result = cardano_plutus_v2_script_set_set_use_tag(witness_set->plutus_v2_scripts, true);
  CARDANO_UNUSED(result);

  result = cardano_plutus_v3_script_set_set_use_tag(witness_set->plutus_v3_scripts, true);
  CARDANO_UNUSED(result);
}

cardano_error_t
cardano_witness_set_add_script(cardano_witness_set_t* witness_set, cardano_script_t* script)
{
  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  cardano_script_language_t language;

  result = cardano_script_get_language(script, &language);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
    {
      cardano_native_script_set_t* scripts = cardano_witness_set_get_native_scripts(witness_set);

      if (scripts == NULL)
      {
        result = cardano_native_script_set_new(&scripts);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        result = cardano_witness_set_set_native_scripts(witness_set, scripts);

        assert(result == CARDANO_SUCCESS);
        CARDANO_UNUSED(result);
      }

      cardano_native_script_set_unref(&scripts);

      cardano_native_script_t* native_script = NULL;

      result = cardano_script_to_native(script, &native_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_set_add(scripts, native_script);
      cardano_native_script_unref(&native_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      cardano_plutus_v1_script_set_t* scripts = cardano_witness_set_get_plutus_v1_scripts(witness_set);

      if (scripts == NULL)
      {
        result = cardano_plutus_v1_script_set_new(&scripts);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        result = cardano_witness_set_set_plutus_v1_scripts(witness_set, scripts);

        assert(result == CARDANO_SUCCESS);
        CARDANO_UNUSED(result);
      }

      cardano_plutus_v1_script_set_unref(&scripts);

      cardano_plutus_v1_script_t* plutus_v1_script = NULL;

      result = cardano_script_to_plutus_v1(script, &plutus_v1_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_plutus_v1_script_set_add(scripts, plutus_v1_script);
      cardano_plutus_v1_script_unref(&plutus_v1_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      cardano_plutus_v2_script_set_t* scripts = cardano_witness_set_get_plutus_v2_scripts(witness_set);

      if (scripts == NULL)
      {
        result = cardano_plutus_v2_script_set_new(&scripts);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        result = cardano_witness_set_set_plutus_v2_scripts(witness_set, scripts);

        assert(result == CARDANO_SUCCESS);
        CARDANO_UNUSED(result);
      }

      cardano_plutus_v2_script_set_unref(&scripts);

      cardano_plutus_v2_script_t* plutus_v2_script = NULL;

      result = cardano_script_to_plutus_v2(script, &plutus_v2_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_plutus_v2_script_set_add(scripts, plutus_v2_script);
      cardano_plutus_v2_script_unref(&plutus_v2_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      cardano_plutus_v3_script_set_t* scripts = cardano_witness_set_get_plutus_v3_scripts(witness_set);

      if (scripts == NULL)
      {
        result = cardano_plutus_v3_script_set_new(&scripts);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        result = cardano_witness_set_set_plutus_v3_scripts(witness_set, scripts);

        assert(result == CARDANO_SUCCESS);
        CARDANO_UNUSED(result);
      }

      cardano_plutus_v3_script_set_unref(&scripts);

      cardano_plutus_v3_script_t* plutus_v3_script = NULL;

      result = cardano_script_to_plutus_v3(script, &plutus_v3_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_plutus_v3_script_set_add(scripts, plutus_v3_script);
      cardano_plutus_v3_script_unref(&plutus_v3_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    default:
    {
      return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
    }
  }

  return CARDANO_SUCCESS;
}

void
cardano_witness_set_unref(cardano_witness_set_t** witness_set)
{
  if ((witness_set == NULL) || (*witness_set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*witness_set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *witness_set = NULL;
    return;
  }
}

void
cardano_witness_set_ref(cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return;
  }

  cardano_object_ref(&witness_set->base);
}

size_t
cardano_witness_set_refcount(const cardano_witness_set_t* witness_set)
{
  if (witness_set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&witness_set->base);
}

void
cardano_witness_set_set_last_error(cardano_witness_set_t* witness_set, const char* message)
{
  cardano_object_set_last_error(&witness_set->base, message);
}

const char*
cardano_witness_set_get_last_error(const cardano_witness_set_t* witness_set)
{
  return cardano_object_get_last_error(&witness_set->base);
}