/**
 * \file json_object_serialization.h
 *
 * \author angel.castillo
 * \date   Nov 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_SERIALIZATION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_SERIALIZATION_H

/* INCLUDES ******************************************************************/

#include <cardano/json/json_object.h>
#include <cardano/json/json_writer.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Writes a JSON object to a JSON writer.
 *
 * This function serializes a \ref cardano_json_object_t instance into its JSON representation
 * and writes it to the specified \ref cardano_json_writer_t.
 *
 * \param[in,out] writer A pointer to the \ref cardano_json_writer_t instance responsible for writing the serialized JSON data.
 *                       This parameter must not be \c NULL.
 * \param[in]     object A pointer to the \ref cardano_json_object_t instance to be serialized and written. This parameter
 *                       must not be \c NULL.
 *
 * \return \ref CARDANO_SUCCESS if the object is successfully written; otherwise, an appropriate error code is returned.
 */
cardano_error_t
cardano_write_json_object(
  cardano_json_writer_t*       writer,
  const cardano_json_object_t* object);

/**
 * \brief Writes a JSON object of type `CARDANO_JSON_OBJECT_TYPE_OBJECT` to a JSON writer.
 *
 * This function serializes a JSON object with key-value pairs and writes it to the provided
 * JSON writer. It handles proper formatting and escaping to ensure the output conforms to the
 * JSON standard.
 *
 * \param[in,out] writer A pointer to the \ref cardano_json_writer_t instance responsible for writing
 *                       the serialized JSON data. This parameter must not be \c NULL.
 * \param[in]     object A pointer to the \ref cardano_json_object_t instance of type
 *                       `CARDANO_JSON_OBJECT_TYPE_OBJECT` to be serialized and written. This parameter
 *                       must not be \c NULL and must represent a valid object with key-value pairs.
 */
void
cardano_write_json_object_type_object(
  cardano_json_writer_t*       writer,
  const cardano_json_object_t* object);

/**
 * \brief Writes a JSON object of type `CARDANO_JSON_OBJECT_TYPE_ARRAY` to a JSON writer.
 *
 * This function serializes a JSON array and writes it to the provided JSON writer. It handles
 * proper formatting to ensure the output conforms to the JSON standard.
 *
 * \param[in,out] writer A pointer to the \ref cardano_json_writer_t instance responsible for writing
 *                       the serialized JSON data. This parameter must not be \c NULL.
 * \param[in]     array  A pointer to the \ref cardano_json_object_t instance of type
 *                       `CARDANO_JSON_OBJECT_TYPE_ARRAY` to be serialized and written. This parameter
 *                       must not be \c NULL and must represent a valid JSON array.
 */
void
cardano_write_json_object_type_array(
  cardano_json_writer_t*       writer,
  const cardano_json_object_t* array);

/**
 * \brief Writes a JSON object of type `CARDANO_JSON_OBJECT_TYPE_STRING` to a JSON writer.
 *
 * This function serializes a JSON string and writes it to the provided JSON writer, handling
 * necessary escaping and formatting to ensure the output conforms to the JSON standard.
 *
 * \param[in,out] writer     A pointer to the \ref cardano_json_writer_t instance responsible for writing
 *                           the serialized JSON data. This parameter must not be \c NULL.
 * \param[in]     string_obj A pointer to the \ref cardano_json_object_t instance of type
 *                           `CARDANO_JSON_OBJECT_TYPE_STRING` to be serialized and written. This parameter
 *                           must not be \c NULL and must represent a valid JSON string.
 */
void
cardano_write_json_object_type_string(
  cardano_json_writer_t*       writer,
  const cardano_json_object_t* string_obj);

/**
 * \brief Writes a JSON object of type `CARDANO_JSON_OBJECT_TYPE_NUMBER` to a JSON writer.
 *
 * This function serializes a JSON number and writes it to the provided JSON writer. It supports
 * both integer and floating-point numbers, ensuring proper formatting according to JSON standards.
 *
 * \param[in,out] writer     A pointer to the \ref cardano_json_writer_t instance responsible for writing
 *                           the serialized JSON data. This parameter must not be \c NULL.
 * \param[in]     number_obj A pointer to the \ref cardano_json_object_t instance of type
 *                           `CARDANO_JSON_OBJECT_TYPE_NUMBER` to be serialized and written. This parameter
 *                           must not be \c NULL and must represent a valid JSON number.
 */
void
cardano_write_json_object_type_number(
  cardano_json_writer_t*       writer,
  const cardano_json_object_t* number_obj);

/**
 * \brief Writes a JSON object of type `CARDANO_JSON_OBJECT_TYPE_BOOLEAN` to a JSON writer.
 *
 * This function serializes a JSON boolean value (true or false) and writes it to the provided
 * JSON writer in the correct JSON format.
 *
 * \param[in,out] writer    A pointer to the \ref cardano_json_writer_t instance responsible for writing
 *                          the serialized JSON data. This parameter must not be \c NULL.
 * \param[in]     bool_obj  A pointer to the \ref cardano_json_object_t instance of type
 *                          `CARDANO_JSON_OBJECT_TYPE_BOOLEAN` to be serialized and written. This parameter
 *                          must not be \c NULL and must represent a valid JSON boolean value.
 */
void
cardano_write_json_object_type_boolean(
  cardano_json_writer_t*       writer,
  const cardano_json_object_t* bool_obj);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_SERIALIZATION_H