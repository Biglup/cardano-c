/**
 * \file cbor_reader_core.h
 *
 * \author luisd.bianchi
 * \date   Mar 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_CORE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_CORE_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/cbor/cbor_reader_state.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

#include "../../collections/array.h"

/* STRUCTURES ****************************************************************/

/**
 * \brief The stack frame to keep track of nested item data.
 */
typedef struct cbor_reader_stack_frame_t
{
    cardano_object_t          base;
    cardano_cbor_major_type_t type;
    uint64_t                  frame_offset;
    int64_t                   definite_length;
    uint64_t                  items_read;
    int64_t                   current_key_offset;
} cbor_reader_stack_frame_t;

/**
 * \brief A simple reader for Concise Binary Object Representation (CBOR) encoded data.
 */
typedef struct cardano_cbor_reader_t
{
    cardano_object_t            base;
    cardano_buffer_t*           buffer;
    uint64_t                    offset;
    cardano_array_t*            nested_items;
    bool                        is_tag_context;
    cbor_reader_stack_frame_t   current_frame;
    cardano_cbor_reader_state_t cached_state;
} cardano_cbor_reader_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Pushes a new data item onto the CBOR reader's state stack.
 *
 * This internal function is used to update the CBOR reader's state when it encounters a new data item
 * of a specific major type and length. It handles both definite and indefinite length data items.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that is parsing the CBOR data.
 * \param[in] type The CBOR major type of the new data item encountered.
 * \param[in] definite_length The length of the data item if it is of definite length. For indefinite length
 *                            data items, this parameter should be set to a special value (e.g., -1) to indicate
 *                            the indefinite length nature of the item.
 *
 * \return A \ref cardano_error_t indicating the success of the operation. \c CARDANO_SUCCESS is returned if
 *         the new data item was successfully pushed onto the state stack, allowing for correct parsing of
 *         nested structures. If an error occurs, an appropriate error code is returned, indicating the issue
 *         encountered during this state update operation.
 */
cardano_error_t
_cbor_reader_push_data_item(cardano_cbor_reader_t* reader, cardano_cbor_major_type_t type, int64_t definite_length);

/**
 * \brief Removes the most recently read data item from the reader's internal stack, verifying it matches
 * the expected major type.
 *
 * This function is used to ensure the data item just read from the CBOR stream matches an expected major type.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that is processing the CBOR stream.
 * \param[in] expected_type The \ref cardano_cbor_major_type_t that the most recently read data item is expected to be.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. If the most recently read data
 * item matches the expected major type, \ref CARDANO_SUCCESS is returned, and the item is removed from
 * the reader's internal stack. If the data item does not match the expected type, an error code is returned
 * to indicate the type mismatch. Other error codes may be returned to indicate different failure conditions,
 * such as attempting to pop an item from an empty stack.
 */
cardano_error_t
_cbor_reader_pop_data_item(cardano_cbor_reader_t* reader, cardano_cbor_major_type_t expected_type);

/**
 * \brief Peeks at the next initial byte in the CBOR stream without advancing the reader.
 *
 * This function allows for inspecting the next initial byte in the CBOR data stream to verify it matches
 * an expected CBOR major type.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance, representing the CBOR stream being parsed.
 * \param[in] expected_type The CBOR major type that the function expects to find as the next initial byte. This is used
 * to validate the data's structure before proceeding with parsing operations.
 * \param[out] initial_byte A pointer to a byte variable where the function will store the value of the next initial byte
 * if it matches the expected type. This parameter allows the caller to inspect the initial byte value for further processing.
 *
 * \return A \ref cardano_error_t indicating the success of the operation. \c CARDANO_SUCCESS is returned if the next initial
 * byte matches the expected type and the operation completes successfully. If the next initial byte does not match the expected
 * type or if another error occurs, an appropriate error code is returned.
 */
cardano_error_t
_cbor_reader_peek_initial_byte(cardano_cbor_reader_t* reader, cardano_cbor_major_type_t expected_type, byte_t* initial_byte);

/**
 * \brief Advances the internal buffer of the CBOR reader by a specified length.
 *
 * This function is used to move the CBOR reader's internal pointer forward, effectively skipping over
 * a given number of bytes in the buffer.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance.
 * \param[in] length The number of bytes to advance the buffer by. The reader's internal pointer will be
 * moved forward by this amount, bypassing the corresponding segment of the buffer.
 *
 * \return A \ref cardano_error_t indicating the success of the operation. \c CARDANO_SUCCESS is returned
 * if the buffer was successfully advanced. If attempting to advance beyond the bounds of the buffer or
 * another error occurs, an appropriate error code is returned.
 */
cardano_error_t
_cbor_reader_advance_buffer(cardano_cbor_reader_t* reader, size_t length);

/**
 * \brief The function increments the reader's data item counter, which tracks the number of data items
 * that have been parsed.
 *
 * This internal utility function updates the CBOR reader's state to reflect the completion
 * of parsing a data item. It is typically called after a data item has been successfully
 * read from the buffer, to prepare the reader for processing the next item.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance. This instance's internal
 * state will be updated to account for the advancement past a data item.
 */
void
_cbor_reader_advance_data_item_counters(cardano_cbor_reader_t* reader);

/**
 * \brief Skips the next node in the CBOR data structure, tracking the depth.
 *
 * This function advances the reader past the next data node, which may be a single data item
 * or a more complex structure such as an array or a map. When encountering nested structures,
 * the function recursively processes each nested node, ensuring that the reader is correctly
 * positioned after the entire structure has been skipped.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance. This instance's position
 * will be advanced past the next node in the CBOR data stream.
 * \param[out] depth A pointer to a size_t variable where the function can store the depth
 * of nesting encountered while skipping the node.
 *
 * \return A \ref cardano_error_t indicating the result of the skip operation. \ref CARDANO_SUCCESS is
 * returned if the next node is successfully skipped. If an error occurs during the skip, such as
 * encountering malformed CBOR data or reaching the end of the buffer unexpectedly, an appropriate
 * error code is returned indicating the failure reason.
 */
cardano_error_t
_cbor_reader_skip_next_node(cardano_cbor_reader_t* reader, size_t* depth);

/**
 * \brief Peeks at the current state of the CBOR reader without advancing its position.
 *
 * This function examines the current position of the CBOR reader to determine the state of the
 * next data item in the CBOR stream. Allowing the caller to make informed decisions on how to proceed
 * with parsing.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance whose current state is
 * being queried.
 * \param[out] state A pointer to a \ref cardano_cbor_reader_state_t structure where the function will
 * store information about the current state of the reader.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. \ref CARDANO_SUCCESS is
 * returned if the reader's state is successfully obtained. If an error occurs, such as if the reader
 * is at the end of the stream or if there is malformed CBOR data, an appropriate error code is
 * returned indicating the failure reason.
 */
cardano_error_t
_cbor_reader_peek_state(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_CORE_H