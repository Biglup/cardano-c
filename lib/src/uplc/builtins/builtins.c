/**
 * \file builtins.c
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

/* INCLUDES ******************************************************************/

#include "builtins.h"

#include "../../allocators.h"
#include "../arena/uplc_arena.h"
#include "../ast/uplc_int.h"
#include "bls.h"

#include <blst.h>

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/byte_order.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include <sodium/crypto_generichash.h>
#include <sodium/crypto_hash_sha256.h>

#include <ripemd160.h>
#include <sha3.h>

#ifndef SECP256K1_STATIC
#define SECP256K1_STATIC
#endif

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include <stddef.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Number of distinct byte values, the modulus for the wrapping
 *        \c consByteString and the exclusive upper bound for a byte literal.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_BUILTIN_BYTE_MODULUS = 256;

/**
 * \brief Highest valid byte value, used by \c consByteString and
 *        \c replicateByte range checks and as the all-ones complement mask.
 */
static const int64_t CARDANO_UPLC_BUILTIN_BYTE_MAX = 255;

/**
 * \brief Maximum output length \c integerToByteString accepts, in bytes.
 *
 * Caps both the requested width and the minimal-width encoding; a value that would
 * exceed this length is a script error.
 */
static const int64_t CARDANO_UPLC_BUILTIN_INT_TO_BS_MAX = 8192;

/**
 * \brief Output length, in bytes, of a Blake2b-224 digest.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_BLAKE2B_224_SIZE = 28U;

/**
 * \brief Output length, in bytes, of a Blake2b-256 digest.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_BLAKE2B_256_SIZE = 32U;

/**
 * \brief Length, in bytes, of an Ed25519 public key.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_ED25519_KEY_SIZE = 32U;

/**
 * \brief Length, in bytes, of an Ed25519 signature.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_ED25519_SIG_SIZE = 64U;

/**
 * \brief Length, in bytes, of a compressed secp256k1 ECDSA public key.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_ECDSA_KEY_SIZE = 33U;

/**
 * \brief Length, in bytes, of the pre-hashed message an ECDSA verify consumes.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_ECDSA_MSG_SIZE = 32U;

/**
 * \brief Length, in bytes, of a compact secp256k1 ECDSA signature.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_ECDSA_SIG_SIZE = 64U;

/**
 * \brief Length, in bytes, of an x-only BIP340 Schnorr public key.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_SCHNORR_KEY_SIZE = 32U;

/**
 * \brief Length, in bytes, of a BIP340 Schnorr signature.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_SCHNORR_SIG_SIZE = 64U;

/**
 * \brief Output length, in bytes, of a SHA3-256 or Keccak-256 digest.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_SHA3_256_SIZE = 32U;

/**
 * \brief Output length, in bytes, of a RIPEMD-160 digest.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t CARDANO_UPLC_BUILTIN_RIPEMD_160_SIZE = 20U;

/**
 * \brief Domain-separation pad byte selecting FIPS-202 SHA-3 in \c sha3_final_pad.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t CARDANO_UPLC_BUILTIN_SHA3_PAD = 0x06U;

/**
 * \brief Domain-separation pad byte selecting the original Keccak in \c sha3_final_pad.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t CARDANO_UPLC_BUILTIN_KECCAK_PAD = 0x01U;

/**
 * \brief Width in bytes of a BLS12-381 scalar.
 */
#define CARDANO_UPLC_BLS_SCALAR_SIZE ((size_t)32U)

/**
 * \brief Width in bits of a BLS12-381 scalar.
 */
static const size_t CARDANO_UPLC_BLS_SCALAR_BITS = 256U;

/**
 * \brief Maximum length in bytes of a hash-to-curve domain separation tag.
 *
 * A domain separation tag longer than this is rejected by the BLS12-381 G1 and G2
 * hash-to-group builtins as a script error.
 */
static const size_t CARDANO_UPLC_BLS_DST_MAX = 255U;

/**
 * \brief The BLS12-381 scalar field order \c r in big-endian bytes.
 *
 * A scalar argument to \c scalarMul is reduced modulo this period before the
 * multiplication.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const byte_t CARDANO_UPLC_BLS_SCALAR_PERIOD[CARDANO_UPLC_BLS_SCALAR_SIZE] = {
  0x73U,
  0xedU,
  0xa7U,
  0x53U,
  0x29U,
  0x9dU,
  0x7dU,
  0x48U,
  0x33U,
  0x39U,
  0xd8U,
  0x08U,
  0x09U,
  0xa1U,
  0xd8U,
  0x05U,
  0x53U,
  0xbdU,
  0xa4U,
  0x02U,
  0xffU,
  0xfeU,
  0x5bU,
  0xfeU,
  0xffU,
  0xffU,
  0xffU,
  0xffU,
  0x00U,
  0x00U,
  0x00U,
  0x01U
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Reads the constant a value wraps, if it is a constant value.
 *
 * A non-constant value (a delay, lambda, builtin or constr) is reported by
 * returning \c false.
 *
 * \param[in] value The value to read. Must not be NULL.
 * \param[out] out On success, the borrowed constant; left untouched otherwise.
 *
 * \return \c true when \p value is a constant, \c false otherwise.
 */
static bool
as_constant(const cardano_uplc_value_t* value, const cardano_uplc_constant_t** out)
{
  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return false;
  }

  *out = value->as.constant;

  return true;
}

/**
 * \brief Spends a builtin's computed cost on the step accumulator.
 *
 * Computes the cpu/mem cost from the ex-mem sizes of \p args and charges it
 * before the body runs.
 *
 * \param[in,out] acc The accumulator to charge. Must not be NULL.
 * \param[in] costs The per-builtin costs. Must not be NULL.
 * \param[in] semantics The semantics variant selecting the string ex-mem measure.
 * \param[in] func The builtin being run.
 * \param[in] args The saturated argument values.
 * \param[in] arg_count The number of arguments.
 *
 * \return \ref CARDANO_SUCCESS on success, propagating any accumulator failure.
 */
static cardano_error_t
spend_builtin_cost(
  cardano_uplc_step_accumulator_t*    acc,
  const cardano_uplc_builtin_costs_t* costs,
  cardano_uplc_builtin_semantics_t    semantics,
  cardano_uplc_builtin_t              func,
  const cardano_uplc_value_t* const*  args,
  size_t                              arg_count)
{
  bool                  by_utf8 = cardano_uplc_semantics_costs_strings_by_utf8_bytes(semantics);
  cardano_uplc_budget_t cost    = cardano_uplc_builtin_cost(costs, func, args, arg_count, by_utf8);

  return cardano_uplc_step_accumulator_charge(acc, cost);
}

/* BODY HELPERS *************************************************************/

/**
 * \brief Wraps an owned bigint as an arena result value and drops the caller ref.
 *
 * Builds an integer constant that borrows \p value (the constructor takes its own
 * reference and registers the arena unref hook), wraps it in a constant value, and
 * always releases the reference \p value held on entry. On failure \p value is
 * still released so the helper never leaks.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in,out] value The owned bigint to publish; released before return.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_integer(
  struct cardano_uplc_arena_t* arena,
  cardano_bigint_t*            value,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = CARDANO_SUCCESS;
  int64_t                  small    = 0;

  if (value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (cardano_uplc_int_bigint_fits_int64(value, &small))
  {
    error = cardano_uplc_constant_new_integer_small(arena, small, &constant);
  }
  else
  {
    error = cardano_uplc_constant_new_integer(arena, value, &constant);
  }

  if (error != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&value);

    return error;
  }

  cardano_bigint_unref(&value);

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Builds an integer result value from a host \c int64_t.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] n The integer value.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_integer_from_int(
  struct cardano_uplc_arena_t* arena,
  int64_t                      n,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = cardano_uplc_constant_new_integer_small(arena, n, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief A read-only view of an integer argument as either inline or bigint.
 *
 * \c is_small selects the active field: \c small holds the value when inline and
 * \c big the value when not. A big integer is by construction out of \c int64_t
 * range, so \c is_small alone distinguishes the two domains.
 */
typedef struct int_view_t
{
    bool                           is_small;
    int64_t                        small;
    const cardano_uplc_constant_t* constant;
} int_view_t;

/**
 * \brief Reads a value as an integer view without materializing a bigint.
 *
 * \param[in] value The value to inspect.
 * \param[out] view On success, the integer view.
 *
 * \return \c true when \p value is a constant integer, \c false otherwise.
 */
static bool
as_int_view(const cardano_uplc_value_t* value, int_view_t* view)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_INTEGER)
  {
    return false;
  }

  view->constant = constant;
  view->is_small = cardano_uplc_constant_int_is_small(constant);
  view->small    = view->is_small ? cardano_uplc_constant_int_small(constant) : 0;

  return true;
}

/**
 * \brief Reads a value as a \ref cardano_bigint_t, materializing inline values.
 *
 * The cold-path accessor for builtins that genuinely need a bigint (modular
 * exponentiation, the integer/bytestring conversions, scalar reduction, and the
 * data and multi-asset boundaries). For an inline integer it builds a bigint once
 * and caches it in the arena-owned constant; the returned pointer is owned by the
 * arena and must not be unref'd by the caller.
 *
 * \param[in] arena The arena the materialized bigint is registered with.
 * \param[in] value The value to read.
 * \param[out] out On success, the bigint view.
 *
 * \return \c true when \p value is a constant integer and a bigint is available,
 *         \c false when \p value is not an integer or materialization fails.
 */
static bool
as_integer_big(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_value_t*  value,
  const cardano_bigint_t**     out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_INTEGER)
  {
    return false;
  }

  return cardano_uplc_constant_int_materialize(arena, constant, out) == CARDANO_SUCCESS;
}

/**
 * \brief Reads the integer arm of a constant as a \ref cardano_bigint_t.
 *
 * Like \ref as_integer_big but takes a constant directly, for the writeBits,
 * multi-asset and multi-scalar paths that inspect list or pair items rather than a
 * top-level value.
 *
 * \param[in] arena The arena the materialized bigint is registered with.
 * \param[in] constant An integer constant. Must not be NULL.
 * \param[out] out On success, the bigint view.
 *
 * \return \c true on success, \c false if \p constant is not an integer or
 *         materialization fails.
 */
static bool
constant_integer_big(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_constant_t* constant,
  const cardano_bigint_t**       out)
{
  if ((constant == NULL) || (constant->kind != CARDANO_UPLC_TYPE_INTEGER))
  {
    return false;
  }

  return cardano_uplc_constant_int_materialize(arena, constant, out) == CARDANO_SUCCESS;
}

/**
 * \brief Returns a never-NULL byte pointer for a possibly-empty span.
 *
 * The arena byte-string representation uses a NULL pointer for the empty string, but
 * some vendored crypto routines (e.g. \c ripemd160) assert a non-NULL input pointer
 * even for a zero length. This yields a stable dummy address in that case so the
 * one-shot hashers and verifiers can be fed \c (ptr, 0) safely.
 *
 * \param[in] data The span pointer, possibly NULL when \p size is 0.
 * \param[in] size The span length.
 *
 * \return \p data when non-NULL, otherwise a valid non-dereferenced dummy pointer.
 */
static const byte_t*
nonnull_bytes(const byte_t* data, size_t size)
{
  static const byte_t empty = 0;

  (void)size;

  return (data != NULL) ? data : &empty;
}

/**
 * \brief Compares two raw byte spans lexicographically.
 *
 * Compares the common prefix byte by byte and breaks a tie by length, the ordering
 * \c equalsByteString, \c lessThanByteString and the value-map key checks rely on. A
 * NULL span is treated as empty, valid only when its length is 0.
 *
 * \param[in] da The first span, or NULL when \p la is 0.
 * \param[in] la The first span length.
 * \param[in] db The second span, or NULL when \p lb is 0.
 * \param[in] lb The second span length.
 *
 * \return A negative, zero or positive value as the first span is less than, equal
 *         to, or greater than the second.
 */
static int
byte_view_compare(const byte_t* da, size_t la, const byte_t* db, size_t lb)
{
  size_t n = (la < lb) ? la : lb;
  size_t i = 0U;

  for (i = 0U; i < n; ++i)
  {
    if (da[i] != db[i])
    {
      return (da[i] < db[i]) ? -1 : 1;
    }
  }

  if (la == lb)
  {
    return 0;
  }

  return (la < lb) ? -1 : 1;
}

/**
 * \brief Builds a byte-string result value by copying a raw byte span into the arena.
 *
 * The single byte-string producer for every byte-string builtin: it bump-allocates a
 * copy of \p data in the arena, wraps it in a constant and a value, and never
 * mallocs or refcounts a library buffer. An empty span yields the empty byte string.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] data The bytes to copy, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_bytes_from(
  struct cardano_uplc_arena_t* arena,
  const byte_t*                data,
  size_t                       size,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = cardano_uplc_int_constant_new_byte_string_copy(arena, data, size, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Wraps an already-arena byte span as a byte-string result with no copy.
 *
 * The zero-copy counterpart of \ref result_bytes_from for bodies that have
 * just filled a span allocated from the arena (a bitwise or encoding scratch): the
 * constant adopts the span directly. \p data must live in \p arena.
 *
 * \param[in] arena The arena the result is allocated from and that owns \p data.
 * \param[in] data The arena bytes to adopt, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_bytes_take(
  struct cardano_uplc_arena_t* arena,
  const byte_t*                data,
  size_t                       size,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = cardano_uplc_int_constant_new_byte_string_take(arena, data, size, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Builds a boolean result value.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] value The boolean value.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_bool(
  struct cardano_uplc_arena_t* arena,
  bool                         value,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = cardano_uplc_constant_new_bool(arena, value, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Computes the floor quotient and remainder of two bigints.
 *
 * The library bigint primitives round toward zero (truncated division), so floor
 * division is derived: when the truncated remainder is non-zero and its sign
 * differs from the divisor's, the quotient is decremented and the divisor added to
 * the remainder. This is the rounding \c divideInteger and \c modInteger use. The
 * caller must have ruled out a zero divisor.
 *
 * \param[in] dividend The dividend.
 * \param[in] divisor The non-zero divisor.
 * \param[out] quotient Pre-initialised bigint receiving the floor quotient.
 * \param[out] remainder Pre-initialised bigint receiving the floor remainder.
 */
static void
floor_div_mod(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       quotient,
  cardano_bigint_t*       remainder)
{
  cardano_bigint_divide_and_reminder(dividend, divisor, quotient, remainder);

  if (cardano_bigint_signum(remainder) != 0)
  {
    if (cardano_bigint_signum(remainder) != cardano_bigint_signum(divisor))
    {
      cardano_bigint_t* one = NULL;

      if (cardano_bigint_from_int(1, &one) == CARDANO_SUCCESS)
      {
        cardano_bigint_subtract(quotient, one, quotient);
        cardano_bigint_add(remainder, divisor, remainder);
        cardano_bigint_unref(&one);
      }
    }
  }
}

/**
 * \brief Validates that a byte range is well-formed UTF-8.
 *
 * Applies the Unicode well-formed-UTF-8 rules so \c decodeUtf8 fails the script
 * on invalid input. Lead bytes 0xC2..0xDF, 0xE0..0xEF and 0xF0..0xF4 introduce
 * 2-, 3- and 4-byte sequences whose continuation bytes are 0x80..0xBF, with the
 * ranges narrowed to reject overlong encodings, UTF-16 surrogates and code points
 * above U+10FFFF.
 *
 * \param[in] data The bytes to validate. May be NULL only when \p size is 0.
 * \param[in] size The number of bytes.
 *
 * \return \c true if the range is valid UTF-8, \c false otherwise.
 */
static bool
is_valid_utf8(const byte_t* data, size_t size)
{
  size_t i = 0U;

  while (i < size)
  {
    const byte_t b0 = data[i];

    if (b0 <= 0x7FU)
    {
      i += 1U;
    }
    else if ((b0 >= 0xC2U) && (b0 <= 0xDFU))
    {
      if ((i + 1U) >= size)
      {
        return false;
      }

      if ((data[i + 1U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      i += 2U;
    }
    else if ((b0 >= 0xE0U) && (b0 <= 0xEFU))
    {
      byte_t lo = 0x80U;
      byte_t hi = 0xBFU;

      if ((i + 2U) >= size)
      {
        return false;
      }

      if (b0 == 0xE0U)
      {
        lo = 0xA0U;
      }
      else if (b0 == 0xEDU)
      {
        hi = 0x9FU;
      }
      else
      {
        lo = 0x80U;
        hi = 0xBFU;
      }

      if ((data[i + 1U] < lo) || (data[i + 1U] > hi))
      {
        return false;
      }

      if ((data[i + 2U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      i += 3U;
    }
    else if ((b0 >= 0xF0U) && (b0 <= 0xF4U))
    {
      byte_t lo = 0x80U;
      byte_t hi = 0xBFU;

      if ((i + 3U) >= size)
      {
        return false;
      }

      if (b0 == 0xF0U)
      {
        lo = 0x90U;
      }
      else if (b0 == 0xF4U)
      {
        hi = 0x8FU;
      }
      else
      {
        lo = 0x80U;
        hi = 0xBFU;
      }

      if ((data[i + 1U] < lo) || (data[i + 1U] > hi))
      {
        return false;
      }

      if ((data[i + 2U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      if ((data[i + 3U] & 0xC0U) != 0x80U)
      {
        return false;
      }

      i += 4U;
    }
    else
    {
      return false;
    }
  }

  return true;
}

/**
 * \brief Tests two type descriptors for structural equality.
 *
 * Compares the kinds, and for a list the element type and for a pair both
 * component types, recursively. Used to compare a list head that is itself a
 * list against the declared element type.
 *
 * \param[in] a The first type descriptor. Must not be NULL.
 * \param[in] b The second type descriptor. Must not be NULL.
 *
 * \return \c true when the descriptors are structurally equal, \c false otherwise.
 */
static bool
type_equals(const cardano_uplc_type_t* a, const cardano_uplc_type_t* b)
{
  if ((a == NULL) || (b == NULL))
  {
    return false;
  }

  if (a->kind != b->kind)
  {
    return false;
  }

  switch (a->kind)
  {
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      return type_equals(a->fst, b->fst);
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      if (!type_equals(a->fst, b->fst))
      {
        return false;
      }

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      return type_equals(a->snd, b->snd);
    }
    case CARDANO_UPLC_TYPE_INTEGER:
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    case CARDANO_UPLC_TYPE_STRING:
    case CARDANO_UPLC_TYPE_UNIT:
    case CARDANO_UPLC_TYPE_BOOL:
    case CARDANO_UPLC_TYPE_DATA:
    case CARDANO_UPLC_TYPE_VALUE:
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    default:
    {
      return true;
    }
  }
}

/**
 * \brief Tests whether a constant's type matches a type descriptor.
 *
 * The element-type guard \c mkCons enforces: a list of element type \c T accepts
 * a head only when the head's type equals \c T, where a list head's type is the
 * element type it carries and a pair head's type is the pair of its component
 * types, compared recursively.
 *
 * \param[in] type The element type descriptor the list declares. Must not be NULL.
 * \param[in] constant The candidate head constant. Must not be NULL.
 *
 * \return \c true when the constant's type matches \p type, \c false otherwise.
 */
static bool
constant_has_type(const cardano_uplc_type_t* type, const cardano_uplc_constant_t* constant)
{
  if ((type == NULL) || (constant == NULL))
  {
    return false;
  }

  if (type->kind != constant->kind)
  {
    return false;
  }

  switch (constant->kind)
  {
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      const cardano_uplc_type_t* inner = type->fst;
      const cardano_uplc_type_t* item  = constant->as.list.element_type;

      if ((inner == NULL) || (item == NULL))
      {
        return false;
      }

      return type_equals(inner, item);
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      if ((type->fst == NULL) || (type->snd == NULL))
      {
        return false;
      }

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      if (!constant_has_type(type->fst, constant->as.pair.fst))
      {
        return false;
      }

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      return constant_has_type(type->snd, constant->as.pair.snd);
    }
    case CARDANO_UPLC_TYPE_INTEGER:
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    case CARDANO_UPLC_TYPE_STRING:
    case CARDANO_UPLC_TYPE_UNIT:
    case CARDANO_UPLC_TYPE_BOOL:
    case CARDANO_UPLC_TYPE_DATA:
    case CARDANO_UPLC_TYPE_VALUE:
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    default:
    {
      return true;
    }
  }
}

/* DEFINITIONS ***************************************************************/

bool
cardano_uplc_builtin_as_integer(const cardano_uplc_value_t* value, const cardano_bigint_t** out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (out == NULL)
  {
    return false;
  }

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_INTEGER)
  {
    return false;
  }

  if (constant->as.integer.big == NULL)
  {
    return false;
  }

  *out = constant->as.integer.big;

  return true;
}

bool
cardano_uplc_builtin_as_byte_string(const cardano_uplc_value_t* value, cardano_uplc_byte_view_t* out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (out == NULL)
  {
    return false;
  }

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_BYTE_STRING)
  {
    return false;
  }

  out->data = constant->as.bytes.data;
  out->size = constant->as.bytes.size;

  return true;
}

bool
cardano_uplc_builtin_as_string(const cardano_uplc_value_t* value, cardano_uplc_byte_view_t* out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (out == NULL)
  {
    return false;
  }

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_STRING)
  {
    return false;
  }

  out->data = constant->as.string.data;
  out->size = constant->as.string.size;

  return true;
}

bool
cardano_uplc_builtin_as_bool(const cardano_uplc_value_t* value, bool* out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (out == NULL)
  {
    return false;
  }

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_BOOL)
  {
    return false;
  }

  *out = constant->as.boolean;

  return true;
}

bool
cardano_uplc_builtin_as_unit(const cardano_uplc_value_t* value)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  return (constant->kind == CARDANO_UPLC_TYPE_UNIT);
}

bool
cardano_uplc_builtin_as_data(const cardano_uplc_value_t* value, const cardano_uplc_data_t** out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (out == NULL)
  {
    return false;
  }

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_DATA)
  {
    return false;
  }

  *out = constant->as.data;

  return true;
}

bool
cardano_uplc_builtin_as_list(
  const cardano_uplc_value_t* value,
  const cardano_uplc_type_t** element_type,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_constant_t* const** items,
  size_t*                                count)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_LIST)
  {
    return false;
  }

  if (element_type != NULL)
  {
    *element_type = constant->as.list.element_type;
  }

  if (items != NULL)
  {
    *items = constant->as.list.items;
  }

  if (count != NULL)
  {
    *count = constant->as.list.count;
  }

  return true;
}

