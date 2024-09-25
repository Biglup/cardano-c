/**
 * \file addr_common.h
 *
 * \author angel.castillo
 * \date   Apr 15, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_INTERNALS_COMMON_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_INTERNALS_COMMON_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address_type.h>
#include <cardano/address/base_address.h>
#include <cardano/address/byron_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/address/stake_pointer.h>
#include <cardano/common/credential.h>
#include <cardano/common/network_id.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

#include "byron_address_content.h"

/* STRUCTURES ***************************************************************/

/**
 * \brief Represents a Cardano address.
 *
 * This structure encapsulates all the necessary information for a Cardano address.
 */
typedef struct cardano_address_t
{
    cardano_object_t                 base;
    cardano_address_type_t           type;
    char                             address_str[1024];
    byte_t                           address_data[128];
    size_t                           address_data_size;
    cardano_network_id_t*            network_id;
    cardano_stake_pointer_t*         stake_pointer;
    cardano_credential_t*            payment_credential;
    cardano_credential_t*            stake_credential;
    cardano_byron_address_content_t* byron_content;

} cardano_address_t;

/* FUNCTIONS ***************************************************************/

/**
 * \brief Retrieves the Bech32 prefix for a given address type and network ID.
 *
 * This function determines the appropriate Bech32 prefix for addresses based on their type
 * and the network they belong to.
 *
 * \param[in] type The type of the address for which the Bech32 prefix is required.
 * \param[in] network_id The network ID where the address is being used.
 * \param[out] size The length of the Bech32 prefix string.
 *
 * \return A constant string representing the Bech32 prefix corresponding to the given address type
 *         and network ID. If the combination of address type and network ID is not recognized,
 *         the function returns NULL.
 */
const char* _cardano_get_bech32_prefix(cardano_address_type_t type, cardano_network_id_t network_id, size_t* size);

/**
 * \brief Checks whether the given string has a valid bech32 prefix.
 *
 * \param[in] address The string to be validated.
 * \param[in] length The length of the address string not including the null terminator.
 *
 * \return Returns \c true if the address string has one of the valid bech32 prefixes, otherwise returns \c false.
 */
bool _cardano_has_valid_bech32_prefix(const char* address, size_t length);

/**
 * \brief Casts a cardano_base_address_t pointer to a cardano_address_t pointer.
 *
 * This function is used to cast a base address structure pointer to a more general address
 * structure pointer. It allows for the use of base address specific data in functions or
 * contexts where a general address structure is required. This casting is safe as long as
 * the base address structure is correctly aligned and compatible with the general address
 * structure.
 *
 * \param[in] base_address A pointer to the cardano_base_address_t structure that is to be cast.
 *
 * \return A pointer to the cardano_address_t structure derived from the provided base_address pointer.
 *         If the input pointer is NULL, the function will return NULL.
 */
cardano_address_t* _cardano_from_base_to_address(cardano_base_address_t* base_address);

/**
 * \brief Converts a pointer to a cardano_base_address_t pointer into a pointer to a cardano_address_t pointer.
 *
 * \param[in] base_address A double pointer to the cardano_base_address_t structure that is to be converted.
 *
 * \return A double pointer to the cardano_address_t structure derived from the provided base_address double pointer.
 *         If the input double pointer is NULL, the function will return NULL.
 */
cardano_address_t** _cardano_from_base_pointer_to_address_pointer(cardano_base_address_t** base_address);

/**
 * \brief Casts a constant pointer to cardano_base_address_t into a constant pointer to cardano_address_t.
 *
 * \param[in] base_address A constant pointer to the cardano_base_address_t structure to be converted.
 *
 * \return A constant pointer to a cardano_address_t structure, pointing to the same address data as the input.
 *         If the input pointer is NULL, the function also returns NULL.
 */
const cardano_address_t* _cardano_from_base_to_address_const(const cardano_base_address_t* base_address);

/**
 * \brief Casts a pointer from the general cardano_address_t type to the more specific cardano_base_address_t type.
 *
 * \param[in] address A pointer to the cardano_address_t structure to be converted.
 *
 * \return A pointer to a cardano_base_address_t structure, which is a more specific type derived from the input
 *         general address type. If the input pointer is NULL, or if the address type does not match the expected
 *         base address type, the function returns NULL.
 */
