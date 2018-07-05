

#ifndef _QUICK_SORT_H_
#define _QUICK_SORT_H_
#define MY_TEMPLATE template <typename T, int (*C)(T*,T*) = compare<T>>

// TODO some of these might want to be in a namespace
MY_TEMPLATE static T *get_pivot(T *arr, uint32_t count);
MY_TEMPLATE static void insertion_sort(T *arr, uint32_t count);
MY_TEMPLATE static Array<uint32_t,2> partition(T *arr, uint32_t count, T *pivot_ptr);
MY_TEMPLATE static void quick_sort(T *arr, uint32_t count);

#undef MY_TEMPLATE
#endif

#ifdef QUICK_SORT_IMPLEMENTATION
#define MY_TEMPLATE template <typename T, int (*C)(T*,T*)>

// TODO simplify this
MY_TEMPLATE static T *get_pivot(T *arr, uint32_t count) {
  assert(count);
  T *first = arr;
  T *middle = arr + count / 2;
  T *last = arr + count - 1;

  int cmp12 = C(first, middle);
  if (cmp12 == 0) return first;

  int cmp13 = C(first, last);
  if (cmp13 == 0) return first;

  int cmp23 = C(middle, last);
  if (cmp23 == 0) return last;

  if (cmp12 > 0) {
    if (cmp13 < 0) return first;
    // else : e3 < e1 > e2
    if (cmp23 > 0) return middle;
    // else : e1 > e3 > e2
    return last;
  }
  // else : e2 > e1
  if (cmp23 < 0) return middle;
  // else : e1 < e2 > e3
  if (cmp13 > 0) return first;
  // else : e2 > e3 > e1
  return last;
}

MY_TEMPLATE static void insertion_sort(T *arr, uint32_t count) {

  for (uint32_t i = 1; i < count; i++) {
    T temp = arr[i];
    uint32_t j;
    for (j = i; j > 0 && C(arr + j - 1, &temp) > 0; j--) {
      arr[j] = arr[j-1];
    }
    arr[j] = temp;
  }
}

MY_TEMPLATE static Array<uint32_t, 2> partition(T *arr, uint32_t count, T *pivot_ptr) {

  uint32_t lt = 0, i = 0, gt = count - 1;
  T pivot = *pivot_ptr;

  //   ---------> ------->          <------------ 
  // [......... lt ..... i ....... gt ............]
  //  less than    equal   unknown    greater than
  while (i <= gt) {
    int cmp = C(arr + i, &pivot);

    if (cmp == 0) i++;
    else if (cmp < 0) {
      if (lt != i) SWAP(arr + lt, arr + i);
      lt++, i++;
    } else {
      if (gt != i) SWAP(arr + i, arr + gt);
      gt--;
    }
  }

  return {lt, gt};
}

MY_TEMPLATE static void quick_sort(T *arr, uint32_t count) {
  if (count <= 10) {
    insertion_sort<T,C>(arr, count);
    return;
  }

  T *pivot = get_pivot<T,C>(arr, count);

  //uint32_t idx = partition(arr, count, pivot);
  Array<uint32_t, 2> idxs = partition<T,C>(arr, count, pivot);
  uint32_t lt = idxs[0];
  uint32_t gt = idxs[1];

  quick_sort<T,C>(arr, lt);
  quick_sort<T,C>(arr + gt, count - gt);
}

#undef MY_TEMPLATE
#endif