bool
cardano_uplc_builtin_as_pair(
  const cardano_uplc_value_t*     value,
  const cardano_uplc_constant_t** fst,
  const cardano_uplc_constant_t** snd)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_PAIR)
  {
    return false;
  }

  if (fst != NULL)
  {
    *fst = constant->as.pair.fst;
  }

  if (snd != NULL)
  {
    *snd = constant->as.pair.snd;
  }

  return true;
}

/* BUILTIN BODIES **********************************************************/

/**
 * \brief Runs the binary integer-arithmetic builtins.
 *
 * Covers \c addInteger, \c subtractInteger and \c multiplyInteger. Each unwraps
 * two integer arguments (a non-integer is a script error) and publishes the
 * integer result, using an inline \c int64_t fast path with overflow detection
 * and falling back to bigint arithmetic.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] func The arithmetic builtin to run.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_int_arith(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_t             func,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  int_view_t              a      = { false, 0, NULL };
  int_view_t              b      = { false, 0, NULL };
  const cardano_bigint_t* lhs    = NULL;
  const cardano_bigint_t* rhs    = NULL;
  cardano_bigint_t*       result = NULL;

  if (!as_int_view(args[0], &a) || !as_int_view(args[1], &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (a.is_small && b.is_small)
  {
    int64_t out      = 0;
    bool    overflow = false;

    if (func == CARDANO_UPLC_BUILTIN_ADD_INTEGER)
    {
      overflow = __builtin_add_overflow(a.small, b.small, &out);
    }
    else if (func == CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER)
    {
      overflow = __builtin_sub_overflow(a.small, b.small, &out);
    }
    else
    {
      overflow = __builtin_mul_overflow(a.small, b.small, &out);
    }

    if (!overflow)
    {
      *host_error = result_integer_from_int(arena, out, out_result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  if (!constant_integer_big(arena, a.constant, &lhs) || !constant_integer_big(arena, b.constant, &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_bigint_from_int(0, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (func == CARDANO_UPLC_BUILTIN_ADD_INTEGER)
  {
    cardano_bigint_add(lhs, rhs, result);
  }
  else if (func == CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER)
  {
    cardano_bigint_subtract(lhs, rhs, result);
  }
  else
  {
    cardano_bigint_multiply(lhs, rhs, result);
  }

  *host_error = result_integer(arena, result, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs the integer division-family builtins.
 *
 * Covers \c divideInteger and \c modInteger (floor rounding, toward negative
 * infinity) and \c quotientInteger and \c remainderInteger (truncated rounding,
 * toward zero). A zero divisor is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] func The division builtin to run.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_int_div(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_t             func,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  int_view_t              a         = { false, 0, NULL };
  int_view_t              b         = { false, 0, NULL };
  const cardano_bigint_t* lhs       = NULL;
  const cardano_bigint_t* rhs       = NULL;
  cardano_bigint_t*       quotient  = NULL;
  cardano_bigint_t*       remainder = NULL;
  bool                    want_quot = (func == CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER) || (func == CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER);
  bool                    is_floor  = (func == CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER) || (func == CARDANO_UPLC_BUILTIN_MOD_INTEGER);

  if (!as_int_view(args[0], &a) || !as_int_view(args[1], &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (a.is_small && b.is_small)
  {
    if (b.small == 0)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (!((a.small == (int64_t)INT64_MIN) && (b.small == -1)))
    {
      int64_t q = a.small / b.small;
      int64_t r = a.small % b.small;

      if (is_floor && (r != 0) && (((r < 0) ? -1 : 1) != ((b.small < 0) ? -1 : 1)))
      {
        q -= 1;
        r += b.small;
      }

      *host_error = result_integer_from_int(arena, want_quot ? q : r, out_result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  if (!constant_integer_big(arena, a.constant, &lhs) || !constant_integer_big(arena, b.constant, &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_signum(rhs) == 0)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_bigint_from_int(0, &quotient);

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_bigint_from_int(0, &remainder);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&quotient);
    cardano_bigint_unref(&remainder);

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (is_floor)
  {
    floor_div_mod(lhs, rhs, quotient, remainder);
  }
  else
  {
    cardano_bigint_divide_and_reminder(lhs, rhs, quotient, remainder);
  }

  if (want_quot)
  {
    cardano_bigint_unref(&remainder);
    *host_error = result_integer(arena, quotient, out_result);
  }
  else
  {
    cardano_bigint_unref(&quotient);
    *host_error = result_integer(arena, remainder, out_result);
  }

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs the integer comparison builtins.
 *
 * Covers \c equalsInteger, \c lessThanInteger and \c lessThanEqualsInteger,
 * each producing a boolean from a signed bigint comparison.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] func The comparison builtin to run.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_int_compare(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_t             func,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  int_view_t              a     = { false, 0, NULL };
  int_view_t              b     = { false, 0, NULL };
  const cardano_bigint_t* lhs   = NULL;
  const cardano_bigint_t* rhs   = NULL;
  int32_t                 cmp   = 0;
  bool                    value = false;

  if (!as_int_view(args[0], &a) || !as_int_view(args[1], &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (a.is_small && b.is_small)
  {
    cmp = (a.small < b.small) ? (int32_t)-1 : ((a.small > b.small) ? (int32_t)1 : (int32_t)0);
  }
  else
  {
    if (!constant_integer_big(arena, a.constant, &lhs) || !constant_integer_big(arena, b.constant, &rhs))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cmp = cardano_bigint_compare(lhs, rhs);
  }

  if (func == CARDANO_UPLC_BUILTIN_EQUALS_INTEGER)
  {
    value = (cmp == 0);
  }
  else if (func == CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER)
  {
    value = (cmp < 0);
  }
  else
  {
    value = (cmp <= 0);
  }

  *host_error = result_bool(arena, value, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c expModInteger: base raised to an exponent modulo a modulus.
 *
 * A modulus of zero or less is a script error. A modulus of one yields zero. A
 * zero exponent yields one. A positive exponent uses modular exponentiation. A
 * negative exponent inverts the base modulo the modulus (a script error when the
 * base is not invertible, or when the base is zero) then exponentiates by the
 * magnitude.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (base, exponent, modulus).
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_exp_mod_integer(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t* base     = NULL;
  const cardano_bigint_t* exponent = NULL;
  const cardano_bigint_t* modulus  = NULL;
  cardano_bigint_t*       result   = NULL;

  if (!as_integer_big(arena, args[0], &base) || !as_integer_big(arena, args[1], &exponent) || !as_integer_big(arena, args[2], &modulus))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_signum(modulus) <= 0)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_bigint_from_int(0, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  {
    cardano_bigint_t* one = NULL;

    *host_error = cardano_bigint_from_int(1, &one);

    if (*host_error != CARDANO_SUCCESS)
    {
      cardano_bigint_unref(&result);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_bigint_compare(modulus, one) == 0)
    {
      cardano_bigint_unref(&one);

      *host_error = result_integer_from_int(arena, 0, out_result);
      cardano_bigint_unref(&result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_bigint_signum(exponent) == 0)
    {
      cardano_bigint_unref(&one);

      *host_error = result_integer_from_int(arena, 1, out_result);
      cardano_bigint_unref(&result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_bigint_signum(exponent) > 0)
    {
      cardano_bigint_unref(&one);

      cardano_bigint_mod_pow(base, exponent, modulus, result);

      *host_error = result_integer(arena, result, out_result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_bigint_signum(base) == 0)
    {
      cardano_bigint_unref(&one);
      cardano_bigint_unref(&result);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    {
      cardano_bigint_t* reduced = NULL;
      cardano_bigint_t* gcd     = NULL;
      cardano_bigint_t* inverse = NULL;
      cardano_bigint_t* abs_exp = NULL;

      *host_error = cardano_bigint_from_int(0, &reduced);

      if (*host_error == CARDANO_SUCCESS)
      {
        *host_error = cardano_bigint_from_int(0, &gcd);
      }

      if (*host_error == CARDANO_SUCCESS)
      {
        *host_error = cardano_bigint_from_int(0, &inverse);
      }

      if (*host_error == CARDANO_SUCCESS)
      {
        *host_error = cardano_bigint_from_int(0, &abs_exp);
      }

      if (*host_error != CARDANO_SUCCESS)
      {
        cardano_bigint_unref(&reduced);
        cardano_bigint_unref(&gcd);
        cardano_bigint_unref(&inverse);
        cardano_bigint_unref(&abs_exp);
        cardano_bigint_unref(&one);
        cardano_bigint_unref(&result);

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      floor_div_mod(base, modulus, inverse, reduced);
      cardano_bigint_gcd(reduced, modulus, gcd);

      if (cardano_bigint_compare(gcd, one) != 0)
      {
        cardano_bigint_unref(&reduced);
        cardano_bigint_unref(&gcd);
        cardano_bigint_unref(&inverse);
        cardano_bigint_unref(&abs_exp);
        cardano_bigint_unref(&one);
        cardano_bigint_unref(&result);

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      cardano_bigint_mod_inverse(reduced, modulus, inverse);
      cardano_bigint_abs(exponent, abs_exp);
      cardano_bigint_mod_pow(inverse, abs_exp, modulus, result);

      cardano_bigint_unref(&reduced);
      cardano_bigint_unref(&gcd);
      cardano_bigint_unref(&inverse);
      cardano_bigint_unref(&abs_exp);
      cardano_bigint_unref(&one);

      *host_error = result_integer(arena, result, out_result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }
}

/**
 * \brief Runs \c appendByteString: the concatenation of two byte strings.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_append_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t lhs     = { NULL, 0U };
  cardano_uplc_byte_view_t rhs     = { NULL, 0U };
  size_t                   total   = 0U;
  byte_t*                  scratch = NULL;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &lhs) || !cardano_uplc_builtin_as_byte_string(args[1], &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  total = lhs.size + rhs.size;

  if (total == 0U)
  {
    *host_error = result_bytes_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, total, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (lhs.size > 0U)
  {
    (void)memcpy(scratch, lhs.data, lhs.size);
  }

  if (rhs.size > 0U)
  {
    // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
    (void)memcpy(scratch + lhs.size, rhs.data, rhs.size);
  }

  *host_error = result_bytes_take(arena, scratch, total, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c consByteString: prepends a byte to a byte string.
 *
 * With range checks on, a leading integer outside 0..255 is a script error; with
 * range checks off the byte wraps modulo 256. The semantics variant selects which
 * behaviour applies.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] semantics The semantics variant selecting the range-check behaviour.
 * \param[in] args The two saturated argument values (byte, byte string).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_cons_byte_string(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_semantics_t   semantics,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t*  head    = NULL;
  cardano_uplc_byte_view_t tail    = { NULL, 0U };
  byte_t                   byte    = 0;
  byte_t*                  scratch = NULL;

  if (!as_integer_big(arena, args[0], &head) || !cardano_uplc_builtin_as_byte_string(args[1], &tail))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_uplc_semantics_cons_byte_string_range_checks(semantics))
  {
    cardano_bigint_t* upper = NULL;

    if (cardano_bigint_signum(head) < 0)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *host_error = cardano_bigint_from_int(CARDANO_UPLC_BUILTIN_BYTE_MAX, &upper);

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_bigint_compare(head, upper) > 0)
    {
      cardano_bigint_unref(&upper);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cardano_bigint_unref(&upper);

    byte = (byte_t)(cardano_bigint_to_unsigned_int(head) & 0xFFU);
  }
  else
  {
    cardano_bigint_t* modulus = NULL;
    cardano_bigint_t* wrapped = NULL;
    cardano_bigint_t* discard = NULL;

    *host_error = cardano_bigint_from_int(CARDANO_UPLC_BUILTIN_BYTE_MODULUS, &modulus);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_bigint_from_int(0, &wrapped);
    }

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_bigint_from_int(0, &discard);
    }

    if (*host_error != CARDANO_SUCCESS)
    {
      cardano_bigint_unref(&modulus);
      cardano_bigint_unref(&wrapped);
      cardano_bigint_unref(&discard);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    floor_div_mod(head, modulus, discard, wrapped);

    byte = (byte_t)(cardano_bigint_to_unsigned_int(wrapped) & 0xFFU);

    cardano_bigint_unref(&modulus);
    cardano_bigint_unref(&wrapped);
    cardano_bigint_unref(&discard);
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, tail.size + 1U, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch[0] = byte;

  if (tail.size > 0U)
  {
    // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
    (void)memcpy(scratch + 1U, tail.data, tail.size);
  }

  *host_error = result_bytes_take(arena, scratch, tail.size + 1U, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Converts a non-negative bigint to a clamped \c size_t skip/take count.
 *
 * A negative value clamps to zero (so a negative skip or take selects nothing); a
 * value above the byte-string length clamps to the length.
 *
 * \param[in] value The bigint to clamp.
 * \param[in] limit The upper bound (the byte-string length).
 *
 * \return The clamped count in 0..limit.
 */
static size_t
clamp_index(const cardano_bigint_t* value, size_t limit)
{
  uint64_t raw = 0U;

  if (cardano_bigint_signum(value) < 0)
  {
    return 0U;
  }

  if (cardano_bigint_bit_length(value) > 63U)
  {
    return limit;
  }

  raw = cardano_bigint_to_unsigned_int(value);

  if (raw > (uint64_t)limit)
  {
    return limit;
  }

  return (size_t)raw;
}

/**
 * \brief Runs \c sliceByteString: a clamped sub-string by start and length.
 *
 * The start and length are clamped to the byte-string bounds, so an out-of-range
 * request yields a shorter or empty result rather than an error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (start, length, byte string).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_slice_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t*  start_arg = NULL;
  const cardano_bigint_t*  len_arg   = NULL;
  cardano_uplc_byte_view_t bytes     = { NULL, 0U };
  size_t                   skip      = 0U;
  size_t                   take      = 0U;
  size_t                   available = 0U;

  if (!as_integer_big(arena, args[0], &start_arg) || !as_integer_big(arena, args[1], &len_arg) || !cardano_uplc_builtin_as_byte_string(args[2], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  skip = clamp_index(start_arg, bytes.size);
  take = clamp_index(len_arg, bytes.size);

  available = bytes.size - skip;

  if (take > available)
  {
    take = available;
  }

  // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
  *host_error = result_bytes_from(arena, (bytes.data != NULL) ? (bytes.data + skip) : NULL, take, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c lengthOfByteString: the length of a byte string as an integer.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_length_of_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes = { NULL, 0U };

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_integer_from_int(arena, (int64_t)bytes.size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c indexByteString: the byte at an index, as an integer.
 *
 * An index below zero or at or beyond the length is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (byte string, index).
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_index_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes = { NULL, 0U };
  const cardano_bigint_t*  index = NULL;
  uint64_t                 raw   = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes) || !as_integer_big(arena, args[1], &index))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((cardano_bigint_signum(index) < 0) || (cardano_bigint_bit_length(index) > 63U))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  raw = cardano_bigint_to_unsigned_int(index);

  if (raw >= (uint64_t)bytes.size)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_integer_from_int(arena, (int64_t)bytes.data[raw], out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs the byte-string comparison builtins.
 *
 * Covers \c equalsByteString, \c lessThanByteString and
 * \c lessThanEqualsByteString, each producing a boolean from a lexicographic
 * comparison of the two byte strings.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] func The comparison builtin to run.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_byte_string_compare(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_t             func,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t lhs   = { NULL, 0U };
  cardano_uplc_byte_view_t rhs   = { NULL, 0U };
  int                      cmp   = 0;
  bool                     value = false;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &lhs) || !cardano_uplc_builtin_as_byte_string(args[1], &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  cmp = byte_view_compare(lhs.data, lhs.size, rhs.data, rhs.size);

  if (func == CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING)
  {
    value = (cmp == 0);
  }
  else if (func == CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING)
  {
    value = (cmp < 0);
  }
  else
  {
    value = (cmp <= 0);
  }

  *host_error = result_bool(arena, value, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/* BITWISE BODIES **********************************************************/

