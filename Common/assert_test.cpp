
#include <common.h>
#include <lstring.h>

#undef HALT_ON_ASSERT
#define HALT_ON_ASSERT false

static FILE *log_file;

#undef ASSERT_LOG
#define ASSERT_LOG() log_file

void other_function() {
  ASSERT(!"Inside other function.");
}

u32 ret_true(int i, float f) {
  return (u32) (sqrt(f / ((i % 3) + 1)) * 5);
}

bool ret_false(int i, float f) {
  return !sqrt(f / ((i % 3) + 1));
}

enum AssertEnum {
  AENUM_A,
  AENUM_B,
  AENUM_C,
};

void test_switch_statement(int num) {
  switch (num) {
    case AENUM_A : break;
    case AENUM_B : break;
    case AENUM_C : break;
    default : INVALID_SWITCH_CASE(num);
  }
}

void test_assert_equals() {
  int a = 0; int b = 0; int c = 1;
  float d = 0; float e = 0; float f = 1;
  ASSERT_EQUALS(a, b);
  ASSERT_EQUALS(d, e);
  ASSERT_EQUALS(a, d);
  ASSERT_EQUALS(c, f);
  ASSERT_EQUALS(a, f);
}

void print_log_file() {
  int err = fflush(log_file);
  rewind(log_file);

  assert(!err);
  const int buf_size = 1000;
  char buf[buf_size];
  printf("Printing contents to stdout : \n" KBLU);
  assert(log_file);
  auto str = fgets(buf, buf_size, log_file);
  assert(str);
  while (str) {
    printf("%s", str);
    str = fgets(buf, buf_size, log_file);
  }
}

int main() {
  log_file = tmpfile();
  assert(log_file);

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

  test_switch_statement(0);
  test_switch_statement(1);
  test_switch_statement(2);
  test_switch_statement(3);

  int *test_unused = NULL; UNUSED(test_unused[0] = 1);

  test_assert_equals();

  print_log_file();

  fclose(log_file);
}

