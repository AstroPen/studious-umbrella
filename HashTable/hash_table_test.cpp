
// TODO generalize these tests and randomize the inputs

#include "common.h"

#define HASH_SET_KEYTYPE_U32
//#define HASH_SET_TYPE_NAME HashSet
#include "hash_set.h"

#define HASH_SET_KEYTYPE_S32
#define HASH_SET_TYPE_NAME HashSet_S32
#include "hash_set.h"

#define HASH_SET_KEYTYPE_PTR char
#define HASH_SET_TYPE_NAME StringSet
#include "hash_set.h"

//
// TODO define this as some sort of general utility function:
//

inline bool memequal(void *a, void *b, uint32_t size) {
  uint8_t *p = (uint8_t *)a;
  uint8_t *q = (uint8_t *)b;
  for (uint32_t i = 0; i < size; i++, p++, q++) {
    if (*p != *q) return false;
  }
  return true;
}

#define DEFAULT_COMPARISON(Type) \
inline bool operator==(Type a, Type b) { return memequal(&a, &b, sizeof(Type)); } \
inline bool operator!=(Type a, Type b) { return !(a == b); } \

//
//
//

struct Point {
  int x;
  int y;
};

DEFAULT_COMPARISON(Point);

// TODO maybe use something that wasn't copied off stackoverflow
uint32_t get_hash_value(Point p) {
  uint32_t seed = 2;
  seed ^= p.x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= p.y + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}

struct PointEntity {
  const char *name;
  int x;
  int y;
};

#define HASH_TABLE_KEY_TYPE Point
#define HASH_TABLE_VALUE_TYPE PointEntity
#define HASH_KEY_EMPTY Point{INT_MAX, INT_MAX}
#define HASH_KEY_REMOVED Point{INT_MIN, INT_MIN}
#include "hash_table.h"

static bool array_contains(uint32_t *a, int len, uint32_t value) {
  for (int i = 0; i < len; i++) {
    if (a[i] == value) return true;
  }
  return false;
}

static bool array_contains(char **a, int len, char *value) {
  for (int i = 0; i < len; i++) {
    if (a[i] == value) return true;
  }
  return false;
}

static void test_hash_set_u32(int size) {
  printf("HashSet U32 test (size %d) begin.\n", size);

  auto memory = malloc(calc_hashset_memory_size(size, uint32_t));

  HashSet my_set_;
  HashSet *my_set = &my_set_;
  init_hash_set(my_set, memory, size);

  //
  // Test first insert
  //

  assert(count(my_set) == 0);
  assert(remaining(my_set) == size);
  assert(insert(my_set, 12345));
  assert(count(my_set) == 1);
  assert(remaining(my_set) == size - 1);
  assert(contains(my_set, 12345));
  assert(!contains(my_set, 0));
  assert(!contains(my_set, 3247));
  assert(!contains(my_set, 12345 % 7));

  //
  // Test second insert
  //

  assert(insert(my_set, 2342));
  assert(count(my_set) == 2);
  assert(remaining(my_set) == size - 2);
  assert(contains(my_set, 12345));
  assert(contains(my_set, 2342));
  assert(!contains(my_set, 0));
  assert(!contains(my_set, 3247));
  assert(!contains(my_set, 12345 % 7));
  assert(!contains(my_set, 2342 % 7));

  //
  // Test insertion special cases
  //
  
  assert(insert(my_set, 739901) == 1);
  assert(insert(my_set, 739901) == 2);
  assert(insert(my_set, 739901) == 2);
  assert(contains(my_set, 739901));
  assert(count(my_set) == 3);
  assert(remaining(my_set) == size - 3);

  for (int i = 0; i < 4; i++) {
    assert(insert(my_set, i) == 1);
    assert(count(my_set) == 4 + i);
  }
  assert(remaining(my_set) == size - 7);
  
  if (size == 7) assert(!insert(my_set, 7));

  //
  // Test direct iteration
  //
  
  uint32_t values[7] = {};
  int idx = 0;

  printf("Iteration : ");
  for (auto it = get_first(my_set); it; it = get_next(my_set, it)) {
    assert(it);
    assert(*it != hash_set_internal::EMPTY_U32);
    assert(*it != hash_set_internal::REMOVED_U32);
    assert(idx >= 0);
    printf("%u, ", *it);
    assert(idx < 7);
    values[idx] = *it;
    idx++;
  }
  printf("\n");
  assert(idx == 7);

  assert(array_contains(values, 7, 12345));
  assert(array_contains(values, 7, 2342));
  assert(array_contains(values, 7, 739901));
  assert(array_contains(values, 7, 0));
  assert(array_contains(values, 7, 1));
  assert(array_contains(values, 7, 2));
  assert(array_contains(values, 7, 3));

  // TODO test removing/inserting

  //
  // Test iterator
  //
  
  HashSet_Iterator iter_ = get_iterator(my_set);
  HashSet_Iterator *iter = &iter_;

  uint32_t values2[7] = {};
  idx = 0;

  while (has_next(iter)) {
    auto it = get_next(iter);
    assert(it);
    assert(*it != hash_set_internal::EMPTY_U32);
    assert(*it != hash_set_internal::REMOVED_U32);
    assert(idx >= 0);
    assert(idx < 7);
    values2[idx] = *it;
    idx++;
  }
  assert(idx == 7);

  assert(array_contains(values2, 7, 12345));
  assert(array_contains(values2, 7, 2342));
  assert(array_contains(values2, 7, 739901));
  assert(array_contains(values2, 7, 0));
  assert(array_contains(values2, 7, 1));
  assert(array_contains(values2, 7, 2));
  assert(array_contains(values2, 7, 3));

  //
  // Test remove
  //
  
  assert(remove(my_set, 12345) == true);
  assert(contains(my_set, 12345) == false);
  assert(count(my_set) == 6);
  assert(remaining(my_set) == size - 6);
  assert(remove(my_set, 739901) == true);
  assert(remove(my_set, 739901) == false);
  assert(count(my_set) == 5);
  assert(remaining(my_set) == size - 5);

  assert(insert(my_set, 2424) == 1);
  assert(count(my_set) == 6);
  assert(remaining(my_set) == size - 6);
  assert(insert(my_set, 739901) == 1);
  assert(count(my_set) == 7);
  assert(remaining(my_set) == size - 7);

  clear(my_set);
  assert(count(my_set) == 0);
  assert(remaining(my_set) == size);

  free(memory);

  printf("HashSet U32 test (size %d) successful.\n\n", size);
}