/**
 * \brief Runs the logical binary bitwise builtins over two byte strings.
 *
 * Covers \c andByteString, \c orByteString and \c xorByteString. The leading
 * boolean selects padding: when set, the shorter operand is virtually extended so
 * the result has the length of the longer operand, the surplus bytes carried
 * through unchanged (which matches AND-with-0xFF, OR-with-0x00 and XOR-with-0x00);
 * when clear, the result length is the shorter operand.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] func The logical builtin to run.
 * \param[in] args The three saturated argument values (pad flag, two byte strings).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_logical_byte_string(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_t             func,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  bool                     should_pad = false;
  cardano_uplc_byte_view_t lhs        = { NULL, 0U };
  cardano_uplc_byte_view_t rhs        = { NULL, 0U };
  const byte_t*            a          = NULL;
  const byte_t*            b          = NULL;
  size_t                   la         = 0U;
  size_t                   lb         = 0U;
  size_t                   shorter    = 0U;
  size_t                   out_len    = 0U;
  byte_t*                  scratch    = NULL;
  size_t                   i          = 0U;

  if (!cardano_uplc_builtin_as_bool(args[0], &should_pad) || !cardano_uplc_builtin_as_byte_string(args[1], &lhs) || !cardano_uplc_builtin_as_byte_string(args[2], &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  a  = lhs.data;
  b  = rhs.data;
  la = lhs.size;
  lb = rhs.size;

  shorter = (la < lb) ? la : lb;
  out_len = should_pad ? ((la > lb) ? la : lb) : shorter;

  if (out_len == 0U)
  {
    *host_error = result_bytes_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, out_len, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; i < shorter; ++i)
  {
    if (func == CARDANO_UPLC_BUILTIN_AND_BYTE_STRING)
    {
      scratch[i] = (byte_t)(a[i] & b[i]);
    }
    else if (func == CARDANO_UPLC_BUILTIN_OR_BYTE_STRING)
    {
      scratch[i] = (byte_t)(a[i] | b[i]);
    }
    else
    {
      scratch[i] = (byte_t)(a[i] ^ b[i]);
    }
  }

  for (i = shorter; i < out_len; ++i)
  {
    scratch[i] = (i < la) ? a[i] : b[i];
  }

  *host_error = result_bytes_take(arena, scratch, out_len, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c complementByteString: every byte XOR-ed with 0xFF.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_complement_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes   = { NULL, 0U };
  byte_t*                  scratch = NULL;
  size_t                   i       = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (bytes.size == 0U)
  {
    *host_error = result_bytes_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, bytes.size, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; i < bytes.size; ++i)
  {
    scratch[i] = (byte_t)(bytes.data[i] ^ (byte_t)CARDANO_UPLC_BUILTIN_BYTE_MAX);
  }

  *host_error = result_bytes_take(arena, scratch, bytes.size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Resolves a CIP-0122 bit index to a byte offset and intra-byte bit.
 *
 * The bit index is counted from the least-significant bit of the last byte, so
 * byte offset \c index/8 measures from the end of the string and the intra-byte
 * offset \c index%8 counts from the low bit. The caller must have checked the
 * index is in 0..len*8.
 *
 * \param[in] bit_index The non-negative bit index, known to be in range.
 * \param[in] length The byte-string length.
 * \param[out] byte_pos The resolved index into the byte array (from the front).
 * \param[out] bit_offset The intra-byte bit offset from the low bit (0..7).
 */
static void
resolve_bit(uint64_t bit_index, size_t length, size_t* byte_pos, unsigned int* bit_offset)
{
  uint64_t from_end = bit_index / 8U;

  *bit_offset = (unsigned int)(bit_index % 8U);
  *byte_pos   = (length - 1U) - (size_t)from_end;
}

/**
 * \brief Runs \c readBit: reads one bit of a byte string as a boolean.
 *
 * An empty byte string, or a bit index outside 0..len*8, is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (byte string, bit index).
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_read_bit(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes      = { NULL, 0U };
  const cardano_bigint_t*  bit_index  = NULL;
  uint64_t                 index      = 0U;
  size_t                   byte_pos   = 0U;
  unsigned int             bit_offset = 0U;
  bool                     value      = false;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes) || !as_integer_big(arena, args[1], &bit_index))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (bytes.size == 0U)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((cardano_bigint_signum(bit_index) < 0) || (cardano_bigint_bit_length(bit_index) > 63U))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  index = cardano_bigint_to_unsigned_int(bit_index);

  if (index >= (uint64_t)bytes.size * 8U)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  resolve_bit(index, bytes.size, &byte_pos, &bit_offset);

  value = (((bytes.data[byte_pos] >> bit_offset) & 1U) == 1U);

  *host_error = result_bool(arena, value, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c writeBits: sets or clears each listed bit of a byte string.
 *
 * The first argument is the byte string, the second a list of integer bit
 * indices, the third the boolean value to write. Any index outside 0..len*8 is a
 * script error and no result is produced.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (byte string, index list, bit).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_write_bits(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t              bytes   = { NULL, 0U };
  const cardano_uplc_type_t*            element = NULL;
  const cardano_uplc_constant_t* const* items   = NULL;
  size_t                                count   = 0U;
  bool                                  set_bit = false;
  byte_t*                               scratch = NULL;
  size_t                                i       = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes) || !cardano_uplc_builtin_as_list(args[1], &element, &items, &count) || !cardano_uplc_builtin_as_bool(args[2], &set_bit))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (bytes.size > 0U)
  {
    scratch = (byte_t*)cardano_uplc_arena_alloc(arena, bytes.size, 1U);

    if (scratch == NULL)
    {
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    (void)memcpy(scratch, bytes.data, bytes.size);
  }

  for (i = 0U; i < count; ++i)
  {
    const cardano_uplc_constant_t* item       = items[i];
    uint64_t                       index      = 0U;
    size_t                         byte_pos   = 0U;
    unsigned int                   bit_offset = 0U;
    byte_t                         mask       = 0;

    if ((item == NULL) || (item->kind != CARDANO_UPLC_TYPE_INTEGER))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_uplc_constant_int_is_small(item))
    {
      const int64_t small = cardano_uplc_constant_int_small(item);

      if (small < 0)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      index = (uint64_t)small;
    }
    else
    {
      const cardano_bigint_t* big = item->as.integer.big;

      if ((cardano_bigint_signum(big) < 0) || (cardano_bigint_bit_length(big) > 63U))
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      index = cardano_bigint_to_unsigned_int(big);
    }

    if (index >= (uint64_t)bytes.size * 8U)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    resolve_bit(index, bytes.size, &byte_pos, &bit_offset);

    mask = (byte_t)(1U << bit_offset);

    if (set_bit)
    {
      scratch[byte_pos] = (byte_t)(scratch[byte_pos] | mask);
    }
    else
    {
      scratch[byte_pos] = (byte_t)(scratch[byte_pos] & (byte_t)(~mask));
    }
  }

  *host_error = result_bytes_take(arena, scratch, bytes.size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c replicateByte: a byte string of one repeated byte.
 *
 * The first argument is the count (non-negative, already bounded by the cost
 * model), the second the byte value (a value outside 0..255 is a script error).
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (count, byte).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_replicate_byte(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t* size_arg = NULL;
  const cardano_bigint_t* byte_arg = NULL;
  uint64_t                size     = 0U;
  byte_t                  byte     = 0;
  byte_t*                 scratch  = NULL;

  if (!as_integer_big(arena, args[0], &size_arg) || !as_integer_big(arena, args[1], &byte_arg))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((cardano_bigint_signum(size_arg) < 0) || (cardano_bigint_bit_length(size_arg) > 63U))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_to_unsigned_int(size_arg) > (uint64_t)CARDANO_UPLC_BUILTIN_INT_TO_BS_MAX)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  {
    cardano_bigint_t* upper = NULL;

    if (cardano_bigint_signum(byte_arg) < 0)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *host_error = cardano_bigint_from_int(CARDANO_UPLC_BUILTIN_BYTE_MAX, &upper);

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_bigint_compare(byte_arg, upper) > 0)
    {
      cardano_bigint_unref(&upper);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cardano_bigint_unref(&upper);
  }

  size = cardano_bigint_to_unsigned_int(size_arg);
  byte = (byte_t)(cardano_bigint_to_unsigned_int(byte_arg) & 0xFFU);

  if (size == 0U)
  {
    *host_error = result_bytes_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, (size_t)size, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)memset(scratch, byte, (size_t)size);

  *host_error = result_bytes_take(arena, scratch, (size_t)size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c shiftByteString: a logical bit shift, MSB-first, zero-filled.
 *
 * The shift count is signed: a positive count shifts left (toward the most
 * significant bit), a negative count shifts right; the result keeps the input
 * length and vacated bits are zero. A magnitude at or beyond the bit length
 * yields an all-zero string. Bits are addressed most-significant-first.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (byte string, shift).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_shift_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes     = { NULL, 0U };
  const cardano_bigint_t*  shift     = NULL;
  const byte_t*            data      = NULL;
  size_t                   size      = 0U;
  uint64_t                 bits      = 0U;
  uint64_t                 magnitude = 0U;
  bool                     is_left   = false;
  byte_t*                  scratch   = NULL;
  uint64_t                 dst_bit   = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes) || !as_integer_big(arena, args[1], &shift))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  data = bytes.data;
  size = bytes.size;
  bits = (uint64_t)size * 8U;

  is_left = (cardano_bigint_signum(shift) >= 0);

  if (cardano_bigint_bit_length(shift) > 63U)
  {
    magnitude = bits;
  }
  else
  {
    cardano_bigint_t* abs_shift = NULL;

    *host_error = cardano_bigint_from_int(0, &abs_shift);

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cardano_bigint_abs(shift, abs_shift);
    magnitude = cardano_bigint_to_unsigned_int(abs_shift);
    cardano_bigint_unref(&abs_shift);
  }

  if (size == 0U)
  {
    *host_error = result_bytes_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, size, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)memset(scratch, 0, size);

  for (dst_bit = 0U; dst_bit < bits; ++dst_bit)
  {
    uint64_t src_bit  = 0U;
    bool     in_range = false;

    if (is_left)
    {
      src_bit  = dst_bit + magnitude;
      in_range = (src_bit < bits);
    }
    else
    {
      if (dst_bit >= magnitude)
      {
        src_bit  = dst_bit - magnitude;
        in_range = true;
      }
    }

    if (in_range)
    {
      size_t       src_byte = (size_t)(src_bit / 8U);
      unsigned int src_off  = (unsigned int)(7U - (src_bit % 8U));
      size_t       dst_byte = (size_t)(dst_bit / 8U);
      unsigned int dst_off  = (unsigned int)(7U - (dst_bit % 8U));

      if (((data[src_byte] >> src_off) & 1U) == 1U)
      {
        scratch[dst_byte] = (byte_t)(scratch[dst_byte] | (byte_t)(1U << dst_off));
      }
    }
  }

  *host_error = result_bytes_take(arena, scratch, size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c rotateByteString: a circular bit rotation, MSB-first.
 *
 * The bit rotation count is reduced modulo the bit length (floor modulo, so a
 * negative count rotates right). Bits shifted off one end re-enter at the other;
 * the result keeps the input length. An empty input is returned unchanged.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (byte string, rotation).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_rotate_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes   = { NULL, 0U };
  const cardano_bigint_t*  shift   = NULL;
  const byte_t*            data    = NULL;
  size_t                   size    = 0U;
  uint64_t                 bits    = 0U;
  uint64_t                 rot     = 0U;
  byte_t*                  scratch = NULL;
  uint64_t                 dst_bit = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes) || !as_integer_big(arena, args[1], &shift))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  data = bytes.data;
  size = bytes.size;

  if (size == 0U)
  {
    *host_error = result_bytes_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  bits = (uint64_t)size * 8U;

  {
    cardano_bigint_t* modulus  = NULL;
    cardano_bigint_t* reduced  = NULL;
    cardano_bigint_t* quotient = NULL;

    *host_error = cardano_bigint_from_unsigned_int(bits, &modulus);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_bigint_from_int(0, &reduced);
    }

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_bigint_from_int(0, &quotient);
    }

    if (*host_error != CARDANO_SUCCESS)
    {
      cardano_bigint_unref(&modulus);
      cardano_bigint_unref(&reduced);
      cardano_bigint_unref(&quotient);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    floor_div_mod(shift, modulus, quotient, reduced);
    rot = cardano_bigint_to_unsigned_int(reduced);

    cardano_bigint_unref(&modulus);
    cardano_bigint_unref(&reduced);
    cardano_bigint_unref(&quotient);
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, size, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)memset(scratch, 0, size);

  for (dst_bit = 0U; dst_bit < bits; ++dst_bit)
  {
    uint64_t     src_bit  = (dst_bit + rot) % bits;
    size_t       src_byte = (size_t)(src_bit / 8U);
    unsigned int src_off  = (unsigned int)(7U - (src_bit % 8U));
    size_t       dst_byte = (size_t)(dst_bit / 8U);
    unsigned int dst_off  = (unsigned int)(7U - (dst_bit % 8U));

    if (((data[src_byte] >> src_off) & 1U) == 1U)
    {
      scratch[dst_byte] = (byte_t)(scratch[dst_byte] | (byte_t)(1U << dst_off));
    }
  }

  *host_error = result_bytes_take(arena, scratch, size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c countSetBits: the population count of a byte string.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_count_set_bits(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes = { NULL, 0U };
  const byte_t*            data  = NULL;
  size_t                   size  = 0U;
  int64_t                  total = 0;
  size_t                   i     = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  data = bytes.data;
  size = bytes.size;

  for (i = 0U; i < size; ++i)
  {
    byte_t b = data[i];

    while (b != 0U)
    {
      total += (int64_t)b & (int64_t)1;
      b     = (byte_t)(b >> 1);
    }
  }

  *host_error = result_integer_from_int(arena, total, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c findFirstSetBit: the index of the lowest set bit, else -1.
 *
 * The bit index is counted from the least-significant bit of the last byte, the
 * same ordering as \c readBit. An all-zero or empty string yields -1.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_find_first_set_bit(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes  = { NULL, 0U };
  const byte_t*            data   = NULL;
  size_t                   size   = 0U;
  int64_t                  result = -1;
  size_t                   i      = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  data = bytes.data;
  size = bytes.size;

  for (i = 0U; (i < size) && (result < 0); ++i)
  {
    byte_t b = data[size - 1U - i];

    if (b != 0U)
    {
      unsigned int bit = 0U;

      while (((b >> bit) & 1U) == 0U)
      {
        ++bit;
      }

      result = ((int64_t)i * 8) + (int64_t)bit;
    }
  }

  *host_error = result_integer_from_int(arena, result, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c integerToByteString: a fixed-or-minimal-width encoding.
 *
 * The first argument is the endianness flag (true for big-endian), the second the
 * requested width (zero meaning the minimal width), the third the non-negative
 * value. A negative value, a negative or over-large width, or a width too small to
 * hold the value, is a script error; a zero width with a value that would exceed
 * the maximum output length is likewise a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (endianness, width, value).
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_integer_to_byte_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  bool                    big_endian  = false;
  const cardano_bigint_t* width_arg   = NULL;
  const cardano_bigint_t* value       = NULL;
  uint64_t                width       = 0U;
  size_t                  value_bytes = 0U;
  byte_t*                 raw         = NULL;
  byte_t*                 scratch     = NULL;
  size_t                  out_len     = 0U;
  cardano_error_t         conv        = CARDANO_SUCCESS;

  if (!cardano_uplc_builtin_as_bool(args[0], &big_endian) || !as_integer_big(arena, args[1], &width_arg) || !as_integer_big(arena, args[2], &value))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_signum(width_arg) < 0)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_bit_length(width_arg) > 63U)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  width = cardano_bigint_to_unsigned_int(width_arg);

  if (width > (uint64_t)CARDANO_UPLC_BUILTIN_INT_TO_BS_MAX)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_signum(value) < 0)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((width == 0U) && (cardano_bigint_signum(value) != 0))
  {
    size_t needed = (cardano_bigint_bit_length(value) + 7U) / 8U;

    if (needed > (size_t)CARDANO_UPLC_BUILTIN_INT_TO_BS_MAX)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  if (cardano_bigint_signum(value) == 0)
  {
    if (width == 0U)
    {
      *host_error = result_bytes_from(arena, NULL, 0U, out_result);

      return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    scratch = (byte_t*)cardano_uplc_arena_alloc(arena, (size_t)width, 1U);

    if (scratch == NULL)
    {
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    (void)memset(scratch, 0, (size_t)width);
    *host_error = result_bytes_take(arena, scratch, (size_t)width, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  value_bytes = cardano_bigint_get_bytes_size(value);
  raw         = (byte_t*)_cardano_malloc(value_bytes);

  if (raw == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  conv = cardano_bigint_to_bytes(value, CARDANO_BYTE_ORDER_BIG_ENDIAN, raw, value_bytes);

  if (conv != CARDANO_SUCCESS)
  {
    _cardano_free(raw);
    *host_error = conv;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  {
    size_t lead = 0U;

    while ((lead < (value_bytes - 1U)) && (raw[lead] == 0U))
    {
      ++lead;
    }

    {
      size_t significant = value_bytes - lead;
      // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
      byte_t* msb      = raw + lead;
      size_t  width_sz = (size_t)width;

      if ((width_sz != 0U) && (value_bytes > (width_sz + lead)))
      {
        _cardano_free(raw);

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      out_len = (width_sz != 0U) ? width_sz : significant;

      scratch = (byte_t*)cardano_uplc_arena_alloc(arena, out_len, 1U);

      if (scratch == NULL)
      {
        _cardano_free(raw);
        *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      (void)memset(scratch, 0, out_len);

      if (big_endian)
      {
        // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
        (void)memcpy(scratch + (out_len - significant), msb, significant);
      }
      else
      {
        size_t k = 0U;

        for (k = 0U; k < significant; ++k)
        {
          scratch[k] = msb[significant - 1U - k];
        }
      }

      _cardano_free(raw);
    }
  }

  *host_error = result_bytes_take(arena, scratch, out_len, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c byteStringToInteger: decodes a byte string as a non-negative int.
 *
 * The first argument is the endianness flag (true for big-endian), the second the
 * byte string. An empty input decodes to zero. The result is always non-negative.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (endianness, byte string).
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_byte_string_to_integer(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  bool                     big_endian = false;
  cardano_uplc_byte_view_t bytes      = { NULL, 0U };
  const byte_t*            data       = NULL;
  size_t                   size       = 0U;
  cardano_bigint_t*        value      = NULL;

  if (!cardano_uplc_builtin_as_bool(args[0], &big_endian) || !cardano_uplc_builtin_as_byte_string(args[1], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  data = bytes.data;
  size = bytes.size;

  if (size == 0U)
  {
    *host_error = result_integer_from_int(arena, 0, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_bigint_from_bytes(
    data,
    size,
    big_endian ? CARDANO_BYTE_ORDER_BIG_ENDIAN : CARDANO_BYTE_ORDER_LITTLE_ENDIAN,
    &value);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_integer(arena, value, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/* BOOL, UNIT, STRING, PAIR AND LIST BODIES *******************************/

