/**
 * \file enterprise_addr_pack.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ENTERPRISE_ADDRESS_PACK_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ENTERPRISE_ADDRESS_PACK_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address_type.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Packs a Cardano enterprise address into a byte array.
 *
 * This function takes a Cardano enterprise address and encodes it into a byte array format.
 *
 * \param[in] address Pointer to the Cardano enterprise address structure to be packed.
 * \param[out] data Pointer to the byte array where the packed address will be stored.
 * \param[in] size Size of the data byte array in bytes.
 *
 * \note This function assumes that the `data` buffer has enough space to hold the packed
 *       address. The caller is responsible for ensuring that there is sufficient space
 *       and for managing the memory of the `data` buffer.
 */
void
_cardano_pack_enterprise_address(const cardano_address_t* address, byte_t* data, size_t size);

/**
 * \brief Unpacks a byte array into a Cardano enterprise address structure.
 *
 * This function takes a byte array containing the serialized form of a Cardano enterprise address
 * and decodes it into a Cardano enterprise address structure.
 *
 * \param[in] data Pointer to the byte array containing the serialized Cardano enterprise address.
 * \param[in] size Size of the byte array in bytes.
 * \param[out] address Double pointer to where the newly created Cardano enterprise address structure will be stored.
 *
 * \return Returns \ref CARDANO_SUCCESS if the unpacking is successful. Returns an error code if
 *         the unpacking fails due to invalid format, insufficient data provided, or memory allocation issues.
 *
 * \note The caller is responsible for managing the lifecycle of the created Cardano enterprise address object.
 *       This includes deallocating the address structure using the appropriate function when it is no longer needed.
 */
cardano_error_t
_cardano_unpack_enterprise_address(const byte_t* data, size_t size, cardano_enterprise_address_t** address);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ENTERPRISE_ADDRESS_PACK_H