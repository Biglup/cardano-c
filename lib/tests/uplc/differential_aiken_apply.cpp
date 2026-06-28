/**
 * \file differential_aiken_apply.cpp
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/uplc/uplc_apply_params.h>

#include <gmock/gmock.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <dlfcn.h>

// This is a ground-truth differential validation of apply-params-to-script.
//
// It is GATED behind the AIKEN_DYLIB environment variable: when it is not set,
// every test reports a skip and passes, so CI (which does not have the aiken
// dylib) is unaffected. When AIKEN_DYLIB points at libaiken_c_ffi.dylib, the
// dylib is dlopen()'d at runtime and the native
// cardano_uplc_apply_params_to_script output is compared byte-for-byte against
// the aiken reference apply_params_to_plutus_script for a corpus of real flat
// scripts and several parameter sets.

/* TYPES *********************************************************************/

typedef const char* (*aiken_apply_fn)(const char* params_hex, const char* script_hex);
typedef void (*aiken_drop_fn)(const char* pointer);

/* CONSTANTS *****************************************************************/

/**
 * \brief The in-repo flat program corpus used as apply-params inputs.
 */
static const char* kScriptFiles[] = {
  "auction_1-1.flat",
  "coop-1.flat",
  "crowdfunding-success-3.flat",
  "future-increase-margin-2.flat",
  "future-settle-early-2.flat",
  "game-sm-success_2-3.flat",
  "multisig-sm-02.flat",
  "ping-pong_2-1.flat",
  "stablecoin_1-3.flat",
  "token-account-2.flat"
};

/* STATIC HELPERS ************************************************************/

namespace
{

/**
 * \brief Resolves the in-repo flat corpus directory from this source location.
 */
std::string
flat_dir()
{
  std::filesystem::path file = std::filesystem::path(__FILE__);
  return (file.parent_path() / ".." / ".." / ".." / "fuzz" / "corpus" / "uplc_flat").string();
}

/**
 * \brief Reads a binary file fully into a byte vector.
 */
bool
read_file_bytes(const std::string& path, std::vector<unsigned char>& out)
{
  FILE* fp = fopen(path.c_str(), "rb");
  if (fp == nullptr)
  {
    return false;
  }

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size < 0)
  {
    fclose(fp);
    return false;
  }

  out.resize(static_cast<size_t>(size));

  size_t read = (size == 0) ? 0U : fread(out.data(), 1U, static_cast<size_t>(size), fp);
  fclose(fp);

  return read == static_cast<size_t>(size);
}

/**
 * \brief CBOR-wraps raw flat bytes in a single CBOR bytestring.
 *
 * Both aiken Program::from_cbor and our decoder peel exactly one CBOR layer, so
 * the "compiled script" both sides expect is the flat program wrapped once.
 */
cardano_buffer_t*
cbor_wrap(const std::vector<unsigned char>& flat)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_NE(writer, nullptr);

  EXPECT_EQ(cardano_cbor_writer_write_bytestring(writer, flat.data(), flat.size()), CARDANO_SUCCESS);

  size_t            size   = cardano_cbor_writer_get_encode_size(writer);
  cardano_buffer_t* buffer = cardano_buffer_new(size);
  EXPECT_NE(buffer, nullptr);

  std::vector<unsigned char> tmp(size);
  EXPECT_EQ(cardano_cbor_writer_encode(writer, tmp.data(), size), CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return cardano_buffer_new_from(tmp.data(), tmp.size());
}

/**
 * \brief Converts a buffer to a lowercase hex string.
 */
std::string
buffer_to_hex(const cardano_buffer_t* buffer)
{
  size_t      hex_size = cardano_buffer_get_hex_size(buffer);
  std::string hex(hex_size, '\0');

  EXPECT_EQ(cardano_buffer_to_hex(buffer, hex.data(), hex_size), CARDANO_SUCCESS);

  // The buffer hex includes a NUL terminator in its size; drop the trailing NUL.
  if (!hex.empty() && hex.back() == '\0')
  {
    hex.pop_back();
  }

  return hex;
}

/**
 * \brief A parameter set built in both forms: a plutus_list for our function and
 *        the CBOR hex of the PlutusData Array for aiken.
 */
struct ParamSet
{
    std::string            name;
    cardano_plutus_list_t* list;      // owned, may be empty
    std::string            array_hex; // CBOR hex of a PlutusData array of the same params
};