/**
 * \brief Builds a string result value by copying a raw UTF-8 span into the arena.
 *
 * The span must already hold valid UTF-8; the constructor does not re-validate. Like
 * the byte-string producer it bump-allocates the copy in the arena and never mallocs
 * or refcounts a library buffer.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] data The UTF-8 bytes to copy, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_string_from(
  struct cardano_uplc_arena_t* arena,
  const byte_t*                data,
  size_t                       size,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = cardano_uplc_int_constant_new_string_copy(arena, data, size, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Runs \c ifThenElse: selects one of two already-evaluated values.
 *
 * The leading argument is a boolean; a non-boolean is a script error. The result
 * is the second argument when the boolean is true and the third otherwise. Both
 * branch arguments are returned by value unchanged.
 *
 * \param[in] args The three saturated argument values (condition, then, else).
 * \param[out] out_result On success, the selected branch value.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_if_then_else(
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result)
{
  bool condition = false;

  if (!cardano_uplc_builtin_as_bool(args[0], &condition))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = condition ? args[1] : args[2];

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c chooseUnit: forces the unit type then returns its second value.
 *
 * The first argument must be the unit constant (a type check; any other value is a
 * script error); the result is the second argument returned unchanged.
 *
 * \param[in] args The two saturated argument values (unit, value).
 * \param[out] out_result On success, the second argument value.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_choose_unit(
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result)
{
  if (!cardano_uplc_builtin_as_unit(args[0]))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = args[1];

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c trace: type-checks the trace string then returns its second value.
 *
 * The first argument must be a string (a type check; a non-string is a script
 * error). Trace output is not surfaced by this machine, so the text is
 * type-checked and discarded. The result is the second argument returned
 * unchanged.
 *
 * \param[in] args The two saturated argument values (message, value).
 * \param[out] out_result On success, the second argument value.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_trace(
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result)
{
  cardano_uplc_byte_view_t message = { NULL, 0U };

  if (!cardano_uplc_builtin_as_string(args[0], &message))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)message;

  *out_result = args[1];

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c appendString: concatenates two UTF-8 text strings.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_append_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t lhs     = { NULL, 0U };
  cardano_uplc_byte_view_t rhs     = { NULL, 0U };
  size_t                   total   = 0U;
  byte_t*                  scratch = NULL;

  if (!cardano_uplc_builtin_as_string(args[0], &lhs) || !cardano_uplc_builtin_as_string(args[1], &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  total = lhs.size + rhs.size;

  if (total == 0U)
  {
    *host_error = result_string_from(arena, NULL, 0U, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  scratch = (byte_t*)cardano_uplc_arena_alloc(arena, total, 1U);

  if (scratch == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (lhs.size > 0U)
  {
    (void)memcpy(scratch, lhs.data, lhs.size);
  }

  if (rhs.size > 0U)
  {
    // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
    (void)memcpy(scratch + lhs.size, rhs.data, rhs.size);
  }

  *host_error = result_string_from(arena, scratch, total, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c equalsString: tests two UTF-8 text strings for equality.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_equals_string(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t lhs = { NULL, 0U };
  cardano_uplc_byte_view_t rhs = { NULL, 0U };

  if (!cardano_uplc_builtin_as_string(args[0], &lhs) || !cardano_uplc_builtin_as_string(args[1], &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bool(arena, byte_view_compare(lhs.data, lhs.size, rhs.data, rhs.size) == 0, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c encodeUtf8: the UTF-8 bytes of a text string as a byte string.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_encode_utf8(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t text = { NULL, 0U };

  if (!cardano_uplc_builtin_as_string(args[0], &text))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bytes_from(arena, text.data, text.size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c decodeUtf8: a byte string interpreted as UTF-8 text.
 *
 * A byte string that is not well-formed UTF-8 is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_decode_utf8(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes = { NULL, 0U };

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!is_valid_utf8(bytes.data, bytes.size))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_string_from(arena, bytes.data, bytes.size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c fstPair and \c sndPair: a pair component as a value.
 *
 * The argument must be a pair (a non-pair is a script error). The result wraps the
 * selected component constant in a constant value.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] want_first Selects the first component when true, the second otherwise.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the component value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_pair_component(
  struct cardano_uplc_arena_t*       arena,
  bool                               want_first,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* fst    = NULL;
  const cardano_uplc_constant_t* snd    = NULL;
  cardano_uplc_value_t*          result = NULL;

  if (!cardano_uplc_builtin_as_pair(args[0], &fst, &snd))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, want_first ? fst : snd, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c chooseList: branches on whether a list is empty.
 *
 * The first argument must be a list (a non-list is a script error). The result is
 * the second argument when the list is empty and the third otherwise, returned by
 * value unchanged.
 *
 * \param[in] args The three saturated argument values (list, empty case, non-empty).
 * \param[out] out_result On success, the selected branch value.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_choose_list(
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result)
{
  size_t count = 0U;

  if (!cardano_uplc_builtin_as_list(args[0], NULL, NULL, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = (count == 0U) ? args[1] : args[2];

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c mkCons: prepends an element to a list of matching element type.
 *
 * The first argument is the element constant, the second a list whose element type
 * must equal the element's type (a mismatch is a script error). The result is a new
 * list carrying the declared element type with the element at the head.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (element, list).
 * \param[out] out_result On success, the list result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_mk_cons(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t*        head         = NULL;
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  const cardano_uplc_constant_t**       merged       = NULL;
  cardano_uplc_constant_t*              constant     = NULL;
  cardano_uplc_value_t*                 result       = NULL;
  size_t                                i            = 0U;

  if (!as_constant(args[0], &head))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!cardano_uplc_builtin_as_list(args[1], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!constant_has_type(element_type, head))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  merged = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, sizeof(*merged) * (count + 1U), 0U);

  if (merged == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  merged[0] = head;

  for (i = 0U; i < count; ++i)
  {
    merged[i + 1U] = items[i];
  }

  *host_error = cardano_uplc_constant_new_list(arena, element_type, merged, count + 1U, &constant);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c headList: the first element of a list as a value.
 *
 * An empty list is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the head element value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_head_list(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* const* items  = NULL;
  size_t                                count  = 0U;
  cardano_uplc_value_t*                 result = NULL;

  if (!cardano_uplc_builtin_as_list(args[0], NULL, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (count == 0U)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, items[0], &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c tailList: a list minus its first element.
 *
 * An empty list is a script error. The result carries the same element type and
 * the items from index one onward.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the tail list value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_tail_list(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  cardano_uplc_constant_t*              constant     = NULL;
  cardano_uplc_value_t*                 result       = NULL;

  if (!cardano_uplc_builtin_as_list(args[0], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (count == 0U)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_constant_new_list(
    arena,
    element_type,
    // cppcheck-suppress misra-c2012-12.1; Reason: operator precedence is explicit and intentional
    // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
    (count > 1U) ? (items + 1) : NULL,
    count - 1U,
    &constant);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c nullList: whether a list is empty, as a boolean.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_null_list(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  size_t count = 0U;

  if (!cardano_uplc_builtin_as_list(args[0], NULL, NULL, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bool(arena, count == 0U, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/* DATA BODIES ************************************************************/

/**
 * \brief Wraps an arena data node as a constant value result.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] data The arena data node to publish. Must live in \p arena.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_data_node(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_data_t*   data,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = CARDANO_SUCCESS;

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  error = cardano_uplc_int_constant_new_data_node(arena, data, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Wraps an owned library plutus-data object as an arena data result.
 *
 * Converts \p data into an arena data tree, wraps it in a constant value, and always
 * releases the reference \p data held on entry, on success and failure alike. Used
 * by the \c valueData builtin, which builds the library plutus-data type before
 * crossing into the arena.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in,out] data The owned plutus-data to publish; released before return.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_data(
  struct cardano_uplc_arena_t* arena,
  cardano_plutus_data_t*       data,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_data_t* node  = NULL;
  cardano_error_t      error = CARDANO_SUCCESS;

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  error = cardano_uplc_data_from_plutus_data(arena, data, &node);

  cardano_plutus_data_unref(&data);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  return result_data_node(arena, node, out);
}

/**
 * \brief Builds the \c data type descriptor in an arena.
 *
 * \param[in] arena The arena the descriptor is allocated from.
 * \param[out] out On success, the descriptor; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
type_data(struct cardano_uplc_arena_t* arena, cardano_uplc_type_t** out)
{
  return cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_DATA, NULL, NULL, out);
}

/**
 * \brief Builds the \c (pair data data) type descriptor in an arena.
 *
 * \param[in] arena The arena the descriptor is allocated from.
 * \param[out] out On success, the descriptor; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
type_pair_data(struct cardano_uplc_arena_t* arena, cardano_uplc_type_t** out)
{
  cardano_uplc_type_t* data_a = NULL;
  cardano_uplc_type_t* data_b = NULL;
  cardano_error_t      error  = type_data(arena, &data_a);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = type_data(arena, &data_b);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  return cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, data_a, data_b, out);
}

/**
 * \brief Reads the arena data node wrapped by a data constant.
 *
 * \param[in] constant The candidate constant. Must not be NULL.
 * \param[out] out On success, the borrowed data node; left untouched otherwise.
 *
 * \return \c true when \p constant is a data constant, \c false otherwise.
 */
static bool
constant_as_data_node(const cardano_uplc_constant_t* constant, const cardano_uplc_data_t** out)
{
  if ((constant == NULL) || (constant->kind != CARDANO_UPLC_TYPE_DATA))
  {
    return false;
  }

  *out = constant->as.data;

  return true;
}

/**
 * \brief Tests whether a constant type descriptor is \c (pair data data).
 *
 * \param[in] type The descriptor to test.
 *
 * \return \c true when \p type names a pair of two data components.
 */
static bool
type_is_pair_data(const cardano_uplc_type_t* type)
{
  if ((type == NULL) || (type->kind != CARDANO_UPLC_TYPE_PAIR))
  {
    return false;
  }

  if ((type->fst == NULL) || (type->snd == NULL))
  {
    return false;
  }

  return (type->fst->kind == CARDANO_UPLC_TYPE_DATA) && (type->snd->kind == CARDANO_UPLC_TYPE_DATA);
}

/**
 * \brief Collects the data nodes of a proto-list of data into an arena node array.
 *
 * The list element type must be \c data and every item a data constant. The
 * returned array lives in \p arena.
 *
 * \param[in] arena The arena the array is allocated from.
 * \param[in] element_type The list element type descriptor.
 * \param[in] items The element constants.
 * \param[in] count The number of elements.
 * \param[out] out_nodes On success, the arena node array, or NULL when empty.
 *
 * \return \c true on success, \c false on a shape mismatch or allocation failure.
 */
static bool
collect_data_nodes(
  struct cardano_uplc_arena_t*          arena,
  const cardano_uplc_type_t*            element_type,
  const cardano_uplc_constant_t* const* items,
  size_t                                count,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_data_t* const** out_nodes)
{
  const cardano_uplc_data_t** nodes = NULL;
  size_t                      i     = 0U;

  if ((element_type == NULL) || (element_type->kind != CARDANO_UPLC_TYPE_DATA))
  {
    return false;
  }

  if (count > 0U)
  {
    nodes = (const cardano_uplc_data_t**)cardano_uplc_arena_alloc(arena, sizeof(*nodes) * count, 0U);

    if (nodes == NULL)
    {
      return false;
    }
  }

  for (i = 0U; i < count; ++i)
  {
    const cardano_uplc_data_t* node = NULL;

    if (!constant_as_data_node(items[i], &node))
    {
      return false;
    }

    nodes[i] = node;
  }

  *out_nodes = (const cardano_uplc_data_t* const*)nodes;

  return true;
}

