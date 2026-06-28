/**
 * \file uplc_value_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_VALUE_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_VALUE_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The kind of a CEK runtime value.
 *
 * Selects the active arm of \ref cardano_uplc_value_t.
 */
typedef enum
{
  /** \brief A constant of the constant universe. */
  CARDANO_UPLC_VALUE_CONSTANT = 0,
  /** \brief A delayed computation closed over its environment. */
  CARDANO_UPLC_VALUE_DELAY = 1,
  /** \brief A lambda closure closed over its environment. */
  CARDANO_UPLC_VALUE_LAMBDA = 2,
  /** \brief A builtin with the forces and arguments accumulated so far. */
  CARDANO_UPLC_VALUE_BUILTIN = 3,
  /** \brief A constructor value carrying a tag and resolved fields. */
  CARDANO_UPLC_VALUE_CONSTR = 4
} cardano_uplc_value_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_VALUE_KIND_H */
