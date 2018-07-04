
#ifndef _HEAP_SORT_H_
#define _HEAP_SORT_H_

// NOTE : This was initially copied from chess.cpp.

// TODO INCOMPLETE : This sorts arrays backwards. I don't really care to fix it right now.

template <typename T> static void heapify(T *arr, uint32_t count, uint32_t start_idx);
template <typename T> static void heap_sort(T *arr, uint32_t count);
template <typename T> static bool is_heap(T* arr, uint32_t count, uint32_t start_idx);
template <typename T> static bool is_sorted(T *arr, uint32_t count);
template <typename T> static bool is_reverse_sorted(T *arr, uint32_t count);

#endif

#ifdef HEAP_SORT_IMPLEMENTATION


// TODO make an overlaod that doesn't take a start_idx that makes the whole thing a heap (without assumptions)?
template <typename T> 
static void heapify(T *arr, uint32_t count, uint32_t start_idx) {

  uint32_t min_idx = start_idx;
  uint32_t l = 2*start_idx + 1;
  uint32_t r = 2*start_idx + 2;

  // If left child is larger than root
  if (l < count) {
    if (CMP_LT(arr + l, arr + min_idx)) min_idx = l;
  }

  // If right child is larger than largest so far
  if (r < count) {
    if (CMP_LT(arr + r, arr + min_idx)) min_idx = r;
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
    if (CMP_LT(arr + l, arr + start_idx)) return false;
    if (!is_heap(arr, count, l)) return false;
  }

  // If right child is larger than largest so far
  if (r < count) {
    if (CMP_LT(arr + r, arr + start_idx)) return false;
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
    if (CMP_LT(arr + i, arr + i + 1)) return false;
  }
  return true;
}

template <typename T>
static bool is_sorted(T *arr, uint32_t count) {
  for (uint32_t i = 0; i < count - 1; i++) {
    if (CMP_GT(arr + i, arr + i + 1)) return false;
  }
  return true;
}

#endif

