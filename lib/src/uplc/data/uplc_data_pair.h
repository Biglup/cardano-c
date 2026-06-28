/**
 * \file uplc_data_pair.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_PAIR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_PAIR_H

/* FORWARD DECLARATIONS ******************************************************/

/**
 * \brief A lean, arena-allocated Plutus-data node, defined in \c uplc_data.h.
 *
 * A data pair references its key and value by pointer, so only a forward
 * declaration is needed here.
 */
struct cardano_uplc_data_t;

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A single (key, value) entry of an arena data map.
 */
typedef struct cardano_uplc_data_pair_t
{
    const struct cardano_uplc_data_t* key;
    const struct cardano_uplc_data_t* value;
} cardano_uplc_data_pair_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_PAIR_H */
