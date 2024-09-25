/**
 * \file set.c
 *
 * \author luisd.bianchi
 * \date   Mar 04, 2024
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

#include "set.h"

#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t BUCKET_COUNT = 128U;

/* STRUCTS *******************************************************************/

/**
 * \brief Represents an entry in a Cardano set.
 *
 * This structure is a linked list node that contains a pointer to the stored object
 * and a pointer to the next entry in the bucket.
 */
typedef struct cardano_set_entry_t
{
    cardano_object_t*           object;
    struct cardano_set_entry_t* next;
} cardano_set_entry_t;

/**
 * \brief Represents a set data structure in the Cardano C library.
 *
 * This structure provides a hash set implementation. It stores unique elements in an array of
 * linked lists (buckets) to manage collisions.
 */
typedef struct cardano_set_t
{
    cardano_object_t           base;
    cardano_set_entry_t*       buckets[128];
    size_t                     size;
    cardano_set_compare_item_t compare;
    cardano_set_hash_func_t    hash;
} cardano_set_t;

/* STATIC FUNCTIONS ***********************************************************/

/**
 * \brief Deallocates a set object.
 *
 * This function is responsible for properly deallocating a set object (`cardano_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_set_deallocate(void* object)
{
  cardano_set_t* set = (cardano_set_t*)object;
  assert(set != NULL);

  for (size_t i = 0U; i < BUCKET_COUNT; ++i)
  {
    cardano_set_entry_t* entry = set->buckets[i];

    while (entry != NULL)
    {
      cardano_set_entry_t* next = entry->next;

      cardano_object_unref(&(entry->object));

      _cardano_free(entry);

      entry = next;
    }

    set->buckets[i] = NULL;
  }

  _cardano_free(set);
}

/* DEFINITIONS ****************************************************************/

cardano_set_t*
cardano_set_new(cardano_set_compare_item_t compare, cardano_set_hash_func_t hash)
{
  cardano_set_t* set = (cardano_set_t*)_cardano_malloc(sizeof(cardano_set_t));

  if (set == NULL)
  {
    return NULL;
  }

  CARDANO_UNUSED(memset(set->buckets, 0, sizeof(set->buckets)));

  set->size               = 0;
  set->compare            = compare;
  set->hash               = hash;
  set->base.ref_count     = 1;
  set->base.last_error[0] = '\0';
  set->base.deallocator   = cardano_set_deallocate;

  return set;
}

cardano_set_t*
cardano_set_from_array(cardano_array_t* array, cardano_set_compare_item_t compare, cardano_set_hash_func_t hash)
{
  if ((array == NULL) || (compare == NULL) || (hash == NULL))
  {
    return NULL;
  }

  cardano_set_t* set = cardano_set_new(compare, hash);

  if (set == NULL)
  {
    return NULL;
  }

  for (size_t i = 0; i < cardano_array_get_size(array); ++i)
  {
    cardano_object_t* item = cardano_array_get(array, i);

    assert(item != NULL);

    if (cardano_set_add(set, item) == 0U)
    {
      cardano_set_unref(&set);
      cardano_object_unref(&item);
      return NULL;
    }

    cardano_object_unref(&item);
  }

  return set;
}

void
cardano_set_unref(cardano_set_t** set)
{
  if ((set == NULL) || (*set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *set = NULL;
    return;
  }
}

void
cardano_set_ref(cardano_set_t* set)
{
  if (set == NULL)
  {
    return;
  }

  cardano_object_ref(&set->base);
}

size_t
cardano_set_refcount(const cardano_set_t* set)
{
  if (set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&set->base);
}

size_t
cardano_set_add(cardano_set_t* set, cardano_object_t* item)
{
  if ((set == NULL) || (item == NULL))
  {
    return 0;
  }

  size_t index = set->hash(item) % BUCKET_COUNT;

  cardano_set_entry_t** entry = &(set->buckets[index]);

  while (*entry != NULL)
  {
    if (set->compare((*entry)->object, item) == 0)
    {
      return set->size;
    }

    entry = &((*entry)->next);
  }

  *entry = (cardano_set_entry_t*)_cardano_malloc(sizeof(cardano_set_entry_t));

  if (*entry == NULL)
  {
    return 0;
  }

  cardano_object_ref(item);
  (*entry)->object = item;
  (*entry)->next   = NULL;

  ++set->size;

  return set->size;
}

bool
cardano_set_has(cardano_set_t* set, cardano_object_t* item)
{
  if ((set == NULL) || (item == NULL))
  {
    return false;
  }

  size_t index = set->hash(item) % BUCKET_COUNT;

  for (cardano_set_entry_t* entry = set->buckets[index]; entry != NULL; entry = entry->next)
  {
    if (set->compare(entry->object, item) == 0)
    {
      return true;
    }
  }

  return false;
}

bool
cardano_set_delete(cardano_set_t* set, cardano_object_t* item)
{
  if ((set == NULL) || (item == NULL))
  {
    return false;
  }

  size_t                index = set->hash(item) % BUCKET_COUNT;
  cardano_set_entry_t** entry = &(set->buckets[index]);

  while (*entry != NULL)
  {
    if (set->compare((*entry)->object, item) == 0)
    {
      cardano_set_entry_t* toDelete = *entry;
      *entry                        = (*entry)->next;

      cardano_object_unref(&(toDelete->object));
      _cardano_free(toDelete);

      --set->size;

      return true;
    }

    entry = &((*entry)->next);
  }

  return false;
}

cardano_array_t*
cardano_get_entries(cardano_set_t* set)
{
  if (set == NULL)
  {
    return NULL;
  }

  cardano_array_t* array = cardano_array_new(set->size);

  if (array == NULL)
  {
    return NULL;
  }

  for (size_t i = 0; i < BUCKET_COUNT; ++i)
  {
    for (cardano_set_entry_t* entry = set->buckets[i]; entry != NULL; entry = entry->next)
    {
      if (!cardano_array_push(array, entry->object))
      {
        cardano_array_unref(&array);
        return NULL;
      }
    }
  }

  return array;
}

size_t
cardano_set_get_size(const cardano_set_t* set)
{
  if (set == NULL)
  {
    return 0;
  }

  return set->size;
}

void
cardano_set_clear(cardano_set_t* set)
{
  if (set == NULL)
  {
    return;
  }

  for (size_t i = 0; i < BUCKET_COUNT; ++i)
  {
    cardano_set_entry_t* entry = set->buckets[i];

    while (entry != NULL)
    {
      cardano_set_entry_t* next = entry->next;
      cardano_object_unref(&entry->object); // Assuming this is the correct function name.
      _cardano_free(entry);
      entry = next;
    }

    set->buckets[i] = NULL;
  }

  set->size = 0;
}

cardano_object_t*
cardano_set_find(const cardano_set_t* set, cardano_set_unary_predicate_t predicate, const void* context)
{
  if ((set == NULL) || (predicate == NULL))
  {
    return NULL;
  }

  for (size_t i = 0; i < BUCKET_COUNT; ++i)
  {
    for (cardano_set_entry_t* entry = set->buckets[i]; entry != NULL; entry = entry->next)
    {
      if (predicate(entry->object, context))
      {
        cardano_object_ref(entry->object);
        return entry->object;
      }
    }
  }

  return NULL;
}

void
cardano_set_set_last_error(cardano_set_t* set, const char* message)
{
  cardano_object_set_last_error(&set->base, message);
}

const char*
cardano_set_get_last_error(const cardano_set_t* set)
{
  return cardano_object_get_last_error(&set->base);
}
