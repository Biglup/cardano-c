/**
 * \file proposal_procedure_set.cpp
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#include <cardano/proposal_procedures/proposal_procedure_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                     = "d9010284841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_WITHOUT_TAG         = "84841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* PROPOSAL_PROCEDURE1_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* PROPOSAL_PROCEDURE2_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* PROPOSAL_PROCEDURE3_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* PROPOSAL_PROCEDURE4_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

/**
 * Creates a new default instance of the proposal_procedure.
 * @return A new instance of the proposal_procedure.
 */
static cardano_proposal_procedure_t*
new_default_proposal_procedure(const char* cbor)
{
  cardano_proposal_procedure_t* proposal_procedure = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_proposal_procedure_unref(&proposal_procedure);
    return nullptr;
  }

  return proposal_procedure;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_proposal_procedure_set_new, canCreateProposalProcedureSet)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(proposal_procedure_set, testing::Not((cardano_proposal_procedure_set_t*)nullptr));

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_new, returnsErrorIfProposalProcedureSetIsNull)
{
  // Act
  cardano_error_t error = cardano_proposal_procedure_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(proposal_procedure_set, (cardano_proposal_procedure_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(proposal_procedure_set, (cardano_proposal_procedure_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_set_to_cbor, canSerializeAnEmptyProposalProcedureSet)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposal_procedure_set_to_cbor(proposal_procedure_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_proposal_procedure_set_to_cbor, canSerializeProposalProcedureSet)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* proposal_procedures[] = { PROPOSAL_PROCEDURE1_CBOR, PROPOSAL_PROCEDURE2_CBOR, PROPOSAL_PROCEDURE3_CBOR, PROPOSAL_PROCEDURE4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(proposal_procedures[i]);

    cardano_error_t result = cardano_proposal_procedure_set_add(proposal_procedure_set, proposal_procedure);
    EXPECT_EQ(result, CARDANO_SUCCESS);

    cardano_proposal_procedure_unref(&proposal_procedure);
  }

  // Act
  error = cardano_proposal_procedure_set_to_cbor(proposal_procedure_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_proposal_procedure_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_proposal_procedure_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;

  cardano_error_t error = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposal_procedure_set_to_cbor(proposal_procedure_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &proposal_procedure_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposal_procedure_set_to_cbor(proposal_procedure_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_proposal_procedure_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &proposal_procedure_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposal_procedure_set_to_cbor(proposal_procedure_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_proposal_procedure_set_from_cbor, canDeserializeProposalProcedureSet)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &proposal_procedure_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(proposal_procedure_set, testing::Not((cardano_proposal_procedure_set_t*)nullptr));

  const size_t length = cardano_proposal_procedure_set_get_length(proposal_procedure_set);

  EXPECT_EQ(length, 4);

  cardano_proposal_procedure_t* elem1 = NULL;
  cardano_proposal_procedure_t* elem2 = NULL;
  cardano_proposal_procedure_t* elem3 = NULL;
  cardano_proposal_procedure_t* elem4 = NULL;

  EXPECT_EQ(cardano_proposal_procedure_set_get(proposal_procedure_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_get(proposal_procedure_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_get(proposal_procedure_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_get(proposal_procedure_set, 3, &elem4), CARDANO_SUCCESS);

  const char* proposal_procedures[] = { PROPOSAL_PROCEDURE1_CBOR, PROPOSAL_PROCEDURE2_CBOR, PROPOSAL_PROCEDURE3_CBOR, PROPOSAL_PROCEDURE4_CBOR };

  cardano_proposal_procedure_t* proposal_procedures_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_proposal_procedure_to_cbor(proposal_procedures_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(proposal_procedures[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, proposal_procedures[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  cardano_cbor_reader_unref(&reader);

  cardano_proposal_procedure_unref(&elem1);
  cardano_proposal_procedure_unref(&elem2);
  cardano_proposal_procedure_unref(&elem3);
  cardano_proposal_procedure_unref(&elem4);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfProposalProcedureSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(nullptr, &proposal_procedure_set);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &proposal_procedure_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(proposal_procedure_set, (cardano_proposal_procedure_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_proposal_procedure_set_t* list   = nullptr;
  cardano_cbor_reader_t*            reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfInvalidElements)
{
  // Arrange
  cardano_proposal_procedure_set_t* list   = nullptr;
  cardano_cbor_reader_t*            reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_proposal_procedure_set_t* list   = nullptr;
  cardano_cbor_reader_t*            reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_proposal_procedure_set_t* list   = nullptr;
  cardano_cbor_reader_t*            reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposal_procedure_set_ref(proposal_procedure_set);

  // Assert
  EXPECT_THAT(proposal_procedure_set, testing::Not((cardano_proposal_procedure_set_t*)nullptr));
  EXPECT_EQ(cardano_proposal_procedure_set_refcount(proposal_procedure_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_proposal_procedure_set_ref(nullptr);
}

TEST(cardano_proposal_procedure_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;

  // Act
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_proposal_procedure_set_unref((cardano_proposal_procedure_set_t**)nullptr);
}

TEST(cardano_proposal_procedure_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposal_procedure_set_ref(proposal_procedure_set);
  size_t ref_count = cardano_proposal_procedure_set_refcount(proposal_procedure_set);

  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  size_t updated_ref_count = cardano_proposal_procedure_set_refcount(proposal_procedure_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposal_procedure_set_ref(proposal_procedure_set);
  size_t ref_count = cardano_proposal_procedure_set_refcount(proposal_procedure_set);

  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
  size_t updated_ref_count = cardano_proposal_procedure_set_refcount(proposal_procedure_set);

  cardano_proposal_procedure_set_unref(&proposal_procedure_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(proposal_procedure_set, (cardano_proposal_procedure_set_t*)nullptr);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_proposal_procedure_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_proposal_procedure_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_proposal_procedure_set_set_last_error(proposal_procedure_set, message);

  // Assert
  EXPECT_STREQ(cardano_proposal_procedure_set_get_last_error(proposal_procedure_set), "Object is NULL.");
}

TEST(cardano_proposal_procedure_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_proposal_procedure_set_set_last_error(proposal_procedure_set, message);

  // Assert
  EXPECT_STREQ(cardano_proposal_procedure_set_get_last_error(proposal_procedure_set), "");

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_get_length, returnsZeroIfProposalProcedureSetIsNull)
{
  // Act
  size_t length = cardano_proposal_procedure_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_proposal_procedure_set_get_length, returnsZeroIfProposalProcedureSetIsEmpty)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_proposal_procedure_set_get_length(proposal_procedure_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_get, returnsErrorIfProposalProcedureSetIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposal_procedure_set_get(proposal_procedure_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposal_procedure_t* data = nullptr;
  error                              = cardano_proposal_procedure_set_get(proposal_procedure_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}

TEST(cardano_proposal_procedure_set_add, returnsErrorIfProposalProcedureSetIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_proposal_procedure_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_proposal_procedure_set_t* proposal_procedure_set = nullptr;
  cardano_error_t                   error                  = cardano_proposal_procedure_set_new(&proposal_procedure_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposal_procedure_set_add(proposal_procedure_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_set_unref(&proposal_procedure_set);
}
