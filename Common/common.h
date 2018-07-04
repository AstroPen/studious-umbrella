
#ifndef _COMMON_H_
#define _COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stdalign.h>
#include <cstring>
#include <cmath>
#include <climits>

union bit32 {
  int i;
  uint32_t u;

  inline bit32(int _i) { i = _i; }
  inline bit32(uint32_t _u) { u = _u; }
  
  inline operator int() { return i; }
  inline operator uint32_t() { return u; }
};

// NOTE : this does not work if val is a rvalue
#define bit_cast(type, val) (*((type *)(&(val))))
#define count_of(array) (sizeof(array)/sizeof(*(array)))
#define SWAP(a, b) { auto temp = *(a); *(a) = *(b); *(b) = temp; }

// TODO consider static check that T is not a pointer (to reduce errors)
template <typename T> inline int compare(T *a, T *b) {
  if (*a > *b) return 1;
  if (*a == *b) return 0;
  return -1;
}

#define CMP_GT(a, b) (compare(a, b) > 0)
#define CMP_EQ(a, b) (compare(a, b) == 0)
#define CMP_LT(a, b) (compare(a, b) < 0)

template <typename T, uint32_t N>
struct Array {
  T e[N];
  inline operator T*() { return e; }
};

#include "scalar_math.h"
#include "push_allocator.h"
#endif


