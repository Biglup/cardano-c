/**
 * \file cbor_tag.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_tag.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_cbor_tag_to_string(const cardano_cbor_tag_t tag)
{
  const char* message;

  switch (tag)
  {
    case CARDANO_CBOR_TAG_DATE_TIME_STRING:
      message = "Tag: Date Time String";
      break;
    case CARDANO_CBOR_TAG_UNIX_TIME_SECONDS:
      message = "Tag: Unix Time Seconds";
      break;
    case CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM:
      message = "Tag: Unsigned Bignum";
      break;
    case CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM:
      message = "Tag: Negative Bignum";
      break;
    case CARDANO_CBOR_TAG_DECIMAL_FRACTION:
      message = "Tag: Decimal Fraction";
      break;
    case CARDANO_CBOR_TAG_BIG_FLOAT:
      message = "Tag: Big Float";
      break;
    case CARDANO_ENCODED_CBOR_RATIONAL_NUMBER:
      message = "Tag: Rational Number";
      break;
    case CARDANO_ENCODED_CBOR_DATA_ITEM:
      message = "Tag: CBOR Data Item";
      break;
    case CARDANO_CBOR_TAG_SET:
      message = "Tag: Set";
      break;
    case CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR:
      message = "Tag: Self Describe CBOR";
      break;
    default:
      message = "Tag: Custom";
      break;
  }

  return message;
}
