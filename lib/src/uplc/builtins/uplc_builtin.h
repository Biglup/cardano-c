/**
 * \file uplc_builtin.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_H

/* INCLUDES ******************************************************************/

#include "../ast/uplc_lang_version.h"
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The Untyped Plutus Core default (builtin) functions, identified by
 *        their flat-encoding tag.
 *
 * Each enumerator value is the integer the flat decoder reads for that builtin,
 * so the enum is the authoritative tag list the decoder and the CEK machine
 * switch on. Each tag is the explicit 7-bit number the flat encoding assigns to
 * that builtin; it is consensus-critical, since a wrong number silently
 * misdecodes on-chain scripts.
 *
 * Tags 0..87 cover the V1-V3 builtins. Tags 88..100 cover the post-V3 ("V4")
 * builtins: the array operations, the BLS12-381 multi-scalar multiplications, and
 * the value/coin operations. The V4 builtins became available at the van Rossem
 * hard fork.
 */
typedef enum
{
  /* Integer functions (V1) */
  CARDANO_UPLC_BUILTIN_ADD_INTEGER              = 0,
  CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER         = 1,
  CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER         = 2,
  CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER           = 3,
  CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER         = 4,
  CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER        = 5,
  CARDANO_UPLC_BUILTIN_MOD_INTEGER              = 6,
  CARDANO_UPLC_BUILTIN_EQUALS_INTEGER           = 7,
  CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER        = 8,
  CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER = 9,

  /* ByteString functions (V1) */
  CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING           = 10,
  CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING             = 11,
  CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING            = 12,
  CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING        = 13,
  CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING            = 14,
  CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING           = 15,
  CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING        = 16,
  CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING = 17,

  /* Cryptography and hash functions (V1) */
  CARDANO_UPLC_BUILTIN_SHA2_256                 = 18,
  CARDANO_UPLC_BUILTIN_SHA3_256                 = 19,
  CARDANO_UPLC_BUILTIN_BLAKE2B_256              = 20,
  CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE = 21,

  /* String functions (V1) */
  CARDANO_UPLC_BUILTIN_APPEND_STRING = 22,
  CARDANO_UPLC_BUILTIN_EQUALS_STRING = 23,
  CARDANO_UPLC_BUILTIN_ENCODE_UTF8   = 24,
  CARDANO_UPLC_BUILTIN_DECODE_UTF8   = 25,

  /* Bool function (V1) */
  CARDANO_UPLC_BUILTIN_IF_THEN_ELSE = 26,

  /* Unit function (V1) */
  CARDANO_UPLC_BUILTIN_CHOOSE_UNIT = 27,

  /* Tracing function (V1) */
  CARDANO_UPLC_BUILTIN_TRACE = 28,

  /* Pair functions (V1) */
  CARDANO_UPLC_BUILTIN_FST_PAIR = 29,
  CARDANO_UPLC_BUILTIN_SND_PAIR = 30,

  /* List functions (V1) */
  CARDANO_UPLC_BUILTIN_CHOOSE_LIST = 31,
  CARDANO_UPLC_BUILTIN_MK_CONS     = 32,
  CARDANO_UPLC_BUILTIN_HEAD_LIST   = 33,
  CARDANO_UPLC_BUILTIN_TAIL_LIST   = 34,
  CARDANO_UPLC_BUILTIN_NULL_LIST   = 35,

  /* Data functions (V1) */
  CARDANO_UPLC_BUILTIN_CHOOSE_DATA    = 36,
  CARDANO_UPLC_BUILTIN_CONSTR_DATA    = 37,
  CARDANO_UPLC_BUILTIN_MAP_DATA       = 38,
  CARDANO_UPLC_BUILTIN_LIST_DATA      = 39,
  CARDANO_UPLC_BUILTIN_I_DATA         = 40,
  CARDANO_UPLC_BUILTIN_B_DATA         = 41,
  CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA = 42,
  CARDANO_UPLC_BUILTIN_UN_MAP_DATA    = 43,
  CARDANO_UPLC_BUILTIN_UN_LIST_DATA   = 44,
  CARDANO_UPLC_BUILTIN_UN_I_DATA      = 45,
  CARDANO_UPLC_BUILTIN_UN_B_DATA      = 46,
  CARDANO_UPLC_BUILTIN_EQUALS_DATA    = 47,

  /* Misc constructors (V1) */
  CARDANO_UPLC_BUILTIN_MK_PAIR_DATA     = 48,
  CARDANO_UPLC_BUILTIN_MK_NIL_DATA      = 49,
  CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA = 50,

  /* Serialisation (V2) */
  CARDANO_UPLC_BUILTIN_SERIALISE_DATA = 51,

  /* ECDSA / Schnorr (V2) */
  CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE   = 52,
  CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE = 53,

  /* BLS12-381 G1 (V3) */
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD           = 54,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG           = 55,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL    = 56,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL         = 57,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS      = 58,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS    = 59,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP = 60,

  /* BLS12-381 G2 (V3) */
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD           = 61,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG           = 62,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL    = 63,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL         = 64,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS      = 65,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS    = 66,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP = 67,

  /* BLS12-381 pairing (V3) */
  CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP   = 68,
  CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT = 69,
  CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY  = 70,

  /* Additional hash functions (V3) */
  CARDANO_UPLC_BUILTIN_KECCAK_256  = 71,
  CARDANO_UPLC_BUILTIN_BLAKE2B_224 = 72,

  /* Integer/ByteString conversions and bitwise operations (V3) */
  CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING = 73,
  CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER = 74,
  CARDANO_UPLC_BUILTIN_AND_BYTE_STRING        = 75,
  CARDANO_UPLC_BUILTIN_OR_BYTE_STRING         = 76,
  CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING        = 77,
  CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING = 78,
  CARDANO_UPLC_BUILTIN_READ_BIT               = 79,
  CARDANO_UPLC_BUILTIN_WRITE_BITS             = 80,
  CARDANO_UPLC_BUILTIN_REPLICATE_BYTE         = 81,
  CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING      = 82,
  CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING     = 83,
  CARDANO_UPLC_BUILTIN_COUNT_SET_BITS         = 84,
  CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT     = 85,

  /* RIPEMD-160 (V3) */
  CARDANO_UPLC_BUILTIN_RIPEMD_160 = 86,

  /* Modular exponentiation (V3) */
  CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER = 87,

  /* Post-V3 ("V4") builtins, tags 88..100. */
  CARDANO_UPLC_BUILTIN_DROP_LIST                     = 88,
  CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY               = 89,
  CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY                 = 90,
  CARDANO_UPLC_BUILTIN_INDEX_ARRAY                   = 91,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL = 92,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL = 93,
  CARDANO_UPLC_BUILTIN_INSERT_COIN                   = 94,
  CARDANO_UPLC_BUILTIN_LOOKUP_COIN                   = 95,
  CARDANO_UPLC_BUILTIN_UNION_VALUE                   = 96,
  CARDANO_UPLC_BUILTIN_VALUE_CONTAINS                = 97,
  CARDANO_UPLC_BUILTIN_VALUE_DATA                    = 98,
  CARDANO_UPLC_BUILTIN_UN_VALUE_DATA                 = 99,
  CARDANO_UPLC_BUILTIN_SCALE_VALUE                   = 100
} cardano_uplc_builtin_t;

