/**
 * \file mir_cert_pot_type.h
 *
 * \author angel.castillo
 * \date   jun 19, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license");
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 *     www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_POT_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_POT_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the target pots for Move Instantaneous Reward (MIR) certificates.
 *
 * MIR certificates in Cardano can move funds between different accounting pots.
 * This enumeration defines the types of pots to which funds can be transferred.
 */
typedef enum
{
  /**
   * \brief Indicates that the MIR certificate moves funds to the reserve pot.
   *
   * The reserve pot in Cardano is a pool of ADA coins that are gradually released into
   * circulation. This reserve is used to provide a continuous supply of ADA for staking
   * rewards and other incentives. The reserve helps ensure the long-term sustainability
   * of the Cardano network by maintaining a steady flow of rewards for network participants.
   */
  CARDANO_MIR_CERT_POT_TYPE_RESERVE = 0,

  /**
   * \brief Indicates that the MIR certificate moves funds to the treasury pot.
   *
   * The treasury pot in Cardano is a fund allocated for the development and improvement
   * of the Cardano ecosystem. For example, it is used to finance projects, proposals, and initiatives
   * through the Project Catalyst governance system. The treasury is filled by a portion
   * of transaction fees and monetary expansion.
   */
  CARDANO_MIR_CERT_POT_TYPE_TREASURY = 1
} cardano_mir_cert_pot_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_POT_TYPE_H