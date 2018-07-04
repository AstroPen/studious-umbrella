
#ifndef _QUICK_SORT_H_
#define _QUICK_SORT_H_

// TODO some of these might want to be in a namespace
template <typename T> static T *get_pivot(T *arr, uint32_t count);
template <typename T> static void insertion_sort(T *arr, uint32_t count);
template <typename T> static Array<uint32_t,2> partition(T *arr, uint32_t count, T *pivot_ptr);
template <typename T> static void quick_sort(T *arr, uint32_t count);

#endif

#ifdef QUICK_SORT_IMPLEMENTATION

// TODO simplify this
template <typename T>
static T *get_pivot(T *arr, uint32_t count) {
  assert(count);
  T *first = arr;
  T *middle = arr + count / 2;
  T *last = arr + count - 1;

  T e1 = *first;
  T e2 = *middle;
  T e3 = *last;
  // TODO switch to using compare
  if (e1 == e2) return first;
  if (e1 == e3) return first;
  if (e2 == e3) return last;

  if (e1 > e2) {
    if (e1 < e3) return first;
    // else : e3 < e1 > e2
    if (e2 > e3) return middle;
    // else : e1 > e3 > e2
    return last;
  }
  // else : e2 > e1
  if (e2 < e3) return middle;
  // else : e1 < e2 > e3
  if (e1 > e3) return first;
  // else : e2 > e3 > e1
  return last;
}

template <typename T>
void insertion_sort(T *arr, uint32_t count) {

  for (uint32_t i = 1; i < count; i++) {
    T temp = arr[i];
    uint32_t j;
    for (j = i; j > 0 && CMP_GT(arr + j - 1, &temp); j--) {
      arr[j] = arr[j-1];
    }
    arr[j] = temp;
  }
}

template <typename T>
static Array<uint32_t, 2> partition(T *arr, uint32_t count, T *pivot_ptr) {

  uint32_t lt = 0, i = 0, gt = count - 1;
  T pivot = *pivot_ptr;

  //   ---------> ------->          <------------ 
  // [......... lt ..... i ....... gt ............]
  //  less than    equal   unknown    greater than
  while (i <= gt) {
    if (arr[i] == pivot) i++;
    else if (arr[i] < pivot) {
      if (lt != i) SWAP(arr + lt, arr + i);
      lt++, i++;
    } else {
      if (gt != i) SWAP(arr + i, arr + gt);
      gt--;
    }
  }

  return {lt, gt};
}

template <typename T>
static void quick_sort(T *arr, uint32_t count) {
  if (count <= 10) {
    insertion_sort(arr, count);
    return;
  }

  T *pivot = get_pivot(arr, count);

  //uint32_t idx = partition(arr, count, pivot);
  Array<uint32_t, 2> idxs = partition(arr, count, pivot);
  uint32_t lt = idxs[0];
  uint32_t gt = idxs[1];

  quick_sort(arr, lt);
  quick_sort(arr + gt, count - gt);
}

#endif

