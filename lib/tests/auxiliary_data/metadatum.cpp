/**
 * \file metadatum.cpp
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#include <cardano/error.h>

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_list.h>
#include <cardano/auxiliary_data/metadatum_map.h>
#include <cardano/buffer.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* METADATUM_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";
static const char* JSON_1 =
  "{\n"
  "  \"160b85e53e25ef49272c421f04b702bc32184d102865fd1dc8815cde\": {\n"
  "    \"Horrocube00726x666666ED\": {\n"
  "      \"cards\": [\n"
  "        {\n"
  "          \"name\": \"HERMIT\"\n"
  "        },\n"
  "        {\n"
  "          \"name\": \"TEMPERANCE\"\n"
  "        },\n"
  "        {\n"
  "          \"name\": \"DEVIL\"\n"
  "        }\n"
  "      ],\n"
  "      \"image\": \"ipfs://QmTqFMxoDzQE13oxVatkWsda4uZGJRL5tksJLQv9JRvNrQ\",\n"
  "      \"name\": \"Horrocube #00726\",\n"
  "      \"properties\": {\n"
  "        \"aspect\": \"Anguish\",\n"
  "        \"background\": \"Green\",\n"
  "        \"commuter\": \"Oneiric\",\n"
  "        \"core\": \"Golden Kadathian Steel Fragment\",\n"
  "        \"mechanism\": \"Gaian Ripper\",\n"
  "        \"ornament\": \"Molten Voonith Tusks\",\n"
  "        \"supports\": \"Luciferian Shackle\",\n"
  "        \"id\": 0\n"
  "      },\n"
  "      \"signature\": {\n"
  "        \"r\": \"AJRpTJaMu7CV7k6IRckO+qFzbQHMyVlQnGhllgqz2uED\",\n"
  "        \"s\": \"AOtRBblVdO05Owuck9UCU15l6xZO/f/cSkcMRAToVTSs\",\n"
  "        \"securityAlgorithm\": \"EcdsaSecp256k1Sha256\"\n"
  "      }\n"
  "    }\n"
  "  }\n"
  "}";

const char* JSON_1_CBOR = "a178383136306238356535336532356566343932373263343231663034623730326263333231383464313032383635666431646338383135636465a177486f72726f637562653030373236783636363636364544a56563617264739fa1646e616d65664845524d4954a1646e616d656a54454d504552414e4345a1646e616d6565444556494cff65696d6167657835697066733a2f2f516d5471464d786f447a514531336f785661746b5773646134755a474a524c35746b734a4c5176394a52764e7251646e616d6570486f72726f63756265202330303732366a70726f70657274696573a86661737065637467416e67756973686a6261636b67726f756e6465477265656e68636f6d6d75746572674f6e656972696364636f7265781f476f6c64656e204b616461746869616e20537465656c20467261676d656e74696d656368616e69736d6c476169616e20526970706572686f726e616d656e74744d6f6c74656e20566f6f6e697468205475736b7368737570706f727473724c75636966657269616e20536861636b6c6562696400697369676e6174757265a36172782c414a5270544a614d75374356376b364952636b4f2b71467a6251484d79566c516e47686c6c67717a327545446173782c414f745242626c56644f30354f7775636b3955435531356c36785a4f2f662f63536b634d5241546f56545373717365637572697479416c676f726974686d744563647361536563703235366b31536861323536";

static const char* CIP116_JSON =
  "{\n"
  "  \"tag\": \"map\",\n"
  "  \"contents\": [\n"
  "    {\n"
  "      \"key\": {\n"
  "        \"tag\": \"string\",\n"
  "        \"value\": \"160b85e53e25ef49272c421f04b702bc32184d102865fd1dc8815cde\"\n"
  "      },\n"
  "      \"value\": {\n"
  "        \"tag\": \"map\",\n"
  "        \"contents\": [\n"
  "          {\n"
  "            \"key\": {\n"
  "              \"tag\": \"string\",\n"
  "              \"value\": \"Horrocube00726x666666ED\"\n"
  "            },\n"
  "            \"value\": {\n"
  "              \"tag\": \"map\",\n"
  "              \"contents\": [\n"
  "                {\n"
  "                  \"key\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"cards\"\n"
  "                  },\n"
  "                  \"value\": {\n"
  "                    \"tag\": \"list\",\n"
  "                    \"contents\": [\n"
  "                      {\n"
  "                        \"tag\": \"map\",\n"
  "                        \"contents\": [\n"
  "                          {\n"
  "                            \"key\": {\n"
  "                              \"tag\": \"string\",\n"
  "                              \"value\": \"name\"\n"
  "                            },\n"
  "                            \"value\": {\n"
  "                              \"tag\": \"string\",\n"
  "                              \"value\": \"HERMIT\"\n"
  "                            }\n"
  "                          }\n"
  "                        ]\n"
  "                      },\n"
  "                      {\n"
  "                        \"tag\": \"map\",\n"
  "                        \"contents\": [\n"
  "                          {\n"
  "                            \"key\": {\n"
  "                              \"tag\": \"string\",\n"
  "                              \"value\": \"name\"\n"
  "                            },\n"
  "                            \"value\": {\n"
  "                              \"tag\": \"string\",\n"
  "                              \"value\": \"TEMPERANCE\"\n"
  "                            }\n"
  "                          }\n"
  "                        ]\n"
  "                      },\n"
  "                      {\n"
  "                        \"tag\": \"map\",\n"
  "                        \"contents\": [\n"
  "                          {\n"
  "                            \"key\": {\n"
  "                              \"tag\": \"string\",\n"
  "                              \"value\": \"name\"\n"
  "                            },\n"
  "                            \"value\": {\n"
  "                              \"tag\": \"string\",\n"
  "                              \"value\": \"DEVIL\"\n"
  "                            }\n"
  "                          }\n"
  "                        ]\n"
  "                      }\n"
  "                    ]\n"
  "                  }\n"
  "                },\n"
  "                {\n"
  "                  \"key\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"image\"\n"
  "                  },\n"
  "                  \"value\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"ipfs://QmTqFMxoDzQE13oxVatkWsda4uZGJRL5tksJLQv9JRvNrQ\"\n"
  "                  }\n"
  "                },\n"
  "                {\n"
  "                  \"key\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"name\"\n"
  "                  },\n"
  "                  \"value\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"Horrocube #00726\"\n"
  "                  }\n"
  "                },\n"
  "                {\n"
  "                  \"key\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"properties\"\n"
  "                  },\n"
  "                  \"value\": {\n"
  "                    \"tag\": \"map\",\n"
  "                    \"contents\": [\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"aspect\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Anguish\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"background\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Green\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"commuter\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Oneiric\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"core\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Golden Kadathian Steel Fragment\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"mechanism\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Gaian Ripper\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"ornament\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Molten Voonith Tusks\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"supports\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"Luciferian Shackle\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"id\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"int\",\n"
  "                          \"value\": ,\"0\"\n"
  "                        }\n"
  "                      }\n"
  "                    ]\n"
  "                  }\n"
  "                },\n"
  "                {\n"
  "                  \"key\": {\n"
  "                    \"tag\": \"string\",\n"
  "                    \"value\": \"signature\"\n"
  "                  },\n"
  "                  \"value\": {\n"
  "                    \"tag\": \"map\",\n"
  "                    \"contents\": [\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"r\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"AJRpTJaMu7CV7k6IRckO+qFzbQHMyVlQnGhllgqz2uED\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"s\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"AOtRBblVdO05Owuck9UCU15l6xZO/f/cSkcMRAToVTSs\"\n"
  "                        }\n"
  "                      },\n"
  "                      {\n"
  "                        \"key\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"securityAlgorithm\"\n"
  "                        },\n"
  "                        \"value\": {\n"
  "                          \"tag\": \"string\",\n"
  "                          \"value\": \"EcdsaSecp256k1Sha256\"\n"
  "                        }\n"
  "                      }\n"
  "                    ]\n"
  "                  }\n"
  "                }\n"
  "              ]\n"
  "            }\n"
  "          }\n"
  "        ]\n"
  "      }\n"
  "    }\n"
  "  ]\n"
  "}";

/* UNIT TESTS ****************************************************************/