static void test_hash_set_string(int size) {
  printf("HashSet String test (size %d) begin.\n", size);

  auto memory = malloc(calc_hashset_memory_size(size, char *));

  StringSet my_set_;
  StringSet *my_set = &my_set_;
  init_hash_set(my_set, memory, size);

  char * str0 = "Hi";
  char * str1 = "World";
  char * str2 = "????";

  char * str3 = "No!";
  char * str4 = "What";

  char *strs[4] = {"Yes", "AAA", "BBB", "010101"};

  //
  // Test first insert
  //

  assert(count(my_set) == 0);
  assert(remaining(my_set) == size);
  assert(insert(my_set, str0));
  assert(count(my_set) == 1);
  assert(remaining(my_set) == size - 1);
  assert(contains(my_set, str0));
  assert(!contains(my_set, str1));
  assert(!contains(my_set, str2));
  assert(!contains(my_set, str3));

  //
  // Test second insert
  //

  assert(insert(my_set, str1));
  assert(count(my_set) == 2);
  assert(remaining(my_set) == size - 2);
  assert(contains(my_set, str0));
  assert(contains(my_set, str1));
  assert(!contains(my_set, str2));
  assert(!contains(my_set, str3));

  //
  // Test insertion special cases
  //
  
  assert(insert(my_set, str2) == 1);
  assert(insert(my_set, str2) == 2);
  assert(insert(my_set, str2) == 2);
  assert(contains(my_set, str2));
  assert(count(my_set) == 3);
  assert(remaining(my_set) == size - 3);

  for (int i = 0; i < 4; i++) {
    assert(insert(my_set, strs[i]) == 1);
    assert(count(my_set) == 4 + i);
  }
  assert(remaining(my_set) == size - 7);
  
  if (size == 7) assert(!insert(my_set, "Too full"));

  //
  // Test direct iteration
  //
  
  char *values[7] = {};
  int idx = 0;

  printf("Iteration : ");
  for (auto it = get_first(my_set); it; it = get_next(my_set, it)) {
    assert(it);
    assert(*it != hash_set_internal::EMPTY_PTR);
    assert(*it != hash_set_internal::REMOVED_PTR);
    assert(idx >= 0);
    printf("%s, ", *it);
    assert(idx < 7);
    values[idx] = *it;
    idx++;
  }
  printf("\n");
  assert(idx == 7);

  assert(array_contains(values, 7, str0));
  assert(array_contains(values, 7, str1));
  assert(array_contains(values, 7, str2));
  assert(array_contains(values, 7, strs[0]));
  assert(array_contains(values, 7, strs[1]));
  assert(array_contains(values, 7, strs[2]));
  assert(array_contains(values, 7, strs[3]));

  // TODO test removing/inserting

  //
  // Test iterator
  //
  
  StringSet_Iterator iter_ = get_iterator(my_set);
  StringSet_Iterator *iter = &iter_;

  char *values2[7] = {};
  idx = 0;

  while (has_next(iter)) {
    auto it = get_next(iter);
    assert(it);
    assert(*it != hash_set_internal::EMPTY_PTR);
    assert(*it != hash_set_internal::REMOVED_PTR);
    assert(idx >= 0);
    assert(idx < 7);
    values2[idx] = *it;
    idx++;
  }
  assert(idx == 7);

  assert(array_contains(values, 7, str0));
  assert(array_contains(values, 7, str1));
  assert(array_contains(values, 7, str2));
  assert(array_contains(values, 7, strs[0]));
  assert(array_contains(values, 7, strs[1]));
  assert(array_contains(values, 7, strs[2]));
  assert(array_contains(values, 7, strs[3]));

  //
  // Test remove
  //
  
  assert(remove(my_set, str0) == true);
  assert(contains(my_set, str0) == false);
  assert(count(my_set) == 6);
  assert(remaining(my_set) == size - 6);
  assert(remove(my_set, str2) == true);
  assert(remove(my_set, str2) == false);
  assert(count(my_set) == 5);
  assert(remaining(my_set) == size - 5);

  assert(insert(my_set, str4) == 1);
  assert(count(my_set) == 6);
  assert(remaining(my_set) == size - 6);
  assert(insert(my_set, str2) == 1);
  assert(count(my_set) == 7);
  assert(remaining(my_set) == size - 7);

  clear(my_set);
  assert(count(my_set) == 0);
  assert(remaining(my_set) == size);

  free(memory);

  printf("HashSet String test (size %d) successful.\n\n", size);
}

