// TODO Write tests and actually get this code running.
// Probably will finish this when I actually need a hash table for something.

#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_
// TODO change this when I move push_allocator.h
#include "../GameEngine/push_allocator.h"

namespace hash_table_internal {

  struct FreeListNode {
    FreeListNode *next_free;
  };

  static inline bool is_prime(uint32_t count) {
    if (count <= 1) return false;
    // TODO more thorough checking for prime
    if (count > 2 && count % 2 == 0) return false;
    if (count > 3 && count % 3 == 0) return false;
    return true;
  }
}

// NOTE : Set this to choose the name
#ifdef HASH_TABLE_TYPE_NAME
#define HashTable_Internal HASH_TABLE_TYPE_NAME
#else
#define HashTable_Internal HashTable
#endif


#ifndef HASH_TABLE_VALUE_TYPE
#error "HASH_TABLE_VALUE_TYPE must be user-defined."
#endif

// TODO require EMPTY and REMOVED
#ifndef HASH_TABLE_KEY_TYPE
#error "HASH_TABLE_KEY_TYPE must be user-defined."
#endif

#define Key HASH_TABLE_KEY_TYPE

#define HASH_TABLE_CONTAINER_NAME__(type) type ## _Container
#define HASH_TABLE_CONTAINER_NAME_(type) HASH_TABLE_CONTAINER_NAME__(type)
#define HashValue_Internal HASH_TABLE_CONTAINER_NAME_(HASH_TABLE_VALUE_TYPE)

union HashValue_Internal {
  HASH_TABLE_VALUE_TYPE value;
  hash_table_internal::FreeListNode node;
};

struct HashTable_Internal {
  Key *key_set;
  HashValue_Internal **value_set;

  HashValue_Internal *allocated_data;
  hash_table_internal::FreeListNode *free_list;

  uint32_t count;
  uint32_t max_count;
  uint32_t set_length;
};


namespace hash_table_internal {

  inline void assert_initialized(HashTable_Internal *table) {
    assert(table);
    assert(table->key_set);
    assert(table->value_set);
    assert(table->allocated_data);
    assert(table->max_count);
    assert(table->set_length);
    assert(table->max_count == table->count || table->free_list);
  }

  inline bool is_initialized(HashTable_Internal *table) {
    if (!table) return false;
    if (!table->key_set) return false;
    if (!table->value_set) return false;
    if (!table->allocated_data) return false;
    if (!table->max_count) return false;
    if (!table->set_length) return false;
    if (table->max_count != table->count && !table->free_list) return false;
    return true;
  }

  inline uint32_t probe(HashTable_Internal *table, Key key, bool insert_mode = false) {
    assert(key != EMPTY); // Illegal values
    assert(key != REMOVED);

    uint32_t hash_index = get_hash_value(key) % table->set_length;
    Key stored = table->key_set[hash_index];

    uint32_t first_remove = UINT_MAX;
    uint32_t upper_limit = table->count + 1;

    for (uint32_t i = 1; i <= upper_limit && i <= table->max_count; i++) {
      assert(i <= table->set_length);
      // TODO Check how large i usually gets under different conditions. It needs to stay 
      // small for this to be performant.

      if (stored == EMPTY || stored == key) {
        return hash_index;
      } 

      if (stored == REMOVED) {

        // NOTE : Removed values need to be kept separate from empty values in order for 
        // probing to work. However, this means we may iterate over more than s->count 
        // locations. To account for this, the upper_limit is incremented each time we 
        // find a removed value.
        upper_limit++;

        if (insert_mode && first_remove != UINT_MAX) {
          first_remove = hash_index;
        }
      }

      // Linear probing :
      int skip = 1;
      hash_index = (hash_index + skip) % table->set_length;
      assert(hash_index >= 0 && hash_index < table->set_length);
      stored = table->key_set[hash_index];
    }

    if (insert_mode) return first_remove;
    return UINT_MAX;
  }
}


