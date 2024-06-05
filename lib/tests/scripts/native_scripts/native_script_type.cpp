/**
 * \file native_script_type.cpp
 *
 * \author angel.castillo
 * \date   Jun 03, 2024
 *
 * \section LICENSE
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

#include <cardano/scripts/native_scripts/native_script_type.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_native_script_type_to_string, canConvertRequirePubkey)
{
  const char* message = cardano_native_script_type_to_string(CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY);

  ASSERT_STREQ("Native Script Type: Require Pubkey", message);
}

TEST(cardano_native_script_type_to_string, canConvertRequireAllOf)
{
  const char* message = cardano_native_script_type_to_string(CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF);

  ASSERT_STREQ("Native Script Type: Require All Of", message);
}

TEST(cardano_native_script_type_to_string, canConvertRequireAnyOf)
{
  const char* message = cardano_native_script_type_to_string(CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF);

  ASSERT_STREQ("Native Script Type: Require Any Of", message);
}

TEST(cardano_native_script_type_to_string, canConvertRequireNOfK)
{
  const char* message = cardano_native_script_type_to_string(CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K);

  ASSERT_STREQ("Native Script Type: Require N Of K", message);
}

TEST(cardano_native_script_type_to_string, canConvertInvalidBefore)
{
  const char* message = cardano_native_script_type_to_string(CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE);

  ASSERT_STREQ("Native Script Type: Invalid Before", message);
}

TEST(cardano_native_script_type_to_string, canConvertInvalidAfter)
{
  const char* message = cardano_native_script_type_to_string(CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER);

  ASSERT_STREQ("Native Script Type: Invalid After", message);
}

TEST(cardano_native_script_type_to_string, canConvertUnknown)
{
  const char* message = cardano_native_script_type_to_string((cardano_native_script_type_t)100);

  ASSERT_STREQ("Native Script Type: Unknown", message);
}
