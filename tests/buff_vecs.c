#include "nv/iter/buff.h"

#include "nv/iter/vec.h"

#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/memory/arena.h"
#include "nv/memory/virt.h"
#include "unity.h"

static Arena* ARENA = nullptr;

static Allocator ALLOC;

void setUp(void) {
  ARENA = arena_new(GIGABYTES(1), os_page_size());
  ALLOC = arena_allocator(ARENA);

  assert(ARENA);

  
}

void tearDown(void) {
  arena_destroy(ARENA);
  ARENA = nullptr;
}

void buffs_works(void) {


  Buff* b = buff_new(KILOBYTES(4), ALLOC);
  TEST_ASSERT_NOT_NULL(b);
  
  #define FIRST "test string"
  #define SEC " appended!"
  #define FULL FIRST SEC

  LOG_DBG("Test: %s", FULL);

  const sslice str = buff_append_str(b, FIRST);
  TEST_ASSERT_EQUAL_STRING_LEN(FIRST, str.begin, str.len);

  const sslice end = buff_append_str(b, SEC);

  TEST_ASSERT_EQUAL_STRING_LEN(SEC, end.begin, end.len);

  const sslice full = buff_as_string(b);

  TEST_ASSERT_EQUAL_STRING_LEN(FULL, full.begin, full.len);

}


void vecs_works(void) {
  Vec(i32) v = vec_new(i32, 100, ALLOC);
  TEST_ASSERT_EQUAL(100, vec_capacity(v));

  vec_push(v, 50);
  TEST_ASSERT_EQUAL(1, vec_len(v));

  TEST_ASSERT_EQUAL(50, v[0]);

  vec_push(v, 100);

  TEST_ASSERT_EQUAL(100, v[1]);

  TEST_ASSERT_EQUAL(2, vec_len(v));
  
  
}



i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(buffs_works);
  RUN_TEST(vecs_works);

  return UNITY_END();
}
