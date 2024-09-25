/**
 * \file stake_pointer.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_STAKE_POINTER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_STAKE_POINTER_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a pointer to a stake key registration location on the blockchain.
 *
 * This structure is used to indirectly reference a stake key through its registration certificate location on the blockchain.
 * It is used in Cardano pointer addresses, which allow for the specification of stake rights without directly embedding
 * the stake key itself in the address.
 */
typedef struct cardano_stake_pointer_t
{
    /**
     * \brief The slot number on the blockchain where the stake key registration certificate is recorded.
     * This is a uint64_t value that represents the absolute slot since the genesis block of the blockchain.
     */
    uint64_t slot;

    /**
     * \brief The index of the transaction within the specified slot where the stake key registration certificate is located.
     * This value helps pinpoint the exact transaction in a given slot that contains the certificate.
     */
    uint64_t tx_index;

    /**
     * \brief The index of the certificate within the transaction. Since a single transaction can contain multiple certificates,
     * this value specifies which certificate in the transaction pertains to the stake key registration.
     */
    uint64_t cert_index;
} cardano_stake_pointer_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_STAKE_POINTER_H