/**
 * \brief Runs \c constrData: a constructor index and a data list into Data Constr.
 *
 * The first argument is the constructor alternative (an integer), the second a
 * proto-list of data. The raw alternative is stored; the compact CBOR tag is
 * derived by the serializer.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values (index, data list).
 * \param[out] out_result On success, the data result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_constr_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t*               index        = NULL;
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  const cardano_uplc_data_t* const*     fields       = NULL;
  cardano_uplc_data_t*                  data         = NULL;
  uint64_t                              alternative  = 0U;

  if (!as_integer_big(arena, args[0], &index))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!cardano_uplc_builtin_as_list(args[1], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!collect_data_nodes(arena, element_type, items, count, &fields))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  alternative = cardano_bigint_to_unsigned_int(index);

  *host_error = cardano_uplc_data_new_constr(arena, alternative, fields, count, &data);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_data_node(arena, data, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c mapData: a proto-list of data pairs into Data Map.
 *
 * The argument must be a list whose element type is \c (pair data data); each pair
 * contributes a key and a value.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the data result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_map_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  cardano_uplc_data_pair_t*             entries      = NULL;
  cardano_uplc_data_t*                  data         = NULL;
  size_t                                i            = 0U;

  if (!cardano_uplc_builtin_as_list(args[0], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!type_is_pair_data(element_type))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (count > 0U)
  {
    entries = (cardano_uplc_data_pair_t*)cardano_uplc_arena_alloc(arena, sizeof(*entries) * count, 0U);

    if (entries == NULL)
    {
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  for (i = 0U; i < count; ++i)
  {
    const cardano_uplc_constant_t* entry = items[i];
    const cardano_uplc_data_t*     key   = NULL;
    const cardano_uplc_data_t*     value = NULL;

    if ((entry == NULL) || (entry->kind != CARDANO_UPLC_TYPE_PAIR) || !constant_as_data_node(entry->as.pair.fst, &key) || !constant_as_data_node(entry->as.pair.snd, &value))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    entries[i].key   = key;
    entries[i].value = value;
  }

  *host_error = cardano_uplc_data_new_map(arena, entries, count, false, &data);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_data_node(arena, data, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c listData: a proto-list of data into Data List.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the data result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_list_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  const cardano_uplc_data_t* const*     nodes        = NULL;
  cardano_uplc_data_t*                  data         = NULL;

  if (!cardano_uplc_builtin_as_list(args[0], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (!collect_data_nodes(arena, element_type, items, count, &nodes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_data_new_list(arena, nodes, count, &data);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_data_node(arena, data, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c iData: an integer into Data Integer.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the data result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_i_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t* integer = NULL;
  cardano_uplc_data_t*    data    = NULL;

  if (!as_integer_big(arena, args[0], &integer))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
  *host_error = cardano_uplc_data_new_integer(arena, (cardano_bigint_t*)((const void*)integer), &data);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_data_node(arena, data, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bData: a byte string into Data Bytes.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the data result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_b_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes = { NULL, 0U };
  cardano_uplc_data_t*     data  = NULL;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_data_new_bytes(arena, bytes.data, bytes.size, &data);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_data_node(arena, data, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Builds a proto-list of data constants from an arena data node array.
 *
 * Each data node is wrapped in a fresh data constant; the result is a list constant
 * carrying element type \c data. Used by the deconstructors that surface a Data List
 * or the fields of a Data Constr.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] nodes The data nodes to expose, or NULL when \p count is 0.
 * \param[in] count The number of nodes.
 * \param[out] out On success, the list constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
proto_list_from_data_nodes(
  struct cardano_uplc_arena_t*      arena,
  const cardano_uplc_data_t* const* nodes,
  size_t                            count,
  cardano_uplc_constant_t**         out)
{
  cardano_uplc_type_t*            data_type = NULL;
  const cardano_uplc_constant_t** items     = NULL;
  size_t                          i         = 0U;
  cardano_error_t                 error     = type_data(arena, &data_type);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  if (count > 0U)
  {
    items = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, sizeof(*items) * count, 0U);

    if (items == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  for (i = 0U; i < count; ++i)
  {
    cardano_uplc_constant_t* constant = NULL;

    error = cardano_uplc_int_constant_new_data_node(arena, nodes[i], &constant);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }

    items[i] = constant;
  }

  return cardano_uplc_constant_new_list(arena, data_type, items, count, out);
}

/**
 * \brief Runs \c unConstrData: a Data Constr into a (pair integer (list data)).
 *
 * A non-Constr argument is a script error. The first component is the constructor
 * alternative as an integer, the second the fields as a proto-list of data.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the pair result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_un_constr_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t* data       = NULL;
  cardano_uplc_constant_t*   tag_const  = NULL;
  cardano_uplc_constant_t*   list_const = NULL;
  cardano_uplc_constant_t*   pair_const = NULL;
  cardano_uplc_value_t*      result     = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (data->kind != CARDANO_UPLC_DATA_KIND_CONSTR)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_constant_new_integer_small(arena, (int64_t)data->as.constr.tag, &tag_const);

  if ((*host_error == CARDANO_SUCCESS) && (data->as.constr.tag > (uint64_t)INT64_MAX))
  {
    cardano_bigint_t* tag_value = NULL;

    *host_error = cardano_bigint_from_unsigned_int(data->as.constr.tag, &tag_value);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_uplc_constant_new_integer(arena, tag_value, &tag_const);
      cardano_bigint_unref(&tag_value);
    }
  }

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = proto_list_from_data_nodes(arena, data->as.constr.fields, data->as.constr.count, &list_const);
  }

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_constant_new_pair(arena, tag_const, list_const, &pair_const);
  }

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_value_new_constant(arena, pair_const, &result);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c unMapData: a Data Map into a proto-list of (pair data data).
 *
 * A non-Map argument is a script error. Each entry becomes a proto-pair of its key
 * and value.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the list result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_un_map_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t*      data       = NULL;
  cardano_uplc_type_t*            pair_type  = NULL;
  const cardano_uplc_constant_t** items      = NULL;
  cardano_uplc_constant_t*        list_const = NULL;
  cardano_uplc_value_t*           result     = NULL;
  size_t                          count      = 0U;
  size_t                          i          = 0U;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (data->kind != CARDANO_UPLC_DATA_KIND_MAP)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = type_pair_data(arena, &pair_type);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  count = data->as.map.count;

  if (count > 0U)
  {
    items = (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, sizeof(*items) * count, 0U);

    if (items == NULL)
    {
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  for (i = 0U; i < count; ++i)
  {
    cardano_uplc_constant_t* key_const  = NULL;
    cardano_uplc_constant_t* val_const  = NULL;
    cardano_uplc_constant_t* pair_const = NULL;

    *host_error = cardano_uplc_int_constant_new_data_node(arena, data->as.map.entries[i].key, &key_const);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_uplc_int_constant_new_data_node(arena, data->as.map.entries[i].value, &val_const);
    }

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_uplc_constant_new_pair(arena, key_const, val_const, &pair_const);
    }

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    items[i] = pair_const;
  }

  *host_error = cardano_uplc_constant_new_list(arena, pair_type, items, count, &list_const);

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_value_new_constant(arena, list_const, &result);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c unListData: a Data List into a proto-list of data.
 *
 * A non-List argument is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the list result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_un_list_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t* data       = NULL;
  cardano_uplc_constant_t*   list_const = NULL;
  cardano_uplc_value_t*      result     = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (data->kind != CARDANO_UPLC_DATA_KIND_LIST)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = proto_list_from_data_nodes(arena, data->as.list.items, data->as.list.count, &list_const);

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_value_new_constant(arena, list_const, &result);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c unIData: a Data Integer into an integer.
 *
 * A non-Integer argument is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_un_i_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t* data = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (data->kind != CARDANO_UPLC_DATA_KIND_INTEGER)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (data->as.integer.is_small)
  {
    cardano_uplc_constant_t* constant = NULL;
    cardano_uplc_value_t*    result   = NULL;

    *host_error = cardano_uplc_constant_new_integer_small(arena, data->as.integer.small, &constant);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_uplc_value_new_constant(arena, constant, &result);
    }

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *out_result = result;

    return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
  }

  {
    const cardano_bigint_t*  integer  = NULL;
    cardano_uplc_constant_t* constant = NULL;
    cardano_uplc_value_t*    result   = NULL;

    *host_error = cardano_uplc_data_integer_materialize(arena, data, &integer);

    if (*host_error == CARDANO_SUCCESS)
    {
      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      *host_error = cardano_uplc_constant_new_integer(arena, (cardano_bigint_t*)((const void*)integer), &constant);
    }

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_uplc_value_new_constant(arena, constant, &result);
    }

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *out_result = result;
  }

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c unBData: a Data Bytes into a byte string.
 *
 * A non-Bytes argument is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_un_b_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t* data = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (data->kind != CARDANO_UPLC_DATA_KIND_BYTES)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bytes_from(arena, data->as.bytes.data, data->as.bytes.size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c chooseData: selects a branch by the data's kind.
 *
 * The first argument must be data (a non-data is a script error); a Constr selects
 * the second argument, a Map the third, a List the fourth, an Integer the fifth and
 * Bytes the sixth, returned by value unchanged.
 *
 * \param[in] args The six saturated argument values.
 * \param[out] out_result On success, the selected branch value.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_choose_data(
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result)
{
  const cardano_uplc_data_t* data = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      *out_result = args[1];

      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      *out_result = args[2];

      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      *out_result = args[3];

      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    {
      *out_result = args[4];

      break;
    }
    case CARDANO_UPLC_DATA_KIND_BYTES:
    {
      *out_result = args[5];

      break;
    }
    default:
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c equalsData: tests two data values for structural equality.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_equals_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t* lhs = NULL;
  const cardano_uplc_data_t* rhs = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &lhs) || !cardano_uplc_builtin_as_data(args[1], &rhs))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bool(arena, cardano_uplc_data_equals(lhs, rhs), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c serialiseData: the canonical CBOR encoding of a data value.
 *
 * The serializer always writes the canonical form the ledger expects: indefinite
 * arrays and constr fields except when empty, and maps in their decoded definite
 * or indefinite form.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_serialise_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t* data   = NULL;
  cardano_cbor_writer_t*     writer = NULL;
  cardano_buffer_t*          bytes  = NULL;

  if (!cardano_uplc_builtin_as_data(args[0], &data))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_data_to_cbor(data, writer);

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_cbor_writer_encode_in_buffer(writer, &bytes);
  }

  cardano_cbor_writer_unref(&writer);

  if (*host_error != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&bytes);

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bytes_from(arena, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes), out_result);

  cardano_buffer_unref(&bytes);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c mkPairData: two data values into a proto-pair (pair data data).
 *
 * A non-data argument is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two saturated argument values.
 * \param[out] out_result On success, the pair result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_mk_pair_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* fst        = NULL;
  const cardano_uplc_constant_t* snd        = NULL;
  cardano_uplc_constant_t*       pair_const = NULL;
  cardano_uplc_value_t*          result     = NULL;

  if (!as_constant(args[0], &fst) || !as_constant(args[1], &snd))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((fst->kind != CARDANO_UPLC_TYPE_DATA) || (snd->kind != CARDANO_UPLC_TYPE_DATA))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_constant_new_pair(arena, fst, snd, &pair_const);

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_value_new_constant(arena, pair_const, &result);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c mkNilData and \c mkNilPairData: an empty proto-list.
 *
 * The argument must be unit (anything else is a script error). The element type is
 * \c data for \c mkNilData and \c (pair data data) for \c mkNilPairData.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] pair_elements Selects the \c (pair data data) element type when true.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the empty list result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_mk_nil_data(
  struct cardano_uplc_arena_t*       arena,
  bool                               pair_elements,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_type_t*     element_type = NULL;
  cardano_uplc_constant_t* list_const   = NULL;
  cardano_uplc_value_t*    result       = NULL;

  if (!cardano_uplc_builtin_as_unit(args[0]))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (pair_elements)
  {
    *host_error = type_pair_data(arena, &element_type);
  }
  else
  {
    *host_error = type_data(arena, &element_type);
  }

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_constant_new_list(arena, element_type, NULL, 0U, &list_const);
  }

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_value_new_constant(arena, list_const, &result);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c sha2_256: the SHA-256 digest of a byte string.
 *
 * Produces a 32-byte digest using libsodium's \c crypto_hash_sha256.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_sha2_256(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t input = { NULL, 0U };
  byte_t                   digest[32];

  if (!cardano_uplc_builtin_as_byte_string(args[0], &input))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)crypto_hash_sha256(digest, nonnull_bytes(input.data, input.size), input.size);

  *host_error = result_bytes_from(arena, digest, (size_t)crypto_hash_sha256_BYTES, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs the Blake2b digest builtins.
 *
 * Produces a digest of \p out_size bytes (28 for \c blake2b_224, 32 for
 * \c blake2b_256) using libsodium's \c crypto_generichash (Blake2b). The digest
 * of an empty input is a valid hash, not an error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] out_size The digest length in bytes.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_blake2b(
  struct cardano_uplc_arena_t*       arena,
  size_t                             out_size,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t input = { NULL, 0U };
  byte_t                   digest[32];

  if (!cardano_uplc_builtin_as_byte_string(args[0], &input))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)crypto_generichash(digest, out_size, nonnull_bytes(input.data, input.size), input.size, NULL, 0U);

  *host_error = result_bytes_from(arena, digest, out_size, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs the SHA3-256 and Keccak-256 digest builtins.
 *
 * Both feed the input byte string through the same 32-byte Keccak sponge and
 * differ only in the final domain-separation pad byte: 0x06 yields FIPS-202
 * SHA3-256, 0x01 yields the original Keccak-256. Both hash the single byte-string
 * argument. The digest of an empty input is a valid hash.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] pad The final pad byte selecting SHA3 (0x06) or Keccak (0x01).
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_sha3_keccak(
  struct cardano_uplc_arena_t*       arena,
  uint8_t                            pad,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t input = { NULL, 0U };
  byte_t                   digest[32];
  sha3_ctx_t               ctx;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &input))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  (void)sha3_init(&ctx, (int)CARDANO_UPLC_BUILTIN_SHA3_256_SIZE);
  (void)sha3_update(&ctx, nonnull_bytes(input.data, input.size), input.size);
  (void)sha3_final_pad(digest, &ctx, pad);

  *host_error = result_bytes_from(arena, digest, CARDANO_UPLC_BUILTIN_SHA3_256_SIZE, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c ripemd_160: the RIPEMD-160 digest of a byte string.
 *
 * Produces a 20-byte digest using the vendored one-shot \c ripemd160. The digest
 * of an empty input is a valid hash, not an error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single saturated argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_ripemd_160(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t input = { NULL, 0U };
  byte_t                   digest[20];

  if (!cardano_uplc_builtin_as_byte_string(args[0], &input))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  ripemd160(nonnull_bytes(input.data, input.size), input.size, digest);

  *host_error = result_bytes_from(arena, digest, CARDANO_UPLC_BUILTIN_RIPEMD_160_SIZE, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c verifyEd25519Signature: checks an Ed25519 signature.
 *
 * A public key that is not 32 bytes or a signature that is not 64 bytes is a
 * script error. With valid lengths the result is the boolean verification
 * outcome; a verification failure is a \c false result, not an error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (public key, message, signature).
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_verify_ed25519(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t      key_bytes = { NULL, 0U };
  cardano_uplc_byte_view_t      msg_bytes = { NULL, 0U };
  cardano_uplc_byte_view_t      sig_bytes = { NULL, 0U };
  cardano_ed25519_public_key_t* key       = NULL;
  cardano_ed25519_signature_t*  signature = NULL;
  cardano_error_t               key_error = CARDANO_SUCCESS;
  cardano_error_t               sig_error = CARDANO_SUCCESS;
  bool                          valid     = false;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &key_bytes) || !cardano_uplc_builtin_as_byte_string(args[1], &msg_bytes) || !cardano_uplc_builtin_as_byte_string(args[2], &sig_bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (key_bytes.size != CARDANO_UPLC_BUILTIN_ED25519_KEY_SIZE)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (sig_bytes.size != CARDANO_UPLC_BUILTIN_ED25519_SIG_SIZE)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  key_error = cardano_ed25519_public_key_from_bytes(
    key_bytes.data,
    key_bytes.size,
    &key);

  if (key_error != CARDANO_SUCCESS)
  {
    *host_error = key_error;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  sig_error = cardano_ed25519_signature_from_bytes(
    sig_bytes.data,
    sig_bytes.size,
    &signature);

  if (sig_error != CARDANO_SUCCESS)
  {
    cardano_ed25519_public_key_unref(&key);
    *host_error = sig_error;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  valid = cardano_ed25519_public_verify(key, signature, nonnull_bytes(msg_bytes.data, msg_bytes.size), msg_bytes.size);

  cardano_ed25519_public_key_unref(&key);
  cardano_ed25519_signature_unref(&signature);

  *host_error = result_bool(arena, valid, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c verifyEcdsaSecp256k1Signature: checks a secp256k1 ECDSA signature.
 *
 * The public key must be a 33-byte compressed point, the message a 32-byte
 * pre-hashed digest, and the signature 64 compact bytes; any other length, an
 * unparseable key, or an unparseable signature is a script error. With well-formed
 * inputs the result is the boolean verification outcome. The verifier rejects
 * signatures whose S component is not in low-S form, so a high-S signature is a
 * \c false result rather than \c true.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (public key, message, signature).
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_verify_ecdsa(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t  key_bytes = { NULL, 0U };
  cardano_uplc_byte_view_t  msg_bytes = { NULL, 0U };
  cardano_uplc_byte_view_t  sig_bytes = { NULL, 0U };
  secp256k1_pubkey          pubkey;
  secp256k1_ecdsa_signature signature;
  bool                      valid = false;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &key_bytes) || !cardano_uplc_builtin_as_byte_string(args[1], &msg_bytes) || !cardano_uplc_builtin_as_byte_string(args[2], &sig_bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((key_bytes.size != CARDANO_UPLC_BUILTIN_ECDSA_KEY_SIZE) || (msg_bytes.size != CARDANO_UPLC_BUILTIN_ECDSA_MSG_SIZE) || (sig_bytes.size != CARDANO_UPLC_BUILTIN_ECDSA_SIG_SIZE))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (secp256k1_ec_pubkey_parse(secp256k1_context_static, &pubkey, key_bytes.data, key_bytes.size) != 1)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &signature, sig_bytes.data) != 1)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  valid = (secp256k1_ecdsa_verify(secp256k1_context_static, &signature, msg_bytes.data, &pubkey) == 1);

  *host_error = result_bool(arena, valid, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c verifySchnorrSecp256k1Signature: checks a BIP340 signature.
 *
 * The public key must be a 32-byte x-only point and the signature 64 bytes;
 * either wrong length or an unparseable key is a script error. Unlike the
 * message of the ECDSA builtin the Schnorr message is of arbitrary length. With
 * well-formed key and signature the result is the boolean verification outcome.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The three saturated argument values (public key, message, signature).
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_verify_schnorr(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t key_bytes = { NULL, 0U };
  cardano_uplc_byte_view_t msg_bytes = { NULL, 0U };
  cardano_uplc_byte_view_t sig_bytes = { NULL, 0U };
  secp256k1_xonly_pubkey   pubkey;
  const unsigned char*     msg_data  = NULL;
  size_t                   msg_size  = 0U;
  const unsigned char      msg_empty = 0U;
  bool                     valid     = false;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &key_bytes) || !cardano_uplc_builtin_as_byte_string(args[1], &msg_bytes) || !cardano_uplc_builtin_as_byte_string(args[2], &sig_bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((key_bytes.size != CARDANO_UPLC_BUILTIN_SCHNORR_KEY_SIZE) || (sig_bytes.size != CARDANO_UPLC_BUILTIN_SCHNORR_SIG_SIZE))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (secp256k1_xonly_pubkey_parse(secp256k1_context_static, &pubkey, key_bytes.data) != 1)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  msg_size = msg_bytes.size;
  msg_data = (msg_size == 0U) ? &msg_empty : msg_bytes.data;

  valid = (secp256k1_schnorrsig_verify(secp256k1_context_static, sig_bytes.data, msg_data, msg_size, &pubkey) == 1);

  *host_error = result_bool(arena, valid, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/* BLS12-381 HELPERS ********************************************************/

/**
 * \brief Reads the BLS12-381 point a BLS constant of a given kind carries.
 *
 * A value that is not a constant of the requested BLS kind is a type mismatch,
 * reported here by returning \c false.
 *
 * \param[in] value The argument value.
 * \param[in] kind The required BLS constant kind.
 * \param[out] out On success, the borrowed point struct pointer.
 *
 * \return \c true when \p value is a BLS constant of \p kind, \c false otherwise.
 */
static bool
as_bls(const cardano_uplc_value_t* value, cardano_uplc_type_kind_t kind, const void** out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != kind)
  {
    return false;
  }

  *out = constant->as.bls;

  return true;
}

/**
 * \brief Wraps a BLS12-381 value as an arena BLS result value.
 *
 * Builds a BLS constant copying \p size bytes from \p data and wraps it in a
 * constant value.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] kind The BLS constant kind.
 * \param[in] data The raw point or field value to copy.
 * \param[in] size The number of bytes to copy.
 * \param[out] out On success, the constant value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
result_bls(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_type_kind_t     kind,
  const void*                  data,
  size_t                       size,
  const cardano_uplc_value_t** out)
{
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = cardano_uplc_constant_new_bls(arena, kind, data, size, &constant);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reduces an integer scalar modulo the BLS scalar period into 32 bytes.
 *
 * Computes \c scalar mod r big-endian, left-padded to 32 bytes, the form the
 * scalar-multiplication routines expect.
 *
 * \param[in] scalar The integer scalar.
 * \param[out] out A 32-byte buffer receiving the big-endian reduced scalar.
 *
 * \return \ref CARDANO_SUCCESS on success or a propagated allocation error.
 */
static cardano_error_t
bls_reduce_scalar(const cardano_bigint_t* scalar, byte_t* out)
{
  cardano_bigint_t* period    = NULL;
  cardano_bigint_t* reduced   = NULL;
  cardano_bigint_t* quotient  = NULL;
  cardano_error_t   error     = CARDANO_SUCCESS;
  size_t            byte_size = 0U;
  byte_t*           be_bytes  = NULL;
  size_t            i         = 0U;

  for (i = 0U; i < CARDANO_UPLC_BLS_SCALAR_SIZE; ++i)
  {
    out[i] = 0U;
  }

  error = cardano_bigint_from_bytes(CARDANO_UPLC_BLS_SCALAR_PERIOD, CARDANO_UPLC_BLS_SCALAR_SIZE, CARDANO_BYTE_ORDER_BIG_ENDIAN, &period);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_bigint_from_int(0, &reduced);

  if (error != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&period);

    return error;
  }

  error = cardano_bigint_from_int(0, &quotient);

  if (error != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&period);
    cardano_bigint_unref(&reduced);

    return error;
  }

  floor_div_mod(scalar, period, quotient, reduced);

  if (cardano_bigint_signum(reduced) == 0)
  {
    cardano_bigint_unref(&period);
    cardano_bigint_unref(&reduced);
    cardano_bigint_unref(&quotient);

    return CARDANO_SUCCESS;
  }

  byte_size = cardano_bigint_get_bytes_size(reduced);

  be_bytes = (byte_t*)_cardano_malloc(byte_size);

  if (be_bytes == NULL)
  {
    cardano_bigint_unref(&period);
    cardano_bigint_unref(&reduced);
    cardano_bigint_unref(&quotient);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  error = cardano_bigint_to_bytes(reduced, CARDANO_BYTE_ORDER_BIG_ENDIAN, be_bytes, byte_size);

  if (error == CARDANO_SUCCESS)
  {
    if (byte_size <= CARDANO_UPLC_BLS_SCALAR_SIZE)
    {
      const size_t offset = CARDANO_UPLC_BLS_SCALAR_SIZE - byte_size;

      for (i = 0U; i < byte_size; ++i)
      {
        out[offset + i] = be_bytes[i];
      }
    }
    else
    {
      const size_t offset = byte_size - CARDANO_UPLC_BLS_SCALAR_SIZE;

      for (i = 0U; i < CARDANO_UPLC_BLS_SCALAR_SIZE; ++i)
      {
        out[i] = be_bytes[offset + i];
      }
    }
  }

  _cardano_free(be_bytes);
  cardano_bigint_unref(&period);
  cardano_bigint_unref(&reduced);
  cardano_bigint_unref(&quotient);

  return error;
}

/**
 * \brief Runs \c bls12_381_G1_add: the group addition of two G1 points.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two G1 argument values.
 * \param[out] out_result On success, the G1 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_add(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  const void* b = NULL;
  blst_p1     out;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G1, &a) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G1, &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_p1_add_or_double(&out, (const blst_p1*)a, (const blst_p1*)b);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G1_neg: the group negation of a G1 point.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The G1 argument value.
 * \param[out] out_result On success, the G1 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_neg(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  blst_p1     out;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G1, &a))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  out = *(const blst_p1*)a;
  blst_p1_cneg(&out, true);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G1_scalarMul: scalar multiplication of a G1 point.
 *
 * The integer scalar is reduced modulo the group order before multiplication.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The scalar integer and the G1 point.
 * \param[out] out_result On success, the G1 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_scalar_mul(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t* scalar = NULL;
  const void*             point  = NULL;
  byte_t                  scalar_be[CARDANO_UPLC_BLS_SCALAR_SIZE];
  blst_scalar             blst_scalar_value;
  blst_p1                 out;

  if (!as_integer_big(arena, args[0], &scalar) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G1, &point))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = bls_reduce_scalar(scalar, scalar_be);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_scalar_from_bendian(&blst_scalar_value, scalar_be);
  blst_p1_mult(&out, (const blst_p1*)point, blst_scalar_value.b, CARDANO_UPLC_BLS_SCALAR_BITS);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G1_equal: equality of two G1 points.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two G1 argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_equal(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  const void* b = NULL;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G1, &a) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G1, &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bool(arena, blst_p1_is_equal((const blst_p1*)a, (const blst_p1*)b), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G1_compress: serializes a G1 point to 48 bytes.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The G1 argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_compress(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  byte_t      out[CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE];

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G1, &a))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_p1_compress(out, (const blst_p1*)a);

  *host_error = result_bytes_from(arena, out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G1_uncompress: parses 48 bytes into a G1 point.
 *
 * A wrong length, a bad encoding, or a point outside the subgroup is a script
 * error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The byte-string argument value.
 * \param[out] out_result On success, the G1 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_uncompress(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes    = { NULL, 0U };
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = CARDANO_SUCCESS;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  error = cardano_uplc_int_bls_g1_from_compressed(arena, nonnull_bytes(bytes.data, bytes.size), bytes.size, &constant);

  if (error == CARDANO_ERROR_DECODING)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (error != CARDANO_SUCCESS)
  {
    *host_error = error;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c bls12_381_G1_hashToGroup: hashes a message to a G1 point.
 *
 * The domain separation tag is capped at 255 bytes; a longer tag is a script
 * error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The message bytes and the domain separation tag bytes.
 * \param[out] out_result On success, the G1 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g1_hash_to_group(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t msg = { NULL, 0U };
  cardano_uplc_byte_view_t dst = { NULL, 0U };
  blst_p1                  out;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &msg) || !cardano_uplc_builtin_as_byte_string(args[1], &dst))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (dst.size > CARDANO_UPLC_BLS_DST_MAX)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_hash_to_g1(
    &out,
    nonnull_bytes(msg.data, msg.size),
    msg.size,
    nonnull_bytes(dst.data, dst.size),
    dst.size,
    NULL,
    0U);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G2_add: the group addition of two G2 points.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two G2 argument values.
 * \param[out] out_result On success, the G2 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_add(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  const void* b = NULL;
  blst_p2     out;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G2, &a) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G2, &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_p2_add_or_double(&out, (const blst_p2*)a, (const blst_p2*)b);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G2, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G2_neg: the group negation of a G2 point.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The G2 argument value.
 * \param[out] out_result On success, the G2 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_neg(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  blst_p2     out;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G2, &a))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  out = *(const blst_p2*)a;
  blst_p2_cneg(&out, true);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G2, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G2_scalarMul: scalar multiplication of a G2 point.
 *
 * The integer scalar is reduced modulo the group order before multiplication.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The scalar integer and the G2 point.
 * \param[out] out_result On success, the G2 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_scalar_mul(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t* scalar = NULL;
  const void*             point  = NULL;
  byte_t                  scalar_be[CARDANO_UPLC_BLS_SCALAR_SIZE];
  blst_scalar             blst_scalar_value;
  blst_p2                 out;

  if (!as_integer_big(arena, args[0], &scalar) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G2, &point))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = bls_reduce_scalar(scalar, scalar_be);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_scalar_from_bendian(&blst_scalar_value, scalar_be);
  blst_p2_mult(&out, (const blst_p2*)point, blst_scalar_value.b, CARDANO_UPLC_BLS_SCALAR_BITS);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G2, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G2_equal: equality of two G2 points.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two G2 argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_equal(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  const void* b = NULL;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G2, &a) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G2, &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bool(arena, blst_p2_is_equal((const blst_p2*)a, (const blst_p2*)b), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G2_compress: serializes a G2 point to 96 bytes.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The G2 argument value.
 * \param[out] out_result On success, the byte-string result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_compress(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  byte_t      out[CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE];

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G2, &a))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_p2_compress(out, (const blst_p2*)a);

  *host_error = result_bytes_from(arena, out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_G2_uncompress: parses 96 bytes into a G2 point.
 *
 * A wrong length, a bad encoding, or a point outside the subgroup is a script
 * error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The byte-string argument value.
 * \param[out] out_result On success, the G2 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_uncompress(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t bytes    = { NULL, 0U };
  cardano_uplc_constant_t* constant = NULL;
  cardano_uplc_value_t*    result   = NULL;
  cardano_error_t          error    = CARDANO_SUCCESS;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &bytes))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  error = cardano_uplc_int_bls_g2_from_compressed(arena, nonnull_bytes(bytes.data, bytes.size), bytes.size, &constant);

  if (error == CARDANO_ERROR_DECODING)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (error != CARDANO_SUCCESS)
  {
    *host_error = error;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c bls12_381_G2_hashToGroup: hashes a message to a G2 point.
 *
 * The domain separation tag is capped at 255 bytes; a longer tag is a script
 * error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The message bytes and the domain separation tag bytes.
 * \param[out] out_result On success, the G2 result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_g2_hash_to_group(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t msg = { NULL, 0U };
  cardano_uplc_byte_view_t dst = { NULL, 0U };
  blst_p2                  out;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &msg) || !cardano_uplc_builtin_as_byte_string(args[1], &dst))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (dst.size > CARDANO_UPLC_BLS_DST_MAX)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_hash_to_g2(
    &out,
    nonnull_bytes(msg.data, msg.size),
    msg.size,
    nonnull_bytes(dst.data, dst.size),
    dst.size,
    NULL,
    0U);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G2, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_millerLoop: the Miller loop of a G1 and a G2 point.
 *
 * Both points are converted to affine and passed to the Miller loop with the G2
 * point first, the argument order the BLS12-381 Miller loop requires.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The G1 point and the G2 point.
 * \param[out] out_result On success, the ML-result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_miller_loop(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void*    g1 = NULL;
  const void*    g2 = NULL;
  blst_p1_affine affine1;
  blst_p2_affine affine2;
  blst_fp12      out;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_G1, &g1) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_G2, &g2))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_p1_to_affine(&affine1, (const blst_p1*)g1);
  blst_p2_to_affine(&affine2, (const blst_p2*)g2);
  blst_miller_loop(&out, &affine2, &affine1);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_ML_RESULT, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_mulMlResult: multiplies two Miller-loop results.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two ML-result argument values.
 * \param[out] out_result On success, the ML-result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_mul_ml_result(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  const void* b = NULL;
  blst_fp12   out;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_ML_RESULT, &a) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_ML_RESULT, &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  blst_fp12_mul(&out, (const blst_fp12*)a, (const blst_fp12*)b);

  *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_ML_RESULT, &out, sizeof(out), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c bls12_381_finalVerify: the final-exponentiation pairing check.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two ML-result argument values.
 * \param[out] out_result On success, the boolean result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_final_verify(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const void* a = NULL;
  const void* b = NULL;

  if (!as_bls(args[0], CARDANO_UPLC_TYPE_BLS_ML_RESULT, &a) || !as_bls(args[1], CARDANO_UPLC_TYPE_BLS_ML_RESULT, &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_bool(arena, blst_fp12_finalverify((const blst_fp12*)a, (const blst_fp12*)b), out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/* ARRAY AND LIST V4 BODIES ************************************************/

