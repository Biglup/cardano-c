/**
 * \file cip_1852_constants.c
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
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

#include <cardano/key_handlers/cip_1852_constants.h>

/* DEFINITIONS ***************************************************************/

const uint64_t CARDANO_CIP_1852_COIN_TYPE         = 1815U;
const uint64_t CARDANO_CIP_1852_PURPOSE_STANDARD  = 1852U;
const uint64_t CARDANO_CIP_1852_PURPOSE_MULTI_SIG = 1854U;
const uint64_t CARDANO_CIP_1852_ROLE_EXTERNAL     = 0U;
const uint64_t CARDANO_CIP_1852_ROLE_INTERNAL     = 1U;
const uint64_t CARDANO_CIP_1852_ROLE_STAKING      = 2U;
const uint64_t CARDANO_CIP_1852_ROLE_DREP         = 3U;
const uint64_t CARDANO_CIP_1852_ROLE_CC_COLD      = 4U;
const uint64_t CARDANO_CIP_1852_ROLE_CC_HOT       = 5U;