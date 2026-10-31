/* Copyright 2026 Biglup Labs. Licensed under the Apache License, Version 2.0. */
#include "../lib/src/uplc/ast/uplc_int_math.h"
#include <blst.h>
#include <stdbool.h>
#include <stdio.h>

int
main(void)
{
  const byte     ikm[32]    = { 1 };
  const byte     message[]  = "cardano-c release smoke test";
  const byte     tampered[] = "cardano-c release smoke tesX";
  const byte     dst[]      = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";
  blst_scalar    sk;
  blst_p1        pk;
  blst_p2        hash;
  blst_p2        signature;
  blst_p1_affine pk_affine;
  blst_p2_affine signature_affine;
  blst_keygen(&sk, ikm, sizeof(ikm), NULL, 0);
  blst_sk_to_pk_in_g1(&pk, &sk);
  blst_hash_to_g2(&hash, message, sizeof(message) - 1, dst, sizeof(dst) - 1, NULL, 0);
  blst_sign_pk_in_g1(&signature, &hash, &sk);
  blst_p1_to_affine(&pk_affine, &pk);
  blst_p2_to_affine(&signature_affine, &signature);
  if (blst_core_verify_pk_in_g1(&pk_affine, &signature_affine, true, message, sizeof(message) - 1, dst, sizeof(dst) - 1, NULL, 0) != BLST_SUCCESS)
    return 1;
  if (blst_core_verify_pk_in_g1(&pk_affine, &signature_affine, true, tampered, sizeof(tampered) - 1, dst, sizeof(dst) - 1, NULL, 0) == BLST_SUCCESS)
    return 2;
  int64_t result = 0;
  if (!cardano_uplc_i64_add_overflow(INT64_MAX, 1, &result) || !cardano_uplc_i64_add_overflow(INT64_MIN, -1, &result))
    return 3;
  if (!cardano_uplc_i64_sub_overflow(INT64_MIN, 1, &result) || !cardano_uplc_i64_sub_overflow(INT64_MAX, -1, &result))
    return 4;
  if (!cardano_uplc_i64_mul_overflow(INT64_MIN, -1, &result) || !cardano_uplc_i64_mul_overflow(INT64_MAX, 2, &result))
    return 5;
  if (cardano_uplc_i64_mul_overflow(INT64_MIN, 1, &result) || result != INT64_MIN)
    return 6;
  if (cardano_uplc_i64_add_overflow(INT64_MIN, INT64_MAX, &result) || result != -1)
    return 7;
  if (cardano_uplc_i64_sub_overflow(INT64_MAX, INT64_MAX, &result) || result != 0)
    return 8;
  puts("BLS sign/verify and tampered-message rejection passed");
  return 0;
}