/**
 * \brief Reads the items an array constant carries, rejecting any other value.
 *
 * The \c lengthOfArray and \c indexArray builtins accept only an array constant:
 * a list, even with identical contents, is a type mismatch.
 *
 * \param[in] value The argument value.
 * \param[out] element_type On success, the array element type, or NULL to ignore.
 * \param[out] items On success, the array items, or NULL to ignore.
 * \param[out] count On success, the item count, or NULL to ignore.
 *
 * \return \c true when \p value is an array constant, \c false otherwise.
 */
static bool
as_array(
  const cardano_uplc_value_t* value,
  const cardano_uplc_type_t** element_type,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_constant_t* const** items,
  size_t*                                count)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant))
  {
    return false;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_ARRAY)
  {
    return false;
  }

  if (element_type != NULL)
  {
    *element_type = constant->as.list.element_type;
  }

  if (items != NULL)
  {
    *items = constant->as.list.items;
  }

  if (count != NULL)
  {
    *count = constant->as.list.count;
  }

  return true;
}

/* VALUE BODIES **********************************************************/

/**
 * \brief The maximum byte length of a value policy or token key.
 */
static const size_t PRV_VALUE_KEY_MAX = 32U;

/**
 * \brief The bit width of the signed quantity bound: amounts fit in 128 bits.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t PRV_VALUE_AMOUNT_BITS = 128U;

/**
 * \brief Builds the canonical value element type from the arena.
 *
 * The element type is \c (pair bytestring (list (pair bytestring integer))).
 *
 * \param[in] arena The arena to allocate from.
 * \param[out] out On success, the element type descriptor.
 *
 * \return \ref CARDANO_SUCCESS or a propagated allocation error.
 */
static cardano_error_t
value_elem_type(struct cardano_uplc_arena_t* arena, const cardano_uplc_type_t** out)
{
  cardano_uplc_type_t* bytestring = NULL;
  cardano_uplc_type_t* integer    = NULL;
  cardano_uplc_type_t* token_pair = NULL;
  cardano_uplc_type_t* token_list = NULL;
  cardano_uplc_type_t* policy     = NULL;
  cardano_error_t      error      = CARDANO_SUCCESS;

  error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BYTE_STRING, NULL, NULL, &bytestring);

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, NULL, NULL, &integer);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, integer, &token_pair);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, token_pair, NULL, &token_list);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, token_list, &policy);
  }

  if (error == CARDANO_SUCCESS)
  {
    *out = policy;
  }

  return error;
}

/**
 * \brief Reads the value constant a value argument holds.
 *
 * \param[in] value The argument value.
 * \param[out] out On success, the value constant.
 *
 * \return \c true when \p value is a value constant, \c false otherwise.
 */
static bool
as_value_constant(const cardano_uplc_value_t* value, const cardano_uplc_constant_t** out)
{
  const cardano_uplc_constant_t* constant = NULL;

  if (!as_constant(value, &constant) || (constant == NULL) || (constant->kind != CARDANO_UPLC_TYPE_VALUE))
  {
    return false;
  }

  *out = constant;

  return true;
}

/**
 * \brief Reads the policy bytes and token list of a value policy entry.
 *
 * \param[in] entry The policy entry constant.
 * \param[out] policy On success, the policy byte view.
 * \param[out] tokens On success, the token-pair item array.
 * \param[out] token_count On success, the token count.
 *
 * \return \c true when the entry is well-formed, \c false otherwise.
 */
static bool
value_entry(
  const cardano_uplc_constant_t* entry,
  cardano_uplc_byte_view_t*      policy,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_constant_t* const** tokens,
  size_t*                                token_count)
{
  if ((entry == NULL) || (entry->kind != CARDANO_UPLC_TYPE_PAIR))
  {
    return false;
  }

  if ((entry->as.pair.fst == NULL) || (entry->as.pair.fst->kind != CARDANO_UPLC_TYPE_BYTE_STRING) || (entry->as.pair.snd == NULL) || (entry->as.pair.snd->kind != CARDANO_UPLC_TYPE_LIST))
  {
    return false;
  }

  policy->data = entry->as.pair.fst->as.bytes.data;
  policy->size = entry->as.pair.fst->as.bytes.size;
  *tokens      = entry->as.pair.snd->as.list.items;
  *token_count = entry->as.pair.snd->as.list.count;

  return true;
}

/**
 * \brief Reads the token bytes and amount of a token-pair constant.
 *
 * The amount is materialized to a \ref cardano_bigint_t through \p arena when it is
 * stored inline, so the multi-asset arithmetic can keep operating on bigints.
 *
 * \param[in] arena The arena an inline amount is materialized through.
 * \param[in] token The token-pair constant.
 * \param[out] name On success, the token byte view.
 * \param[out] amount On success, the amount bigint.
 *
 * \return \c true when the token pair is well-formed, \c false otherwise.
 */
static bool
value_token(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_constant_t* token,
  cardano_uplc_byte_view_t*      name,
  const cardano_bigint_t**       amount)
{
  if ((token == NULL) || (token->kind != CARDANO_UPLC_TYPE_PAIR))
  {
    return false;
  }

  if ((token->as.pair.fst == NULL) || (token->as.pair.fst->kind != CARDANO_UPLC_TYPE_BYTE_STRING) || (token->as.pair.snd == NULL) || (token->as.pair.snd->kind != CARDANO_UPLC_TYPE_INTEGER))
  {
    return false;
  }

  name->data = token->as.pair.fst->as.bytes.data;
  name->size = token->as.pair.fst->as.bytes.size;

  return constant_integer_big(arena, token->as.pair.snd, amount);
}

/**
 * \brief Compares two byte views lexicographically.
 *
 * \param[in] a The first view.
 * \param[in] b The second view.
 *
 * \return A negative, zero, or positive value as \p a is less than, equal to, or
 *         greater than \p b.
 */
static int
compare_buffers(cardano_uplc_byte_view_t a, cardano_uplc_byte_view_t b)
{
  return byte_view_compare(a.data, a.size, b.data, b.size);
}

/**
 * \brief Tests whether a quantity fits the signed 128-bit range.
 *
 * The valid range is \c -(2^127) .. \c (2^127 - 1). A magnitude under 128 bits is
 * always valid; exactly 128 bits is valid only for \c -(2^127).
 *
 * \param[in] amount The quantity to check.
 *
 * \return \c true when the quantity is in range, \c false otherwise.
 */
static bool
amount_in_range(const cardano_bigint_t* amount)
{
  cardano_bigint_t* one   = NULL;
  cardano_bigint_t* limit = NULL;
  bool              ok    = false;

  if (cardano_bigint_bit_length(amount) < PRV_VALUE_AMOUNT_BITS)
  {
    return true;
  }

  if (cardano_bigint_from_int(1, &one) != CARDANO_SUCCESS)
  {
    return false;
  }

  if (cardano_bigint_from_int(0, &limit) != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&one);

    return false;
  }

  cardano_bigint_shift_left(one, 127U, limit);

  if (cardano_bigint_signum(amount) >= 0)
  {
    ok = (cardano_bigint_compare(amount, limit) < 0);
  }
  else
  {
    cardano_bigint_t* neg_limit = NULL;

    if (cardano_bigint_from_int(0, &neg_limit) == CARDANO_SUCCESS)
    {
      cardano_bigint_negate(limit, neg_limit);
      ok = (cardano_bigint_compare(amount, neg_limit) >= 0);
      cardano_bigint_unref(&neg_limit);
    }
  }

  cardano_bigint_unref(&one);
  cardano_bigint_unref(&limit);

  return ok;
}

/**
 * \brief Builds a token-pair constant from a token buffer and an amount bigint.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in] name The token bytes; copied into the arena.
 * \param[in] amount The amount; the constructor clones it.
 * \param[out] out On success, the token-pair constant.
 *
 * \return \ref CARDANO_SUCCESS or a propagated allocation error.
 */
static cardano_error_t
make_token_pair(
  struct cardano_uplc_arena_t*    arena,
  cardano_uplc_byte_view_t        name,
  const cardano_bigint_t*         amount,
  const cardano_uplc_constant_t** out)
{
  cardano_bigint_t*        amount_copy = NULL;
  cardano_uplc_constant_t* name_const  = NULL;
  cardano_uplc_constant_t* amt_const   = NULL;
  cardano_uplc_constant_t* pair_const  = NULL;
  cardano_error_t          error       = CARDANO_SUCCESS;

  error = cardano_uplc_int_constant_new_byte_string_copy(arena, name.data, name.size, &name_const);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_bigint_clone(amount, &amount_copy);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_constant_new_integer(arena, amount_copy, &amt_const);
  cardano_bigint_unref(&amount_copy);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_constant_new_pair(arena, name_const, amt_const, &pair_const);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = pair_const;

  return CARDANO_SUCCESS;
}

/**
 * \brief Builds a policy-entry constant from a policy buffer and a token list.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in] policy The policy bytes; copied into the arena.
 * \param[in] tokens The token-pair item array.
 * \param[in] count The token count.
 * \param[out] out On success, the policy-entry constant.
 *
 * \return \ref CARDANO_SUCCESS or a propagated allocation error.
 */
static cardano_error_t
make_policy_entry(
  struct cardano_uplc_arena_t*          arena,
  cardano_uplc_byte_view_t              policy,
  const cardano_uplc_constant_t* const* tokens,
  size_t                                count,
  const cardano_uplc_constant_t**       out)
{
  cardano_uplc_constant_t* policy_const = NULL;
  cardano_uplc_type_t*     token_pair   = NULL;
  cardano_uplc_type_t*     bytestring   = NULL;
  cardano_uplc_type_t*     integer      = NULL;
  cardano_uplc_constant_t* list_const   = NULL;
  cardano_uplc_constant_t* pair_const   = NULL;
  cardano_error_t          error        = CARDANO_SUCCESS;

  error = cardano_uplc_int_constant_new_byte_string_copy(arena, policy.data, policy.size, &policy_const);

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BYTE_STRING, NULL, NULL, &bytestring);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, NULL, NULL, &integer);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, bytestring, integer, &token_pair);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_constant_new_list(arena, token_pair, tokens, count, &list_const);
  }

  if (error == CARDANO_SUCCESS)
  {
    error = cardano_uplc_constant_new_pair(arena, policy_const, list_const, &pair_const);
  }

  if (error == CARDANO_SUCCESS)
  {
    *out = pair_const;
  }

  return error;
}

/**
 * \brief Allocates an arena array of constant pointers.
 *
 * \param[in] arena The arena to allocate from.
 * \param[in] count The number of pointers.
 *
 * \return The array, or NULL on allocation failure (or NULL for a zero count).
 */
static const cardano_uplc_constant_t**
alloc_const_array(struct cardano_uplc_arena_t* arena, size_t count)
{
  if (count == 0U)
  {
    return NULL;
  }

  return (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, sizeof(const cardano_uplc_constant_t*) * count, 0U);
}

/**
 * \brief Wraps a normalized policy-entry array as a value result.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] entries The policy-entry item array.
 * \param[in] count The number of policy entries.
 * \param[out] out_result On success, the value result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
value_result(
  struct cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* const* entries,
  size_t                                count,
  const cardano_uplc_value_t**          out_result,
  cardano_error_t*                      host_error)
{
  const cardano_uplc_type_t* elem_type = NULL;
  cardano_uplc_constant_t*   constant  = NULL;
  cardano_uplc_value_t*      result    = NULL;

  *host_error = value_elem_type(arena, &elem_type);

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_constant_new_value(arena, elem_type, entries, count, &constant);
  }

  if (*host_error == CARDANO_SUCCESS)
  {
    *host_error = cardano_uplc_value_new_constant(arena, constant, &result);
  }

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c insertCoin: sets a coin in a value, deleting it when the amount is zero.
 *
 * Enforces the signed 128-bit amount range and the 32-byte key length (oversize
 * keys are allowed only for a zero amount, which is a no-op). Maintains ascending
 * policy and token order with no duplicates and no zero quantities.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The policy, token, amount and value arguments.
 * \param[out] out_result On success, the value result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_insert_coin(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t        ccy       = { NULL, 0U };
  cardano_uplc_byte_view_t        tok       = { NULL, 0U };
  const cardano_bigint_t*         amount    = NULL;
  const cardano_uplc_constant_t*  value     = NULL;
  const cardano_uplc_constant_t** entries   = NULL;
  size_t                          out_count = 0U;
  size_t                          i         = 0U;
  bool                            is_zero   = false;
  bool                            found_ccy = false;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &ccy) || !cardano_uplc_builtin_as_byte_string(args[1], &tok) || !as_integer_big(arena, args[2], &amount) || !as_value_constant(args[3], &value))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  is_zero = cardano_bigint_is_zero(amount);

  if ((ccy.size > PRV_VALUE_KEY_MAX) || (tok.size > PRV_VALUE_KEY_MAX))
  {
    if (!is_zero)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *out_result = args[3];

    return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
  }

  if (!is_zero && !amount_in_range(amount))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  entries = alloc_const_array(arena, value->as.list.count + 1U);

  if (entries == NULL)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; i < value->as.list.count; ++i)
  {
    cardano_uplc_byte_view_t              policy = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens = NULL;
    size_t                                tcount = 0U;
    int                                   cmp    = 0;

    if (!value_entry(value->as.list.items[i], &policy, &tokens, &tcount))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cmp = compare_buffers(policy, ccy);

    if (cmp < 0)
    {
      entries[out_count] = value->as.list.items[i];
      out_count          += 1U;
    }
    else if (cmp == 0)
    {
      const cardano_uplc_constant_t** new_tokens = alloc_const_array(arena, tcount + 1U);
      size_t                          tout       = 0U;
      size_t                          j          = 0U;
      bool                            found_tok  = false;

      found_ccy = true;

      if (new_tokens == NULL)
      {
        *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      for (j = 0U; j < tcount; ++j)
      {
        cardano_uplc_byte_view_t name = { NULL, 0U };
        const cardano_bigint_t*  amt  = NULL;
        int                      tcmp = 0;

        if (!value_token(arena, tokens[j], &name, &amt))
        {
          return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
        }

        tcmp = compare_buffers(name, tok);

        if (tcmp < 0)
        {
          new_tokens[tout] = tokens[j];
          tout             += 1U;
        }
        else if (tcmp == 0)
        {
          found_tok = true;

          if (!is_zero)
          {
            *host_error = make_token_pair(arena, tok, amount, &new_tokens[tout]);

            if (*host_error != CARDANO_SUCCESS)
            {
              return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
            }

            tout += 1U;
          }
        }
        else
        {
          if (!found_tok)
          {
            found_tok = true;

            if (!is_zero)
            {
              *host_error = make_token_pair(arena, tok, amount, &new_tokens[tout]);

              if (*host_error != CARDANO_SUCCESS)
              {
                return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
              }

              tout += 1U;
            }
          }

          new_tokens[tout] = tokens[j];
          tout             += 1U;
        }
      }

      if (!found_tok && !is_zero)
      {
        *host_error = make_token_pair(arena, tok, amount, &new_tokens[tout]);

        if (*host_error != CARDANO_SUCCESS)
        {
          return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
        }

        tout += 1U;
      }

      if (tout > 0U)
      {
        *host_error = make_policy_entry(arena, policy, new_tokens, tout, &entries[out_count]);

        if (*host_error != CARDANO_SUCCESS)
        {
          return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
        }

        out_count += 1U;
      }
    }
    else
    {
      if (!found_ccy)
      {
        found_ccy = true;

        if (!is_zero)
        {
          const cardano_uplc_constant_t** single = alloc_const_array(arena, 1U);

          if (single == NULL)
          {
            *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

            return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
          }

          *host_error = make_token_pair(arena, tok, amount, &single[0]);

          if (*host_error == CARDANO_SUCCESS)
          {
            *host_error = make_policy_entry(arena, ccy, single, 1U, &entries[out_count]);
          }

          if (*host_error != CARDANO_SUCCESS)
          {
            return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
          }

          out_count += 1U;
        }
      }

      entries[out_count] = value->as.list.items[i];
      out_count          += 1U;
    }
  }

  if (!found_ccy && !is_zero)
  {
    const cardano_uplc_constant_t** single = alloc_const_array(arena, 1U);

    if (single == NULL)
    {
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *host_error = make_token_pair(arena, tok, amount, &single[0]);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = make_policy_entry(arena, ccy, single, 1U, &entries[out_count]);
    }

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    out_count += 1U;
  }

  return value_result(arena, entries, out_count, out_result, host_error);
}

/**
 * \brief Runs \c lookupCoin: the amount of a coin in a value, or zero if absent.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The policy, token and value arguments.
 * \param[out] out_result On success, the integer result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_lookup_coin(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  cardano_uplc_byte_view_t       ccy   = { NULL, 0U };
  cardano_uplc_byte_view_t       tok   = { NULL, 0U };
  const cardano_uplc_constant_t* value = NULL;
  const cardano_bigint_t*        found = NULL;
  size_t                         i     = 0U;

  if (!cardano_uplc_builtin_as_byte_string(args[0], &ccy) || !cardano_uplc_builtin_as_byte_string(args[1], &tok) || !as_value_constant(args[2], &value))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; (i < value->as.list.count) && (found == NULL); ++i)
  {
    cardano_uplc_byte_view_t              policy = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens = NULL;
    size_t                                tcount = 0U;
    int                                   cmp    = 0;

    if (!value_entry(value->as.list.items[i], &policy, &tokens, &tcount))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cmp = compare_buffers(policy, ccy);

    if (cmp == 0)
    {
      size_t j = 0U;

      for (j = 0U; (j < tcount) && (found == NULL); ++j)
      {
        cardano_uplc_byte_view_t name = { NULL, 0U };
        const cardano_bigint_t*  amt  = NULL;

        if (!value_token(arena, tokens[j], &name, &amt))
        {
          return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
        }

        if (compare_buffers(name, tok) == 0)
        {
          found = amt;
        }
      }
    }
  }

  if (found != NULL)
  {
    cardano_bigint_t* clone = NULL;

    *host_error = cardano_bigint_clone(found, &clone);

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *host_error = result_integer(arena, clone, out_result);

    return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_integer_from_int(arena, 0, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c scaleValue: multiplies every amount in a value by a scalar.
 *
 * A zero scalar yields the empty value. The scalar and every product must fit the
 * signed 128-bit range.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The scalar and the value arguments.
 * \param[out] out_result On success, the value result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_scale_value(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t*         scalar    = NULL;
  const cardano_uplc_constant_t*  value     = NULL;
  const cardano_uplc_constant_t** entries   = NULL;
  size_t                          out_count = 0U;
  size_t                          i         = 0U;

  if (!as_integer_big(arena, args[0], &scalar) || !as_value_constant(args[1], &value))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_is_zero(scalar))
  {
    return value_result(arena, NULL, 0U, out_result, host_error);
  }

  if (!amount_in_range(scalar))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  entries = alloc_const_array(arena, value->as.list.count);

  if ((value->as.list.count > 0U) && (entries == NULL))
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; i < value->as.list.count; ++i)
  {
    cardano_uplc_byte_view_t              policy     = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens     = NULL;
    size_t                                tcount     = 0U;
    const cardano_uplc_constant_t**       new_tokens = NULL;
    size_t                                tout       = 0U;
    size_t                                j          = 0U;

    if (!value_entry(value->as.list.items[i], &policy, &tokens, &tcount))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    new_tokens = alloc_const_array(arena, tcount);

    if ((tcount > 0U) && (new_tokens == NULL))
    {
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    for (j = 0U; j < tcount; ++j)
    {
      cardano_uplc_byte_view_t name    = { NULL, 0U };
      const cardano_bigint_t*  amt     = NULL;
      cardano_bigint_t*        product = NULL;

      if (!value_token(arena, tokens[j], &name, &amt))
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      if (cardano_bigint_from_int(0, &product) != CARDANO_SUCCESS)
      {
        *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      cardano_bigint_multiply(amt, scalar, product);

      if (!amount_in_range(product))
      {
        cardano_bigint_unref(&product);

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      *host_error = make_token_pair(arena, name, product, &new_tokens[tout]);
      cardano_bigint_unref(&product);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      tout += 1U;
    }

    if (tout > 0U)
    {
      *host_error = make_policy_entry(arena, policy, new_tokens, tout, &entries[out_count]);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      out_count += 1U;
    }
  }

  return value_result(arena, entries, out_count, out_result, host_error);
}

/**
 * \brief Merges two sorted token lists by adding amounts, dropping zeros.
 *
 * Each summed amount must fit the signed 128-bit range.
 *
 * \param[in] arena The arena the merged tokens are allocated from.
 * \param[in] t1 The first token-pair array.
 * \param[in] n1 The length of \p t1.
 * \param[in] t2 The second token-pair array.
 * \param[in] n2 The length of \p t2.
 * \param[out] out On success, the merged token array.
 * \param[out] out_count On success, the merged length.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return \c true on success, \c false on a range or allocation failure.
 */
