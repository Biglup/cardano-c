/**
 * \file datum_type.h
 *
 * \author luisd.bianchi
 * \date   May 15, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_DATUM_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_DATUM_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents different ways of associating a Datum with a UTxO in a transaction.
 */
typedef enum
{
  /**
   * \brief Instead of including the full Datum directly within the transaction, it's possible to
   * include just a hash of the Datum. This is the DatumHash. By referencing the Datum
   * by its hash, the transaction can be more compact, especially if the Datum itself is large.
   * However, when using a DatumHash, the actual Datum value it represents must be provided
   * in the transaction witness set to ensure that users and validators can verify and use it.
   */
  CARDANO_DATUM_TYPE_DATA_HASH = 0,

  /**
   * \brief This represents the actual Datum value being included directly
   * within the transaction output. So, the Datum is "inlined" in the transaction
   * data itself.
   */
  CARDANO_DATUM_TYPE_INLINE_DATA = 1
} cardano_datum_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_DATUM_TYPE_H