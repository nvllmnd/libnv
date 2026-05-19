


#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}



void formatting_strings(void) {
  static constexpr const i32 FMT_INT = 69696;
  const cstr s = format_string("This is: %s with int: %d", "format_string", FMT_INT);
  const cstr ss = cstr_new("This is: " "format_string " "with int: " STRINGIFY(69696));

  TEST_ASSERT_TRUE(cstr_eq(&s, &ss));
  
}

i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(formatting_strings);

  return UNITY_END();
}
