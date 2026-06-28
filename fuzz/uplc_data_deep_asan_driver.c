/*
 * Throwaway ASan probe for the arena Plutus-data deep-nesting hardening.
 * NOT a libFuzzer target. Builds with -fsanitize=address and constructs a
 * degenerate-deep arena data value, then runs cardano_uplc_data_equals,
 * cardano_uplc_data_node_ex_mem and the CBOR serializer on it. Before the
 * iterative rewrite these would overflow the C stack; after, they complete.
 */

#include "uplc/arena.h"
#include "uplc/uplc_data.h"

#include <cardano/cbor/cbor_writer.h>

#include <stdio.h>
#include <stdlib.h>

static const cardano_uplc_data_t*
build_chain(cardano_uplc_arena_t* arena, size_t depth, int as_list)
{
  cardano_uplc_data_t* node = NULL;

  if (cardano_uplc_data_new_integer_small(arena, 0, &node) != CARDANO_SUCCESS)
  {
    return NULL;
  }

  for (size_t i = 0U; i < depth; ++i)
  {
    const cardano_uplc_data_t** slot =
      (const cardano_uplc_data_t**)cardano_uplc_arena_alloc(arena, sizeof(const cardano_uplc_data_t*), 0U);

    cardano_uplc_data_t* parent = NULL;

    if (slot == NULL)
    {
      return NULL;
    }

    slot[0] = node;

    if (as_list)
    {
      if (cardano_uplc_data_new_list(arena, slot, 1U, &parent) != CARDANO_SUCCESS)
      {
        return NULL;
      }
    }
    else
    {
      if (cardano_uplc_data_new_constr(arena, 0U, slot, 1U, &parent) != CARDANO_SUCCESS)
      {
        return NULL;
      }
    }

    node = parent;
  }

  return node;
}

int
main(void)
{
  const size_t          depth = 1000000U;
  cardano_uplc_arena_t* arena = NULL;

  if (cardano_uplc_arena_new(4096U, &arena) != CARDANO_SUCCESS)
  {
    fprintf(stderr, "arena alloc failed\n");
    return 1;
  }

  const cardano_uplc_data_t* constr_chain = build_chain(arena, depth, 0);
  const cardano_uplc_data_t* list_chain   = build_chain(arena, depth, 1);

  if ((constr_chain == NULL) || (list_chain == NULL))
  {
    fprintf(stderr, "build failed\n");
    cardano_uplc_arena_free(&arena);
    return 1;
  }

  if (!cardano_uplc_data_equals(constr_chain, constr_chain) ||
      !cardano_uplc_data_equals(list_chain, list_chain) ||
      cardano_uplc_data_equals(constr_chain, list_chain))
  {
    fprintf(stderr, "equality result unexpected\n");
    cardano_uplc_arena_free(&arena);
    return 1;
  }

  int64_t ex_mem = cardano_uplc_data_node_ex_mem(constr_chain);
  int64_t count  = cardano_uplc_data_node_count(list_chain);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (cardano_uplc_data_to_cbor(list_chain, writer) != CARDANO_SUCCESS)
  {
    fprintf(stderr, "serialise failed\n");
    cardano_cbor_writer_unref(&writer);
    cardano_uplc_arena_free(&arena);
    return 1;
  }

  cardano_cbor_writer_unref(&writer);
  cardano_uplc_arena_free(&arena);

  printf("OK depth=%zu ex_mem=%lld node_count=%lld\n", depth, (long long)ex_mem, (long long)count);
  return 0;
}
