
#ifndef _DYNAMIC_ARRAY_H_
#define _DYNAMIC_ARRAY_H_
// TODO test this

struct darray_head {
  uint32_t count;
  uint32_t max_count;
};

template <typename T>
struct darray {
  T *p;
  inline darray() { p = NULL; }
  inline darray(T* ptr) { p = ptr; }
  inline operator T*() { return p; }
};

template <typename T>
inline darray_head *header(darray<T> arr) {
  if (!arr) return NULL;
  auto head = (darray_head *) arr.p;
  return head - 1; 
}

template <typename T>
inline uint32_t count(darray<T> arr) {
  if (!arr) return 0;
  return header(arr)->count;
}

template <typename T>
inline uint32_t total_size(darray<T> arr, uint32_t count) {
  return sizeof(T) * count + sizeof(darray_head);
}

template <typename T>
inline bool initialize(darray<T> &arr, uint32_t max_count = 10) {
  assert(!arr); // TODO possibly remove this?
  if (!max_count) return true;
  auto head = (darray_head *) malloc(total_size(arr, max_count));
  if (!head) return false;
  head->count = 0;
  head->max_count = max_count;
  arr = (T *)(head + 1);
  return true;
}

template <typename T>
inline bool expand(darray<T> &arr, uint32_t amount) {
  if (!amount) return true;
  if (!arr) return initialize(arr, amount);
  auto head = header(arr);

  head = (darray_head *) realloc(head, total_size(arr, head->max_count + amount));
  assert(head);
  if (!head) return false;
  head->max_count += amount;
  arr = (T *)(head + 1);
  return true;
}

template <typename T>
inline bool expand(darray<T> &arr) {
  if (!arr) return initialize(arr);
  auto head = header(arr);
  return expand(arr, head->max_count);
}

template <typename T>
inline T *push(darray<T> &arr) {
  if (!arr) initialize(arr);
  if (!arr) return NULL;
  auto head = header(arr);

  assert(head->count <= head->max_count);
  // TODO consider expanding by 1 on failure?
  if (head->count == head->max_count)
    if (!expand(arr)) return NULL;

  head = header(arr);
  head->count++;
  return arr + head->count - 1;
}

template <typename T>
inline T *push(darray<T> &arr, T entry) {
  if (!arr) initialize(arr);
  if (!arr) return NULL;
  auto head = header(arr);

  assert(head->count <= head->max_count);
  // TODO consider expanding by 1 on failure?
  if (head->count == head->max_count)
    if (!expand(arr)) return NULL;

  head = header(arr);
  arr.p[head->count] = entry;
  head->count++;
  return arr + head->count;
}

template <typename T>
inline void dfree(darray<T> &arr) {
  assert(arr);
  if (!arr) return;

  auto head = header(arr);
  free((void *)head);
  arr = NULL;
}

template <typename T>
inline void clear(darray<T> &arr) {
  if (!arr) return;
  auto head = header(arr);
  head->count = 0;
}

template <typename T>
inline T pop(darray<T> &arr) {
  assert(arr);
  auto head = header(arr);
  assert(head->count);
  head->count--;
  return arr[head->count];
}

template <typename T>
inline T *peek(darray<T> &arr) {
  auto c = count(arr);
  if (!c) return NULL;
  return arr + c - 1;
}

// TODO move this to the test file
static void dynamic_array_test() {
  printf("Dynamic array test begin.\n");

  darray<int> ints;
  assert(!ints);
  assert(ints.p == NULL);
  assert(!count(ints));
  assert(!header(ints));
  ints = NULL;
  assert(ints.p == NULL);

  initialize(ints);
  assert(ints);
  assert(ints.p);
  assert(count(ints) == 0);
  assert(push(ints, 2));
  assert(count(ints) == 1);
  assert(ints.p[0] == 2);
  assert(ints[0] == 2);
  assert(ints.p + 1 == ints + 1);
  assert(ints.p + 1 == ints + 1);

  assert(push(ints, 3));
  assert(ints.p[1] == 3);
  assert(count(ints) == 2);

  int *arr = ints;
  assert(arr == ints.p);

  for (int i = 0; i < 300; i++) {
    assert(push(ints, i));
    assert(ints);
    assert(count(ints) == (i + 3));
  }

  dfree(ints);
  assert(!ints);

  assert(push(ints, 37));
  assert(ints[0] == 37);
  assert(count(ints) == 1);

  clear(ints);
  assert(!count(ints));
  assert(push(ints, 4));
  assert(count(ints) == 1);
  assert(ints[0] == 4);
  auto x = push(ints);
  assert(x);
  assert(count(ints) == 2);
  *x = 1;
  assert(ints[0] == 4);
  assert(ints + 1 == x);
  assert(ints[1] == (*x));
  
  int y = pop(ints);
  assert(y == 1);
  assert(count(ints) == 1);
  assert(push(ints, 5));
  assert(count(ints) == 2);
  assert(ints[1] == 5);

  printf("Dynamic array test successful.\n\n");
}

#endif

