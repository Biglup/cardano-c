/**
 * \file pretty.cpp
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include "../../src/uplc/ast/uplc_term.h"

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/syntax/pretty.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>
#include <string>

/* STATIC HELPERS ************************************************************/

static cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  cardano_error_t       error = cardano_uplc_arena_new(4096U, &arena);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return arena;
}

static cardano_bigint_t*
new_bigint(int64_t value)
{
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_int(value, &bigint);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return bigint;
}

static cardano_bigint_t*
new_bigint_from_string(const char* text)
{
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_string(text, strlen(text), 10, &bigint);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return bigint;
}

static cardano_buffer_t*
new_buffer(const char* text)
{
  return cardano_buffer_new_from(reinterpret_cast<const byte_t*>(text), strlen(text));
}

static cardano_buffer_t*
new_buffer_bytes(const std::vector<byte_t>& bytes)
{
  return cardano_buffer_new_from(bytes.data(), bytes.size());
}

static cardano_uplc_constant_t*
const_integer(cardano_uplc_arena_t* arena, int64_t value)
{
  cardano_bigint_t*        bigint   = new_bigint(value);
  cardano_uplc_constant_t* constant = nullptr;
  cardano_error_t          error    = cardano_uplc_constant_new_integer(arena, bigint, &constant);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  cardano_bigint_unref(&bigint);
  return constant;
}

static cardano_uplc_constant_t*
const_bool(cardano_uplc_arena_t* arena, bool value)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_error_t          error    = cardano_uplc_constant_new_bool(arena, value, &constant);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return constant;
}

static cardano_uplc_constant_t*
const_unit(cardano_uplc_arena_t* arena)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_error_t          error    = cardano_uplc_constant_new_unit(arena, &constant);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return constant;
}

static cardano_uplc_term_t*
term_constant(cardano_uplc_arena_t* arena, const cardano_uplc_constant_t* constant)
{
  cardano_uplc_term_t* term  = nullptr;
  cardano_error_t      error = cardano_uplc_term_new_constant(arena, constant, &term);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return term;
}

static std::string
render_term(const cardano_uplc_term_t* term)
{
  cardano_buffer_t* out   = nullptr;
  cardano_error_t   error = cardano_uplc_pretty_print_term(term, &out);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  std::string text(reinterpret_cast<const char*>(cardano_buffer_get_data(out)));
  cardano_buffer_unref(&out);
  return text;
}

