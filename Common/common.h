
#ifndef _COMMON_H_
#define _COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stdalign.h>
#include <cstring>
#include <cmath>
#include <climits>

typedef double float64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


// Needed for stack trace, not available for all compilers :
#include "execinfo.h"

#define UNUSED(x) do { (void)sizeof(x); } while(0)

#define DEFAULT_HALT do { abort(); } while(0)
#define DEFAULT_ASSERT_LOG stderr
#define DEFAULT_ASSERT_MAX_STACK_TRACE 10
#define ASSERT_MAX_STACK_TRACE() DEFAULT_ASSERT_MAX_STACK_TRACE
#define HALT() DEFAULT_HALT
#define ASSERT_LOG() DEFAULT_ASSERT_LOG
#define HALT_ON_ASSERT true
#define PRINT_STACK_TRACE_ON_ASSERT true

// Idea : make an assert_print that takes an lstring as a log
namespace Assert {
  inline void assert_print(FILE *log, const char *condition, const char *function, const char *file, int line) {
    fprintf(log, "Assertion failed : (%s), function %s, file %s, line %d.\n", 
        condition, function, file, line);
  }

// TODO print stack trace using backtrace() from execinfo.h
// https://stackoverflow.com/questions/77005/how-to-automatically-generate-a-stacktrace-when-my-gcc-c-program-crashes
  static void print_stack_trace(FILE *log, int max_count) {
    void *stack_array[max_count];
    size_t count = backtrace(stack_array, max_count);
    if (count) {
      char **symbols = backtrace_symbols(stack_array, count);
      if (symbols) {
        fprintf(log, "Stack backtrace :\n");
        for (u32 i = 0; i < count; i++) {
          fprintf(log, "%s\n", symbols[i]);
        }
      } else {
        fprintf(log, "Error : Unable to allocate memory for stacktrace.\n");
      }
    }
  }
}

// TODO test this fully to make sure it works before putting it in everything
//
// NOTE : Based on this article :
// http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/

// TODO Make an ASSERT_EXPR or something that can be put in if statements and such

#ifndef NDEBUG
  #define ASSERT(cond) \
    do { \
      if (!(cond)) { \
        Assert::assert_print(ASSERT_LOG(), #cond, __FUNCTION__, __FILE__, __LINE__); \
        if (PRINT_STACK_TRACE_ON_ASSERT) Assert::print_stack_trace(ASSERT_LOG(), ASSERT_MAX_STACK_TRACE()); \
        if (HALT_ON_ASSERT) HALT(); \
      } \
    } while(0)  
#else  
  #define ASSERT(cond) UNUSED(cond)
#endif

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

#define PRINT_INT(variable_name) printf(#variable_name " : %d\n", (variable_name));
#define PRINT_UINT(variable_name) printf(#variable_name " : %u\n", (variable_name));
#define PRINT_FLOAT(variable_name) printf(#variable_name " : %f\n", (variable_name));

#include "scalar_math.h"
//#include "push_allocator.h"
#endif


