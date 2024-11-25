/**
 * \file unit_interval.c
 *
 * \author angel.castillo
 * \date   May 06, 2024
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

#include <cardano/common/unit_interval.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t UNIT_INTERVAL_EMBEDDED_GROUP_SIZE = 2;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Computes the greatest common divisor (GCD) of two integers.
 *
 * This function calculates the greatest common divisor (GCD) of two integers using
 * Euclid's algorithm, a recursive algorithm that finds the GCD of two numbers by
 * repeatedly taking the remainder of the division of the larger number by the smaller number
 * until the smaller number is zero.
 *
 * \param[in] a The first integer.
 * \param[in] b The second integer.
 *
 * \return The greatest common divisor of the two integers.
 * \endcode
 */
static uint64_t
compute_greatest_common_divisor(const uint64_t a, const uint64_t b)
{
  uint64_t dividend = a;
  uint64_t divisor  = b;

  while (divisor != 0U)
  {
    uint64_t remainder = dividend % divisor;
    dividend           = divisor;
    divisor            = remainder;
  }

  return dividend;
}

/**
 * \brief Converts a floating-point value to a fraction representation.
 *
 * This function converts a floating-point value to a fraction representation, where
 * the integer part, numerator, and denominator are stored in the provided pointers.
 * The fraction is reduced to its simplest form by dividing both the numerator and
 * denominator by their greatest common divisor.
 *
 * \param[in] value The floating-point value to convert to a fraction.
 * \param[out] int_part Pointer to store the integer part of the fraction.
 * \param[out] numerator Pointer to store the numerator of the fraction.
 * \param[out] denominator Pointer to store the denominator of the fraction.
 */
static void
float_to_fraction(const double value, uint64_t* numerator, uint64_t* denominator)
{
  double int_part = floor(value);
  double dec_part = value - int_part;

  if (dec_part == 0.0)
  {
    *numerator   = (uint64_t)int_part;
    *denominator = 1;
    return;
  }

  int64_t num_digits = snprintf(NULL, 0, "%.15f", dec_part) - 2;

  *numerator   = (uint64_t)floor(dec_part * pow(10, (double)num_digits));
  *denominator = (uint64_t)pow(10, (double)num_digits);

  uint64_t greatest_common_divisor = compute_greatest_common_divisor(*numerator, *denominator);

  *numerator   /= greatest_common_divisor;
  *denominator /= greatest_common_divisor;

  *numerator += (uint64_t)int_part * (*denominator);
}

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano unit interval.
 */
typedef struct cardano_unit_interval_t
{
    cardano_object_t base;
    uint64_t         numerator;
    uint64_t         denominator;
} cardano_unit_interval_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a unit interval object.
 *
 * This function is responsible for properly deallocating a unit interval object (`cardano_unit_interval_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the unit_interval object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_unit_interval_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the unit_interval
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_unit_interval_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_unit_interval_new(
  const uint64_t            numerator,
  const uint64_t            denominator,
  cardano_unit_interval_t** unit_interval)
{
  if (unit_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *unit_interval = _cardano_malloc(sizeof(cardano_unit_interval_t));

  if (*unit_interval == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*unit_interval)->base.deallocator   = cardano_unit_interval_deallocate;
  (*unit_interval)->base.ref_count     = 1;
  (*unit_interval)->base.last_error[0] = '\0';

  (*unit_interval)->numerator   = numerator;
  (*unit_interval)->denominator = denominator;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_unit_interval_from_double(const double value, cardano_unit_interval_t** unit_interval)
{
  if (value < 0.0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  uint64_t numerator   = 0U;
  uint64_t denominator = 0U;

  float_to_fraction(value, &numerator, &denominator);

  return cardano_unit_interval_new(numerator, denominator, unit_interval);
}

cardano_error_t
cardano_unit_interval_from_cbor(cardano_cbor_reader_t* reader, cardano_unit_interval_t** unit_interval)
{
  if (unit_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *unit_interval = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "unit_interval";

  const cardano_error_t expect_tag_result = cardano_cbor_validate_tag(validator_name, reader, CARDANO_ENCODED_CBOR_RATIONAL_NUMBER);

  if (expect_tag_result != CARDANO_SUCCESS)
  {
    *unit_interval = NULL;
    return expect_tag_result;
  }

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)UNIT_INTERVAL_EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *unit_interval = NULL;
    return expect_array_result;
  }

  uint64_t              numerator        = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "numerator",
    reader,
    &numerator,
    0,
    UINT64_MAX);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *unit_interval = NULL;
    return read_uint_result;
  }

  uint64_t              denominator             = 0U;
  const cardano_error_t read_denominator_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "denominator",
    reader,
    &denominator,
    0,
    UINT64_MAX);

  if (read_denominator_result != CARDANO_SUCCESS)
  {
    *unit_interval = NULL;
    return read_denominator_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *unit_interval = NULL;

    return expect_end_array_result;
  }

  return cardano_unit_interval_new(numerator, denominator, unit_interval);
}

cardano_error_t
cardano_unit_interval_to_cbor(const cardano_unit_interval_t* unit_interval, cardano_cbor_writer_t* writer)
{
  if (unit_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_tag_result = cardano_cbor_writer_write_tag(writer, CARDANO_ENCODED_CBOR_RATIONAL_NUMBER);

  if (write_tag_result != CARDANO_SUCCESS)
  {
    return write_tag_result;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    UNIT_INTERVAL_EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, unit_interval->numerator);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return cardano_cbor_writer_write_uint(writer, unit_interval->denominator);
}

uint64_t
cardano_unit_interval_get_numerator(const cardano_unit_interval_t* unit_interval)
{
  if (unit_interval == NULL)
  {
    return 0;
  }

  return unit_interval->numerator;
}

cardano_error_t
cardano_unit_interval_set_numerator(cardano_unit_interval_t* unit_interval, const uint64_t numerator)
{
  if (unit_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  unit_interval->numerator = numerator;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_unit_interval_get_denominator(const cardano_unit_interval_t* unit_interval)
{
  if (unit_interval == NULL)
  {
    return 0;
  }

  return unit_interval->denominator;
}

cardano_error_t
cardano_unit_interval_set_denominator(cardano_unit_interval_t* unit_interval, const uint64_t denominator)
{
  if (unit_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  unit_interval->denominator = denominator;

  return CARDANO_SUCCESS;
}

double
cardano_unit_interval_to_double(const cardano_unit_interval_t* unit_interval)
{
  if (unit_interval == NULL)
  {
    return 0.0;
  }

  return (double)unit_interval->numerator / (double)unit_interval->denominator;
}

void
cardano_unit_interval_unref(cardano_unit_interval_t** unit_interval)
{
  if ((unit_interval == NULL) || (*unit_interval == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*unit_interval)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *unit_interval = NULL;
    return;
  }
}

void
cardano_unit_interval_ref(cardano_unit_interval_t* unit_interval)
{
  if (unit_interval == NULL)
  {
    return;
  }

  cardano_object_ref(&unit_interval->base);
}

size_t
cardano_unit_interval_refcount(const cardano_unit_interval_t* unit_interval)
{
  if (unit_interval == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&unit_interval->base);
}

void
cardano_unit_interval_set_last_error(cardano_unit_interval_t* unit_interval, const char* message)
{
  cardano_object_set_last_error(&unit_interval->base, message);
}

const char*
cardano_unit_interval_get_last_error(const cardano_unit_interval_t* unit_interval)
{
  return cardano_object_get_last_error(&unit_interval->base);
}