static bool
merge_tokens(
  struct cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* const* t1,
  size_t                                n1,
  const cardano_uplc_constant_t* const* t2,
  size_t                                n2,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_constant_t*** out,
  size_t*                          out_count,
  cardano_error_t*                 host_error)
{
  const cardano_uplc_constant_t** merged = alloc_const_array(arena, n1 + n2);
  size_t                          count  = 0U;
  size_t                          i      = 0U;
  size_t                          j      = 0U;

  if (((n1 + n2) > 0U) && (merged == NULL))
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return false;
  }

  while ((i < n1) && (j < n2))
  {
    cardano_uplc_byte_view_t name1 = { NULL, 0U };
    const cardano_bigint_t*  amt1  = NULL;
    cardano_uplc_byte_view_t name2 = { NULL, 0U };
    const cardano_bigint_t*  amt2  = NULL;
    int                      cmp   = 0;

    if (!value_token(arena, t1[i], &name1, &amt1) || !value_token(arena, t2[j], &name2, &amt2))
    {
      return false;
    }

    cmp = compare_buffers(name1, name2);

    if (cmp < 0)
    {
      merged[count] = t1[i];
      count         += 1U;
      i             += 1U;
    }
    else if (cmp > 0)
    {
      merged[count] = t2[j];
      count         += 1U;
      j             += 1U;
    }
    else
    {
      cardano_bigint_t* sum = NULL;

      if (cardano_bigint_from_int(0, &sum) != CARDANO_SUCCESS)
      {
        *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

        return false;
      }

      cardano_bigint_add(amt1, amt2, sum);

      if (!amount_in_range(sum))
      {
        cardano_bigint_unref(&sum);

        return false;
      }

      if (!cardano_bigint_is_zero(sum))
      {
        *host_error = make_token_pair(arena, name1, sum, &merged[count]);

        if (*host_error != CARDANO_SUCCESS)
        {
          cardano_bigint_unref(&sum);

          return false;
        }

        count += 1U;
      }

      cardano_bigint_unref(&sum);
      i += 1U;
      j += 1U;
    }
  }

  while (i < n1)
  {
    merged[count] = t1[i];
    count         += 1U;
    i             += 1U;
  }

  while (j < n2)
  {
    merged[count] = t2[j];
    count         += 1U;
    j             += 1U;
  }

  *out       = merged;
  *out_count = count;

  return true;
}

/**
 * \brief Runs \c unionValue: merges two values by adding matching amounts.
 *
 * Performs a sorted merge of the policy lists that sums shared tokens, drops zero
 * results and range-checks every sum.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The two value arguments.
 * \param[out] out_result On success, the value result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_union_value(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t*  a         = NULL;
  const cardano_uplc_constant_t*  b         = NULL;
  const cardano_uplc_constant_t** entries   = NULL;
  size_t                          out_count = 0U;
  size_t                          i         = 0U;
  size_t                          j         = 0U;

  if (!as_value_constant(args[0], &a) || !as_value_constant(args[1], &b))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  entries = alloc_const_array(arena, a->as.list.count + b->as.list.count);

  if (((a->as.list.count + b->as.list.count) > 0U) && (entries == NULL))
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  while ((i < a->as.list.count) && (j < b->as.list.count))
  {
    cardano_uplc_byte_view_t              pa  = { NULL, 0U };
    const cardano_uplc_constant_t* const* ta  = NULL;
    size_t                                na  = 0U;
    cardano_uplc_byte_view_t              pb  = { NULL, 0U };
    const cardano_uplc_constant_t* const* tb  = NULL;
    size_t                                nb  = 0U;
    int                                   cmp = 0;

    if (!value_entry(a->as.list.items[i], &pa, &ta, &na) || !value_entry(b->as.list.items[j], &pb, &tb, &nb))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    cmp = compare_buffers(pa, pb);

    if (cmp < 0)
    {
      entries[out_count] = a->as.list.items[i];
      out_count          += 1U;
      i                  += 1U;
    }
    else if (cmp > 0)
    {
      entries[out_count] = b->as.list.items[j];
      out_count          += 1U;
      j                  += 1U;
    }
    else
    {
      const cardano_uplc_constant_t** merged = NULL;
      size_t                          mcount = 0U;

      if (!merge_tokens(arena, ta, na, tb, nb, &merged, &mcount, host_error))
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      if (mcount > 0U)
      {
        *host_error = make_policy_entry(arena, pa, merged, mcount, &entries[out_count]);

        if (*host_error != CARDANO_SUCCESS)
        {
          return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
        }

        out_count += 1U;
      }

      i += 1U;
      j += 1U;
    }
  }

  while (i < a->as.list.count)
  {
    entries[out_count] = a->as.list.items[i];
    out_count          += 1U;
    i                  += 1U;
  }

  while (j < b->as.list.count)
  {
    entries[out_count] = b->as.list.items[j];
    out_count          += 1U;
    j                  += 1U;
  }

  return value_result(arena, entries, out_count, out_result, host_error);
}

/**
 * \brief Tests whether a value carries any negative amount.
 *
 * \param[in] value The value constant.
 * \param[out] malformed Set to \c true when the structure is malformed.
 *
 * \return \c true when a negative amount is present.
 */
static bool
value_has_negative(struct cardano_uplc_arena_t* arena, const cardano_uplc_constant_t* value, bool* malformed)
{
  size_t i = 0U;

  for (i = 0U; i < value->as.list.count; ++i)
  {
    cardano_uplc_byte_view_t              policy = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens = NULL;
    size_t                                tcount = 0U;
    size_t                                j      = 0U;

    if (!value_entry(value->as.list.items[i], &policy, &tokens, &tcount))
    {
      *malformed = true;

      return false;
    }

    for (j = 0U; j < tcount; ++j)
    {
      cardano_uplc_byte_view_t name = { NULL, 0U };
      const cardano_bigint_t*  amt  = NULL;

      if (!value_token(arena, tokens[j], &name, &amt))
      {
        *malformed = true;

        return false;
      }

      if (cardano_bigint_signum(amt) < 0)
      {
        return true;
      }
    }
  }

  return false;
}

/**
 * \brief Looks up an amount in a value, returning whether it was found.
 *
 * \param[in] value The value constant.
 * \param[in] ccy The policy byte view.
 * \param[in] tok The token byte view.
 * \param[out] out On success and when found, the amount bigint.
 *
 * \return \c true when the (policy, token) pair is present.
 */
static bool
value_lookup(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_constant_t* value,
  cardano_uplc_byte_view_t       ccy,
  cardano_uplc_byte_view_t       tok,
  const cardano_bigint_t**       out)
{
  size_t i = 0U;

  for (i = 0U; i < value->as.list.count; ++i)
  {
    cardano_uplc_byte_view_t              policy = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens = NULL;
    size_t                                tcount = 0U;

    if (!value_entry(value->as.list.items[i], &policy, &tokens, &tcount))
    {
      return false;
    }

    if (compare_buffers(policy, ccy) == 0)
    {
      size_t j = 0U;

      for (j = 0U; j < tcount; ++j)
      {
        cardano_uplc_byte_view_t name = { NULL, 0U };
        const cardano_bigint_t*  amt  = NULL;

        if (!value_token(arena, tokens[j], &name, &amt))
        {
          return false;
        }

        if (compare_buffers(name, tok) == 0)
        {
          *out = amt;

          return true;
        }
      }
    }
  }

  return false;
}

/**
 * \brief Runs \c valueContains: whether the first value covers the second.
 *
 * Both values must be free of negative amounts (a negative is a script error).
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The covering value and the required value.
 * \param[out] out_result On success, the boolean result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_value_contains(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* covering  = NULL;
  const cardano_uplc_constant_t* required  = NULL;
  bool                           malformed = false;
  bool                           contains  = true;
  size_t                         i         = 0U;

  if (!as_value_constant(args[0], &covering) || !as_value_constant(args[1], &required))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (value_has_negative(arena, required, &malformed) || malformed)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (value_has_negative(arena, covering, &malformed) || malformed)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; (i < required->as.list.count) && contains; ++i)
  {
    cardano_uplc_byte_view_t              policy = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens = NULL;
    size_t                                tcount = 0U;
    size_t                                j      = 0U;

    if (!value_entry(required->as.list.items[i], &policy, &tokens, &tcount))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    for (j = 0U; (j < tcount) && contains; ++j)
    {
      cardano_uplc_byte_view_t name      = { NULL, 0U };
      const cardano_bigint_t*  needed    = NULL;
      const cardano_bigint_t*  available = NULL;

      if (!value_token(arena, tokens[j], &name, &needed))
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      if (!value_lookup(arena, covering, policy, name, &available) || (cardano_bigint_compare(available, needed) < 0))
      {
        contains = false;
      }
    }
  }

  *host_error = result_bool(arena, contains, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c valueData: encodes a value as Data Map of policy to token map.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The value argument.
 * \param[out] out_result On success, the data result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_value_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* value = NULL;
  cardano_plutus_map_t*          outer = NULL;
  cardano_plutus_data_t*         data  = NULL;
  size_t                         i     = 0U;

  if (!as_value_constant(args[0], &value))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_plutus_map_new(&outer) != CARDANO_SUCCESS)
  {
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; i < value->as.list.count; ++i)
  {
    cardano_uplc_byte_view_t              policy   = { NULL, 0U };
    const cardano_uplc_constant_t* const* tokens   = NULL;
    size_t                                tcount   = 0U;
    cardano_plutus_map_t*                 inner    = NULL;
    cardano_plutus_data_t*                key_data = NULL;
    cardano_plutus_data_t*                val_data = NULL;
    size_t                                j        = 0U;

    if (!value_entry(value->as.list.items[i], &policy, &tokens, &tcount))
    {
      cardano_plutus_map_unref(&outer);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (cardano_plutus_map_new(&inner) != CARDANO_SUCCESS)
    {
      cardano_plutus_map_unref(&outer);
      *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    for (j = 0U; j < tcount; ++j)
    {
      cardano_uplc_byte_view_t name     = { NULL, 0U };
      const cardano_bigint_t*  amt      = NULL;
      cardano_plutus_data_t*   tok_data = NULL;
      cardano_plutus_data_t*   amt_data = NULL;
      cardano_bigint_t*        amt_copy = NULL;

      if (!value_token(arena, tokens[j], &name, &amt))
      {
        cardano_plutus_map_unref(&inner);
        cardano_plutus_map_unref(&outer);

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      {
        static const byte_t empty = 0;

        *host_error = cardano_plutus_data_new_bytes((name.data != NULL) ? name.data : &empty, name.size, &tok_data);
      }

      if (*host_error == CARDANO_SUCCESS)
      {
        *host_error = cardano_bigint_clone(amt, &amt_copy);
      }

      if (*host_error == CARDANO_SUCCESS)
      {
        *host_error = cardano_plutus_data_new_integer(amt_copy, &amt_data);
        cardano_bigint_unref(&amt_copy);
      }

      if (*host_error == CARDANO_SUCCESS)
      {
        *host_error = cardano_plutus_map_insert(inner, tok_data, amt_data);
      }

      cardano_plutus_data_unref(&tok_data);
      cardano_plutus_data_unref(&amt_data);

      if (*host_error != CARDANO_SUCCESS)
      {
        cardano_plutus_map_unref(&inner);
        cardano_plutus_map_unref(&outer);

        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }
    }

    {
      static const byte_t empty = 0;

      *host_error = cardano_plutus_data_new_bytes((policy.data != NULL) ? policy.data : &empty, policy.size, &key_data);
    }

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_plutus_data_new_map(inner, &val_data);
    }

    cardano_plutus_map_unref(&inner);

    if (*host_error == CARDANO_SUCCESS)
    {
      *host_error = cardano_plutus_map_insert(outer, key_data, val_data);
    }

    cardano_plutus_data_unref(&key_data);
    cardano_plutus_data_unref(&val_data);

    if (*host_error != CARDANO_SUCCESS)
    {
      cardano_plutus_map_unref(&outer);

      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }
  }

  *host_error = cardano_plutus_data_new_map(outer, &data);
  cardano_plutus_map_unref(&outer);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_data(arena, data, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Decodes one data-map token entry into a token-pair constant.
 *
 * Enforces a byte-string key of at most 32 bytes that ascends strictly past
 * \p prev, a non-zero integer amount in the signed 128-bit range, and writes the
 * token bytes used for the next ordering check.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in] key The token-key data.
 * \param[in] val The amount data.
 * \param[in] prev The previous token bytes, or NULL for the first entry.
 * \param[out] out On success, the token-pair constant.
 * \param[out] next On success, the bytes of \p key.
 *
 * \return \c true on success, \c false on a validation or allocation failure.
 */
static bool
decode_token(
  struct cardano_uplc_arena_t*    arena,
  cardano_plutus_data_t*          key,
  cardano_plutus_data_t*          val,
  const cardano_buffer_t*         prev,
  const cardano_uplc_constant_t** out,
  cardano_buffer_t**              next)
{
  cardano_plutus_data_kind_t key_kind = CARDANO_PLUTUS_DATA_KIND_INTEGER;
  cardano_plutus_data_kind_t val_kind = CARDANO_PLUTUS_DATA_KIND_INTEGER;
  cardano_buffer_t*          name     = NULL;
  cardano_bigint_t*          amount   = NULL;
  bool                       ok       = false;

  if ((cardano_plutus_data_get_kind(key, &key_kind) != CARDANO_SUCCESS) || (key_kind != CARDANO_PLUTUS_DATA_KIND_BYTES))
  {
    return false;
  }

  if ((cardano_plutus_data_get_kind(val, &val_kind) != CARDANO_SUCCESS) || (val_kind != CARDANO_PLUTUS_DATA_KIND_INTEGER))
  {
    return false;
  }

  if (cardano_plutus_data_to_bounded_bytes(key, &name) != CARDANO_SUCCESS)
  {
    return false;
  }

  if (cardano_plutus_data_to_integer(val, &amount) != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&name);

    return false;
  }

  {
    cardano_uplc_byte_view_t name_view = { cardano_buffer_get_data(name), cardano_buffer_get_size(name) };
    cardano_uplc_byte_view_t prev_view = { NULL, 0U };

    if (prev != NULL)
    {
      prev_view.data = cardano_buffer_get_data(prev);
      prev_view.size = cardano_buffer_get_size(prev);
    }

    if ((name_view.size <= PRV_VALUE_KEY_MAX) && ((prev == NULL) || (compare_buffers(prev_view, name_view) < 0)) && !cardano_bigint_is_zero(amount) && amount_in_range(amount))
    {
      cardano_buffer_t* name_copy = cardano_buffer_new_from(name_view.data, name_view.size);

      if (name_copy != NULL)
      {
        ok = (make_token_pair(arena, name_view, amount, out) == CARDANO_SUCCESS);

        if (ok)
        {
          *next = name_copy;
        }
        else
        {
          cardano_buffer_unref(&name_copy);
        }
      }
    }
  }

  cardano_bigint_unref(&amount);
  cardano_buffer_unref(&name);

  return ok;
}