static void test_hash_set_s32(int size) {
  printf("HashSet S32 test (size %d) begin.\n", size);

  auto memory = malloc(calc_hashset_memory_size(size, int32_t));

  HashSet_S32 my_set_;
  HashSet_S32 *my_set = &my_set_;
  init_hash_set(my_set, memory, size);

  //
  // Test first insert
  //

  assert(count(my_set) == 0);
  assert(remaining(my_set) == size);
  assert(insert(my_set, -12345));
  assert(count(my_set) == 1);
  assert(remaining(my_set) == size - 1);
  assert(contains(my_set, -12345));
  assert(!contains(my_set, 0));
  assert(!contains(my_set, 3247));
  assert(!contains(my_set, 12345 % 7));

  //
  // Test second insert
  //

  assert(insert(my_set, 2342));
  assert(count(my_set) == 2);
  assert(remaining(my_set) == size - 2);
  assert(contains(my_set, -12345));
  assert(contains(my_set, 2342));
  assert(!contains(my_set, 0));
  assert(!contains(my_set, 3247));
  assert(!contains(my_set, 12345 % 7));
  assert(!contains(my_set, 2342 % 7));

  //
  // Test insertion special cases
  //
  
  assert(insert(my_set, 739901) == 1);
  assert(insert(my_set, 739901) == 2);
  assert(insert(my_set, 739901) == 2);
  assert(contains(my_set, 739901));
  assert(count(my_set) == 3);
  assert(remaining(my_set) == size - 3);

  for (int i = 0; i < 4; i++) {
    assert(insert(my_set, i) == 1);
    assert(count(my_set) == 4 + i);
  }
  assert(remaining(my_set) == size - 7);
  
  if (size == 7) assert(!insert(my_set, 7));

  //
  // Test direct iteration
  //
  
  uint32_t values[7] = {};
  int idx = 0;

  printf("Iteration : ");
  for (auto it = get_first(my_set); it; it = get_next(my_set, it)) {
    assert(it);
    assert(*it != hash_set_internal::EMPTY_U32);
    assert(*it != hash_set_internal::REMOVED_U32);
    assert(idx >= 0);
    printf("%d, ", *it);
    assert(idx < 7);
    values[idx] = *it;
    idx++;
  }
  printf("\n");
  assert(idx == 7);

  assert(array_contains(values, 7, -12345));
  assert(array_contains(values, 7, 2342));
  assert(array_contains(values, 7, 739901));
  assert(array_contains(values, 7, 0));
  assert(array_contains(values, 7, 1));
  assert(array_contains(values, 7, 2));
  assert(array_contains(values, 7, 3));

  // TODO test removing/inserting

  //
  // Test iterator
  //
  
  HashSet_S32_Iterator iter_ = get_iterator(my_set);
  HashSet_S32_Iterator *iter = &iter_;

  uint32_t values2[7] = {};
  idx = 0;

  while (has_next(iter)) {
    auto it = get_next(iter);
    assert(it);
    assert(*it != hash_set_internal::EMPTY_U32);
    assert(*it != hash_set_internal::REMOVED_U32);
    assert(idx >= 0);
    assert(idx < 7);
    values2[idx] = *it;
    idx++;
  }
  assert(idx == 7);

  assert(array_contains(values2, 7, -12345));
  assert(array_contains(values2, 7, 2342));
  assert(array_contains(values2, 7, 739901));
  assert(array_contains(values2, 7, 0));
  assert(array_contains(values2, 7, 1));
  assert(array_contains(values2, 7, 2));
  assert(array_contains(values2, 7, 3));

  //
  // Test remove
  //
  
  assert(remove(my_set, -12345) == true);
  assert(contains(my_set, -12345) == false);
  assert(count(my_set) == 6);
  assert(remaining(my_set) == size - 6);
  assert(remove(my_set, 739901) == true);
  assert(remove(my_set, 739901) == false);
  assert(count(my_set) == 5);
  assert(remaining(my_set) == size - 5);

  assert(insert(my_set, 2424) == 1);
  assert(count(my_set) == 6);
  assert(remaining(my_set) == size - 6);
  assert(insert(my_set, 739901) == 1);
  assert(count(my_set) == 7);
  assert(remaining(my_set) == size - 7);

  clear(my_set);
  assert(count(my_set) == 0);
  assert(remaining(my_set) == size);

  free(memory);

  printf("HashSet S32 test (size %d) successful.\n\n", size);
}