cardano_base_address_t* _cardano_from_address_to_base(cardano_address_t* address);

/**
 * \brief Casts a pointer from the specific cardano_byron_address_t type to the general cardano_address_t type.
 *
 * \param[in] byron_address A pointer to the cardano_byron_address_t structure to be converted.
 *
 * \return A pointer to a cardano_address_t structure, representing the general address type that can be used
 *         in broader Cardano-related operations. If the input pointer is NULL, the function returns NULL,
 *         reflecting the inability to perform the conversion.
 */
cardano_address_t* _cardano_from_byron_to_address(cardano_byron_address_t* byron_address);

/**
 * \brief Converts a pointer to a pointer from the Byron-specific address type (cardano_byron_address_t)
 * to the general address type (cardano_address_t) used across the Cardano platform.
 *
 * \param[in] byron_address A double pointer to the cardano_byron_address_t structure to be converted.
 *
 * \return A double pointer to a cardano_address_t structure. This allows for direct modification of the
 *         underlying address pointer in higher-level functions. If the input double pointer is NULL,
 *         or if it points to NULL, the function returns NULL to indicate that conversion cannot be performed.
 */
cardano_address_t** _cardano_from_byron_pointer_to_address_pointer(cardano_byron_address_t** byron_address);

/**
 * \brief Converts a constant pointer from a Byron-specific address type (cardano_byron_address_t)
 * to a constant pointer of the general address type (cardano_address_t).
 *
 * \param[in] byron_address A constant pointer to the cardano_byron_address_t structure to be converted.
 *
 * \return A constant pointer to a cardano_address_t structure, allowing read-only access to the Byron address
 *         in contexts that require a general address type. If the input pointer is NULL, the function returns NULL,
 *         indicating that there is no address to convert.
 */
const cardano_address_t* _cardano_from_byron_to_address_const(const cardano_byron_address_t* byron_address);

/**
 * \brief Converts a pointer from a general Cardano address type (cardano_address_t)
 * to a Byron-specific address type (cardano_byron_address_t).
 *
 * \param[in] address A pointer to the cardano_address_t structure to be converted.
 *
 * \return A pointer to a cardano_byron_address_t structure that represents the Byron address.
 *         If the input pointer is NULL, or if the address does not conform to the Byron format,
 *         the function returns NULL, indicating that the conversion could not be performed.
 */
cardano_byron_address_t* _cardano_from_address_to_byron(cardano_address_t* address);

/**
 * \brief Casts a cardano_enterprise_address_t pointer to a cardano_address_t pointer.
 *
 * This function is used to cast a enterprise address structure pointer to a more general address
 * structure pointer. It allows for the use of enterprise address specific data in functions or
 * contexts where a general address structure is required. This casting is safe as long as
 * the enterprise address structure is correctly aligned and compatible with the general address
 * structure.
 *
 * \param[in] enterprise_address A pointer to the cardano_enterprise_address_t structure that is to be cast.
 *
 * \return A pointer to the cardano_address_t structure derived from the provided enterprise_address pointer.
 *         If the input pointer is NULL, the function will return NULL.
 */
cardano_address_t* _cardano_from_enterprise_to_address(cardano_enterprise_address_t* enterprise_address);

/**
 * \brief Converts a pointer to a cardano_enterprise_address_t pointer into a pointer to a cardano_address_t pointer.
 *
 * \param[in] enterprise_address A double pointer to the cardano_enterprise_address_t structure that is to be converted.
 *
 * \return A double pointer to the cardano_address_t structure derived from the provided enterprise_address double pointer.
 *         If the input double pointer is NULL, the function will return NULL.
 */
cardano_address_t** _cardano_from_enterprise_pointer_to_address_pointer(cardano_enterprise_address_t** enterprise_address);

/**
 * \brief Casts a constant pointer to cardano_enterprise_address_t into a constant pointer to cardano_address_t.
 *
 * \param[in] enterprise_address A constant pointer to the cardano_enterprise_address_t structure to be converted.
 *
 * \return A constant pointer to a cardano_address_t structure, pointing to the same address data as the input.
 *         If the input pointer is NULL, the function also returns NULL.
 */