/**
 * \brief Number of distinct builtin tags, equal to the largest valid tag plus one.
 *
 * The largest tag is 100 (scaleValue), so the count is 101. Valid tags are
 * 0 .. CARDANO_UPLC_BUILTIN_COUNT - 1.
 */
#define CARDANO_UPLC_BUILTIN_COUNT 101

/**
 * \brief Returns the number of term arguments a builtin must be applied to before
 *        it can run.
 *
 * \param[in] builtin The builtin tag to query.
 * \param[out] arity On success, set to the arity (one of 1, 2, 3, 4 or 6); left
 *             untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arity is NULL, or \ref CARDANO_ERROR_INVALID_ARGUMENT if \p builtin is
 *         not a valid tag in 0 .. CARDANO_UPLC_BUILTIN_COUNT - 1.
 */
cardano_error_t
cardano_uplc_builtin_arity(cardano_uplc_builtin_t builtin, size_t* arity);

/**
 * \brief Returns the number of \c force operations a builtin must receive before
 *        it can run.
 *
 * \param[in] builtin The builtin tag to query.
 * \param[out] force_count On success, set to the force count (one of 0, 1 or 2);
 *             left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p force_count is NULL, or \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p builtin is not a valid tag in 0 .. CARDANO_UPLC_BUILTIN_COUNT - 1.
 */
cardano_error_t
cardano_uplc_builtin_force_count(cardano_uplc_builtin_t builtin, size_t* force_count);

/**
 * \brief Returns the Plutus language version a builtin first became available in.
 *
 * \param[in] builtin The builtin tag to query.
 * \param[out] version On success, set to the first version the builtin is available
 *             in; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p version is NULL, or \ref CARDANO_ERROR_INVALID_ARGUMENT if \p builtin
 *         is not a valid tag in 0 .. CARDANO_UPLC_BUILTIN_COUNT - 1.
 */
cardano_error_t
cardano_uplc_builtin_first_version(cardano_uplc_builtin_t builtin, cardano_uplc_lang_version_t* version);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_H */
