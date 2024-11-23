/**
 * \file bip39.cpp
 *
 * \author angel.castillo
 * \date   Nov 22, 2024
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

#include <cardano/bip39.h>
#include <gmock/gmock.h>

/* TEST VECTOR ***************************************************************/

#define ENTROPY_MAX_LEN 32
#define MAX_WORDS       24

#define ARRAY_SIZEOF(x) (sizeof(x) / sizeof(x[0]))

typedef struct
{
    const byte_t entropy[ENTROPY_MAX_LEN];
    const int    entropy_len;
    const char*  words[MAX_WORDS];
    const int    nr_words;
} bip39_test_vectors_t;

static bip39_test_vectors_t BIP39_TEST_VECTOR[] = {
  {
    .entropy     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    .entropy_len = 16,
    .words       = { "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "about" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f },
    .entropy_len = 16,
    .words       = { "legal", "winner", "thank", "year", "wave", "sausage", "worth", "useful", "legal", "winner", "thank", "yellow" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 },
    .entropy_len = 16,
    .words       = { "letter", "advice", "cage", "absurd", "amount", "doctor", "acoustic", "avoid", "letter", "advice", "cage", "above" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .entropy_len = 16,
    .words       = { "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "wrong" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    .entropy_len = 24,
    .words       = { "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "agent" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f },
    .entropy_len = 24,
    .words       = { "legal", "winner", "thank", "year", "wave", "sausage", "worth", "useful", "legal", "winner", "thank", "year", "wave", "sausage", "worth", "useful", "legal", "will" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 },
    .entropy_len = 24,
    .words       = { "letter", "advice", "cage", "absurd", "amount", "doctor", "acoustic", "avoid", "letter", "advice", "cage", "absurd", "amount", "doctor", "acoustic", "avoid", "letter", "always" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .entropy_len = 24,
    .words       = { "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "when" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    .entropy_len = 32,
    .words       = { "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "art" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f },
    .entropy_len = 32,
    .words       = { "legal", "winner", "thank", "year", "wave", "sausage", "worth", "useful", "legal", "winner", "thank", "year", "wave", "sausage", "worth", "useful", "legal", "winner", "thank", "year", "wave", "sausage", "worth", "title" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 },
    .entropy_len = 32,
    .words       = { "letter", "advice", "cage", "absurd", "amount", "doctor", "acoustic", "avoid", "letter", "advice", "cage", "absurd", "amount", "doctor", "acoustic", "avoid", "letter", "advice", "cage", "absurd", "amount", "doctor", "acoustic", "bless" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .entropy_len = 32,
    .words       = { "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "zoo", "vote" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0x9e, 0x88, 0x5d, 0x95, 0x2a, 0xd3, 0x62, 0xca, 0xeb, 0x4e, 0xfe, 0x34, 0xa8, 0xe9, 0x1b, 0xd2 },
    .entropy_len = 16,
    .words       = { "ozone", "drill", "grab", "fiber", "curtain", "grace", "pudding", "thank", "cruise", "elder", "eight", "picnic" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0x66, 0x10, 0xb2, 0x59, 0x67, 0xcd, 0xcc, 0xa9, 0xd5, 0x98, 0x75, 0xf5, 0xcb, 0x50, 0xb0, 0xea, 0x75, 0x43, 0x33, 0x11, 0x86, 0x9e, 0x93, 0x0b },
    .entropy_len = 24,
    .words       = { "gravity", "machine", "north", "sort", "system", "female", "filter", "attitude", "volume", "fold", "club", "stay", "feature", "office", "ecology", "stable", "narrow", "fog" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0x68, 0xa7, 0x9e, 0xac, 0xa2, 0x32, 0x48, 0x73, 0xea, 0xcc, 0x50, 0xcb, 0x9c, 0x6e, 0xca, 0x8c, 0xc6, 0x8e, 0xa5, 0xd9, 0x36, 0xf9, 0x87, 0x87, 0xc6, 0x0c, 0x7e, 0xbc, 0x74, 0xe6, 0xce, 0x7c },
    .entropy_len = 32,
    .words       = { "hamster", "diagram", "private", "dutch", "cause", "delay", "private", "meat", "slide", "toddler", "razor", "book", "happy", "fancy", "gospel", "tennis", "maple", "dilemma", "loan", "word", "shrug", "inflict", "delay", "length" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0xc0, 0xba, 0x5a, 0x8e, 0x91, 0x41, 0x11, 0x21, 0x0f, 0x2b, 0xd1, 0x31, 0xf3, 0xd5, 0xe0, 0x8d },
    .entropy_len = 16,
    .words       = { "scheme", "spot", "photo", "card", "baby", "mountain", "device", "kick", "cradle", "pact", "join", "borrow" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0x6d, 0x9b, 0xe1, 0xee, 0x6e, 0xbd, 0x27, 0xa2, 0x58, 0x11, 0x5a, 0xad, 0x99, 0xb7, 0x31, 0x7b, 0x9c, 0x8d, 0x28, 0xb6, 0xd7, 0x64, 0x31, 0xc3 },
    .entropy_len = 24,
    .words       = { "horn", "tenant", "knee", "talent", "sponsor", "spell", "gate", "clip", "pulse", "soap", "slush", "warm", "silver", "nephew", "swap", "uncle", "crack", "brave" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0x9f, 0x6a, 0x28, 0x78, 0xb2, 0x52, 0x07, 0x99, 0xa4, 0x4e, 0xf1, 0x8b, 0xc7, 0xdf, 0x39, 0x4e, 0x70, 0x61, 0xa2, 0x24, 0xd2, 0xc3, 0x3c, 0xd0, 0x15, 0xb1, 0x57, 0xd7, 0x46, 0x86, 0x98, 0x63 },
    .entropy_len = 32,
    .words       = { "panda", "eyebrow", "bullet", "gorilla", "call", "smoke", "muffin", "taste", "mesh", "discover", "soft", "ostrich", "alcohol", "speed", "nation", "flash", "devote", "level", "hobby", "quick", "inner", "drive", "ghost", "inside" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0x23, 0xdb, 0x81, 0x60, 0xa3, 0x1d, 0x3e, 0x0d, 0xca, 0x36, 0x88, 0xed, 0x94, 0x1a, 0xdb, 0xf3 },
    .entropy_len = 16,
    .words       = { "cat", "swing", "flag", "economy", "stadium", "alone", "churn", "speed", "unique", "patch", "report", "train" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0x81, 0x97, 0xa4, 0xa4, 0x7f, 0x04, 0x25, 0xfa, 0xea, 0xa6, 0x9d, 0xee, 0xbc, 0x05, 0xca, 0x29, 0xc0, 0xa5, 0xb5, 0xcc, 0x76, 0xce, 0xac, 0xc0 },
    .entropy_len = 24,
    .words       = { "light", "rule", "cinnamon", "wrap", "drastic", "word", "pride", "squirrel", "upgrade", "then", "income", "fatal", "apart", "sustain", "crack", "supply", "proud", "access" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0x06, 0x6d, 0xca, 0x1a, 0x2b, 0xb7, 0xe8, 0xa1, 0xdb, 0x28, 0x32, 0x14, 0x8c, 0xe9, 0x93, 0x3e, 0xea, 0x0f, 0x3a, 0xc9, 0x54, 0x8d, 0x79, 0x31, 0x12, 0xd9, 0xa9, 0x5c, 0x94, 0x07, 0xef, 0xad },
    .entropy_len = 32,
    .words       = { "all", "hour", "make", "first", "leader", "extend", "hole", "alien", "behind", "guard", "gospel", "lava", "path", "output", "census", "museum", "junior", "mass", "reopen", "famous", "sing", "advance", "salt", "reform" },
    .nr_words    = 24,
  },
  {
    .entropy     = { 0xf3, 0x0f, 0x8c, 0x1d, 0xa6, 0x65, 0x47, 0x8f, 0x49, 0xb0, 0x01, 0xd9, 0x4c, 0x5f, 0xc4, 0x52 },
    .entropy_len = 16,
    .words       = { "vessel", "ladder", "alter", "error", "federal", "sibling", "chat", "ability", "sun", "glass", "valve", "picture" },
    .nr_words    = 12,
  },
  {
    .entropy     = { 0xc1, 0x0e, 0xc2, 0x0d, 0xc3, 0xcd, 0x9f, 0x65, 0x2c, 0x7f, 0xac, 0x2f, 0x12, 0x30, 0xf7, 0xa3, 0xc8, 0x28, 0x38, 0x9a, 0x14, 0x39, 0x2f, 0x05 },
    .entropy_len = 24,
    .words       = { "scissors", "invite", "lock", "maple", "supreme", "raw", "rapid", "void", "congress", "muscle", "digital", "elegant", "little", "brisk", "hair", "mango", "congress", "clump" },
    .nr_words    = 18,
  },
  {
    .entropy     = { 0xf5, 0x85, 0xc1, 0x1a, 0xec, 0x52, 0x0d, 0xb5, 0x7d, 0xd3, 0x53, 0xc6, 0x95, 0x54, 0xb2, 0x1a, 0x89, 0xb2, 0x0f, 0xb0, 0x65, 0x09, 0x66, 0xfa, 0x0a, 0x9d, 0x6f, 0x74, 0xfd, 0x98, 0x9d, 0x8f },
    .entropy_len = 32,
    .words       = { "void", "come", "effort", "suffer", "camp", "survey", "warrior", "heavy", "shoot", "primary", "clutch", "crush", "open", "amazing", "screen", "patrol", "group", "space", "point", "ten", "exist", "slush", "involve", "unfold" },
    .nr_words    = 24,
  }
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_bip39_entropy_to_mnemonic_words, canConvertFromEntropyToMnemonics)
{
  for (size_t i = 0; i < ARRAY_SIZEOF(BIP39_TEST_VECTOR); ++i)
  {
    size_t      nr_words         = 0;
    const char* words[MAX_WORDS] = { 0 };

    cardano_error_t result = cardano_bip39_entropy_to_mnemonic_words(BIP39_TEST_VECTOR[i].entropy, BIP39_TEST_VECTOR[i].entropy_len, words, &nr_words);

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(BIP39_TEST_VECTOR[i].nr_words, nr_words);

    for (size_t j = 0; j < BIP39_TEST_VECTOR[i].nr_words; ++j)
    {
      EXPECT_STREQ(BIP39_TEST_VECTOR[i].words[j], words[j]);
    }
  }
}

TEST(cardano_bip39_entropy_to_mnemonic_words, returnsErrorIfGivenNull)
{
  EXPECT_EQ(cardano_bip39_entropy_to_mnemonic_words(NULL, 0, NULL, NULL), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip39_entropy_to_mnemonic_words, returnsErrorIfInvalidEntropySize)
{
  byte_t      entropy[ENTROPY_MAX_LEN] = { 0 };
  const char* words[MAX_WORDS]         = { nullptr };
  size_t      nr_words                 = 0;

  EXPECT_EQ(cardano_bip39_entropy_to_mnemonic_words(entropy, 0, words, &nr_words), CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_bip39_mnemonic_words_to_entropy, canConvertFromMnemonicsToEntropy)
{
  for (auto& i: BIP39_TEST_VECTOR)
  {
    byte_t entropy[ENTROPY_MAX_LEN] = { 0 };
    size_t entropy_len              = 0;

    cardano_error_t result = cardano_bip39_mnemonic_words_to_entropy(
      i.words,
      i.nr_words,
      entropy,
      sizeof(entropy),
      &entropy_len);

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(i.entropy_len, entropy_len);

    for (size_t j = 0; j < i.entropy_len; ++j)
    {
      EXPECT_EQ(i.entropy[j], entropy[j]);
    }
  }
}

TEST(cardano_bip39_mnemonic_words_to_entropy, returnsErrorIfGivenNull)
{
  EXPECT_EQ(cardano_bip39_mnemonic_words_to_entropy(NULL, 0, NULL, 0, NULL), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip39_mnemonic_words_to_entropy, returnsErrorIfInvalidWordCount)
{
  byte_t entropy[ENTROPY_MAX_LEN] = { 0 };
  size_t entropy_len              = 0;

  EXPECT_EQ(cardano_bip39_mnemonic_words_to_entropy((const char**)"", 0, entropy, sizeof(entropy), &entropy_len), CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_bip39_mnemonic_words_to_entropy, returnsErrorIfEntropyBufferIsTooSmall)
{
  byte_t entropy[ENTROPY_MAX_LEN] = { 0 };
  size_t entropy_len              = 0;

  const char* words[] = { "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "about" };

  EXPECT_EQ(cardano_bip39_mnemonic_words_to_entropy(words, 12, entropy, 0, &entropy_len), CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_bip39_mnemonic_words_to_entropy, returnsErrorIfGivenInvalidWord)
{
  byte_t entropy[ENTROPY_MAX_LEN] = { 0 };
  size_t entropy_len              = 0;

  const char* words[] = { "invalid", "winner", "thank", "year", "wave", "sausage", "worth", "useful", "legal", "winner", "thank", "yellow" };

  EXPECT_EQ(cardano_bip39_mnemonic_words_to_entropy(words, 12, entropy, sizeof(entropy), &entropy_len), CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_bip39_mnemonic_words_to_entropy, returnsErrorIfInvalidChecksum)
{
  byte_t entropy[ENTROPY_MAX_LEN] = { 0 };
  size_t entropy_len              = 0;

  const char* words[] = { "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon", "abandon" };

  EXPECT_EQ(cardano_bip39_mnemonic_words_to_entropy(words, 12, entropy, sizeof(entropy), &entropy_len), CARDANO_ERROR_INVALID_CHECKSUM);
}