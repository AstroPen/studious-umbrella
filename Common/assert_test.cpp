
#include <common.h>
#include <lstring.h>

#undef HALT_ON_ASSERT
#define HALT_ON_ASSERT false

static FILE *log_file;
static FILE *fail_msgs;

static int expected_failure_count = 0;
static int expected_success_count = 0;

#define EXPECT_SUCCESS \
  log_file = stdout; \
  expected_success_count++;

#define EXPECT_FAIL \
  log_file = fail_msgs; \
  expected_failure_count++;

#undef ASSERT_LOG
#define ASSERT_LOG() log_file

void other_function() {
  EXPECT_FAIL;
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
  EXPECT_SUCCESS;
  ASSERT_EQUAL(a, b);
  ASSERT_EQUAL(d, e);
  ASSERT_EQUAL(a, d);
  ASSERT_EQUAL(c, f);
  EXPECT_FAIL;
  ASSERT_EQUAL(a, f);
}

void print_fail_msgs() {
  int err = fflush(fail_msgs);
  rewind(log_file);

  assert(!err);
  const int buf_size = 1000;
  char buf[buf_size];
  printf(KNRM "Printing contents to stdout : \n" KBLU);
  assert(log_file);
  auto str = fgets(buf, buf_size, log_file);
  assert(str);
  while (str) {
    printf("%s", str);
    str = fgets(buf, buf_size, log_file);
  }
  printf(KNRM "\n");
}

int main() {
  fail_msgs = tmpfile();
  assert(fail_msgs);

  printf(KRED);

  EXPECT_SUCCESS;
  ASSERT(true || false);

  EXPECT_FAIL;
  ASSERT(false);
#undef PRINT_STACK_TRACE_ON_ASSERT
#define PRINT_STACK_TRACE_ON_ASSERT 0
  ASSERT(!true);
  ASSERT(1 - 1);

  other_function();
  u32 result = VERIFY(ret_false(4, 7.12));
  if (result) PRINT_UINT(result);

  EXPECT_SUCCESS;
  result = VERIFY(ret_true(4, 7.12));
  ASSERT_EQUAL(result, ret_true(4, 7.12));
  if (result != ret_true(4, 7.12)) PRINT_UINT(result);

  result = VERIFY(1 + 1);
  ASSERT_EQUAL(result, 1 + 1);
  if (result != 1 + 1) PRINT_UINT(result); 

  result = VERIFY(2 * 3 + 1);
  if (result != 2 * 3 + 1) PRINT_UINT(result);

  hstring h1 = VERIFY(hash_string("Hello World."));
  ASSERT(str_equal(h1, h1));
  ASSERT(str_equal(h1, hash_string("Hello World.")));
  if (!h1) print_hstring(h1);

  EXPECT_FAIL;
  hstring h2 = VERIFY(hash_string(""));
  if (h2) print_hstring(h2);

  EXPECT_SUCCESS;
  auto h3 = VERIFY(hash_string("A"));
  if (!h3 || !str_equal(h3, hash_string("A"))) print_hstring(h3);

  EXPECT_FAIL;
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
  EXPECT_SUCCESS;
  ASSERT_EQUAL(a, b);
  EXPECT_FAIL;
  ASSERT_EQUAL(a, c);

  EXPECT_SUCCESS;
  test_switch_statement(0);
  test_switch_statement(1);
  test_switch_statement(2);
  EXPECT_FAIL;
  test_switch_statement(3);

  int *test_unused = NULL; UNUSED(test_unused[0] = 1);

  test_assert_equals();

  print_fail_msgs();
  //PRINT_INT(expected_failure_count);
  //PRINT_INT(expected_success_count);

  fclose(log_file);
}

