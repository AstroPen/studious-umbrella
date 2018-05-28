
// TODO generalize these tests and randomize the inputs

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stdalign.h>
#include <cstring>
#include <cmath>
#include <climits>

#define HASH_SET_KEYTYPE_U32
//#define HASH_SET_TYPE_NAME HashSet
#include "hash_set.h"

#define HASH_SET_KEYTYPE_S32
#define HASH_SET_TYPE_NAME HashSet_S32
#include "hash_set.h"

#define HASH_SET_KEYTYPE_PTR char
#define HASH_SET_TYPE_NAME StringSet
#include "hash_set.h"

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

  /*
  int const size = 7;
  uint32_t memory[size];

  HashTable my_table_ = hash_table(memory, size);
  HashTable *my_table = &my_table_;
  insert(my_table, 
  */


  printf("HashTable test successful.\n");
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

  printf("All tests successful.\n");
  return 0;
}

