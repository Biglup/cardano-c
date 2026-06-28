/**
 * \file uplc_builtin.c
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

#include "uplc_builtin.h"

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/*
 * The builtin flat tags are the 7-bit identifiers assigned to each default
 * function in the UPLC binary encoding. Tags 88..100 cover the array, BLS
 * multi-scalar-multiplication, and multi-asset value builtins: dropList 88,
 * lengthOfArray 89, listToArray 90, indexArray 91, bls12_381_G1_multiScalarMul
 * 92, bls12_381_G2_multiScalarMul 93, insertCoin 94, lookupCoin 95, unionValue
 * 96, valueContains 97, valueData 98, unValueData 99, scaleValue 100. The total
 * number of builtins is 101 (tags 0..100).
 *
 * The tables below record, for each builtin, its tag (the enumerator order), its
 * arity (the number of value arguments it saturates on), its force count (the
 * number of type arguments it takes before its value arguments), and the first
 * language version in which it is available. The builtins on tags 88..100 are
 * labelled CARDANO_UPLC_LANG_VERSION_V4, the module-local label for the batch of
 * builtins introduced after PlutusV3.
 */

/**
 * \brief Arity of every builtin, indexed by \ref cardano_uplc_builtin_t.
 *
 * The number of value arguments each builtin saturates on before its body runs.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t BUILTIN_ARITY[CARDANO_UPLC_BUILTIN_COUNT] = {
  [CARDANO_UPLC_BUILTIN_ADD_INTEGER]                        = 2U,
  [CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER]                   = 2U,
  [CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER]                   = 2U,
  [CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER]                     = 2U,
  [CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER]                   = 2U,
  [CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER]                  = 2U,
  [CARDANO_UPLC_BUILTIN_MOD_INTEGER]                        = 2U,
  [CARDANO_UPLC_BUILTIN_EQUALS_INTEGER]                     = 2U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER]                  = 2U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER]           = 2U,
  [CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING]                 = 2U,
  [CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING]                   = 2U,
  [CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING]                  = 3U,
  [CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING]              = 1U,
  [CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING]                  = 2U,
  [CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING]                 = 2U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING]              = 2U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING]       = 2U,
  [CARDANO_UPLC_BUILTIN_SHA2_256]                           = 1U,
  [CARDANO_UPLC_BUILTIN_SHA3_256]                           = 1U,
  [CARDANO_UPLC_BUILTIN_BLAKE2B_256]                        = 1U,
  [CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE]           = 3U,
  [CARDANO_UPLC_BUILTIN_APPEND_STRING]                      = 2U,
  [CARDANO_UPLC_BUILTIN_EQUALS_STRING]                      = 2U,
  [CARDANO_UPLC_BUILTIN_ENCODE_UTF8]                        = 1U,
  [CARDANO_UPLC_BUILTIN_DECODE_UTF8]                        = 1U,
  [CARDANO_UPLC_BUILTIN_IF_THEN_ELSE]                       = 3U,
  [CARDANO_UPLC_BUILTIN_CHOOSE_UNIT]                        = 2U,
  [CARDANO_UPLC_BUILTIN_TRACE]                              = 2U,
  [CARDANO_UPLC_BUILTIN_FST_PAIR]                           = 1U,
  [CARDANO_UPLC_BUILTIN_SND_PAIR]                           = 1U,
  [CARDANO_UPLC_BUILTIN_CHOOSE_LIST]                        = 3U,
  [CARDANO_UPLC_BUILTIN_MK_CONS]                            = 2U,
  [CARDANO_UPLC_BUILTIN_HEAD_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_TAIL_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_NULL_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_CHOOSE_DATA]                        = 6U,
  [CARDANO_UPLC_BUILTIN_CONSTR_DATA]                        = 2U,
  [CARDANO_UPLC_BUILTIN_MAP_DATA]                           = 1U,
  [CARDANO_UPLC_BUILTIN_LIST_DATA]                          = 1U,
  [CARDANO_UPLC_BUILTIN_I_DATA]                             = 1U,
  [CARDANO_UPLC_BUILTIN_B_DATA]                             = 1U,
  [CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA]                     = 1U,
  [CARDANO_UPLC_BUILTIN_UN_MAP_DATA]                        = 1U,
  [CARDANO_UPLC_BUILTIN_UN_LIST_DATA]                       = 1U,
  [CARDANO_UPLC_BUILTIN_UN_I_DATA]                          = 1U,
  [CARDANO_UPLC_BUILTIN_UN_B_DATA]                          = 1U,
  [CARDANO_UPLC_BUILTIN_EQUALS_DATA]                        = 2U,
  [CARDANO_UPLC_BUILTIN_MK_PAIR_DATA]                       = 2U,
  [CARDANO_UPLC_BUILTIN_MK_NIL_DATA]                        = 1U,
  [CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA]                   = 1U,
  [CARDANO_UPLC_BUILTIN_SERIALISE_DATA]                     = 1U,
  [CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE]   = 3U,
  [CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE] = 3U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD]                   = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG]                   = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL]            = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL]                 = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS]              = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS]            = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP]         = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD]                   = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG]                   = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL]            = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL]                 = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS]              = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS]            = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP]         = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP]              = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT]            = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY]             = 2U,
  [CARDANO_UPLC_BUILTIN_KECCAK_256]                         = 1U,
  [CARDANO_UPLC_BUILTIN_BLAKE2B_224]                        = 1U,
  [CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING]             = 3U,
  [CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER]             = 2U,
  [CARDANO_UPLC_BUILTIN_AND_BYTE_STRING]                    = 3U,
  [CARDANO_UPLC_BUILTIN_OR_BYTE_STRING]                     = 3U,
  [CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING]                    = 3U,
  [CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING]             = 1U,
  [CARDANO_UPLC_BUILTIN_READ_BIT]                           = 2U,
  [CARDANO_UPLC_BUILTIN_WRITE_BITS]                         = 3U,
  [CARDANO_UPLC_BUILTIN_REPLICATE_BYTE]                     = 2U,
  [CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING]                  = 2U,
  [CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING]                 = 2U,
  [CARDANO_UPLC_BUILTIN_COUNT_SET_BITS]                     = 1U,
  [CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT]                 = 1U,
  [CARDANO_UPLC_BUILTIN_RIPEMD_160]                         = 1U,
  [CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER]                    = 3U,
  [CARDANO_UPLC_BUILTIN_DROP_LIST]                          = 2U,
  [CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY]                    = 1U,
  [CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY]                      = 1U,
  [CARDANO_UPLC_BUILTIN_INDEX_ARRAY]                        = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL]      = 2U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL]      = 2U,
  [CARDANO_UPLC_BUILTIN_INSERT_COIN]                        = 4U,
  [CARDANO_UPLC_BUILTIN_LOOKUP_COIN]                        = 3U,
  [CARDANO_UPLC_BUILTIN_UNION_VALUE]                        = 2U,
  [CARDANO_UPLC_BUILTIN_VALUE_CONTAINS]                     = 2U,
  [CARDANO_UPLC_BUILTIN_VALUE_DATA]                         = 1U,
  [CARDANO_UPLC_BUILTIN_UN_VALUE_DATA]                      = 1U,
  [CARDANO_UPLC_BUILTIN_SCALE_VALUE]                        = 2U
};

/**
 * \brief Force count of every builtin, indexed by \ref cardano_uplc_builtin_t.
 *
 * The number of type arguments (forces) each builtin takes before its value
 * arguments. The V4 builtins that are polymorphic in their element type take one
 * force (dropList, lengthOfArray, listToArray, indexArray); the value/coin and
 * multi-scalar-mul builtins are monomorphic and take none.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t BUILTIN_FORCE_COUNT[CARDANO_UPLC_BUILTIN_COUNT] = {
  [CARDANO_UPLC_BUILTIN_ADD_INTEGER]                        = 0U,
  [CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER]                   = 0U,
  [CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER]                   = 0U,
  [CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER]                     = 0U,
  [CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER]                   = 0U,
  [CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER]                  = 0U,
  [CARDANO_UPLC_BUILTIN_MOD_INTEGER]                        = 0U,
  [CARDANO_UPLC_BUILTIN_EQUALS_INTEGER]                     = 0U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER]                  = 0U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER]           = 0U,
  [CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING]                 = 0U,
  [CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING]                   = 0U,
  [CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING]                  = 0U,
  [CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING]              = 0U,
  [CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING]                  = 0U,
  [CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING]                 = 0U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING]              = 0U,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING]       = 0U,
  [CARDANO_UPLC_BUILTIN_SHA2_256]                           = 0U,
  [CARDANO_UPLC_BUILTIN_SHA3_256]                           = 0U,
  [CARDANO_UPLC_BUILTIN_BLAKE2B_256]                        = 0U,
  [CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE]           = 0U,
  [CARDANO_UPLC_BUILTIN_APPEND_STRING]                      = 0U,
  [CARDANO_UPLC_BUILTIN_EQUALS_STRING]                      = 0U,
  [CARDANO_UPLC_BUILTIN_ENCODE_UTF8]                        = 0U,
  [CARDANO_UPLC_BUILTIN_DECODE_UTF8]                        = 0U,
  [CARDANO_UPLC_BUILTIN_IF_THEN_ELSE]                       = 1U,
  [CARDANO_UPLC_BUILTIN_CHOOSE_UNIT]                        = 1U,
  [CARDANO_UPLC_BUILTIN_TRACE]                              = 1U,
  [CARDANO_UPLC_BUILTIN_FST_PAIR]                           = 2U,
  [CARDANO_UPLC_BUILTIN_SND_PAIR]                           = 2U,
  [CARDANO_UPLC_BUILTIN_CHOOSE_LIST]                        = 2U,
  [CARDANO_UPLC_BUILTIN_MK_CONS]                            = 1U,
  [CARDANO_UPLC_BUILTIN_HEAD_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_TAIL_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_NULL_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_CHOOSE_DATA]                        = 1U,
  [CARDANO_UPLC_BUILTIN_CONSTR_DATA]                        = 0U,
  [CARDANO_UPLC_BUILTIN_MAP_DATA]                           = 0U,
  [CARDANO_UPLC_BUILTIN_LIST_DATA]                          = 0U,
  [CARDANO_UPLC_BUILTIN_I_DATA]                             = 0U,
  [CARDANO_UPLC_BUILTIN_B_DATA]                             = 0U,
  [CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA]                     = 0U,
  [CARDANO_UPLC_BUILTIN_UN_MAP_DATA]                        = 0U,
  [CARDANO_UPLC_BUILTIN_UN_LIST_DATA]                       = 0U,
  [CARDANO_UPLC_BUILTIN_UN_I_DATA]                          = 0U,
  [CARDANO_UPLC_BUILTIN_UN_B_DATA]                          = 0U,
  [CARDANO_UPLC_BUILTIN_EQUALS_DATA]                        = 0U,
  [CARDANO_UPLC_BUILTIN_MK_PAIR_DATA]                       = 0U,
  [CARDANO_UPLC_BUILTIN_MK_NIL_DATA]                        = 0U,
  [CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA]                   = 0U,
  [CARDANO_UPLC_BUILTIN_SERIALISE_DATA]                     = 0U,
  [CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE]   = 0U,
  [CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE] = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD]                   = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG]                   = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL]            = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL]                 = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS]              = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS]            = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP]         = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD]                   = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG]                   = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL]            = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL]                 = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS]              = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS]            = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP]         = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP]              = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT]            = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY]             = 0U,
  [CARDANO_UPLC_BUILTIN_KECCAK_256]                         = 0U,
  [CARDANO_UPLC_BUILTIN_BLAKE2B_224]                        = 0U,
  [CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING]             = 0U,
  [CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER]             = 0U,
  [CARDANO_UPLC_BUILTIN_AND_BYTE_STRING]                    = 0U,
  [CARDANO_UPLC_BUILTIN_OR_BYTE_STRING]                     = 0U,
  [CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING]                    = 0U,
  [CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING]             = 0U,
  [CARDANO_UPLC_BUILTIN_READ_BIT]                           = 0U,
  [CARDANO_UPLC_BUILTIN_WRITE_BITS]                         = 0U,
  [CARDANO_UPLC_BUILTIN_REPLICATE_BYTE]                     = 0U,
  [CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING]                  = 0U,
  [CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING]                 = 0U,
  [CARDANO_UPLC_BUILTIN_COUNT_SET_BITS]                     = 0U,
  [CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT]                 = 0U,
  [CARDANO_UPLC_BUILTIN_RIPEMD_160]                         = 0U,
  [CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER]                    = 0U,
  [CARDANO_UPLC_BUILTIN_DROP_LIST]                          = 1U,
  [CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY]                    = 1U,
  [CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY]                      = 1U,
  [CARDANO_UPLC_BUILTIN_INDEX_ARRAY]                        = 1U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL]      = 0U,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL]      = 0U,
  [CARDANO_UPLC_BUILTIN_INSERT_COIN]                        = 0U,
  [CARDANO_UPLC_BUILTIN_LOOKUP_COIN]                        = 0U,
  [CARDANO_UPLC_BUILTIN_UNION_VALUE]                        = 0U,
  [CARDANO_UPLC_BUILTIN_VALUE_CONTAINS]                     = 0U,
  [CARDANO_UPLC_BUILTIN_VALUE_DATA]                         = 0U,
  [CARDANO_UPLC_BUILTIN_UN_VALUE_DATA]                      = 0U,
  [CARDANO_UPLC_BUILTIN_SCALE_VALUE]                        = 0U
};

/**
 * \brief First language version of every builtin, indexed by
 *        \ref cardano_uplc_builtin_t.
 *
 * Stored as the integer value of \ref cardano_uplc_lang_version_t. The builtins
 * introduced after PlutusV3 are labelled V4 here.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t BUILTIN_FIRST_VERSION[CARDANO_UPLC_BUILTIN_COUNT] = {
  [CARDANO_UPLC_BUILTIN_ADD_INTEGER]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER]                  = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MOD_INTEGER]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_EQUALS_INTEGER]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER]                  = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER]           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING]                 = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING]                  = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING]              = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING]                  = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING]                 = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING]              = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING]       = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_SHA2_256]                           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_SHA3_256]                           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_BLAKE2B_256]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE]           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_APPEND_STRING]                      = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_EQUALS_STRING]                      = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_ENCODE_UTF8]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_DECODE_UTF8]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_IF_THEN_ELSE]                       = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_CHOOSE_UNIT]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_TRACE]                              = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_FST_PAIR]                           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_SND_PAIR]                           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_CHOOSE_LIST]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MK_CONS]                            = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_HEAD_LIST]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_TAIL_LIST]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_NULL_LIST]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_CHOOSE_DATA]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_CONSTR_DATA]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MAP_DATA]                           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_LIST_DATA]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_I_DATA]                             = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_B_DATA]                             = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_UN_MAP_DATA]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_UN_LIST_DATA]                       = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_UN_I_DATA]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_UN_B_DATA]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_EQUALS_DATA]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MK_PAIR_DATA]                       = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MK_NIL_DATA]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V1,
  [CARDANO_UPLC_BUILTIN_SERIALISE_DATA]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V2,
  [CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE]   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V2,
  [CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE] = (uint8_t)CARDANO_UPLC_LANG_VERSION_V2,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL]            = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL]                 = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS]              = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS]            = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP]         = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG]                   = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL]            = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL]                 = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS]              = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS]            = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP]         = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP]              = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT]            = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY]             = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_KECCAK_256]                         = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BLAKE2B_224]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING]             = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER]             = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_AND_BYTE_STRING]                    = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_OR_BYTE_STRING]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING]                    = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING]             = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_READ_BIT]                           = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_WRITE_BITS]                         = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_REPLICATE_BYTE]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING]                  = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING]                 = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_COUNT_SET_BITS]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT]                 = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_RIPEMD_160]                         = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER]                    = (uint8_t)CARDANO_UPLC_LANG_VERSION_V3,
  [CARDANO_UPLC_BUILTIN_DROP_LIST]                          = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY]                    = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY]                      = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_INDEX_ARRAY]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL]      = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL]      = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_INSERT_COIN]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_LOOKUP_COIN]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_UNION_VALUE]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_VALUE_CONTAINS]                     = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_VALUE_DATA]                         = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_UN_VALUE_DATA]                      = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4,
  [CARDANO_UPLC_BUILTIN_SCALE_VALUE]                        = (uint8_t)CARDANO_UPLC_LANG_VERSION_V4
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Reports whether a builtin tag is in the valid range.
 *
 * \param[in] builtin The builtin tag to check.
 *
 * \return Nonzero if \p builtin is in 0 .. CARDANO_UPLC_BUILTIN_COUNT - 1, zero
 *         otherwise.
 */
static int
is_valid_builtin(const cardano_uplc_builtin_t builtin)
{
  return ((int)builtin >= 0) && ((size_t)builtin < (size_t)CARDANO_UPLC_BUILTIN_COUNT);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_builtin_arity(const cardano_uplc_builtin_t builtin, size_t* arity)
{
  if (arity == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!is_valid_builtin(builtin))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *arity = (size_t)BUILTIN_ARITY[(size_t)builtin];

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_builtin_force_count(const cardano_uplc_builtin_t builtin, size_t* force_count)
{
  if (force_count == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!is_valid_builtin(builtin))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *force_count = (size_t)BUILTIN_FORCE_COUNT[(size_t)builtin];

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_builtin_first_version(const cardano_uplc_builtin_t builtin, cardano_uplc_lang_version_t* version)
{
  if (version == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!is_valid_builtin(builtin))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *version = (cardano_uplc_lang_version_t)BUILTIN_FIRST_VERSION[(size_t)builtin];

  return CARDANO_SUCCESS;
}