const cardano_address_t* _cardano_from_enterprise_to_address_const(const cardano_enterprise_address_t* enterprise_address);

/**
 * \brief Casts a pointer from the general cardano_address_t type to the more specific cardano_enterprise_address_t type.
 *
 * \param[in] address A pointer to the cardano_address_t structure to be converted.
 *
 * \return A pointer to a cardano_enterprise_address_t structure, which is a more specific type derived from the input
 *         general address type. If the input pointer is NULL, or if the address type does not match the expected
 *         enterprise address type, the function returns NULL.
 */
cardano_enterprise_address_t* _cardano_from_address_to_enterprise(cardano_address_t* address);

/**
 * \brief Casts a cardano_pointer_address_t pointer to a cardano_address_t pointer.
 *
 * This function is used to cast a pointer address structure pointer to a more general address
 * structure pointer. It allows for the use of pointer address specific data in functions or
 * contexts where a general address structure is required. This casting is safe as long as
 * the pointer address structure is correctly aligned and compatible with the general address
 * structure.
 *
 * \param[in] pointer_address A pointer to the cardano_pointer_address_t structure that is to be cast.
 *
 * \return A pointer to the cardano_address_t structure derived from the provided pointer_address pointer.
 *         If the input pointer is NULL, the function will return NULL.
 */
cardano_address_t* _cardano_from_pointer_to_address(cardano_pointer_address_t* pointer_address);

/**
 * \brief Converts a pointer to a cardano_pointer_address_t pointer into a pointer to a cardano_address_t pointer.
 *
 * \param[in] pointer_address A double pointer to the cardano_pointer_address_t structure that is to be converted.
 *
 * \return A double pointer to the cardano_address_t structure derived from the provided pointer_address double pointer.
 *         If the input double pointer is NULL, the function will return NULL.
 */
cardano_address_t** _cardano_from_pointer_pointer_to_address_pointer(cardano_pointer_address_t** pointer_address);

/**
 * \brief Casts a constant pointer to cardano_pointer_address_t into a constant pointer to cardano_address_t.
 *
 * \param[in] pointer_address A constant pointer to the cardano_pointer_address_t structure to be converted.
 *
 * \return A constant pointer to a cardano_address_t structure, pointing to the same address data as the input.
 *         If the input pointer is NULL, the function also returns NULL.
 */
const cardano_address_t* _cardano_from_pointer_to_address_const(const cardano_pointer_address_t* pointer_address);

/**
 * \brief Casts a pointer from the general cardano_address_t type to the more specific cardano_pointer_address_t type.
 *
 * \param[in] address A pointer to the cardano_address_t structure to be converted.
 *
 * \return A pointer to a cardano_pointer_address_t structure, which is a more specific type derived from the input
 *         general address type. If the input pointer is NULL, or if the address type does not match the expected
 *         pointer address type, the function returns NULL.
 */
cardano_pointer_address_t* _cardano_from_address_to_pointer(cardano_address_t* address);

/**
 * \brief Casts a cardano_reward_address_t pointer to a cardano_address_t pointer.
 *
 * This function is used to cast a reward address structure pointer to a more general address
 * structure pointer. It allows for the use of reward address specific data in functions or
 * contexts where a general address structure is required. This casting is safe as long as
 * the reward address structure is correctly aligned and compatible with the general address
 * structure.
 *
 * \param[in] reward_address A pointer to the cardano_reward_address_t structure that is to be cast.
 *
 * \return A pointer to the cardano_address_t structure derived from the provided reward_address pointer.
 *         If the input pointer is NULL, the function will return NULL.
 */
cardano_address_t* _cardano_from_reward_to_address(cardano_reward_address_t* reward_address);

/**
 * \brief Converts a pointer to a cardano_reward_address_t pointer into a pointer to a cardano_address_t pointer.
 *
 * \param[in] reward_address A double pointer to the cardano_reward_address_t structure that is to be converted.
 *
 * \return A double pointer to the cardano_address_t structure derived from the provided reward_address double pointer.
 *         If the input double pointer is NULL, the function will return NULL.
 */
