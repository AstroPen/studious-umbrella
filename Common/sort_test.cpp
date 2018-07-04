
#include "common.h"

void print_ints(int *arr, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    printf("%d, ", arr[i]);
  }
  printf("\n");
}

#define HEAP_SORT_IMPLEMENTATION
#include "heap_sort.h"
#define QUICK_SORT_IMPLEMENTATION
#include "quick_sort.h"
#include "dynamic_array.h"


#define SORT_TEST(sort_name, array, count) \
  sort_name(array, count); \
  print_ints(array, count); \
  assert(is_sorted(array, count));

#define SORT_TEST_LARGE(sort_name, array, count) \
  sort_name(array, count); \
  printf("%d, %d, ... , %d, %d\n", array[0], array[1], array[count - 2], array[count - 1]); \
  assert(is_sorted(array, count));

#define PIVOT_TEST(array, count, pivot) \
  assert(*(get_pivot(array, count)) == pivot);

// TODO Finish testing thoroughly

void test_heap_sort() {
  printf("Heap sort test begin.\n");
  int ints[] = { 3, 7, -1, 8, 4, 10, 1, 3, 100, -100};
  uint32_t count = count_of(ints);
  assert(!is_sorted(ints, count));
  heap_sort(ints, count);
  print_ints(ints, count);
  // TODO It sorts things backwards, I'll fix later. Maybe.
  assert(is_reverse_sorted(ints, count));
  printf("Heap sort test successful.\n\n");
}

void test_insertion_sort() {
  printf("Insertion sort test begin.\n");
  int ints1[] = { 3, 7, -1, 8, 4, 10, 1, 3, 100, -100};
  assert(!is_sorted(ints1, count_of(ints1)));
  SORT_TEST(insertion_sort, ints1, count_of(ints1));

  int ints2[] = {1,1,1};
  SORT_TEST(insertion_sort, ints2, count_of(ints2));
  printf("Insertion sort test successful.\n\n");
}

// TODO consider testing the partition separately
void test_quick_sort() {
  printf("Quick sort test begin.\n");
  int ints1[] = { 3, 7, -1, 8, 4, 10, 1, 3, 100, -100};

  assert(!is_sorted(ints1, count_of(ints1)));
  PIVOT_TEST(ints1, count_of(ints1), 3);
  SORT_TEST(quick_sort, ints1, count_of(ints1));

  int ints2[] = {1,1,1};
  PIVOT_TEST(ints2, count_of(ints2), 1);
  SORT_TEST(quick_sort, ints2, count_of(ints2));

  int ints3[] = { 5, 2, 87, 6, -20, 400, -1, 5, 30, 32, 2, 103, -25, -1, 5, 1 };
  assert(count_of(ints3) > 10); 
  PIVOT_TEST(ints3, count_of(ints3), 5);
  SORT_TEST(quick_sort, ints3, count_of(ints3));

  int pivots[] = { 1, 2, 3, 1, 2, 1, 0 };
  PIVOT_TEST(pivots, 3, 2);
  PIVOT_TEST(pivots + 1, 3, 2);
  PIVOT_TEST(pivots + 2, 3, 2);
  PIVOT_TEST(pivots + 3, 3, 1);
  PIVOT_TEST(pivots + 4, 3, 1);
  

  darray<int> ints4;
  for (int i = 0; i < 2046; i++) {
    push(ints4, rand());
  }
  assert(ints4);
  SORT_TEST_LARGE(quick_sort, ints4.p, count(ints4));

  printf("Quick sort test successful.\n\n");
}

int main() {
  srand(1234);
  test_heap_sort();
  test_insertion_sort();
  test_quick_sort();
  return EXIT_SUCCESS;
}

