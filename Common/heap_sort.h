
#ifndef _HEAP_SORT_H_
#define _HEAP_SORT_H_

// NOTE : This was initially copied from chess.cpp.

// TODO INCOMPLETE : This sorts arrays backwards. I don't really care to fix it right now.

// TODO Separate these out into header/cpp so that types don't need to be defined before this
template <typename T> static void heapify(T *arr, uint32_t count, uint32_t start_idx);
template <typename T> static void heap_sort(T *arr, uint32_t count);
template <typename T> static bool is_heap(T* arr, uint32_t count, uint32_t start_idx);
template <typename T> static bool is_sorted(T *arr, uint32_t count);

// TODO I think compare might actually deserve to go in common.h...
template <typename T> inline int compare(T *a, T *b) {
  if (*a > *b) return 1;
  if (*a == *b) return 0;
  return -1;
}

// TODO switch to using this :
#define CMP_GT(a, b) (compare(a, b) > 0)
#define CMP_EQ(a, b) (compare(a, b) == 0)
#define CMP_LT(a, b) (compare(a, b) < 0)

// TODO make an overlaod that doesn't take a start_idx that makes the whole thing a heap (without assumptions)?
template <typename T> 
static void heapify(T *arr, uint32_t count, uint32_t start_idx) {

  uint32_t min_idx = start_idx;
  uint32_t l = 2*start_idx + 1;
  uint32_t r = 2*start_idx + 2;

  // If left child is larger than root
  if (l < count) {
    if (compare(arr + l, arr + min_idx) < 0) min_idx = l;
  }

  // If right child is larger than largest so far
  if (r < count) {
    if (compare(arr + r, arr + min_idx) < 0) min_idx = r;
  }

  // If largest is not root
  if (min_idx != start_idx) {

    SWAP(arr + start_idx, arr + min_idx);
    /*
    T temp = arr[start_idx];
    arr[start_idx] = arr[min_idx];
    arr[min_idx] = temp;
    */

    // Recursively heapify the affected sub-tree
    heapify(arr, count, min_idx);
  }
}

// TODO move this to a namespace
template <typename T>
static bool is_heap(T *arr, uint32_t count, uint32_t start_idx) {
  int l = 2*start_idx + 1;
  int r = 2*start_idx + 2;

  assert(start_idx < count);

  if (l < count) {
    if (compare(arr + l, arr + start_idx) < 0) return false;
    if (!is_heap(arr, count, l)) return false;
  }

  // If right child is larger than largest so far
  if (r < count) {
    if (compare(arr + r, arr + start_idx) < 0) return false;
    if (!is_heap(arr, count, r)) return false;
  }

  return true; 
}

template <typename T>
static void heap_sort(T *arr, uint32_t count) {
  if (!count) return;

  for (int i = count / 2 - 1; i >= 0; i--) {
    heapify(arr, count, i);
  }

  // TODO comment this out, make a test instead
  assert(is_heap(arr, count, 0));

  // One by one extract an element from heap
  for (uint32_t i = count - 1; i > 0; i--) {
    SWAP(arr, arr + i); // TODO make "pop min" function or something?
    heapify(arr, i, 0);
  }
}

template <typename T>
static bool is_reverse_sorted(T *arr, uint32_t count) {
  for (uint32_t i = 0; i < count - 1; i++) {
    if (compare(arr + i, arr + i + 1) < 0) return false;
  }
  return true;
}

template <typename T>
static bool is_sorted(T *arr, uint32_t count) {
  for (uint32_t i = 0; i < count - 1; i++) {
    if (compare(arr + i, arr + i + 1) > 0) return false;
  }
  return true;
}

#endif