cardano_address_t** _cardano_from_reward_pointer_to_address_pointer(cardano_reward_address_t** reward_address);

/**
 * \brief Casts a constant pointer to cardano_reward_address_t into a constant pointer to cardano_address_t.
 *
 * \param[in] reward_address A constant pointer to the cardano_reward_address_t structure to be converted.
 *
 * \return A constant pointer to a cardano_address_t structure, pointing to the same address data as the input.
 *         If the input pointer is NULL, the function also returns NULL.
 */
const cardano_address_t* _cardano_from_reward_to_address_const(const cardano_reward_address_t* reward_address);

/**
 * \brief Casts a pointer from the general cardano_address_t type to the more specific cardano_reward_address_t type.
 *
 * \param[in] address A pointer to the cardano_address_t structure to be converted.
 *
 * \return A pointer to a cardano_reward_address_t structure, which is a more specific type derived from the input
 *         general address type. If the input pointer is NULL, or if the address type does not match the expected
 *         reward address type, the function returns NULL.
 */
cardano_reward_address_t* _cardano_from_address_to_reward(cardano_address_t* address);

/**
 * \brief Retrieves the payment credential type associated with a specific Cardano address type.
 *
 * This function is used to determine the type of payment credential (e.g., public key hash, script hash)
 * associated with a particular Cardano address type.
 *
 * \param[in] type The Cardano address type for which the payment credential type is to be determined.
 * \param[out] credential_type Pointer to a cardano_credential_type_t variable where the result will be stored.
 *             This variable will hold the credential type corresponding to the specified address type after
 *             successful execution of the function.
 *
 * \return A cardano_error_t \ref CARDANO_SUCCESS if the operation is successful and the credential type is
 *                           correctly retrieved, or an error code indicating the reason for failure.
 */
cardano_error_t _cardano_get_payment_credential_type(cardano_address_type_t type, cardano_credential_type_t* credential_type);

/**
 * \brief Retrieves the stake credential type associated with a specific Cardano address type.
 *
 * This function determines the type of stake credential (e.g., public key hash, script hash)
 * that corresponds to a particular Cardano address type.
 *
 * \param[in] type The Cardano address type for which the stake credential type is to be determined.
 * \param[out] credential_type Pointer to a cardano_credential_type_t variable where the result will be stored.
 *             This variable will hold the stake credential type corresponding to the specified address type after
 *             successful execution of the function.
 *
 * \return A cardano_error_t \ref CARDANO_SUCCESS if the operation is successful and the credential type is
 *                           correctly retrieved, or an error code indicating the reason for failure.
 */
cardano_error_t _cardano_get_stake_credential_type(cardano_address_type_t type, cardano_credential_type_t* credential_type);

/**
 * \brief Gets whether the given address has a valid payment address prefix.
 *
 * \param address The address to check the prefix for.
 * \param length The length of the address without the null terminator.
 *
 * \return \c true if the prefix is valid; otherwise; \c false.
 */
bool _cardano_is_valid_payment_address_prefix(const char* address, size_t length);

/**
 * \brief Gets whether the given address has a valid stake address prefix.
 *
 * \param address The address to check the prefix for.
 * \param length The length of the address without the null terminator.
 *
 * \return \c true if the prefix is valid; otherwise; \c false.
 */
bool _cardano_is_valid_stake_address_prefix(const char* address, size_t length);

/**
 * \brief Encodes the given address data into bech32.
 *
 * \param[in] data The address binary data.
 * \param[in] data_size The size of the binary data.
 * \param[in] network_id The network id.
 * \param[in] type The address type.
 * \param[out] address The buffer where to write the address data to.
 * \param[in] address_size The size of the address buffer.
 */
void _cardano_to_bech32_addr(
  const byte_t*          data,
  size_t                 data_size,
  cardano_network_id_t   network_id,
  cardano_address_type_t type,
  char*                  address,
  size_t                 address_size);

/**
 * \brief Deallocates a address object.
 *
 * This function is responsible for properly deallocating a address object (`cardano_address_t`)
 * and its associated resources.
 *
 * \param[in] object A void pointer to the address object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_address_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the address
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
void _cardano_address_deallocate(void* object);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_INTERNALS_COMMON_H