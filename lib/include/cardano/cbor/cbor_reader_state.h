/**
 * \file cbor_reader_state.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * Copyright 2023 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_STATE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_STATE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Specifies the state of a CborReader instance.
 *
 * This enumeration outlines the possible states of a CborReader as it processes
 * CBOR data items.
 */
typedef enum
{
  /**
   * \brief Indicates the undefined state.
   *
   * This state is used when the CborReader has not yet begun processing
   * or the state is otherwise unknown.
   */
  CARDANO_CBOR_READER_STATE_UNDEFINED = 0,

  /**
   * \brief Indicates that the next CBOR data item is an unsigned integer (major type 0).
   */
  CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER,

  /**
   * \brief Indicates that the next CBOR data item is a negative integer (major type 1).
   */
  CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER,

  /**
   * \brief Indicates that the next CBOR data item is a byte string (major type 2).
   */
  CARDANO_CBOR_READER_STATE_BYTESTRING,

  /**
   * \brief Indicates the start of an indefinite-length byte string (major type 2).
   */
  CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING,

  /**
   * \brief Indicates the end of an indefinite-length byte string (major type 2).
   */
  CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_BYTESTRING,

  /**
   * \brief Indicates that the next CBOR data item is a UTF-8 string (major type 3).
   */
  CARDANO_CBOR_READER_STATE_TEXTSTRING,

  /**
   * \brief Indicates the start of an indefinite-length UTF-8 text string (major type 3).
   */
  CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING,

  /**
   * \brief Indicates the end of an indefinite-length UTF-8 text string (major type 3).
   */
  CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_TEXTSTRING,

  /**
   * \brief Indicates the start of an array (major type 4).
   */
  CARDANO_CBOR_READER_STATE_START_ARRAY,

  /**
   * \brief Indicates the end of an array (major type 4).
   */
  CARDANO_CBOR_READER_STATE_END_ARRAY,

  /**
   * \brief Indicates the start of a map (major type 5).
   */
  CARDANO_CBOR_READER_STATE_START_MAP,

  /**
   * \brief Indicates the end of a map (major type 5).
   */
  CARDANO_CBOR_READER_STATE_END_MAP,

  /**
   * \brief Indicates that the next CBOR data item is a semantic reader_state (major type 6).
   */
  CARDANO_CBOR_READER_STATE_TAG,

  /**
   * \brief Indicates that the next CBOR data item is a simple value (major type 7).
   */
  CARDANO_CBOR_READER_STATE_SIMPLE_VALUE,

  /**
   * \brief Indicates an IEEE 754 Half-Precision float (major type 7).
   */
  CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT,

  /**
   * \brief Indicates an IEEE 754 Single-Precision float (major type 7).
   */
  CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT,

  /**
   * \brief Indicates an IEEE 754 Double-Precision float (major type 7).
   */
  CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT,

  /**
   * \brief Indicates a null literal (major type 7).
   */
  CARDANO_CBOR_READER_STATE_NULL,

  /**
   * \brief Indicates a bool value (major type 7).
   */
  CARDANO_CBOR_READER_STATE_BOOLEAN,

  /**
   * \brief Indicates the completion of reading a full CBOR document.
   *
   * This state is reached when the CborReader has successfully processed
   * an entire CBOR document and there are no more data items to read.
   */
  CARDANO_CBOR_READER_STATE_FINISHED
} cardano_cbor_reader_state_t;

/**
 * \brief Converts CBOR reader states to their human readable form if possible.
 *
 * \param[in] reader_state The reader state to get the string representation for.
 * \return Human readable form of the given reader state. If the reader state is unknown,
 * returns "Reader State: Unknown".
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cbor_reader_state_to_string(cardano_cbor_reader_state_t reader_state);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_STATE_H