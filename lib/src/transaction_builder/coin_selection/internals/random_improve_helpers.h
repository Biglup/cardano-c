/**
 * \file random_improve_helpers.h
 *
 * \author angel.castillo
 * \date   Jul 02, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_HELPERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_HELPERS_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Advances the given SplitMix64 state and returns the next pseudo-random number.
 *
 * SplitMix64 is a well-known, statistically strong 64-bit pseudo-random number generator with a
 * single 64-bit word of state ("Fast Splittable Pseudorandom Number Generators", Steele, Lea and
 * Flood, OOPSLA 2014; the constants are from Sebastiano Vigna's public domain reference
 * implementation). It is used by the random improve coin selector because its tiny state makes it
 * trivial to re-seed deterministically on every selection, which keeps repeated invocations from
 * the transaction balancing loop reproducible.
 *
 * \param[in,out] rng_state The generator state. Updated in place on every call.
 *
 * \return The next pseudo-random 64-bit number in the sequence.
 *
 * \note This generator is NOT cryptographically secure. It must only be used for input selection
 *       randomization, never for key material or any security sensitive purpose.
 */
uint64_t
_cardano_random_improve_rng_next(uint64_t* rng_state);

/**
 * \brief Returns a pseudo-random index in the range [0, bound).
 *
 * \param[in,out] rng_state The generator state. Updated in place. See \ref _cardano_random_improve_rng_next.
 * \param[in]     bound     The exclusive upper bound. Must be greater than zero.
 *
 * \return A pseudo-random index in the range [0, bound).
 *
 * \note The index is derived by modulo reduction, which introduces a negligible bias for the small
 *       bounds used during input selection (bounded by the number of UTXOs in a wallet).
 */
size_t
_cardano_random_improve_rng_below(uint64_t* rng_state, size_t bound);

/**
 * \brief Returns the absolute difference between two quantities.
 *
 * This is a port of `distance` from the cardano-coin-selection library. It is used by the
 * "improve" phase of the selection algorithm to decide whether an additional UTXO moves the
 * selected quantity closer to its target.
 *
 * \param[in] a The first quantity.
 * \param[in] b The second quantity.
 *
 * \return The absolute difference between the two quantities.
 */
uint64_t
_cardano_random_improve_distance(uint64_t a, uint64_t b);

/**
 * \brief Partitions a target quantity into parts proportional to the given weights.
 *
 * This is a port of `partitionNatural` from the cardano-coin-selection library. The result is
 * computed with the largest-remainder method: each part receives the floor of its ideal
 * proportional share, and the leftover units are awarded to the parts with the largest
 * remainders (ties broken by the larger integral part, then by original position).
 *
 * The sum of the resulting parts is always exactly equal to the target quantity.
 *
 * \param[in]  target  The quantity to partition.
 * \param[in]  weights The weight of each part.
 * \param[in]  size    The number of parts. Must be greater than zero.
 * \param[out] parts   The resulting parts, in the same order as the given weights.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_INVALID_ARGUMENT if the sum
 *         of the weights is zero.
 */
cardano_error_t
_cardano_random_improve_partition(
  uint64_t        target,
  const uint64_t* weights,
  size_t          size,
  uint64_t*       parts);

/**
 * \brief Adjusts a list of quantities to a given length, preserving its sum and ascending order.
 *
 * This is a port of `padCoalesce` from the cardano-coin-selection library:
 *
 * - The source quantities are first sorted into ascending order.
 * - If the source is shorter than the target length, zeros are inserted at the start.
 * - If the source is longer than the target length, the two smallest quantities are repeatedly
 *   coalesced (summed and re-inserted in sorted position) until the target length is reached.
 *
 * \param[in]  quantities     The source quantities.
 * \param[in]  size           The number of source quantities.
 * \param[in]  target_size    The desired number of output quantities. Must be greater than zero.
 * \param[out] result         The resulting quantities, sorted in ascending order.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
cardano_error_t
_cardano_random_improve_pad_coalesce(
  const uint64_t* quantities,
  size_t          size,
  size_t          target_size,
  uint64_t*       result);

/**
 * \brief Reduces the total value of an ascending list of quantities by a reduction target.
 *
 * This is a port of `reduceTokenQuantities` from the cardano-coin-selection library. It walks
 * the list from left to right (smallest first), zeroing out each quantity until the total
 * reduction is equal to the reduction target, or until the list is exhausted.
 *
 * \param[in]     reduction_target The total amount by which to reduce the quantities.
 * \param[in,out] quantities       The quantities to reduce, sorted in ascending order.
 * \param[in]     size             The number of quantities.
 */
void
_cardano_random_improve_reduce_quantities(
  uint64_t  reduction_target,
  uint64_t* quantities,
  size_t    size);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_HELPERS_H