TEST(cardano_metadatum_new_integer_from_int, returnsErrorWhenMemoryAllocationFailes)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_map, canCreateNewMap)
{
  // Arrange
  cardano_metadatum_t*     metadatum     = nullptr;
  cardano_metadatum_map_t* map_metadatum = nullptr;

  EXPECT_EQ(cardano_metadatum_map_new(&map_metadatum), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_metadatum_new_map(map_metadatum, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  // Cleanup
  cardano_metadatum_map_unref(&map_metadatum);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_new_map, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_map(nullptr, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_map, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_metadatum_map_t* map_metadatum = nullptr;
  EXPECT_EQ(cardano_metadatum_map_new(&map_metadatum), CARDANO_SUCCESS);

  cardano_error_t error = cardano_metadatum_new_map(map_metadatum, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_metadatum_map_unref(&map_metadatum);
}

TEST(cardano_metadatum_new_map, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_metadatum_t*     metadatum     = nullptr;
  cardano_metadatum_map_t* map_metadatum = nullptr;
  EXPECT_EQ(cardano_metadatum_map_new(&map_metadatum), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_metadatum_new_map(map_metadatum, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_metadatum_map_unref(&map_metadatum);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_list, canCreateANewList)
{
  // Arrange
  cardano_metadatum_t*      metadatum = nullptr;
  cardano_metadatum_list_t* list      = nullptr;

  EXPECT_EQ(cardano_metadatum_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_metadatum_new_list(list, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  // Cleanup
  cardano_metadatum_list_unref(&list);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_new_list, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_list(nullptr, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_list, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_metadatum_list_t* list = nullptr;
  EXPECT_EQ(cardano_metadatum_list_new(&list), CARDANO_SUCCESS);

  cardano_error_t error = cardano_metadatum_new_list(list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_metadatum_list_unref(&list);
}

TEST(cardano_metadatum_new_list, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_metadatum_t*      metadatum = nullptr;
  cardano_metadatum_list_t* list      = nullptr;
  EXPECT_EQ(cardano_metadatum_list_new(&list), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_metadatum_new_list(list, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_metadatum_list_unref(&list);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_integer_from_int, canCreateAnIntegerMetadatum)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_new_integer_from_int, returnsErrorIfMetadatumIsNull)
{
  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_integer_from_int, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_integer_from_int, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_bytes, canCreateABytesMetadatum)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  const uint8_t        bytes[]   = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(bytes, sizeof(bytes), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_new_bytes, canCreateAStringMetadatum)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  const char*          text      = "Hello World!";

  // Act
  cardano_error_t error = cardano_metadatum_new_string(text, strlen(text), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_new_bytes, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  const uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(bytes, sizeof(bytes), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_bytes, returnsErrorIfBytesIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  const uint8_t        bytes[]   = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(nullptr, sizeof(bytes), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_bytes, returnsErrorIfStringIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(nullptr, 10, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  const uint8_t bytes[] = { 0x85, 0x01, 0x02, 0x03, 0x04, 0x05 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(bytes, sizeof(bytes), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_bytes, returnsErrorIfMemoryEventuallyAllocationFails)
{
  // Arrange
  const uint8_t bytes[] = { 0x85, 0x01, 0x02, 0x03, 0x04, 0x05 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(bytes, sizeof(bytes), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_bytes_from_hex, canCreateABytesMetadatumFromHex)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  const char*          hex       = "850102030405";

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes_from_hex(hex, strlen(hex), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_new_bytes_from_hex, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  const char* hex = "850102030405";

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes_from_hex(hex, strlen(hex), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_bytes_from_hex, returnsErrorIfHexIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes_from_hex(nullptr, 0, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  const char* hex = "850102030405";

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes_from_hex(hex, strlen(hex), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_bytes_from_hex, returnsErrorIfMemoryAllocationEventuallyFails)
{
  // Arrange
  const char* hex = "850102030405";

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes_from_hex(hex, strlen(hex), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, canDeserializeAnIntegerMetadatum)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, canDeserializeAnStringMetadatum)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("6474657374", strlen("6474657374"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  const size_t size = cardano_metadatum_get_string_size(metadatum);
  char*        text = (char*)malloc(size);
  EXPECT_EQ(cardano_metadatum_to_string(metadatum, text, size), CARDANO_SUCCESS);

  EXPECT_STREQ(text, "test");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
  free(text);
}

TEST(cardano_metadatum_from_cbor, canDecodeNegativeInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("24", strlen("24"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), -5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, canDecodeBigPositiveInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 72057594037927936);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfMemoryInt)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("24", strlen("24"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfMemoryInt2)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("24", strlen("24"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfInvalidBigPositiveInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c2490001000000000000", strlen("c2490001000000000000"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigPositiveInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigPositiveInteger2)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, canDecodeBigNegativeInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), -72057594037927936);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfInvalidBigNegativeInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c3490001000000000000", strlen("c3490001000000000000"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigNegativeInteger)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigNegativeInteger2)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, canDeserializeABytesMetadatum)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("450102030405", strlen("450102030405"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_buffer_t* buffer = NULL;
  EXPECT_EQ(cardano_metadatum_to_bounded_bytes(metadatum, &buffer), CARDANO_SUCCESS);

  const uint8_t expected_bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  EXPECT_EQ(cardano_buffer_get_size(buffer), sizeof(expected_bytes));

  const byte_t* bytes = cardano_buffer_get_data(buffer);
  for (size_t i = 0; i < sizeof(expected_bytes); ++i)
  {
    EXPECT_EQ(bytes[i], expected_bytes[i]);
  }

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfMemoryBytes)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("450102030405", strlen("450102030405"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfMemoryText)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("6474657374", strlen("6474657374"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, canDeserializeAListMetadatum)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("9f0102030405ff", strlen("9f0102030405ff"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_metadatum_list_t* list = NULL;

  EXPECT_EQ(cardano_metadatum_to_list(metadatum, &list), CARDANO_SUCCESS);
  cardano_metadatum_unref(&metadatum);

  const size_t length = cardano_metadatum_list_get_length(list);

  EXPECT_EQ(length, 5);

  cardano_metadatum_t* elem1 = NULL;
  cardano_metadatum_t* elem2 = NULL;
  cardano_metadatum_t* elem3 = NULL;
  cardano_metadatum_t* elem4 = NULL;
  cardano_metadatum_t* elem5 = NULL;

  EXPECT_EQ(cardano_metadatum_list_get(list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(list, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(list, 4, &elem5), CARDANO_SUCCESS);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(elem3, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 3);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(elem4, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 4);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(elem5, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_list_unref(&list);
  cardano_cbor_reader_unref(&reader);
  cardano_metadatum_unref(&elem1);
  cardano_metadatum_unref(&elem2);
  cardano_metadatum_unref(&elem3);
  cardano_metadatum_unref(&elem4);
  cardano_metadatum_unref(&elem5);
}

TEST(cardano_metadatum_from_cbor, canDeserializeAMapMetadatum)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("a3010402050306", strlen("a3010402050306"));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_metadatum_map_t* map = NULL;

  EXPECT_EQ(cardano_metadatum_to_map(metadatum, &map), CARDANO_SUCCESS);
  cardano_metadatum_unref(&metadatum);

  const size_t length = cardano_metadatum_map_get_length(map);

  EXPECT_EQ(length, 3);

  cardano_metadatum_list_t* keys = NULL;
  cardano_metadatum_t*      key1 = NULL;
  cardano_metadatum_t*      key2 = NULL;
  cardano_metadatum_t*      key3 = NULL;

  cardano_metadatum_t* value1 = NULL;
  cardano_metadatum_t* value2 = NULL;
  cardano_metadatum_t* value3 = NULL;

  EXPECT_EQ(cardano_metadatum_map_get_keys(map, &keys), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_list_get(keys, 0, &key1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(keys, 1, &key2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(keys, 2, &key3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_get(map, key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_map_get(map, key2, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_map_get(map, key3, &value3), CARDANO_SUCCESS);

  cardano_bigint_t* key_value = NULL;
  cardano_bigint_t* value     = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(key1, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 1);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_metadatum_to_integer(key2, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 2);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_metadatum_to_integer(key3, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 3);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_metadatum_to_integer(value1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 4);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(value2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 5);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(value3, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 6);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_map_unref(&map);
  cardano_cbor_reader_unref(&reader);
  cardano_metadatum_list_unref(&keys);
  cardano_metadatum_unref(&key1);
  cardano_metadatum_unref(&key2);
  cardano_metadatum_unref(&key3);
  cardano_metadatum_unref(&value1);
  cardano_metadatum_unref(&value2);
  cardano_metadatum_unref(&value3);
}

TEST(cardano_metadatum_from_cbor, returnsErrorIfMemoryMap)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("a3010402050306", strlen("a3010402050306"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_to_cbor, canEncodeMapToCbor)
{
  // Arrange
  cardano_metadatum_t*     metadatum = nullptr;
  cardano_metadatum_map_t* map       = nullptr;
  cardano_metadatum_t*     key1      = nullptr;
  cardano_metadatum_t*     key2      = nullptr;
  cardano_metadatum_t*     key3      = nullptr;
  cardano_metadatum_t*     value1    = nullptr;
  cardano_metadatum_t*     value2    = nullptr;
  cardano_metadatum_t*     value3    = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &key2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(3, &key3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(4, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(5, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(6, &value3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_new(&map), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(map, key1, value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_map_insert(map, key2, value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_map_insert(map, key3, value3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_new_map(map, &metadatum), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_metadatum_to_cbor(metadatum, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("a3010402050306") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a3010402050306");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_map_unref(&map);
  cardano_metadatum_unref(&key1);
  cardano_metadatum_unref(&key2);
  cardano_metadatum_unref(&key3);
  cardano_metadatum_unref(&value1);
  cardano_metadatum_unref(&value2);
  cardano_metadatum_unref(&value3);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_to_cbor, canEncodeSmallByteArray)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  const uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  EXPECT_EQ(cardano_metadatum_new_bytes(bytes, sizeof(bytes), &metadatum), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_metadatum_to_cbor(metadatum, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("450102030405") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "450102030405");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_metadatum_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_metadatum_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_to_cbor(metadatum, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("9f0102ff", strlen("9f0102ff"));
  cardano_cbor_writer_t* writer    = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_to_cbor(metadatum, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("9f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "9f0102ff");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfPlutusListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(METADATUM_CBOR, strlen(METADATUM_CBOR));

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(nullptr, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_metadatum_t*   metadatum = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(METADATUM_CBOR, strlen(METADATUM_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfInvalidMetadatumElements)
{
  // Arrange
  cardano_metadatum_t*   list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for metadatum.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_metadatum_t*   list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_metadatum_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected end of buffer.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_ref(metadatum);

  // Assert
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));
  EXPECT_EQ(cardano_metadatum_refcount(metadatum), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_ref(nullptr);
}

TEST(cardano_metadatum_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_unref((cardano_metadatum_t**)nullptr);
}

TEST(cardano_metadatum_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_ref(metadatum);
  size_t ref_count = cardano_metadatum_refcount(metadatum);

  cardano_metadatum_unref(&metadatum);
  size_t updated_ref_count = cardano_metadatum_refcount(metadatum);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_ref(metadatum);
  size_t ref_count = cardano_metadatum_refcount(metadatum);

  cardano_metadatum_unref(&metadatum);
  size_t updated_ref_count = cardano_metadatum_refcount(metadatum);

  cardano_metadatum_unref(&metadatum);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(metadatum, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_metadatum_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_metadatum_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  const char*          message   = "This is a test message";

  // Act
  cardano_metadatum_set_last_error(metadatum, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_get_last_error(metadatum), "Object is NULL.");
}

TEST(cardano_metadatum_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_metadatum_set_last_error(metadatum, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_get_last_error(metadatum), "");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_integer, returnsTheIntegerValue)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_bigint_to_int(value), 1);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_bigint_unref(&value);
}

TEST(cardano_metadatum_to_integer, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_integer, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_integer, returnsErrorIfMetadatumIsNotAnInteger)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(metadatum, &value), CARDANO_ERROR_INVALID_METADATUM_CONVERSION);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_bytes, returnsTheBytesValue)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_metadatum_to_bounded_bytes(metadatum, &buffer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), 4);
  EXPECT_EQ(memcmp(cardano_buffer_get_data(buffer), "test", 4), 0);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_metadatum_to_bytes, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_metadatum_to_bounded_bytes(metadatum, &buffer), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_metadatum_to_bounded_bytes(metadatum, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_bytes, returnsErrorIfMetadatumIsNotAByteArray)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_integer_from_int(1, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_metadatum_to_bounded_bytes(metadatum, &buffer), CARDANO_ERROR_INVALID_METADATUM_CONVERSION);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_map, returnsTheMapValue)
{
  // Arrange
  cardano_metadatum_t*     metadatum = nullptr;
  cardano_metadatum_map_t* map       = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_map(map, &metadatum);
  cardano_metadatum_map_unref(&map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_metadatum_to_map(metadatum, &map), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(map, testing::Not((cardano_metadatum_map_t*)nullptr));

  // Cleanup
  cardano_metadatum_map_unref(&map);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_map, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_metadatum_map_t* map = NULL;

  EXPECT_EQ(cardano_metadatum_to_map(metadatum, &map), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&map);
}

TEST(cardano_metadatum_to_map, returnsErrorIfMapIsNull)
{
  // Arrange
  cardano_metadatum_t*     metadatum = nullptr;
  cardano_metadatum_map_t* map       = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_map(map, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_metadatum_to_map(metadatum, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_map_unref(&map);
}

TEST(cardano_metadatum_to_map, returnsErrorIfMetadatumIsNotAMap)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_map_t* map = NULL;

  EXPECT_EQ(cardano_metadatum_to_map(metadatum, &map), CARDANO_ERROR_INVALID_METADATUM_CONVERSION);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_list, returnsTheListValue)
{
  // Arrange
  cardano_metadatum_t*      metadatum = nullptr;
  cardano_metadatum_list_t* list      = nullptr;

  cardano_error_t error = cardano_metadatum_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_list(list, &metadatum);
  cardano_metadatum_list_unref(&list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act

  EXPECT_EQ(cardano_metadatum_to_list(metadatum, &list), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_metadatum_list_t*)nullptr));

  // Cleanup
  cardano_metadatum_list_unref(&list);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_to_list, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_metadatum_list_t* list = NULL;

  EXPECT_EQ(cardano_metadatum_to_list(metadatum, &list), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_list_unref(&list);
}

TEST(cardano_metadatum_to_list, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_metadatum_t*      metadatum = nullptr;
  cardano_metadatum_list_t* list      = nullptr;

  cardano_error_t error = cardano_metadatum_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_list(list, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_metadatum_to_list(metadatum, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_list_unref(&list);
}

TEST(cardano_metadatum_to_list, returnsErrorIfMetadatumIsNotAList)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  cardano_error_t      error     = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_list_t* list = NULL;

  EXPECT_EQ(cardano_metadatum_to_list(metadatum, &list), CARDANO_ERROR_INVALID_METADATUM_CONVERSION);

  // Cleanup
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_metadatum_equals, returnsTrueIfBothMetadatumAreEqual)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_integer_from_int(1, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), true);

  // Cleanup
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_equals, returnsFalseIfMetadatumAreDifferent)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_integer_from_int(2, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), false);

  // Cleanup
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_equals, returnsFalseIfMetadatumAreDifferentTypes)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), false);

  // Cleanup
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_equals, returnsFalseIfMetadatumAreNull)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), false);
}

TEST(cardano_metadatum_equals, returnsFalseIfOneMetadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), false);

  // Cleanup
  cardano_metadatum_unref(&metadatum1);
}

TEST(cardano_metadatum_equals, returnsFalseIfBothMetadatumAreNull)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), false);
}

TEST(cardano_metadatum_equals, returnsTrueIfBothArePlutusMapAndEqual)
{
  // Arrange
  cardano_metadatum_t*     metadatum1 = nullptr;
  cardano_metadatum_t*     metadatum2 = nullptr;
  cardano_metadatum_map_t* map1       = nullptr;
  cardano_metadatum_map_t* map2       = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&map1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_map_new(&map2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_map(map1, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_map(map2, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), true);

  // Cleanup
  cardano_metadatum_map_unref(&map1);
  cardano_metadatum_map_unref(&map2);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_equals, returnsTrueIfBothArePlutusListAndEqual)
{
  // Arrange
  cardano_metadatum_t*      metadatum1 = nullptr;
  cardano_metadatum_t*      metadatum2 = nullptr;
  cardano_metadatum_list_t* list1      = nullptr;
  cardano_metadatum_list_t* list2      = nullptr;

  cardano_error_t error = cardano_metadatum_list_new(&list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_new(&list2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_list(list1, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_list(list2, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), true);

  // Cleanup
  cardano_metadatum_list_unref(&list1);
  cardano_metadatum_list_unref(&list2);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_equals, returnsTrueIfBothAreBytesAndEqual)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  cardano_error_t error = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), true);

  // Cleanup
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_equals, returnsTrueIfBothAreStringAndEqual)
{
  // Arrange
  cardano_metadatum_t* metadatum1 = nullptr;
  cardano_metadatum_t* metadatum2 = nullptr;

  cardano_error_t error = cardano_metadatum_new_string("test", 4, &metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_string("test", 4, &metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_metadatum_equals(metadatum1, metadatum2), true);

  // Cleanup
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_metadatum_new_integer, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer(nullptr, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_integer, returnsErrorIfIntegerIsNull)
{
  // Act
  cardano_error_t error = cardano_metadatum_new_integer((const cardano_bigint_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_integer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bigint_t* integer = NULL;
  EXPECT_EQ(cardano_bigint_from_int(1, &integer), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer(integer, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_integer_from_uint, returnsErrorIfMetadatumIsNull)
{
  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_uint(0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_integer_from_uint, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_integer_from_uint, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_integer_from_uint, canReturnUint)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* integer = nullptr;
  error                     = cardano_metadatum_to_integer(data, &integer);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(integer), 0);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_new_integer_from_string, returnsErrorIfMetadatumIsNull)
{
  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_string("0", 1, 10, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_integer_from_string, returnsErrorIfStringIsNull)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_string(nullptr, 0, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_integer_from_string, returnsErrorIfEmptyString)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_string("", 0, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_new_integer_from_string, returnsErrorIfInvalidString)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_string("a", 1, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_CONVERSION_FAILED);

  // Cleanup
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_new_integer_from_string, returnsMetadatumWithCorrectNumber)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_integer_from_string("123", 3, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* integer = nullptr;
  error                     = cardano_metadatum_to_integer(data, &integer);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(integer), 123);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfMemoryAllocationFailsWhileReadingUint)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", strlen("00"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_from_cbor(reader, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_metadatum_unref(&data);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_cbor, returnErrorIfMemoryAllocationFailsWhileReadingUint2)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", strlen("00"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_from_cbor(reader, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_metadatum_unref(&data);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_to_cbor, canSerializeMaxUint64AsUnsignedInt)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_uint(UINT64_MAX, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_metadatum_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "1bffffffffffffffff");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_cbor, canSerializeSmallUint64AsUnsignedInt)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_uint(1U, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_metadatum_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "01");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_cbor, canSerializeMinInt64AsInt)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_int(INT64_MIN, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_metadatum_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "3b7fffffffffffffff");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_cbor, canSerializeSmallIntAsInt)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_int(-1, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_metadatum_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "20");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_cbor, canSerializeBigInteger)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_string("340199290171201906221318119490500689920", strlen("340199290171201906221318119490500689920"), 10, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_metadatum_to_cbor(data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "c250fff00000000000000000000000000000");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_cbor, canSerializeTextString)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_string("test", strlen("test"), &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_metadatum_to_cbor(data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "6474657374");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_string, returnsErrorIfMetadatumIsNull)
{
  // Act
  char* string = nullptr;

  EXPECT_EQ(cardano_metadatum_to_string(nullptr, nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_to_string, returnsErrorIfBufferIsNull)
{
  EXPECT_EQ(cardano_metadatum_to_string((cardano_metadatum_t*)"", nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_to_string, returnsErrorIfBufferIsInsufficient)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_string("test2", strlen("test2"), &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  char string[4];
  EXPECT_EQ(cardano_metadatum_to_string(data, string, 4), CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  // Cleanup
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_string, returnsErrorIfIsNotAStringKind)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_int(1, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  char string[4];
  EXPECT_EQ(cardano_metadatum_to_string(data, string, 4), CARDANO_ERROR_INVALID_METADATUM_CONVERSION);

  // Cleanup
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_get_string_size, returnsZeroIfMetadatumIsNull)
{
  // Act
  size_t size = cardano_metadatum_get_string_size(nullptr);

  EXPECT_EQ(size, 0);
}

TEST(cardano_metadatum_get_string_size, returnsZeroIfNotStringKind)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_integer_from_int(1, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_metadatum_get_string_size(data);

  // Assert
  EXPECT_EQ(size, 0);

  // Cleanup
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_get_kind, returnsUnknownIfMetadatumIsNull)
{
  // Act
  cardano_metadatum_kind_t kind;

  cardano_error_t result = cardano_metadatum_get_kind(nullptr, &kind);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_get_kind, returnsUnknownIfKindIsNull)
{
  // Act
  cardano_error_t result = cardano_metadatum_get_kind((cardano_metadatum_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_string, returnsErrorIfMetadatumIsNull)
{
  // Act
  cardano_metadatum_t* data = nullptr;

  cardano_error_t error = cardano_metadatum_new_string(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_string, returnsErrorIfStringIsNull)
{
  cardano_error_t error = cardano_metadatum_new_string("", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_new_string, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_string("test", 4, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_new_string, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_new_string("test", 4, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_from_json, canCreateMetadatumFromJson)
{
  // Act
  cardano_metadatum_t*   data   = nullptr;
  cardano_error_t        error  = cardano_metadatum_from_json(JSON_1, strlen(JSON_1), &data);
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_to_cbor(data, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, JSON_1_CBOR);

  // Cleanup
  cardano_metadatum_unref(&data);
  cardano_cbor_writer_unref(&writer);
  free(cbor_hex);
}

TEST(cardano_metadatum_from_json, returnsErrorIfHasInvalidField)
{
  // Act
  cardano_metadatum_t* data = nullptr;

  // Assert
  EXPECT_EQ(cardano_metadatum_from_json("{ \"key\": true }", strlen("{ \"key\": true }"), &data), CARDANO_ERROR_INVALID_JSON);
  EXPECT_EQ(cardano_metadatum_from_json("{ \"key\": 1.25 }", strlen("{ \"key\": 1.25 }"), &data), CARDANO_ERROR_INVALID_JSON);
  EXPECT_EQ(cardano_metadatum_from_json("{ \"key\": }", strlen("{ \"key\": }"), &data), CARDANO_ERROR_INVALID_JSON);
  EXPECT_EQ(cardano_metadatum_from_json("@", strlen("@"), &data), CARDANO_ERROR_INVALID_JSON);
  EXPECT_EQ(cardano_metadatum_from_json("", strlen("@"), &data), CARDANO_ERROR_INVALID_JSON);
  EXPECT_EQ(cardano_metadatum_from_json("", strlen(""), &data), CARDANO_ERROR_INVALID_JSON);
  EXPECT_EQ(cardano_metadatum_from_json(NULL, strlen("2"), &data), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_to_json, canConvertToJson)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_from_json(JSON_1, strlen(JSON_1), &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const size_t json_size = cardano_metadatum_get_json_size(data);
  char*        json      = (char*)malloc(json_size);

  error = cardano_metadatum_to_json(data, json, json_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json, JSON_1);

  // Cleanup
  cardano_metadatum_unref(&data);
  free(json);
}

TEST(cardano_metadatum_to_cip116_json, canConvertToJson)
{
  // Arrange
  cardano_metadatum_t*   data   = nullptr;
  cardano_error_t        error  = cardano_metadatum_from_json(JSON_1, strlen(JSON_1), &data);
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_to_cip116_json(data, writer);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  const size_t json_size = cardano_json_writer_get_encoded_size(writer);
  char*        json      = (char*)malloc(json_size);

  error = cardano_json_writer_encode(writer, json, json_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json, CIP116_JSON);

  // Cleanup
  cardano_metadatum_unref(&data);
  free(json);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_metadatum_to_cip116_json, canEncodeBytesMetadatum)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;
  const uint8_t        bytes[]   = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_metadatum_new_bytes(bytes, sizeof(bytes), &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum, testing::Not((cardano_metadatum_t*)nullptr));

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  error                         = cardano_metadatum_to_cip116_json(metadatum, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t json_size = cardano_json_writer_get_encoded_size(writer);
  char*        json      = (char*)malloc(json_size);

  error = cardano_json_writer_encode(writer, json, json_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(json, "{\n  \"tag\": \"bytes\",\n  \"value\": \"0102030405\"\n}");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  free(json);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_metadatum_to_cip116_json, returnErrorIfNullPointer)
{
  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  EXPECT_EQ(cardano_metadatum_to_cip116_json(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_metadatum_to_cip116_json((cardano_metadatum_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_json_writer_unref(&writer);
}

TEST(cardano_metadatum_to_json, returnErrorIfNullPointer)
{
  // Act
  char* json = nullptr;

  EXPECT_EQ(cardano_metadatum_to_json(nullptr, json, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_to_json, returnErrorErrorIfBufferToSmall)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_from_json(JSON_1, strlen(JSON_1), &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  char json[4];
  EXPECT_EQ(cardano_metadatum_to_json(data, json, 4), CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  // Cleanup
  cardano_metadatum_unref(&data);
}

TEST(cardano_metadatum_to_json, returnErrorIfCantBeEncodedInJson)
{
  // Arrange
  cardano_metadatum_t* data  = nullptr;
  cardano_error_t      error = cardano_metadatum_new_bytes((const uint8_t*)"test", 4, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const size_t json_size = cardano_metadatum_get_json_size(data);
  char*        json      = (char*)malloc(json_size);

  error = cardano_metadatum_to_json(data, json, json_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_METADATUM_CONVERSION);

  // Cleanup
  cardano_metadatum_unref(&data);
  free(json);
}

TEST(cardano_metadatum_to_json, returnErrorIfCantBeEncodedInJson2)
{
  //  Arrange
  cardano_metadatum_t*     data = nullptr;
  cardano_metadatum_map_t* map  = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_map(map, &data);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  error                    = cardano_metadatum_new_integer_from_int(1, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* value = nullptr;
  error                      = cardano_metadatum_new_integer_from_int(1, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_map_insert(map, key, value);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const size_t json_size = cardano_metadatum_get_json_size(data);
  char*        json      = (char*)malloc(json_size);

  error = cardano_metadatum_to_json(data, json, json_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_METADATUM_CONVERSION);

  // Cleanup
  cardano_metadatum_map_unref(&map);
  cardano_metadatum_unref(&data);
  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&value);
  free(json);
}

TEST(cardano_metadatum_get_json_size, returnZeroIfGivenNull)
{
  // Act
  size_t size = cardano_metadatum_get_json_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}