/**
 * \brief Runs \c unValueData: decodes a Data Map into a value, validating it.
 *
 * Requires a Map whose policy keys are byte strings of at most 32 bytes in strict
 * ascending order, each mapped to a non-empty token Map with ascending token keys
 * of at most 32 bytes and non-zero amounts in the signed 128-bit range. Any
 * violation is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The data argument.
 * \param[out] out_result On success, the value result.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_un_value_data(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_data_t*      node     = NULL;
  cardano_plutus_data_t*          data     = NULL;
  cardano_plutus_data_kind_t      kind     = CARDANO_PLUTUS_DATA_KIND_INTEGER;
  cardano_plutus_map_t*           outer    = NULL;
  cardano_plutus_list_t*          keys     = NULL;
  cardano_plutus_list_t*          values   = NULL;
  const cardano_uplc_constant_t** entries  = NULL;
  cardano_buffer_t*               prev_ccy = NULL;
  size_t                          count    = 0U;
  size_t                          i        = 0U;
  bool                            failed   = false;

  if (!cardano_uplc_builtin_as_data(args[0], &node))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (node->kind != CARDANO_UPLC_DATA_KIND_MAP)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_data_to_plutus_data(node, &data);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((cardano_plutus_data_get_kind(data, &kind) != CARDANO_SUCCESS) || (kind != CARDANO_PLUTUS_DATA_KIND_MAP))
  {
    cardano_plutus_data_unref(&data);

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_plutus_data_to_map(data, &outer) != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&data);

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if ((cardano_plutus_map_get_keys(outer, &keys) != CARDANO_SUCCESS) || (cardano_plutus_map_get_values(outer, &values) != CARDANO_SUCCESS))
  {
    cardano_plutus_list_unref(&keys);
    cardano_plutus_list_unref(&values);
    cardano_plutus_map_unref(&outer);
    cardano_plutus_data_unref(&data);

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  cardano_plutus_map_unref(&outer);

  count   = cardano_plutus_list_get_length(keys);
  entries = alloc_const_array(arena, count);

  if ((count > 0U) && (entries == NULL))
  {
    cardano_plutus_list_unref(&keys);
    cardano_plutus_list_unref(&values);
    cardano_plutus_data_unref(&data);
    *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  for (i = 0U; (i < count) && !failed; ++i)
  {
    cardano_plutus_data_t*          key_data   = NULL;
    cardano_plutus_data_t*          val_data   = NULL;
    cardano_plutus_data_kind_t      key_kind   = CARDANO_PLUTUS_DATA_KIND_INTEGER;
    cardano_plutus_data_kind_t      val_kind   = CARDANO_PLUTUS_DATA_KIND_INTEGER;
    cardano_buffer_t*               policy     = NULL;
    cardano_plutus_map_t*           inner      = NULL;
    cardano_plutus_list_t*          tok_keys   = NULL;
    cardano_plutus_list_t*          tok_values = NULL;
    const cardano_uplc_constant_t** tokens     = NULL;
    cardano_buffer_t*               prev_tok   = NULL;
    size_t                          tcount     = 0U;
    size_t                          j          = 0U;

    if ((cardano_plutus_list_get(keys, i, &key_data) != CARDANO_SUCCESS) || (cardano_plutus_list_get(values, i, &val_data) != CARDANO_SUCCESS))
    {
      cardano_plutus_data_unref(&key_data);
      failed = true;
      break;
    }

    if ((cardano_plutus_data_get_kind(key_data, &key_kind) != CARDANO_SUCCESS) || (key_kind != CARDANO_PLUTUS_DATA_KIND_BYTES) || (cardano_plutus_data_to_bounded_bytes(key_data, &policy) != CARDANO_SUCCESS))
    {
      failed = true;
    }

    if (!failed && (cardano_buffer_get_size(policy) > PRV_VALUE_KEY_MAX))
    {
      failed = true;
    }

    if (!failed && (prev_ccy != NULL))
    {
      cardano_uplc_byte_view_t prev_view   = { cardano_buffer_get_data(prev_ccy), cardano_buffer_get_size(prev_ccy) };
      cardano_uplc_byte_view_t policy_view = { cardano_buffer_get_data(policy), cardano_buffer_get_size(policy) };

      if (compare_buffers(prev_view, policy_view) >= 0)
      {
        failed = true;
      }
    }

    if (!failed)
    {
      if ((cardano_plutus_data_get_kind(val_data, &val_kind) != CARDANO_SUCCESS) || (val_kind != CARDANO_PLUTUS_DATA_KIND_MAP) || (cardano_plutus_data_to_map(val_data, &inner) != CARDANO_SUCCESS))
      {
        failed = true;
      }
    }

    if (!failed && ((cardano_plutus_map_get_keys(inner, &tok_keys) != CARDANO_SUCCESS) || (cardano_plutus_map_get_values(inner, &tok_values) != CARDANO_SUCCESS)))
    {
      failed = true;
    }

    cardano_plutus_map_unref(&inner);

    if (!failed)
    {
      tcount = cardano_plutus_list_get_length(tok_keys);

      if (tcount == 0U)
      {
        failed = true;
      }
    }

    if (!failed)
    {
      tokens = alloc_const_array(arena, tcount);

      if (tokens == NULL)
      {
        *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        failed      = true;
      }
    }

    for (j = 0U; (j < tcount) && !failed; ++j)
    {
      cardano_plutus_data_t* tk   = NULL;
      cardano_plutus_data_t* tv   = NULL;
      cardano_buffer_t*      next = NULL;

      if ((cardano_plutus_list_get(tok_keys, j, &tk) != CARDANO_SUCCESS) || (cardano_plutus_list_get(tok_values, j, &tv) != CARDANO_SUCCESS))
      {
        cardano_plutus_data_unref(&tk);
        failed = true;
      }
      else if (!decode_token(arena, tk, tv, prev_tok, &tokens[j], &next))
      {
        failed = true;
      }
      else
      {
        cardano_buffer_unref(&prev_tok);
        prev_tok = next;
      }

      cardano_plutus_data_unref(&tk);
      cardano_plutus_data_unref(&tv);
    }

    cardano_buffer_unref(&prev_tok);
    cardano_plutus_list_unref(&tok_keys);
    cardano_plutus_list_unref(&tok_values);

    if (!failed)
    {
      cardano_uplc_byte_view_t policy_view = { cardano_buffer_get_data(policy), cardano_buffer_get_size(policy) };

      *host_error = make_policy_entry(arena, policy_view, tokens, tcount, &entries[i]);

      if (*host_error != CARDANO_SUCCESS)
      {
        failed = true;
      }
      else
      {
        cardano_buffer_unref(&prev_ccy);
        prev_ccy = cardano_buffer_new_from(cardano_buffer_get_data(policy), cardano_buffer_get_size(policy));

        if (prev_ccy == NULL)
        {
          *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
          failed      = true;
        }
      }
    }

    cardano_buffer_unref(&policy);
    cardano_plutus_data_unref(&key_data);
    cardano_plutus_data_unref(&val_data);
  }

  cardano_buffer_unref(&prev_ccy);
  cardano_plutus_list_unref(&keys);
  cardano_plutus_list_unref(&values);
  cardano_plutus_data_unref(&data);

  if (failed)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  return value_result(arena, entries, count, out_result, host_error);
}

/**
 * \brief Runs \c dropList: a list without its first \c n elements.
 *
 * The first argument is the count to drop and the second is the list. A negative
 * count drops nothing; a count at or above the length yields the empty list. The
 * result keeps the list's element type.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The count and the list.
 * \param[out] out_result On success, the resulting list value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_drop_list(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_bigint_t*               n            = NULL;
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  size_t                                start        = 0U;
  cardano_uplc_constant_t*              constant     = NULL;
  cardano_uplc_value_t*                 result       = NULL;

  if (!as_integer_big(arena, args[0], &n) || !cardano_uplc_builtin_as_list(args[1], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_signum(n) <= 0)
  {
    start = 0U;
  }
  else if (cardano_bigint_bit_length(n) > 63U)
  {
    start = count;
  }
  else
  {
    const int64_t n64 = cardano_bigint_to_int(n);

    start = ((uint64_t)n64 >= (uint64_t)count) ? count : (size_t)n64;
  }

  *host_error = cardano_uplc_constant_new_list(
    arena,
    element_type,
    // cppcheck-suppress misra-c2012-12.1; Reason: operator precedence is explicit and intentional
    // cppcheck-suppress misra-c2012-18.4; Reason: pointer arithmetic over a contiguous arena buffer
    (start < count) ? (items + start) : NULL,
    count - start,
    &constant);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c lengthOfArray: the element count of an array, as an integer.
 *
 * The argument must be an array constant; a list is a type mismatch.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single array argument value.
 * \param[out] out_result On success, the integer result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_length_of_array(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  size_t count = 0U;

  if (!as_array(args[0], NULL, NULL, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = result_integer_from_int(arena, (int64_t)count, out_result);

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/**
 * \brief Runs \c listToArray: builds an array from a list's elements.
 *
 * The argument must be a list constant. The result is an array of the same
 * element type carrying the same elements.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The single list argument value.
 * \param[out] out_result On success, the array result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_list_to_array(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_type_t*            element_type = NULL;
  const cardano_uplc_constant_t* const* items        = NULL;
  size_t                                count        = 0U;
  cardano_uplc_constant_t*              constant     = NULL;
  cardano_uplc_value_t*                 result       = NULL;

  if (!cardano_uplc_builtin_as_list(args[0], &element_type, &items, &count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_constant_new_array(arena, element_type, (count > 0U) ? items : NULL, count, &constant);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, constant, &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs \c indexArray: the array element at a given index.
 *
 * The first argument must be an array and the second an integer index. A
 * negative index, an index too large to address, or one at or beyond the length
 * is a script error.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] args The array and the index.
 * \param[out] out_result On success, the element value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_index_array(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* const* items  = NULL;
  size_t                                count  = 0U;
  const cardano_bigint_t*               idx    = NULL;
  int64_t                               i      = 0;
  cardano_uplc_value_t*                 result = NULL;

  if (!as_array(args[0], NULL, &items, &count) || !as_integer_big(arena, args[1], &idx))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_signum(idx) < 0)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  if (cardano_bigint_bit_length(idx) > 63U)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  i = cardano_bigint_to_int(idx);

  if ((uint64_t)i >= (uint64_t)count)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = cardano_uplc_value_new_constant(arena, items[i], &result);

  if (*host_error != CARDANO_SUCCESS)
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *out_result = result;

  return CARDANO_UPLC_BUILTIN_OUTCOME_OK;
}

/**
 * \brief Runs the BLS multi-scalar-multiplication pair (G1 and G2).
 *
 * The first argument is a list of integer scalars and the second a list of group
 * points of \p kind (\ref CARDANO_UPLC_TYPE_BLS_G1 or
 * \ref CARDANO_UPLC_TYPE_BLS_G2). The result is the sum of scalar_i * point_i
 * over the shorter of the two lists; empty lists give the group identity. Each
 * scalar is reduced modulo the scalar period before multiplication, as ordinary
 * scalar multiplication does.
 *
 * \param[in] arena The arena the result is allocated from.
 * \param[in] kind The group kind, G1 or G2.
 * \param[in] args The scalar list and the point list.
 * \param[out] out_result On success, the group result value.
 * \param[out] host_error Set to a host error on allocation failure.
 *
 * \return The script-visible outcome.
 */
static cardano_uplc_int_builtin_outcome_t
body_bls_multi_scalar_mul(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_type_kind_t           kind,
  const cardano_uplc_value_t* const* args,
  const cardano_uplc_value_t**       out_result,
  cardano_error_t*                   host_error)
{
  const cardano_uplc_constant_t* const* scalars      = NULL;
  const cardano_uplc_constant_t* const* points       = NULL;
  size_t                                scalar_count = 0U;
  size_t                                point_count  = 0U;
  size_t                                n            = 0U;
  size_t                                i            = 0U;
  byte_t                                scalar_be[CARDANO_UPLC_BLS_SCALAR_SIZE];
  blst_scalar                           blst_scalar_value;
  blst_p1                               acc1;
  blst_p2                               acc2;

  if (!cardano_uplc_builtin_as_list(args[0], NULL, &scalars, &scalar_count) || !cardano_uplc_builtin_as_list(args[1], NULL, &points, &point_count))
  {
    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  n = (scalar_count < point_count) ? scalar_count : point_count;

  (void)memset(&acc1, 0, sizeof(acc1));
  (void)memset(&acc2, 0, sizeof(acc2));

  for (i = 0U; i < n; ++i)
  {
    const cardano_uplc_constant_t* scalar_const = scalars[i];
    const cardano_uplc_constant_t* point_const  = points[i];
    const cardano_bigint_t*        scalar_big   = NULL;

    if ((scalar_const == NULL) || (scalar_const->kind != CARDANO_UPLC_TYPE_INTEGER))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if ((point_const == NULL) || (point_const->kind != kind))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    if (!constant_integer_big(arena, scalar_const, &scalar_big))
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    *host_error = bls_reduce_scalar(scalar_big, scalar_be);

    if (*host_error != CARDANO_SUCCESS)
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
    }

    blst_scalar_from_bendian(&blst_scalar_value, scalar_be);

    if (kind == CARDANO_UPLC_TYPE_BLS_G1)
    {
      blst_p1 term;

      blst_p1_mult(&term, (const blst_p1*)point_const->as.bls, blst_scalar_value.b, CARDANO_UPLC_BLS_SCALAR_BITS);
      blst_p1_add_or_double(&acc1, &acc1, &term);
    }
    else
    {
      blst_p2 term;

      blst_p2_mult(&term, (const blst_p2*)point_const->as.bls, blst_scalar_value.b, CARDANO_UPLC_BLS_SCALAR_BITS);
      blst_p2_add_or_double(&acc2, &acc2, &term);
    }
  }

  if (kind == CARDANO_UPLC_TYPE_BLS_G1)
  {
    *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &acc1, sizeof(acc1), out_result);
  }
  else
  {
    *host_error = result_bls(arena, CARDANO_UPLC_TYPE_BLS_G2, &acc2, sizeof(acc2), out_result);
  }

  return (*host_error == CARDANO_SUCCESS) ? CARDANO_UPLC_BUILTIN_OUTCOME_OK : CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
}

/* DISPATCH *****************************************************************/

cardano_uplc_int_builtin_outcome_t
cardano_uplc_int_builtin_run(
  struct cardano_uplc_arena_t*        arena,
  cardano_uplc_step_accumulator_t*    acc,
  const cardano_uplc_builtin_costs_t* costs,
  cardano_uplc_builtin_semantics_t    semantics,
  cardano_uplc_builtin_t              func,
  const cardano_uplc_value_t* const*  args,
  size_t                              arg_count,
  const cardano_uplc_value_t**        out_result,
  cardano_error_t*                    host_error)
{
  if ((arena == NULL) || (acc == NULL) || (costs == NULL) || (out_result == NULL) || (host_error == NULL))
  {
    if (host_error != NULL)
    {
      *host_error = CARDANO_ERROR_POINTER_IS_NULL;
    }

    return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
  }

  *host_error = CARDANO_SUCCESS;

  /*
   * Each arm spends its cost before its body runs (cost-before-execute). Budget
   * exhaustion is detected by the caller after the step and takes precedence over
   * any script error this body returns, so spending first is safe. An unsupported
   * builtin falls to the default and reports the unsupported outcome WITHOUT
   * spending. The switch is exhaustive over cardano_uplc_builtin_t so
   * -Wswitch-enum makes a forgotten builtin a build error.
   */
  switch (func)
  {
    case CARDANO_UPLC_BUILTIN_ADD_INTEGER:
    case CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER:
    case CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_int_arith(arena, func, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER:
    case CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER:
    case CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER:
    case CARDANO_UPLC_BUILTIN_MOD_INTEGER:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_int_div(arena, func, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_EQUALS_INTEGER:
    case CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER:
    case CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_int_compare(arena, func, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_exp_mod_integer(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_append_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_cons_byte_string(arena, semantics, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_slice_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_length_of_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_index_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING:
    case CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING:
    case CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_byte_string_compare(arena, func, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_AND_BYTE_STRING:
    case CARDANO_UPLC_BUILTIN_OR_BYTE_STRING:
    case CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_logical_byte_string(arena, func, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_complement_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_READ_BIT:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_read_bit(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_WRITE_BITS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_write_bits(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_REPLICATE_BYTE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_replicate_byte(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_shift_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_rotate_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_COUNT_SET_BITS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_count_set_bits(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_find_first_set_bit(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_integer_to_byte_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_byte_string_to_integer(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_APPEND_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_append_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_EQUALS_STRING:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_equals_string(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_ENCODE_UTF8:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_encode_utf8(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_DECODE_UTF8:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_decode_utf8(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_IF_THEN_ELSE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_if_then_else(args, out_result);
    }
    case CARDANO_UPLC_BUILTIN_CHOOSE_UNIT:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_choose_unit(args, out_result);
    }
    case CARDANO_UPLC_BUILTIN_TRACE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_trace(args, out_result);
    }
    case CARDANO_UPLC_BUILTIN_FST_PAIR:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_pair_component(arena, true, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SND_PAIR:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_pair_component(arena, false, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_CHOOSE_LIST:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_choose_list(args, out_result);
    }
    case CARDANO_UPLC_BUILTIN_MK_CONS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_mk_cons(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_HEAD_LIST:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_head_list(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_TAIL_LIST:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_tail_list(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_NULL_LIST:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_null_list(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_CHOOSE_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_choose_data(args, out_result);
    }
    case CARDANO_UPLC_BUILTIN_CONSTR_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_constr_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_MAP_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_map_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_LIST_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_list_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_I_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_i_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_B_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_b_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_un_constr_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UN_MAP_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_un_map_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UN_LIST_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_un_list_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UN_I_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_un_i_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UN_B_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_un_b_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_EQUALS_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_equals_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_MK_PAIR_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_mk_pair_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_MK_NIL_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_mk_nil_data(arena, false, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_mk_nil_data(arena, true, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SERIALISE_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_serialise_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SHA2_256:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_sha2_256(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLAKE2B_224:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_blake2b(arena, CARDANO_UPLC_BUILTIN_BLAKE2B_224_SIZE, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLAKE2B_256:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_blake2b(arena, CARDANO_UPLC_BUILTIN_BLAKE2B_256_SIZE, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_verify_ed25519(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SHA3_256:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_sha3_keccak(arena, CARDANO_UPLC_BUILTIN_SHA3_PAD, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_KECCAK_256:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_sha3_keccak(arena, CARDANO_UPLC_BUILTIN_KECCAK_PAD, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_RIPEMD_160:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_ripemd_160(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_verify_ecdsa(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_verify_schnorr(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_add(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_neg(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_scalar_mul(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_equal(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_compress(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_uncompress(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g1_hash_to_group(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_add(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_neg(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_scalar_mul(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_equal(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_compress(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_uncompress(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_g2_hash_to_group(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_miller_loop(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_mul_ml_result(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_final_verify(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_DROP_LIST:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_drop_list(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_length_of_array(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_list_to_array(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_INDEX_ARRAY:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_index_array(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_multi_scalar_mul(arena, CARDANO_UPLC_TYPE_BLS_G1, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_bls_multi_scalar_mul(arena, CARDANO_UPLC_TYPE_BLS_G2, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_INSERT_COIN:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_insert_coin(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_LOOKUP_COIN:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_lookup_coin(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UNION_VALUE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_union_value(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_VALUE_CONTAINS:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_value_contains(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_VALUE_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_value_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_UN_VALUE_DATA:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_un_value_data(arena, args, out_result, host_error);
    }
    case CARDANO_UPLC_BUILTIN_SCALE_VALUE:
    {
      *host_error = spend_builtin_cost(acc, costs, semantics, func, args, arg_count);

      if (*host_error != CARDANO_SUCCESS)
      {
        return CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR;
      }

      return body_scale_value(arena, args, out_result, host_error);
    }
    default:
    {
      return CARDANO_UPLC_BUILTIN_OUTCOME_UNSUPPORTED;
    }
  }
}
