/**
 * \file uplc_machine_version.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_MACHINE_VERSION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_MACHINE_VERSION_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The Plutus language version selecting the default builtin costs.
 *
 * The versions differ in their default builtin costs; every version charges the
 * same machine-step costs. An unrecognized value is treated as V3.
 */
typedef enum
{
  /** \brief Plutus V1 default builtin costs. */
  CARDANO_UPLC_MACHINE_VERSION_V1 = 0,
  /** \brief Plutus V2 default builtin costs. */
  CARDANO_UPLC_MACHINE_VERSION_V2 = 1,
  /** \brief Plutus V3 default builtin costs (the default). */
  CARDANO_UPLC_MACHINE_VERSION_V3 = 2
} cardano_uplc_machine_version_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_MACHINE_VERSION_H */
