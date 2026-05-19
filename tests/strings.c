
#include <stdio.h>

#include "nv/core/algo.h"
#include "nv/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/cstr.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

void move_memory_helpers(void) {
  typedef struct Resource {
    const char* buf;
  } Resource;

  {
    static const char* INPUT = "test";
    static const char* OLD = "old value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    b.buf = move_exchange(a.buf, OLD);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_EQUAL_STRING(a.buf, OLD);
  }
  {
    static const char* INPUT = "input value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    b.buf = move(a.buf);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_NULL(a.buf);
  }

  {
    static const char* INPUT = "input value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    static constexpr const char* none = nullptr;
    b.buf = move_exchange(a.buf, none);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_NULL(a.buf);
  }

  {
    static const char* INPUT = "input value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    move_into(a.buf, b.buf);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_NULL(a.buf);
  }



  
}

void tagged_pointers(void) {
  typedef struct Point {
    double x; double y;
  } Point;

  const Point val = make(Point, 1000., 0.5265236);

  const Point* original = &val; 

  const Point* tptr = tptr_new(original, true);
  const Point* untagged = tptr_ptr(tptr);
  TEST_ASSERT_EQUAL(untagged, original);

  const bool tag = tptr_tag(tptr);

  TEST_ASSERT_TRUE(tag);
  
}


void string_compare(void) {
  static constexpr const char STR[] = "this is a test string!";
  const cstr l = cstr_new(STR);
  const cstr r = cstr_new(STR);

  TEST_ASSERT_TRUE_MESSAGE(cstr_eq(&l, &r), "cstr_cmp between 2 strings that should be the same failed!");

  const cstr diff = cstr_new("this is a different string!");

  TEST_ASSERT_FALSE_MESSAGE(cstr_eq(&l, &diff), "Strings should be diff");

  const sslice slice_this = sslice_from_range(cstr_as_ptr(&diff), 0, 4);
  const sslice slice_that = sslice_from_range(cstr_as_ptr(&l), 0, 4);

  TEST_ASSERT_TRUE_MESSAGE(sslice_eq(slice_this, slice_that), "slices should match");
}

i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(string_compare);
  RUN_TEST(move_memory_helpers);
  RUN_TEST(tagged_pointers);

  return UNITY_END();
}
