
#ifndef _CUSTOM_ASSERT_H_
#define _CUSTOM_ASSERT_H_

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

// Idea : make an print_failure that takes an lstring as a log
namespace Assert {

  inline void print_variable(FILE *log, const char *variable_name, int variable) {
    fprintf(log, "%s : %d\n", variable_name, variable);
  }

  inline void print_variable(FILE *log, const char *variable_name, int64 variable) {
    fprintf(log, "%s : %lld\n", variable_name, variable);
  }

  inline void print_variable(FILE *log, const char *variable_name, u32 variable) {
    fprintf(log, "%s : %u\n", variable_name, variable);
  }

  inline void print_variable(FILE *log, const char *variable_name, u64 variable) {
    fprintf(log, "%s : %llu\n", variable_name, variable);
  }

  inline void print_variable(FILE *log, const char *variable_name, float variable) {
    fprintf(log, "%s : %f\n", variable_name, variable);
  }

  inline void print_variable(FILE *log, const char *variable_name, float64 variable) {
    fprintf(log, "%s : %f\n", variable_name, variable);
  }

  template <typename... Types>
  static u32 count_variables(Types... args) {
    return sizeof...(Types);
  }

// TODO print stack trace using backtrace() from execinfo.h
// https://stackoverflow.com/questions/77005/how-to-automatically-generate-a-stacktrace-when-my-gcc-c-program-crashes
  static void print_stack_trace(FILE *log, int max_count) {
    assert(log); assert(max_count < 2048);
    void *stack_array[max_count];
    size_t count = backtrace(stack_array, max_count);
    if (count) {
      char **symbols = backtrace_symbols(stack_array, count);
      if (symbols) {
        fprintf(log, "Stack backtrace :\n");
        for (u32 i = 0; i < count; i++) {
          assert(symbols[i]);
          fprintf(log, "%s\n", symbols[i]);
        }
        free(symbols);
      } else {
        fprintf(log, "Error : Unable to allocate memory for stacktrace.\n");
      }
    }
  }

  template <typename T>
  inline T verify_print(T result, FILE *log, bool do_halt, bool do_stack_trace, int max_count, const char *condition, const char *function, const char *file, int line) {
    assert(log); assert(condition); assert(function); assert(file);
    if (!result) {
      fprintf(log, "Verify expression failed : (%s), function %s, file %s, line %d.\n", 
          condition, function, file, line);
      if (do_stack_trace) print_stack_trace(log, max_count);
      if (do_halt) abort(); // TODO I can't set HALT this way... Maybe use a lambda afterall?
    }

    return result;
  }

  static void print_failure(FILE *log, const char *message, const char *function, const char *file, int line) {
    assert(log); assert(message); assert(function); assert(file);
    fprintf(log, "Assertion failed : (%s), function %s, file %s, line %d.\n", 
        message, function, file, line);
  }
}

// TODO test this fully to make sure it works before putting it in everything
//
// NOTE : Based on this article :
// http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/


#ifndef NDEBUG

#define ASSERT_PRINT_VARIABLES_1(var, ...) \
  Assert::print_variable(ASSERT_LOG(), #var, var)

#define ASSERT_PRINT_VARIABLES_2(var, ...) \
  Assert::print_variable(ASSERT_LOG(), #var, var); \
  ASSERT_PRINT_VARIABLES_1(__VA_ARGS__)

#define ASSERT_PRINT_VARIABLES_3(var, ...) \
  Assert::print_variable(ASSERT_LOG(), #var, var); \
  ASSERT_PRINT_VARIABLES_2(__VA_ARGS__)

// TODO should this be outside the ifndef? IDK.
// TODO I'd like this to be a little nicer, but this will do for now
#define ASSERT_PRINT_VARIABLES(...) \
  do { \
    int assert_dummy_name_ = 0; \
    ASSERT_PRINT_VARIABLES_(2, __VA_ARGS__, assert_dummy_name_, assert_dummy_name_) \
  } while(0)

#define ASSERT_PRINT_VARIABLES_(extras, ...) \
  switch (Assert::count_variables(__VA_ARGS__) - extras) { \
    case 0 : break; \
    case 1 : ASSERT_PRINT_VARIABLES_1(__VA_ARGS__); break; \
    case 2 : ASSERT_PRINT_VARIABLES_2(__VA_ARGS__); break; \
    case 3 : ASSERT_PRINT_VARIABLES_3(__VA_ARGS__); break; \
    default : fprintf(ASSERT_LOG(), "Too many variables were provided.\n"); break; \
  }

  #define ASSERT(cond) \
    do { \
      if (!(cond)) { \
        Assert::print_failure(ASSERT_LOG(), #cond, __FUNCTION__, __FILE__, __LINE__); \
        if (PRINT_STACK_TRACE_ON_ASSERT) Assert::print_stack_trace(ASSERT_LOG(), ASSERT_MAX_STACK_TRACE()); \
        if (HALT_ON_ASSERT) HALT(); \
      } \
    } while(0)

  #define VERIFY(cond) \
    Assert::verify_print((cond), ASSERT_LOG(), HALT_ON_ASSERT, PRINT_STACK_TRACE_ON_ASSERT, ASSERT_MAX_STACK_TRACE(), #cond, __FUNCTION__, __FILE__, __LINE__)

#define FAILURE(msg, ...) \
    do { \
      Assert::print_failure(ASSERT_LOG(), msg, __FUNCTION__, __FILE__, __LINE__); \
      int assert_dummy_name_ = 0; \
      ASSERT_PRINT_VARIABLES_(3, ##__VA_ARGS__, assert_dummy_name_, assert_dummy_name_, assert_dummy_name_); \
      if (PRINT_STACK_TRACE_ON_ASSERT) Assert::print_stack_trace(ASSERT_LOG(), ASSERT_MAX_STACK_TRACE()); \
      if (HALT_ON_ASSERT) HALT(); \
    } while(0)

#define ASSERT_EQUALS(a, b) \
    do { \
      if (!(a == b)) { \
        FAILURE(#a " == " #b, a, b); \
      } \
    } while(0)

#else  
  #define ASSERT(cond) UNUSED(cond)
  #define VERIFY(cond) (cond)
// TODO define a FAILURE call that does nothing
  #define FAILURE(msg, ...) UNUSED(msg); UNUSED(count_variables(__VA_ARGS__));
  #define ASSERT_EQUALS(a, b) UNUSED(a); UNUSED(b)
#endif

// TODO print the value of value
#define INVALID_SWITCH_CASE(value) FAILURE("Invalid switch case for " #value, (value))
#define INVALID_CODE_PATH() FAILURE("Invalid code path")

#endif // _CUSTOM_ASSERT_H_
