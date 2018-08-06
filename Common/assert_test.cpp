
#include <common.h>
#include <lstring.h>

#undef HALT_ON_ASSERT
#define HALT_ON_ASSERT false

void other_function() {
  ASSERT(!"Inside other function.");
}

u32 ret_true(int i, float f) {
  return (u32) (sqrt(f / ((i % 3) + 1)) * 5);
}

bool ret_false(int i, float f) {
  return !sqrt(f / ((i % 3) + 1));
}

int main() {
  ASSERT(true || false);

  ASSERT(false);
#undef PRINT_STACK_TRACE_ON_ASSERT
#define PRINT_STACK_TRACE_ON_ASSERT 0
  ASSERT(!true);
  ASSERT(1 - 1);

  other_function();
  u32 result = VERIFY(ret_false(4, 7.12));
  PRINT_UINT(result);

  result = VERIFY(ret_true(4, 7.12));
  PRINT_UINT(result);

  result = VERIFY(1 + 1);
  PRINT_UINT(result);

  result = VERIFY(2 * 3 + 1);
  PRINT_UINT(result);

  hstring h1 = VERIFY(hash_string("Hello World."));
  print_hstring(h1);

  hstring h2 = VERIFY(hash_string(""));
  if (h2) print_hstring(h2);

  auto h3 = VERIFY(hash_string("A"));
  print_hstring(h3);

  float f = 3.1; int i = -2; u64 u = UINT64_MAX - 2;
  ASSERT_PRINT_VARIABLES(f, i, u);

  FAILURE("Message.");
  FAILURE("Message 1 arg.", i);
  FAILURE("Message 2 arg.", i, f);
  FAILURE("Message 3 arg.", i, f, u);
  FAILURE("Message 4 arg.", i, f, u, result);


  INVALID_SWITCH_CASE(u);
  INVALID_CODE_PATH();
  int a = 0; int b = 0; int c = 1;
  ASSERT_EQUALS(a, b);
  ASSERT_EQUALS(a, c);
}

