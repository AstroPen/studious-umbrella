// TODO Write tests and actually get this code running.
// Probably will finish this when I actually need a hash table for something.


namespace hash_table_internal {

  struct FreeListNode {
    FreeListNode *next_free;
  };

  static inline bool is_prime(uint32_t count) {
    if (count <= 1) return false;
    // TODO more through checking for prime
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

  inline uint32_t probe(HashTable_Internal *table, Key key, bool insert_mode = false) {
    assert(table);
    assert(table->key_set);
    assert(table->value_set);
    assert(table->allocated_data);
    assert(key != EMPTY); // Illegal values
    assert(key != REMOVED);

    uint32_t hash_value = get_hash_value(key, table->set_length);
    Key stored = table->key_set[hash_value];

    Key *first_remove = NULL;
    uint32_t upper_limit = table->count + 1;

    for (uint32_t i = 1; i <= upper_limit && i <= table->max_count; i++) {
      assert(i <= table->set_length);
      // TODO Check how large i usually gets under different conditions. It needs to stay 
      // small for this to be performant.

      if (stored == EMPTY || stored == value) {
        return s->set + hash_value;
      } 

      if (stored == REMOVED) {

        // NOTE : Removed values need to be kept separate from empty values in order for 
        // probing to work. However, this means we may iterate over more than s->count 
        // locations. To account for this, the upper_limit is incremented each time we 
        // find a removed value.
        upper_limit++;

        if (insert_mode && !first_remove) {
          first_remove = table->key_set + hash_value;
        }
      }

      // Linear probing :
      int skip = 1;
      hash_value = (hash_value + skip) % table->set_length;
      assert(hash_value >= 0 && hash_value < table->set_length);
      stored = table->key_set[hash_value];
    }

    if (insert_mode) return first_remove;
    return NULL;
  }
}


static inline void clear(HashTable_Internal *table) {
  using namespace hash_table_internal;

  HashValue_Internal *start_ptr = table->allocated_data + table->max_count - 1;
  FreeListNode *prev = NULL;
  for (auto ptr = start_ptr; ptr >= table->allocated_data; ptr--) {
    auto node = (FreeListNode *) ptr;
    node->next_free = prev;
    prev = node;
  }

  for (uint32_t i = 0; i < table->set_length; i++) {
    table->key_set[i] = EMPTY;
  }

  table->count = 0;
}

// TODO
bool init_hash_table_(HashTable_Internal *table, PushAllocator *allocator, uint32_t count) {
  using namespace hash_table_internal;

  *table = {};
  assert(is_prime(count));

  uint32_t node_size = elem_size + sizeof(NodeHeader);
  uint32_t key_set_size = calc_hashset_memory_size(count, uint32_t);
  uint32_t data_size = count * node_size;
  uint32_t total_size = key_set_size + data_size;

  // TODO there should be a helper for this...
  void *memory = alloc_size(allocator, total_size, 8);
  if (!memory) {
    return false;
  }

  // NOTE : Do NOT use allocator after this point.
  PushAllocator temp = {};
  temp.memory = memory;
  temp.max_size = total_size;

  table->data = create_heap_(&temp, count, node_size, 8);
  assert(table->data.count && table->data.free_list);

  void *key_set_memory = alloc_size(&temp, key_set_size, 8);
  assert(key_set_memory);

  init_hash_set(&table->key_set, key_set_memory, count);
}

static HASH_TABLE_VALUE_TYPE *insert(HashTable_Internal *table, Key key) {
  using namespace hash_table_internal;

  if (table->max_count == table->count) return 0;

  uint32_t index = probe(table, key, true);

  assert(index != EMPTY_U32); // Should never happen
  if (index == EMPTY_U32) return NULL;

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

  uint32_t index = probe(table, key);
  if (index == EMPTY_U32) return false;

  Key *key_location = table->key_set + index;
  HashValue_Internal **value_location = table->value_set + index;

  assert(*key_location == key);
  assert(*value_location);

  return *value_location;
}

static bool remove(HashTable_Internal *table, Key key) {
  using namespace hash_table_internal;

  uint32_t index = probe(table, key);
  if (index == EMPTY_U32) return false;

  Key *key_location = table->key_set + index;
  HashValue_Internal **value_location = table->value_set + index;

  assert(*key_location == key);
  assert(*value_location);
  HashValue_Internal *value_allocation = *value_location;
  auto free_node = value_allocation->node;
  free_node->next_free = table->free_list;
  table->free_list = free_node;

  *key_location = REMOVED;
  *value_location = NULL;

  table->count--;
  return true;
}