static inline void clear(HashTable_Internal *table) {
  using namespace hash_table_internal;

  assert(is_initialized(table));

  HashValue_Internal *start_ptr = table->allocated_data + table->max_count - 1;
  FreeListNode *prev = NULL;
  for (auto ptr = start_ptr; ptr >= table->allocated_data; ptr--) {
    auto node = (FreeListNode *) ptr;
    node->next_free = prev;
    prev = node;
  }
  table->free_list = prev;
  assert(prev == &table->allocated_data->node);

  for (uint32_t i = 0; i < table->set_length; i++) {
    table->key_set[i] = EMPTY;
  }

  table->count = 0;
}

// TODO Make a more elegant interface for this, finish this function
bool init_hash_table(HashTable_Internal *table, PushAllocator *allocator, uint32_t count, uint32_t set_length) {
  using namespace hash_table_internal;

  *table = {};
  assert(set_length >= count);
  assert(is_prime(set_length));

  uint32_t key_set_size = set_length * sizeof(Key);
  uint32_t value_set_size = set_length * sizeof(HashValue_Internal*);
  uint32_t node_size = sizeof(HashValue_Internal);
  uint32_t data_size = count * node_size;
  uint32_t total_size = key_set_size + + value_set_size + data_size;

  // NOTE : Do NOT use allocator after this point.
  PushAllocator temp = new_push_allocator(allocator, total_size);
  if (!is_initialized(&temp)) return false;

  table->allocated_data = alloc_array(&temp, HashValue_Internal, count);
  assert(table->allocated_data);

  table->key_set = alloc_array(&temp, Key, set_length);
  assert(table->key_set);

  table->value_set = alloc_array(&temp, HashValue_Internal*, set_length);
  assert(table->value_set);

  table->max_count = count;
  table->set_length = set_length;
  table->count = count; // NOTE : this is for better error handling

  clear(table);

  return true;
}

static HASH_TABLE_VALUE_TYPE *insert(HashTable_Internal *table, Key key) {
  using namespace hash_table_internal;

  assert(is_initialized(table));
  if (table->max_count == table->count) return NULL;

  uint32_t index = probe(table, key, true);

  assert(index != UINT_MAX); // Should never happen
  if (index == UINT_MAX) return NULL;

  Key *key_location = table->key_set + index;
  HashValue_Internal **value_location = table->value_set + index;

  if (*key_location == EMPTY || *key_location == REMOVED) {
    // NOTE : insert new entry
    auto free_node = table->free_list;
    assert(free_node); // Should never happen
    if (!free_node) return NULL;

    *key_location = key;
    table->count++;
    table->free_list = free_node->next_free;

    HashValue_Internal *value_allocation = (HashValue_Internal *) free_node; 
    *value_allocation = {};

    *value_location = value_allocation;
  }

  // NOTE : value was already in s.
  HashValue_Internal *result = *value_location;
  assert(*key_location == key);
  assert(result);
  return &result->value;
}

static HASH_TABLE_VALUE_TYPE *get(HashTable_Internal *table, Key key) {
  using namespace hash_table_internal;

  assert(is_initialized(table));
  uint32_t index = probe(table, key);
  if (index == UINT_MAX) return NULL;

  Key *key_location = table->key_set + index;
  if (*key_location == EMPTY) return NULL;
  HashValue_Internal **value_location = table->value_set + index;

  assert(*key_location == key);
  assert(*value_location);

  return &(*value_location)->value;
}

static bool remove(HashTable_Internal *table, Key key) {
  using namespace hash_table_internal;

  assert(is_initialized(table));
  uint32_t index = probe(table, key);
  if (index == UINT_MAX) return false;

  Key *key_location = table->key_set + index;
  if (*key_location == EMPTY) return false;

  HashValue_Internal **value_location = table->value_set + index;

  assert(*key_location == key);
  assert(*value_location);
  HashValue_Internal *value_allocation = *value_location;
  auto free_node = &value_allocation->node;
  free_node->next_free = table->free_list;
  table->free_list = free_node;

  *key_location = REMOVED;
  *value_location = NULL;

  table->count--;
  return true;
}

#endif

