/**
 * \file bigint.c
 *
 * \author luisd.bianchi
 * \date   Jun 05, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include "../external/gmp/mini-gmp.h"

#include "../endian.h"
#include <assert.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a large numeric value.
 *
 * The \ref cardano_bigint_t type is used for representing numeric values that are too large to be
 * represented by the standard numeric primitive types, such as int64_t or uint64_t.
 */
typedef struct cardano_bigint_t
{
    cardano_object_t base;
    mpz_t            mpz;
} cardano_bigint_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a bigint object.
 *
 * This function is responsible for properly deallocating a bigint object (`cardano_bigint_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the bigint object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_bigint_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the bigint
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_bigint_deallocate(void* object)
{
  assert(object != NULL);

  cardano_bigint_t* bigint = (cardano_bigint_t*)object;

  mpz_clear(bigint->mpz);

  _cardano_free(bigint);
}

/**
 * \brief Creates a new bigint object.
 *
 * \return A pointer to the newly created bigint object, or `NULL` if the operation failed.
 */
static cardano_bigint_t*
cardano_bigint_new(void)
{
  cardano_bigint_t* data = _cardano_malloc(sizeof(cardano_bigint_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_bigint_deallocate;

  mpz_init(data->mpz);

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_bigint_clone(const cardano_bigint_t* bigint, cardano_bigint_t** clone)
{
  if ((bigint == NULL) || (clone == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *clone = cardano_bigint_new();

  if (*clone == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  mpz_init_set((*clone)->mpz, bigint->mpz);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bigint_from_string(const char* string, const size_t size, const int32_t base, cardano_bigint_t** bigint)
{
  if ((string == NULL) || (bigint == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((size == 0U) || (*string == '\0'))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *bigint = cardano_bigint_new();

  if (*bigint == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  // cppcheck-suppress misra-c2012-11.8; Reason: False positive.
  int ret = mpz_init_set_str((*bigint)->mpz, string, base);

  if (ret != 0)
  {
    cardano_bigint_unref(bigint);

    return CARDANO_ERROR_CONVERSION_FAILED;
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_bigint_get_string_size(cardano_bigint_t* bigint, const int32_t base)
{
  if (bigint == NULL)
  {
    return 0U;
  }

  char* temp = mpz_get_str(NULL, base, bigint->mpz);

  if (temp == NULL)
  {
    return 0;
  }

  const size_t num_digits = mpz_sizeinbase(bigint->mpz, base);
  const size_t max_size   = num_digits + 1U + 1U;

  const size_t digits = cardano_safe_strlen(temp, max_size);

  _cardano_free(temp);

  return digits + 1U;
}

cardano_error_t
cardano_bigint_from_int(int64_t value, cardano_bigint_t** bigint)
{
  if (bigint == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *bigint = cardano_bigint_new();

  if (*bigint == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  mpz_init((*bigint)->mpz);

  int      order     = cardano_is_little_endian() ? -1 : 1;
  uint64_t abs_value = (value < 0) ? (uint64_t)(-value) : (uint64_t)value;

  mpz_import((*bigint)->mpz, 1, order, sizeof(abs_value), 0, 0, &abs_value);

  if (value < 0)
  {
    mpz_neg((*bigint)->mpz, (*bigint)->mpz);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bigint_from_unsigned_int(uint64_t value, cardano_bigint_t** bigint)
{
  if (bigint == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *bigint = cardano_bigint_new();

  if (*bigint == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  mpz_init((*bigint)->mpz);

  int order = cardano_is_little_endian() ? -1 : 1;

  mpz_import((*bigint)->mpz, 1, order, sizeof(value), 0, 0, &value);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bigint_from_bytes(
  const byte_t*              data,
  const size_t               size,
  const cardano_byte_order_t byte_order,
  cardano_bigint_t**         bigint)
{
  if ((data == NULL) || (bigint == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *bigint = cardano_bigint_new();

  if (*bigint == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  int32_t       order         = 1;
  const int32_t size_per_word = 1;
  const int32_t nails         = 0;

  if (byte_order == CARDANO_BYTE_ORDER_LITTLE_ENDIAN)
  {
    order = -1;
  }

  mpz_import((*bigint)->mpz, size, order, size_per_word, 0, nails, data);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bigint_to_string(
  const cardano_bigint_t* bigint,
  char*                   string,
  const size_t            size,
  const int32_t           base)
{
  if ((bigint == NULL) || (string == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  char* temp = mpz_get_str(NULL, base, bigint->mpz);

  if (temp == NULL)
  {
    return CARDANO_ERROR_CONVERSION_FAILED;
  }

  size_t temp_len = cardano_safe_strlen(temp, size);

  if ((temp_len + 1U) > size)
  {
    _cardano_free(temp);

    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(string, size, temp, temp_len);
  string[size - 1U] = '\0';

  _cardano_free(temp);

  return CARDANO_SUCCESS;
}

int64_t
cardano_bigint_to_int(const cardano_bigint_t* bigint)
{
  if (bigint == NULL)
  {
    return 0;
  }

  const int order      = cardano_is_little_endian() ? -1 : 1;
  uint64_t  abs_result = 0;

  mpz_export(&abs_result, NULL, order, sizeof(abs_result), 0, 0, bigint->mpz);

  return (mpz_sgn(bigint->mpz) < 0) ? -(int64_t)abs_result : (int64_t)abs_result;
}

uint64_t
cardano_bigint_to_unsigned_int(const cardano_bigint_t* bigint)
{
  if (bigint == NULL)
  {
    return 0;
  }

  const int order  = cardano_is_little_endian() ? -1 : 1;
  uint64_t  result = 0;

  mpz_export(&result, NULL, order, sizeof(result), 0, 0, bigint->mpz);

  return result;
}

size_t
cardano_bigint_get_bytes_size(const cardano_bigint_t* bigint)
{
  return (mpz_sizeinbase(bigint->mpz, 2) + 7U) / 8U;
}

cardano_error_t
cardano_bigint_to_bytes(
  const cardano_bigint_t* bigint,
  cardano_byte_order_t    byte_order,
  byte_t*                 data,
  size_t                  size)
{
  if ((bigint == NULL) || (data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t required_size = cardano_bigint_get_bytes_size(bigint);

  if (required_size != size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  size_t        count;
  const int32_t order         = (byte_order == CARDANO_BYTE_ORDER_BIG_ENDIAN) ? (int32_t)1 : (int32_t)-1;
  const int32_t size_per_word = 1;
  const int32_t nails         = 0;

  mpz_export(data, &count, order, size_per_word, 0, nails, bigint->mpz);

  if (count != size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  return CARDANO_SUCCESS;
}

void
cardano_bigint_add(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_add(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_subtract(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_sub(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_multiply(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_mul(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_divide(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       result)
{
  if ((dividend == NULL) || (divisor == NULL) || (result == NULL))
  {
    return;
  }

  mpz_tdiv_q(result->mpz, dividend->mpz, divisor->mpz);
}

void
cardano_bigint_divide_and_reminder(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       quotient,
  cardano_bigint_t*       reminder)
{
  if ((dividend == NULL) || (divisor == NULL) || (quotient == NULL) || (reminder == NULL))
  {
    return;
  }

  mpz_tdiv_qr(quotient->mpz, reminder->mpz, dividend->mpz, divisor->mpz);
}

void
cardano_bigint_reminder(
  const cardano_bigint_t* dividend,
  const cardano_bigint_t* divisor,
  cardano_bigint_t*       reminder)
{
  if ((dividend == NULL) || (divisor == NULL) || (reminder == NULL))
  {
    return;
  }

  mpz_tdiv_r(reminder->mpz, dividend->mpz, divisor->mpz);
}

void
cardano_bigint_abs(const cardano_bigint_t* bignum, cardano_bigint_t* result)
{
  if ((bignum == NULL) || (result == NULL))
  {
    return;
  }

  mpz_abs(result->mpz, bignum->mpz);
}

void
cardano_bigint_gcd(const cardano_bigint_t* lhs, const cardano_bigint_t* rhs, cardano_bigint_t* result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_gcd(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_negate(const cardano_bigint_t* bignum, cardano_bigint_t* result)
{
  if ((bignum == NULL) || (result == NULL))
  {
    return;
  }

  mpz_neg(result->mpz, bignum->mpz);
}

int32_t
cardano_bigint_signum(const cardano_bigint_t* bignum)
{
  if (bignum == NULL)
  {
    return 0;
  }

  return mpz_sgn(bignum->mpz);
}

void
cardano_bigint_mod(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  cardano_bigint_reminder(lhs, rhs, result);
}

void
cardano_bigint_mod_pow(
  const cardano_bigint_t* base,
  const cardano_bigint_t* exponent,
  const cardano_bigint_t* modulus,
  cardano_bigint_t*       result)
{
  if ((base == NULL) || (exponent == NULL) || (modulus == NULL) || (result == NULL))
  {
    return;
  }

  mpz_powm(result->mpz, base->mpz, exponent->mpz, modulus->mpz);
}

void
cardano_bigint_mod_inverse(
  const cardano_bigint_t* bignum,
  const cardano_bigint_t* modulus,
  cardano_bigint_t*       result)
{
  if ((bignum == NULL) || (modulus == NULL) || (result == NULL))
  {
    return;
  }

  mpz_invert(result->mpz, bignum->mpz, modulus->mpz);
}

void
cardano_bigint_and(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_and(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_or(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_ior(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_xor(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs,
  cardano_bigint_t*       result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  mpz_xor(result->mpz, lhs->mpz, rhs->mpz);
}

void
cardano_bigint_not(
  const cardano_bigint_t* bigint,
  cardano_bigint_t*       result)
{
  if ((bigint == NULL) || (result == NULL))
  {
    return;
  }

  mpz_com(result->mpz, bigint->mpz);
}

bool
cardano_bigint_test_bit(
  const cardano_bigint_t* bigint,
  uint32_t                n)
{
  if (bigint == NULL)
  {
    return false;
  }

  return mpz_tstbit(bigint->mpz, n);
}

void
cardano_bigint_set_bit(
  cardano_bigint_t* bigint,
  uint32_t          n)
{
  if (bigint == NULL)
  {
    return;
  }

  mpz_setbit(bigint->mpz, n);
}

void
cardano_bigint_clear_bit(
  cardano_bigint_t* bigint,
  uint32_t          n)
{
  if (bigint == NULL)
  {
    return;
  }

  mpz_clrbit(bigint->mpz, n);
}

void
cardano_bigint_flip_bit(
  cardano_bigint_t* bigint,
  uint32_t          n)
{
  if (bigint == NULL)
  {
    return;
  }

  mpz_combit(bigint->mpz, n);
}

size_t
cardano_bigint_bit_count(const cardano_bigint_t* bigint)
{
  if (bigint == NULL)
  {
    return 0;
  }

  mpz_t tmp;
  mpz_init_set(tmp, bigint->mpz);

  mpz_abs(tmp, tmp);

  size_t count = 0;

  while (mpz_cmp_ui(tmp, 0) != 0)
  {
    // cppcheck-suppress misra-c2012-10.4; Reason: False positive.
    if (((int32_t)mpz_odd_p(tmp)) != 0)
    {
      ++count;
    }

    mpz_fdiv_q_2exp(tmp, tmp, 1);
  }

  mpz_clear(tmp);

  return count;
}

size_t
cardano_bigint_bit_length(const cardano_bigint_t* bigint)
{
  if (bigint == NULL)
  {
    return 0;
  }

  return mpz_sizeinbase(bigint->mpz, 2);
}

void
cardano_bigint_min(
  cardano_bigint_t* lhs,
  cardano_bigint_t* rhs,
  cardano_bigint_t* result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  if (mpz_cmp(lhs->mpz, rhs->mpz) <= 0)
  {
    mpz_set(result->mpz, lhs->mpz);
  }
  else
  {
    mpz_set(result->mpz, rhs->mpz);
  }
}

void
cardano_bigint_max(
  cardano_bigint_t* lhs,
  cardano_bigint_t* rhs,
  cardano_bigint_t* result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return;
  }

  if (mpz_cmp(lhs->mpz, rhs->mpz) >= 0)
  {
    mpz_set(result->mpz, lhs->mpz);
  }
  else
  {
    mpz_set(result->mpz, rhs->mpz);
  }
}

void
cardano_bigint_shift_left(
  const cardano_bigint_t* n,
  uint32_t                bits,
  cardano_bigint_t*       result)
{
  if ((n == NULL) || (result == NULL))
  {
    return;
  }

  mpz_mul_2exp(result->mpz, n->mpz, bits);
}

void
cardano_bigint_shift_right(
  const cardano_bigint_t* n,
  uint32_t                bits,
  cardano_bigint_t*       result)
{
  if ((n == NULL) || (result == NULL))
  {
    return;
  }

  mpz_fdiv_q_2exp(result->mpz, n->mpz, bits);
}

bool
cardano_bigint_equals(const cardano_bigint_t* lhs, const cardano_bigint_t* rhs)
{
  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  return mpz_cmp(lhs->mpz, rhs->mpz) == 0;
}

int32_t
cardano_bigint_compare(
  const cardano_bigint_t* lhs,
  const cardano_bigint_t* rhs)
{
  if ((lhs == NULL) || (rhs == NULL))
  {
    return 0;
  }

  return mpz_cmp(lhs->mpz, rhs->mpz);
}

bool
cardano_bigint_is_zero(const cardano_bigint_t* n)
{
  if (n == NULL)
  {
    return false;
  }

  return mpz_sgn(n->mpz) == 0;
}

void
cardano_bigint_increment(cardano_bigint_t* n)
{
  if (n == NULL)
  {
    return;
  }

  mpz_add_ui(n->mpz, n->mpz, 1);
}

void
cardano_bigint_decrement(cardano_bigint_t* n)
{
  if (n == NULL)
  {
    return;
  }

  mpz_sub_ui(n->mpz, n->mpz, 1);
}

void
cardano_bigint_pow(const cardano_bigint_t* base, const uint64_t exponent, cardano_bigint_t* result)
{
  if ((base == NULL) || (result == NULL))
  {
    return;
  }

  mpz_pow_ui(result->mpz, base->mpz, (unsigned long)exponent);
}

void
cardano_bigint_assign(
  const cardano_bigint_t* destination,
  cardano_bigint_t*       source)
{
  if ((destination == NULL) || (source == NULL))
  {
    return;
  }

  mpz_set(source->mpz, destination->mpz);
}

void
cardano_bigint_unref(cardano_bigint_t** bigint)
{
  if ((bigint == NULL) || (*bigint == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*bigint)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *bigint = NULL;
    return;
  }
}

void
cardano_bigint_ref(cardano_bigint_t* bigint)
{
  if (bigint == NULL)
  {
    return;
  }

  cardano_object_ref(&bigint->base);
}

size_t
cardano_bigint_refcount(const cardano_bigint_t* bigint)
{
  if (bigint == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&bigint->base);
}

void
cardano_bigint_set_last_error(cardano_bigint_t* bigint, const char* message)
{
  cardano_object_set_last_error(&bigint->base, message);
}

const char*
cardano_bigint_get_last_error(const cardano_bigint_t* bigint)
{
  return cardano_object_get_last_error(&bigint->base);
}