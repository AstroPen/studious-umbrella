
// TODO Better separate header definions from implementations
// TODO Count the longest probe
// TODO Add option to store hash values
// TODO Implement robin hood hashing
//
// TODO consider using push_macro instead of undefing everything at the end

#ifndef _HASH_SET_H_
#define _HASH_SET_H_

// TODO pull these out into a separate file that can be shared with hash_table
// (along with any other general hashing-related functions).
static inline uint32_t get_hash_value(uint32_t value, uint32_t max_count) {
  return value % max_count;
}

static inline uint32_t get_hash_value(int32_t value, uint32_t max_count) {
  return ((uint32_t)value) % max_count;
}

static inline uint32_t get_hash_value(void *value_ptr, uint32_t max_count) {
  uint64_t value = (uint64_t) value_ptr;
  return value % max_count;
}

static inline bool is_prime(uint32_t count) {
  if (count <= 1) return false;
  // TODO more through checking for prime
  if (count > 2 && count % 2 == 0) return false;
  if (count > 3 && count % 3 == 0) return false;
  return true;
}

namespace hash_set_internal {

  const uint32_t EMPTY_U32 = UINT_MAX;
  const uint32_t REMOVED_U32 = UINT_MAX - 1;

  const int32_t EMPTY_S32 = INT_MIN;
  const int32_t REMOVED_S32 = INT_MAX;

  const void *EMPTY_PTR = NULL;
  const void *REMOVED_PTR = (void *) 0xffffffffffffffff;
}

#define calc_hashset_memory_size(max_element_count, keytype) (max_element_count * sizeof(keytype))

#endif

// NOTE : Set this to choose the name
#ifdef HASH_SET_TYPE_NAME
#define HashSet_Internal HASH_SET_TYPE_NAME
#define ITERATOR_NAME__(type) type ## _Iterator
#define ITERATOR_NAME_(type) ITERATOR_NAME__(type)
#define HashSetIterator_Internal ITERATOR_NAME_(HASH_SET_TYPE_NAME)
#else
#define HashSet_Internal HashSet
#define HashSetIterator_Internal HashSet_Iterator
#endif

// TODO test this
// NOTE : Define this to use a struct for the keys.
#ifdef HASH_SET_KEYTYPE_STRUCT

#undef HASH_SET_KEYTYPE_PTR
#undef HASH_SET_KEYTYPE_U32
#undef HASH_SET_KEYTYPE_S32

#define Key HASH_SET_KEYTYPE_STRUCT
#ifndef EMPTY_STRUCT
#error "To use HASH_SET_KEYTYPE_STRUCT, a value for EMPTY_STRUCT must be user defined."
#elif !defined(REMOVED_STRUCT)
#error "To use HASH_SET_KEYTYPE_STRUCT, a value for REMOVED_STRUCT must be user defined."
#endif
#define EMPTY EMPTY_STRUCT
#define REMOVED REMOVED_STRUCT
#endif

// NOTE : Set this to choose a pointer type for the keys.
#ifdef HASH_SET_KEYTYPE_PTR

#undef HASH_SET_KEYTYPE_U32
#undef HASH_SET_KEYTYPE_S32

#define Key HASH_SET_KEYTYPE_PTR *
#define EMPTY ((Key) EMPTY_PTR)
#define REMOVED ((Key) REMOVED_PTR)
#endif

// NOTE : Define this to use uint32_t for the keys.
#ifdef HASH_SET_KEYTYPE_U32

#undef HASH_SET_KEYTYPE_S32
#define Key uint32_t
#define EMPTY EMPTY_U32
#define REMOVED REMOVED_U32

#endif

// NOTE : Define this to use int32_t for the keys.
#ifdef HASH_SET_KEYTYPE_S32

#define Key int32_t
#define EMPTY EMPTY_S32
#define REMOVED REMOVED_S32

#endif

struct HashSet_Internal {
  Key *set; // NOTE : Key must be user-defined.
  uint32_t count;
  uint32_t max_count;
  uint32_t max_probe_len;
};


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
    // TODO take into account max_probe_len

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

static int insert(HashSet_Internal *s, Key value) {
  using namespace hash_set_internal;

  if (s->max_count == s->count) return 0;

  Key *location = probe(s, value, true);

  assert(location); // Should never happen
  if (!location) return 0;

  if (*location == EMPTY || *location == REMOVED) {
    *location = value;
    s->count++;
    return 1;
  }

  // NOTE : value was already in s.
  assert(*location == value);
  return 2;
}

static bool contains(HashSet_Internal *s, Key value) {
  using namespace hash_set_internal;

  Key *location = probe(s, value);
  if (!location) return false;

  if (*location == EMPTY) return false;

  assert(*location == value);
  return true;
}

static bool remove(HashSet_Internal *s, Key value) {
  using namespace hash_set_internal;

  Key *location = probe(s, value);
  if (!location) return false;

  if (*location == EMPTY) return false;

  assert(*location == value);
  *location = REMOVED;
  s->count--;
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

// TODO double check that this file is polluting the preprocessor namespace with symbols
// TODO rename Key to something longer and unlikely to be user defined
#undef Key

#undef HashSet_Internal
#undef HashSetIterator_Internal

#undef HASH_SET_KEYTYPE_PTR
#undef HASH_SET_KEYTYPE_U32
#undef HASH_SET_KEYTYPE_S32
#undef HASH_SET_TYPE_NAME
#undef EMPTY
#undef REMOVED

