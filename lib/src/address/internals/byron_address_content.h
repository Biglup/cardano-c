/**
 * \file byron_address_content.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_CONTENT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_CONTENT_H

/* INCLUDES ******************************************************************/

#include <cardano/address/byron_address_attributes.h>
#include <cardano/address/byron_address_type.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * Byron address internal structure:
 *
 * ┌────────┬──────────────┬────────┐
 * │  root  │  attributes  │  type  │
 * └────────┴──────────────┴────────┘
 *   ╎        ╎              ╎
 *   ╎        ╎              ╰╌╌ PubKey
 *   ╎        ╎              ╰╌╌ Script
 *   ╎        ╎              ╰╌╌ Redeem
 *   ╎        ╰╌╌ Derivation Path
 *   ╎        ╰╌╌ Network Tag
 *   ╎
 *   ╎                   ┌────────┬─────────────────┬──────────────┐
 *   ╰╌╌╌╌ double-hash ( │  type  │  spending data  │  attributes  │ )
 *                       └────────┴─────────────────┴──────────────┘
 *                                  ╎
 *                                  ╰╌╌ Verification Key
 *                                  ╰╌╌ Redemption Key
 */
typedef struct cardano_byron_address_content_t
{
    byte_t                             root[28];
    cardano_byron_address_attributes_t attributes;
    cardano_byron_address_type_t       type;
} cardano_byron_address_content_t;

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_CONTENT_H