/**
 * \brief Encodes a plutus_list as the CBOR of a PlutusData array, as aiken's
 *        PlutusData::decode_fragment expects.
 */
std::string
list_to_array_hex(cardano_plutus_list_t* list)
{
  // Wrap the list as a PlutusData list value, which encodes to a CBOR array of
  // the elements - exactly what aiken decodes as PlutusData::Array.
  cardano_plutus_data_t* as_list = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_list(list, &as_list), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_plutus_data_to_cbor(as_list, writer), CARDANO_SUCCESS);

  size_t      hex_size = cardano_cbor_writer_get_hex_size(writer);
  std::string hex(hex_size, '\0');
  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex.data(), hex_size), CARDANO_SUCCESS);

  if (!hex.empty() && hex.back() == '\0')
  {
    hex.pop_back();
  }

  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&as_list);

  return hex;
}

cardano_plutus_data_t*
data_int(int64_t value)
{
  cardano_plutus_data_t* data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(value, &data), CARDANO_SUCCESS);
  return data;
}

cardano_plutus_data_t*
data_bytes_from_hex(const char* hex)
{
  cardano_plutus_data_t* data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &data), CARDANO_SUCCESS);
  return data;
}

void
list_add(cardano_plutus_list_t* list, cardano_plutus_data_t* data)
{
  EXPECT_EQ(cardano_plutus_list_add(list, data), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);
}

/**
 * \brief Builds the corpus of parameter sets. Caller frees each list.
 */
std::vector<ParamSet>
build_param_sets()
{
  std::vector<ParamSet> sets;

  // [] - empty (pure re-encode).
  {
    cardano_plutus_list_t* list = nullptr;
    EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
    ParamSet ps;
    ps.name      = "empty";
    ps.list      = list;
    ps.array_hex = list_to_array_hex(list);
    sets.push_back(ps);
  }

  // [I 42]
  {
    cardano_plutus_list_t* list = nullptr;
    EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
    list_add(list, data_int(42));
    ParamSet ps;
    ps.name      = "int42";
    ps.list      = list;
    ps.array_hex = list_to_array_hex(list);
    sets.push_back(ps);
  }

  // [I -7, B #deadbeef]
  {
    cardano_plutus_list_t* list = nullptr;
    EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
    list_add(list, data_int(-7));
    list_add(list, data_bytes_from_hex("deadbeef"));
    ParamSet ps;
    ps.name      = "negint_and_bytes";
    ps.list      = list;
    ps.array_hex = list_to_array_hex(list);
    sets.push_back(ps);
  }

  // [ [I 1, I 2, I 3] ] - a single list-valued param (nested structure).
  {
    cardano_plutus_list_t* inner = nullptr;
    EXPECT_EQ(cardano_plutus_list_new(&inner), CARDANO_SUCCESS);
    list_add(inner, data_int(1));
    list_add(inner, data_int(2));
    list_add(inner, data_int(3));

    cardano_plutus_data_t* inner_data = nullptr;
    EXPECT_EQ(cardano_plutus_data_new_list(inner, &inner_data), CARDANO_SUCCESS);
    cardano_plutus_list_unref(&inner);

    cardano_plutus_list_t* list = nullptr;
    EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list, inner_data), CARDANO_SUCCESS);
    cardano_plutus_data_unref(&inner_data);

    ParamSet ps;
    ps.name      = "nested_list";
    ps.list      = list;
    ps.array_hex = list_to_array_hex(list);
    sets.push_back(ps);
  }

  // [ I 9223372036854775807, B #00 ] - a large int plus an empty-ish bytestring.
  {
    cardano_plutus_list_t* list = nullptr;
    EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
    list_add(list, data_int(9223372036854775807LL));
    list_add(list, data_bytes_from_hex("00"));
    ParamSet ps;
    ps.name      = "maxint_and_byte";
    ps.list      = list;
    ps.array_hex = list_to_array_hex(list);
    sets.push_back(ps);
  }

  return sets;
}

void
free_param_sets(std::vector<ParamSet>& sets)
{
  for (ParamSet& ps: sets)
  {
    cardano_plutus_list_unref(&ps.list);
  }
}

} // namespace

/* TESTS *********************************************************************/

