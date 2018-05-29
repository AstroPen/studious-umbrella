
#define Key uint32_t
#define Value 



namespace hash_set_internal {

  inline Key *probe(HashSet_Internal *s, Key value, bool insert_mode = false) {
    assert(s);
    assert(s->set);
    assert(value != EMPTY); // Illegal values
    assert(value != REMOVED);

    uint32_t hash_value = get_hash_value(value, s->max_count);
    Key stored = s->set[hash_value];

    Key *first_remove = NULL;
    uint32_t upper_limit = s->count + 1;

    for (uint32_t i = 1; i <= upper_limit && i <= s->max_count; i++) {
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
          first_remove = s->set + hash_value;
        }
      }

      // Linear probing :
      int skip = 1;
      hash_value = (hash_value + skip) % s->max_count;
      assert(hash_value >= 0 && hash_value < s->max_count);
      stored = s->set[hash_value];
    }

    if (insert_mode) return first_remove;
    return NULL;
  }
}

static inline void clear(HashSet_Internal *s) {
  using namespace hash_set_internal;

  s->count = 0;
  for (uint32_t i = 0; i < s->max_count; i++) {
    s->set[i] = EMPTY;
  }
}

// NOTE : It is on the user to pass in a prime value for count, but we
// check a few small primes for some safety.
static void init_hash_set(HashSet_Internal *result, void *memory, uint32_t count) {
  assert(memory);
  assert(count > 0);
  assert(is_prime(count));

  *result = {};
  result->set = (Key *) memory;
  result->max_count = count;
  
  clear(result);
}

// NOTE : keeping this an accessor since I'm experimenting
static inline uint32_t count(HashSet_Internal *s) {
  return s->count;
}

static inline uint32_t remaining(HashSet_Internal *s) {
  return s->max_count - s->count;
}

static bool contains(HashSet_Internal *s, Key value) {
  using namespace hash_set_internal;

  Key *location = probe(s, value);
  if (!location) return false;

  if (*location == EMPTY) return false;

  assert(*location == value);
  return true;
}

static inline Key *get_first(HashSet_Internal *s) {
  using namespace hash_set_internal;

  if (s->count == 0) return NULL;
  for (uint32_t i = 0; i < s->max_count; i++) {
    if (s->set[i] != EMPTY && s->set[i] != REMOVED) {
      return s->set + i;
    }
  }
  assert(!"Failed to find first HashSet element with non-zero count.");
  return NULL;
}

// NOTE : This is remove safe, but slower when the set is mostly empty.
static inline Key *get_next(HashSet_Internal *s, Key *current) {
  using namespace hash_set_internal;

  if (s->count == 0) return NULL;
  int current_idx = current - s->set;
  for (int i = current_idx + 1; i < s->max_count; i++) {
    if (s->set[i] != EMPTY && s->set[i] != REMOVED) {
      return s->set + i;
    }
  }
  return NULL;
}

// TODO get_array and a fast iterable HashSet that uses a link list.

// This iterator is NOT remove/insert safe
struct HashSetIterator_Internal {
  HashSet_Internal *set;
  uint32_t current_idx;
  uint32_t current_count;
};

static inline HashSetIterator_Internal get_iterator(HashSet_Internal *s) {
  HashSetIterator_Internal result = {};
  result.set = s;
  return result;
}

static inline bool has_next(HashSetIterator_Internal const *iter) {
  auto s = iter->set;
  assert(iter->current_count <= s->count);
  assert(iter->current_idx <= s->max_count);
  if (iter->current_count == s->count) return false;
  return true;
}

static inline Key *get_next(HashSetIterator_Internal *iter) {
  using namespace hash_set_internal;

  if (!has_next(iter)) return NULL;
  auto s = iter->set;

  for (int i = iter->current_idx; i < s->max_count; i++) {
    if (s->set[i] != EMPTY && s->set[i] != REMOVED) {
      iter->current_idx = i + 1;
      iter->current_count++;
      return s->set + i;
    }
  }
  return NULL;
}


struct HashTable {
  Key *key_set;
  Value **value_set;
  Value *allocated_data;
  Value *free_list;
  uint32_t count;
  uint32_t max_count;
  uint32_t set_length;
};

namespace hash_table_internal {
  const uint32_t EMPTY_U32 = UINT_MAX;
  const uint32_t REMOVED_U32 = UINT_MAX - 1;


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

  inline uint32_t probe(HashTable *table, Key key, bool insert_mode = false) {
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

// TODO make sure Value is at least 8 bytes so the free list works
struct HashTable {
  Key *key_set;
  Value **value_set;
  Value *allocated_data;
  Value *free_list;
  uint32_t count;
  uint32_t max_count;
  uint32_t set_length;
};

static inline void clear(HashTable *table) {
  using namespace hash_table_internal;

  Value *start_ptr = table->allocated_data + table->max_count - 1;
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

bool init_hash_table_(HashTable *table, PushAllocator *allocator, uint32_t count) {
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

static Value *insert(HashTable *table, Key key) {
  using namespace hash_table_internal;

  if (table->max_count == table->count) return 0;

  uint32_t index = probe(table, key, true);

  assert(index != EMPTY_U32); // Should never happen
  if (index == EMPTY_U32) return NULL;

  Key *key_location = table->key_set + index;
  Value **value_location = table->value_set + index;

  if (*key_location == EMPTY || *key_location == REMOVED) {
    // NOTE : insert new entry
    auto free_node = table->free_list;
    assert(free_node); // Should never happen
    if (!free_node) return NULL;

    *key_location = key;
    table->count++;
    table->free_list = free_node->next_free;

    Value *value_allocation = (Value *) free_node; 
    *value_allocation = {};

    *value_location = value_allocation;
  }

  // NOTE : value was already in s.
  assert(*key_location == key);
  assert(*value_location);
  return *value_location;
}

static Value *get(HashTable *table, Key key) {
  using namespace hash_table_internal;

  uint32_t index = probe(table, key);
  if (index == EMPTY_U32) return false;

  Key *key_location = table->key_set + index;
  Value **value_location = table->value_set + index;

  assert(*key_location == key);
  assert(*value_location);

  return *value_location;
}

static bool remove(HashTable *table, Key key) {
  using namespace hash_table_internal;

  uint32_t index = probe(table, key);
  if (index == EMPTY_U32) return false;

  Key *key_location = table->key_set + index;
  Value **value_location = table->value_set + index;

  assert(*key_location == key);
  assert(*value_location);
  Value *value_allocation = *value_location;
  auto free_node = (FreeListNode *) value_allocation;
  free_node->next_free = table->free_list;
  table->free_list = free_node;

  *key_location = REMOVED;
  *value_location = NULL;

  table->count--;
  return true;
}

