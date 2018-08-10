
#ifndef _COMMON_H_
#define _COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stdalign.h>
#include <cstring>
#include <cmath>
#include <climits>

typedef int64_t int64;
typedef double float64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

union bit32 {
  int i;
  uint32_t u;
  float f;

  inline bit32(int _i) { i = _i; }
  inline bit32(uint32_t _u) { u = _u; }
  inline bit32(float _f) { f= _f; }
  
  inline operator int() { return i; }
  inline operator uint32_t() { return u; }
  inline operator float() { return f; }
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

inline void mem_copy(void const *source, void *dest, uint32_t len) {
  auto s = (uint8_t *) source;
  auto d = (uint8_t *) dest;

  for (uint32_t i = 0; i < len; i++) {
    d[i] = s[i];
  }
}

template <typename T>
inline void array_copy(T const *source, T *dest, u32 len) {
  mem_copy(source, dest, len * sizeof(T));
}

#define PRINT_INT(variable_name) printf(#variable_name " : %d\n", (variable_name))
#define PRINT_UINT(variable_name) printf(#variable_name " : %u\n", (variable_name))
#define PRINT_FLOAT(variable_name) printf(#variable_name " : %f\n", (variable_name))

#define FLAG_SET(mask, flag) do { (mask) |= (flag); } while(0)
#define FLAG_UNSET(mask, flag) do { (mask) &= ~(flag); } while(0)
#define FLAG_TOGGLE(mask, flag) do { (mask) ^= (flag); } while(0)
#define FLAG_TEST(mask, flag) ((mask) & (flag))

#define SHIFT_SET(mask, flag) do { (mask) |= 1UL << (flag); } while(0)
#define SHIFT_UNSET(mask, flag) do { (mask) &= ~(1UL << (flag)); } while(0)
#define SHIFT_TOGGLE(mask, flag) do { (mask) ^= 1UL << (flag); } while(0)
#define SHIFT_TEST(mask, flag) ((mask) & (1UL << (flag)))

#include "scalar_math.h"
//#include "push_allocator.h"
#include "custom_assert.h"

#endif