static void test_hash_table() {
  printf("HashTable test begin.\n");


  int const count = 16;
  int const set_length = 29;
  PushAllocator allocator_ = new_push_allocator(1024);
  PushAllocator *allocator = &allocator_;
  assert(is_initialized(allocator));

  HashTable my_table_;
  HashTable *my_table = &my_table_;
  bool success = init_hash_table(my_table, allocator, count, set_length);
  assert(success);
  assert(my_table->count == 0);
  hash_table_internal::assert_initialized(my_table);

  auto value = get(my_table, Point{1, 3});
  assert(!value);
  hash_table_internal::assert_initialized(my_table); 

  value = insert(my_table, Point{1,3});
  auto first = value;
  assert(value);
  assert(my_table->count == 1);
  assert(value->name == NULL && value->x == 0 && value->y == 0);
  *value = {"First one", 1, 3};
  hash_table_internal::assert_initialized(my_table); 

  value = get(my_table, Point{1, 3});
  assert(value);
  assert(value == first);
  assert(my_table->count == 1);
  assert(value->x == 1 && value->y == 3);

  value = get(my_table, Point{1, 1});
  assert(!value);

  value = insert(my_table, Point{2,4});
  auto second = value;
  assert(value);
  assert(value != first);
  assert(my_table->count == 2);
  assert(value->name == NULL && value->x == 0 && value->y == 0);
  *value = {"Second", 2, 4};

  value = get(my_table, Point{1, 3});
  assert(value);
  assert(value == first);
  assert(my_table->count == 2);
  assert(value->x == 1 && value->y == 3);

  value = get(my_table, Point{2, 4});
  assert(value);
  assert(value == second);
  assert(my_table->count == 2);
  assert(value->x == 2 && value->y == 4);

  value = get(my_table, Point{1, 1});
  assert(!value);

  success = remove(my_table, Point{1, 3});
  assert(success);
  assert(my_table->count == 1);

  value = get(my_table, Point{1, 3});
  assert(!value);

  success = remove(my_table, Point{1, 1});
  assert(!success);

  success = remove(my_table, Point{1, 3});
  assert(!success);

  value = get(my_table, Point{2, 4});
  assert(value);
  assert(value == second);

  // TODO test filling up the table
  // TODO test iteration

  printf("HashTable test successful.\n\n");
}

int main() {
  test_hash_set_u32(7);
  test_hash_set_u32(11);
  test_hash_set_u32(101);

  test_hash_set_s32(7);
  test_hash_set_s32(17);
  test_hash_set_s32(197);

  test_hash_set_string(7);
  test_hash_set_string(13);
  test_hash_set_string(199);

  test_hash_table();

  printf("All tests successful.\n");
  return 0;
}