static std::string
render_constant_term(cardano_uplc_arena_t* arena, const cardano_uplc_constant_t* constant)
{
  return render_term(term_constant(arena, constant));
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_pretty_print_term, rendersIntegerConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  // Act
  std::string text = render_constant_term(arena, const_integer(arena, 23));

  // Assert
  EXPECT_EQ(text, "(con integer 23)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersNegativeAndBigInteger)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_bigint_t*        big   = new_bigint_from_string("-123456789012345678901234567890");
  cardano_uplc_constant_t* c     = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, big, &c), CARDANO_SUCCESS);
  cardano_bigint_unref(&big);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con integer -123456789012345678901234567890)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersByteStringConstant)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_buffer_t*        bytes = new_buffer_bytes({ 0xDEU, 0xADU, 0xBEU, 0xEFU });
  cardano_uplc_constant_t* c     = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, bytes, &c), CARDANO_SUCCESS);
  cardano_buffer_unref(&bytes);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con bytestring #deadbeef)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersEmptyByteString)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_buffer_t*        bytes = cardano_buffer_new(0);
  cardano_uplc_constant_t* c     = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, bytes, &c), CARDANO_SUCCESS);
  cardano_buffer_unref(&bytes);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con bytestring #)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersStringConstantWithEscapes)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_buffer_t*        text  = new_buffer("a\"b\\c");
  cardano_uplc_constant_t* c     = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_string(arena, text, &c), CARDANO_SUCCESS);
  cardano_buffer_unref(&text);

  // Act
  std::string out = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(out, "(con string \"a\\\"b\\\\c\")");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersBoolConstants)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  // Act / Assert
  EXPECT_EQ(render_constant_term(arena, const_bool(arena, true)), "(con bool True)");
  EXPECT_EQ(render_constant_term(arena, const_bool(arena, false)), "(con bool False)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersUnitConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  // Act
  std::string text = render_constant_term(arena, const_unit(arena));

  // Assert
  EXPECT_EQ(text, "(con unit ())");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersListConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena   = new_arena();
  cardano_uplc_type_t*  int_ty  = nullptr;
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_ty), CARDANO_SUCCESS);

  const cardano_uplc_constant_t* items[2] = { const_integer(arena, 1), const_integer(arena, 2) };

  cardano_uplc_constant_t* list = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_list(arena, int_ty, items, 2U, &list), CARDANO_SUCCESS);

  // Act
  std::string text = render_constant_term(arena, list);

  // Assert
  EXPECT_EQ(text, "(con (list integer) [1, 2])");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersEmptyListConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_type_t*  bool_ty = nullptr;
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &bool_ty), CARDANO_SUCCESS);

  cardano_uplc_constant_t* list = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_list(arena, bool_ty, nullptr, 0U, &list), CARDANO_SUCCESS);

  // Act
  std::string text = render_constant_term(arena, list);

  // Assert
  EXPECT_EQ(text, "(con (list bool) [])");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersArrayConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_type_t*  int_ty = nullptr;
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_ty), CARDANO_SUCCESS);

  const cardano_uplc_constant_t* items[2] = { const_integer(arena, 1), const_integer(arena, 2) };

  cardano_uplc_constant_t* array = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_array(arena, int_ty, items, 2U, &array), CARDANO_SUCCESS);

  // Act
  std::string text = render_constant_term(arena, array);

  // Assert
  EXPECT_EQ(text, "(con (array integer) [1, 2])");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersEmptyArrayConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena   = new_arena();
  cardano_uplc_type_t*  bool_ty = nullptr;
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &bool_ty), CARDANO_SUCCESS);

  cardano_uplc_constant_t* array = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_array(arena, bool_ty, nullptr, 0U, &array), CARDANO_SUCCESS);

  // Act
  std::string text = render_constant_term(arena, array);

  // Assert
  EXPECT_EQ(text, "(con (array bool) [])");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersPairConstant)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_uplc_constant_t* pair  = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, const_integer(arena, 12345), const_bool(arena, true), &pair), CARDANO_SUCCESS);

  // Act
  std::string text = render_constant_term(arena, pair);

  // Assert
  EXPECT_EQ(text, "(con (pair integer bool) (12345, True))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersNestedPairConstant)
{
  // Arrange
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_uplc_constant_t* inner = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, const_unit(arena), const_bool(arena, true), &inner), CARDANO_SUCCESS);

  cardano_uplc_constant_t* outer = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, const_integer(arena, 12345), inner, &outer), CARDANO_SUCCESS);

  // Act
  std::string text = render_constant_term(arena, outer);

  // Assert
  EXPECT_EQ(text, "(con (pair integer (pair unit bool)) (12345, ((), True)))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersDataIntegerConstant)
{
  // Arrange
  cardano_uplc_arena_t*  arena = new_arena();
  cardano_plutus_data_t* data  = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(42, &data), CARDANO_SUCCESS);

  cardano_uplc_constant_t* c = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, &c), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con data (I 42))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersDataBytesConstant)
{
  // Arrange
  cardano_uplc_arena_t*  arena = new_arena();
  const byte_t           raw[] = { 0x01U, 0x02U };
  cardano_plutus_data_t* data  = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_bytes(raw, sizeof(raw), &data), CARDANO_SUCCESS);

  cardano_uplc_constant_t* c = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, &c), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con data (B #0102))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersDataConstrListMap)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_plutus_data_t* i1 = nullptr;
  cardano_plutus_data_t* i2 = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &i1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &i2), CARDANO_SUCCESS);

  cardano_plutus_list_t* fields = nullptr;
  EXPECT_EQ(cardano_plutus_list_new(&fields), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(fields, i1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(fields, i2), CARDANO_SUCCESS);

  cardano_constr_plutus_data_t* constr = nullptr;
  EXPECT_EQ(cardano_constr_plutus_data_new(0U, fields, &constr), CARDANO_SUCCESS);

  cardano_plutus_data_t* data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_constr(constr, &data), CARDANO_SUCCESS);

  cardano_uplc_constant_t* c = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, &c), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&i1);
  cardano_plutus_data_unref(&i2);
  cardano_plutus_list_unref(&fields);
  cardano_constr_plutus_data_unref(&constr);
  cardano_plutus_data_unref(&data);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con data (Constr 0 [I 1, I 2]))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersDataMap)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_plutus_data_t* k = nullptr;
  cardano_plutus_data_t* v = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &k), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &v), CARDANO_SUCCESS);

  cardano_plutus_map_t* map = nullptr;
  EXPECT_EQ(cardano_plutus_map_new(&map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_insert(map, k, v), CARDANO_SUCCESS);

  cardano_plutus_data_t* data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_map(map, &data), CARDANO_SUCCESS);

  cardano_uplc_constant_t* c = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, &c), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&k);
  cardano_plutus_data_unref(&v);
  cardano_plutus_map_unref(&map);
  cardano_plutus_data_unref(&data);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con data (Map [(I 1, I 2)]))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersDataList)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_plutus_data_t* e = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(7, &e), CARDANO_SUCCESS);

  cardano_plutus_list_t* list = nullptr;
  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(list, e), CARDANO_SUCCESS);

  cardano_plutus_data_t* data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_list(list, &data), CARDANO_SUCCESS);

  cardano_uplc_constant_t* c = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, &c), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&e);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&data);

  // Act
  std::string text = render_constant_term(arena, c);

  // Assert
  EXPECT_EQ(text, "(con data (List [I 7]))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersVariableInsideLambda)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  var   = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);

  cardano_uplc_term_t* lam = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &lam), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(lam);

  // Assert
  EXPECT_EQ(text, "(lam v-0 v-0)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersNestedLambdaBinderLevels)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* outer_var = nullptr;
  cardano_uplc_term_t* inner_var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 2U, &outer_var), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &inner_var), CARDANO_SUCCESS);

  cardano_uplc_term_t* app = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, outer_var, inner_var, &app), CARDANO_SUCCESS);

  cardano_uplc_term_t* inner_lam = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, app, &inner_lam), CARDANO_SUCCESS);

  cardano_uplc_term_t* outer_lam = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, inner_lam, &outer_lam), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(outer_lam);

  // Assert
  EXPECT_EQ(text, "(lam v-0 (lam v-1 [ v-0 v-1 ]))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersFreeVariable)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  var   = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 5U, &var), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(var);

  // Assert
  EXPECT_EQ(text, "free-5");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersDelayForceError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* err = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &err), CARDANO_SUCCESS);

  cardano_uplc_term_t* delayed = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_delay(arena, err, &delayed), CARDANO_SUCCESS);

  cardano_uplc_term_t* forced = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_force(arena, delayed, &forced), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(forced);

  // Assert
  EXPECT_EQ(text, "(force (delay (error)))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersApplication)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* f = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, &f), CARDANO_SUCCESS);

  cardano_uplc_term_t* a = term_constant(arena, const_integer(arena, 1));

  cardano_uplc_term_t* app = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, f, a, &app), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(app);

  // Assert
  EXPECT_EQ(text, "[ (builtin addInteger) (con integer 1) ]");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersBuiltinNames)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  struct
  {
    cardano_uplc_builtin_t builtin;
    const char*            expected;
  } cases[] = {
    { CARDANO_UPLC_BUILTIN_ADD_INTEGER, "(builtin addInteger)" },
    { CARDANO_UPLC_BUILTIN_IF_THEN_ELSE, "(builtin ifThenElse)" },
    { CARDANO_UPLC_BUILTIN_SHA2_256, "(builtin sha2_256)" },
    { CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE, "(builtin verifyEd25519Signature)" },
    { CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD, "(builtin bls12_381_G1_add)" },
    { CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER, "(builtin expModInteger)" },
    { CARDANO_UPLC_BUILTIN_SCALE_VALUE, "(builtin scaleValue)" }
  };

  for (const auto& test_case : cases)
  {
    cardano_uplc_term_t* term = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_builtin(arena, test_case.builtin, &term), CARDANO_SUCCESS);
    EXPECT_EQ(render_term(term), test_case.expected);
  }

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, allBuiltinNamesAreNonEmpty)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  // Act / Assert
  for (int i = 0; i < CARDANO_UPLC_BUILTIN_COUNT; ++i)
  {
    cardano_uplc_term_t* term = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_builtin(arena, static_cast<cardano_uplc_builtin_t>(i), &term), CARDANO_SUCCESS);

    std::string text = render_term(term);
    EXPECT_GT(text.size(), std::string("(builtin )").size());
  }

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersEmptyConstr)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 0U, nullptr, 0U, &term), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(term);

  // Assert
  EXPECT_EQ(text, "(constr 0)");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersConstrWithFields)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* fields[2] = {
    term_constant(arena, const_integer(arena, 1)),
    term_constant(arena, const_integer(arena, 2))
  };

  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 50000U, fields, 2U, &term), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(term);

  // Assert
  EXPECT_EQ(text, "(constr 50000 (con integer 1) (con integer 2))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersCase)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* scrutinee = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 0U, nullptr, 0U, &scrutinee), CARDANO_SUCCESS);

  const cardano_uplc_term_t* branches[2] = {
    term_constant(arena, const_integer(arena, 1)),
    term_constant(arena, const_integer(arena, 2))
  };

  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, scrutinee, branches, 2U, &term), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(term);

  // Assert
  EXPECT_EQ(text, "(case (constr 0) (con integer 1) (con integer 2))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, rendersCaseWithNoBranches)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* scrutinee = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 3U, nullptr, 0U, &scrutinee), CARDANO_SUCCESS);

  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, scrutinee, nullptr, 0U, &term), CARDANO_SUCCESS);

  // Act
  std::string text = render_term(term);

  // Assert
  EXPECT_EQ(text, "(case (constr 3))");

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_program, rendersWholeProgram)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* body = term_constant(arena, const_integer(arena, 23));

  cardano_uplc_term_t* lam = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, &lam), CARDANO_SUCCESS);

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 0U;
  program.version_patch = 0U;
  program.term          = lam;

  cardano_buffer_t* out = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_pretty_print_program(&program, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  std::string text(reinterpret_cast<const char*>(cardano_buffer_get_data(out)));
  EXPECT_EQ(text, "(program 1.0.0 (lam v-0 (con integer 23)))");

  cardano_buffer_unref(&out);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, deeplyNestedTermDoesNotCrash)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  for (int i = 0; i < 10000; ++i)
  {
    cardano_uplc_term_t* wrapped = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_delay(arena, term, &wrapped), CARDANO_SUCCESS);
    term = wrapped;
  }

  cardano_buffer_t* out = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_pretty_print_term(term, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ILLEGAL_STATE);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, returnsErrorOnNullTerm)
{
  // Arrange
  cardano_buffer_t* out = nullptr;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_pretty_print_term(nullptr, &out), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_pretty_print_term, returnsErrorOnNullOut)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  // Act / Assert
  EXPECT_EQ(cardano_uplc_pretty_print_term(term, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_program, returnsErrorOnNullArguments)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 0U;
  program.version_patch = 0U;
  program.term          = nullptr;

  cardano_buffer_t* out = nullptr;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_pretty_print_program(nullptr, &out), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_pretty_print_program(&program, &out), CARDANO_ERROR_POINTER_IS_NULL);

  program.term = term;
  EXPECT_EQ(cardano_uplc_pretty_print_program(&program, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, failsWhenOutputBufferAllocationFails)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_buffer_t* out = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_pretty_print_term(term, &out);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_program, failsWhenOutputBufferAllocationFails)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 0U;
  program.version_patch = 0U;
  program.term          = term;

  cardano_buffer_t* out = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_pretty_print_program(&program, &out);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_pretty_print_term, failsWhenBufferGrowthFailsMidRender)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* fields[64];
  for (int i = 0; i < 64; ++i)
  {
    fields[i] = term_constant(arena, const_integer(arena, 1234567890));
  }

  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 0U, const_cast<const cardano_uplc_term_t* const*>(fields), 64U, &term), CARDANO_SUCCESS);

  cardano_buffer_t* out = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_pretty_print_term(term, &out);

  // Assert
  cardano_set_allocators(malloc, realloc, free);
  EXPECT_NE(error, CARDANO_SUCCESS);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}