TEST(differential_aiken_apply, matchesAikenByteForByte)
{
  const char* dylib_path = std::getenv("AIKEN_DYLIB");

  if ((dylib_path == nullptr) || (dylib_path[0] == '\0'))
  {
    GTEST_SKIP() << "AIKEN_DYLIB not set; skipping aiken differential validation.";
  }

  void* handle = dlopen(dylib_path, RTLD_NOW | RTLD_LOCAL);
  ASSERT_NE(handle, nullptr) << "dlopen failed: " << dlerror();

  aiken_apply_fn apply = reinterpret_cast<aiken_apply_fn>(dlsym(handle, "apply_params_to_plutus_script"));
  aiken_drop_fn  drop  = reinterpret_cast<aiken_drop_fn>(dlsym(handle, "drop_char_pointer"));

  ASSERT_NE(apply, nullptr) << "dlsym apply_params_to_plutus_script failed";
  ASSERT_NE(drop, nullptr) << "dlsym drop_char_pointer failed";

  std::vector<ParamSet> param_sets = build_param_sets();

  int total      = 0;
  int matched    = 0;
  int both_error = 0;

  std::vector<std::string> mismatches;

  for (const char* file: kScriptFiles)
  {
    std::string                path = flat_dir() + "/" + file;
    std::vector<unsigned char> flat;

    if (!read_file_bytes(path, flat))
    {
      ADD_FAILURE() << "could not read flat script: " << path;
      continue;
    }

    cardano_buffer_t* wrapped = cbor_wrap(flat);
    ASSERT_NE(wrapped, nullptr);

    std::string script_hex = buffer_to_hex(wrapped);

    for (const ParamSet& ps: param_sets)
    {
      ++total;

      // aiken reference.
      const char* json = apply(ps.array_hex.c_str(), script_hex.c_str());
      ASSERT_NE(json, nullptr);
      std::string json_str(json);
      drop(json);

      bool        aiken_ok = json_str.find("\"SUCCESS\"") != std::string::npos;
      std::string aiken_compiled;

      if (aiken_ok)
      {
        const std::string key = "\"compiled_code\":\"";
        size_t            pos = json_str.find(key);
        if (pos != std::string::npos)
        {
          pos            += key.size();
          size_t end     = json_str.find('"', pos);
          aiken_compiled = json_str.substr(pos, end - pos);
        }
      }

      // ours.
      cardano_buffer_t* out = nullptr;
      cardano_error_t   err = cardano_uplc_apply_params_to_script(
        ps.list, cardano_buffer_get_data(wrapped), cardano_buffer_get_size(wrapped), &out);

      bool        ours_ok = (err == CARDANO_SUCCESS) && (out != nullptr);
      std::string ours_hex;
      if (ours_ok)
      {
        ours_hex = buffer_to_hex(out);
      }

      std::string label = std::string(file) + " / " + ps.name;

      if (!aiken_ok && !ours_ok)
      {
        ++both_error;
        cardano_buffer_unref(&out);
        continue;
      }

      if (aiken_ok != ours_ok)
      {
        std::string m = label + " : status mismatch (aiken_ok=" + (aiken_ok ? "1" : "0") +
          " ours_ok=" + (ours_ok ? "1" : "0") + " aiken_json=" + json_str + ")";
        mismatches.push_back(m);
        ADD_FAILURE() << m;
        cardano_buffer_unref(&out);
        continue;
      }

      if (ours_hex == aiken_compiled)
      {
        ++matched;
      }
      else
      {
        // Find the first diverging byte/char for the report.
        size_t i = 0;
        while ((i < ours_hex.size()) && (i < aiken_compiled.size()) && (ours_hex[i] == aiken_compiled[i]))
        {
          ++i;
        }

        std::string m = label + " : BYTE MISMATCH at hex index " + std::to_string(i) +
          " ours_len=" + std::to_string(ours_hex.size()) + " aiken_len=" + std::to_string(aiken_compiled.size());
        mismatches.push_back(m);

        ADD_FAILURE() << m << "\n  ours  : " << ours_hex.substr(0, i + 16) << "...\n  aiken : "
                      << aiken_compiled.substr(0, i + 16) << "...";
      }

      cardano_buffer_unref(&out);
    }

    cardano_buffer_unref(&wrapped);
  }

  free_param_sets(param_sets);
  dlclose(handle);

  std::printf(
    "[aiken-diff] pairs=%d byte_identical=%d both_error=%d mismatches=%zu\n",
    total,
    matched,
    both_error,
    mismatches.size());

  for (const std::string& m: mismatches)
  {
    std::printf("[aiken-diff] MISMATCH: %s\n", m.c_str());
  }

  EXPECT_EQ(matched + both_error, total) << "not all (script, param-set) pairs matched aiken";
}
