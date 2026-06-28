/**
 * \file uplc_program.c
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>

#include "uplc_constant.h"
#include "uplc_program.h"
#include "uplc_term.h"

#include "../arena/uplc_arena.h"
#include "../flat/flat_decode.h"
#include "../flat/flat_encode.h"
#include "../flat/flat_reader.h"

#include <cardano/buffer.h>

#include <stddef.h>
#include <stdint.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Reads exactly one CBOR bytestring that consumes the whole reader.
 *
 * Succeeds only when the next item is a definite-length CBOR bytestring (major
 * type 2) and the reader holds nothing after it. The exact-consumption rule
 * rejects any trailing bytes after the bytestring, and the indefinite-length
 * form is rejected because it does not peek as a single bytestring item.
 *
 * \param[in] data The bytes to read. May be NULL only when \p size is 0.
 * \param[in] size The number of bytes in \p data.
 * \param[out] inner On success, the decoded bytestring contents; the caller
 *             releases it with \ref cardano_buffer_unref. Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING when the
 *         item is not a single all-consuming bytestring, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the reader cannot be made.
 */
static cardano_error_t
peel_bytestring(const byte_t* data, const size_t size, cardano_buffer_t** inner)
{
  cardano_cbor_reader_t*      reader    = NULL;
  cardano_buffer_t*           bytes     = NULL;
  cardano_cbor_reader_state_t state     = CARDANO_CBOR_READER_STATE_UNDEFINED;
  size_t                      remaining = 0U;
  cardano_error_t             result    = CARDANO_SUCCESS;

  if (size == 0U)
  {
    return CARDANO_ERROR_DECODING;
  }

  reader = cardano_cbor_reader_new(data, size);

  if (reader == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_cbor_reader_peek_state(reader, &state);

  if ((result != CARDANO_SUCCESS) || (state != CARDANO_CBOR_READER_STATE_BYTESTRING))
  {
    cardano_cbor_reader_unref(&reader);

    return CARDANO_ERROR_DECODING;
  }

  result = cardano_cbor_reader_read_bytestring(reader, &bytes);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader);

    return CARDANO_ERROR_DECODING;
  }

  result = cardano_cbor_reader_get_bytes_remaining(reader, &remaining);

  if ((result != CARDANO_SUCCESS) || (remaining != 0U))
  {
    cardano_buffer_unref(&bytes);
    cardano_cbor_reader_unref(&reader);

    return CARDANO_ERROR_DECODING;
  }

  cardano_cbor_reader_unref(&reader);

  *inner = bytes;

  return CARDANO_SUCCESS;
}

/**
 * \brief Runs the flat program decoder over a buffer of flat bytes.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in] flat The flat-encoded program bytes.
 * \param[out] program On success, the decoded program; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated decode or allocation
 *         error from the flat program decoder.
 */
static cardano_error_t
decode_flat(
  cardano_uplc_arena_t*          arena,
  const cardano_buffer_t*        flat,
  const cardano_uplc_program_t** program)
{
  cardano_uplc_flat_reader_t reader = { NULL, 0U, 0U, 0U };
  cardano_error_t            result = cardano_uplc_flat_reader_init(&reader, cardano_buffer_get_data(flat), cardano_buffer_get_size(flat));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_uplc_flat_decode_program(arena, &reader, program);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_program_from_script_bytes(
  cardano_uplc_arena_t*          arena,
  const byte_t*                  script_bytes,
  size_t                         size,
  const cardano_uplc_program_t** program)
{
  cardano_buffer_t* outer  = NULL;
  cardano_error_t   result = CARDANO_SUCCESS;

  if ((arena == NULL) || (program == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((script_bytes == NULL) && (size != 0U))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = peel_bytestring(script_bytes, size, &outer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = decode_flat(arena, outer, program);

  cardano_buffer_unref(&outer);

  return result;
}

cardano_error_t
cardano_uplc_flat_encode_program(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out_flat)
{
  if ((program == NULL) || (program->term == NULL) || (out_flat == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_uplc_int_flat_encode_program(program, out_flat);
}

cardano_error_t
cardano_uplc_program_to_cbor(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out_cbor)
{
  cardano_buffer_t*      flat   = NULL;
  cardano_cbor_writer_t* writer = NULL;
  cardano_error_t        result = CARDANO_SUCCESS;

  if ((program == NULL) || (program->term == NULL) || (out_cbor == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_flat_encode_program(program, &flat);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    cardano_buffer_unref(&flat);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(flat), cardano_buffer_get_size(flat));

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_cbor_writer_encode_in_buffer(writer, out_cbor);
  }

  cardano_cbor_writer_unref(&writer);
  cardano_buffer_unref(&flat);

  return result;
}

cardano_error_t
cardano_uplc_program_apply_data(
  cardano_uplc_arena_t*         arena,
  const cardano_uplc_program_t* program,
  cardano_plutus_data_t*        param,
  cardano_uplc_program_t**      out)
{
  cardano_uplc_program_t*  result        = NULL;
  cardano_uplc_constant_t* data_constant = NULL;
  cardano_uplc_term_t*     constant_term = NULL;
  cardano_uplc_term_t*     apply_term    = NULL;
  cardano_error_t          build_res     = CARDANO_SUCCESS;

  if ((arena == NULL) || (program == NULL) || (program->term == NULL) || (param == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = (cardano_uplc_program_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  build_res = cardano_uplc_constant_new_data(arena, param, &data_constant);

  if (build_res != CARDANO_SUCCESS)
  {
    return build_res;
  }

  build_res = cardano_uplc_term_new_constant(arena, data_constant, &constant_term);

  if (build_res != CARDANO_SUCCESS)
  {
    return build_res;
  }

  build_res = cardano_uplc_term_new_apply(arena, program->term, constant_term, &apply_term);

  if (build_res != CARDANO_SUCCESS)
  {
    return build_res;
  }

  result->version_major = program->version_major;
  result->version_minor = program->version_minor;
  result->version_patch = program->version_patch;
  result->term          = apply_term;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_program_apply_data_params(
  cardano_uplc_arena_t*         arena,
  const cardano_uplc_program_t* program,
  cardano_plutus_data_t* const* params,
  size_t                        count,
  cardano_uplc_program_t**      out)
{
  cardano_uplc_program_t* current = NULL;
  cardano_uplc_program_t* applied = NULL;
  size_t                  i       = 0U;

  if ((arena == NULL) || (program == NULL) || (program->term == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((params == NULL) && (count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  current = (cardano_uplc_program_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U);

  if (current == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  current->version_major = program->version_major;
  current->version_minor = program->version_minor;
  current->version_patch = program->version_patch;
  current->term          = program->term;

  for (i = 0U; i < count; ++i)
  {
    cardano_error_t apply_res;

    if (params[i] == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    apply_res = cardano_uplc_program_apply_data(arena, current, params[i], &applied);

    if (apply_res != CARDANO_SUCCESS)
    {
      return apply_res;
    }

    current = applied;
  }

  *out = current;

  return CARDANO_SUCCESS;
}
