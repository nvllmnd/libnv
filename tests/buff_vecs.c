#include "buffer.h"
#include "intdefs.h"
#include "log.h"
#include "memory/arena.h"
#include "memory/virt.h"
#include "unity.h"

static VirtMem* VM = nullptr;
static Arena* ARENA = nullptr;

static Allocator ALLOC;

void setUp(void) {
  assert(vmem_init(&VM, 16) == OK);
  ARENA = arena_in_vmem(VM, KILOBYTES(24), true);
  ALLOC = arena_allocator(ARENA);

  assert(ARENA);

  
}

void tearDown(void) {
  vmem_destroy(VM);
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
  Vec(i32) v = vec_new(i32, ALLOC);

  vec_push(v, 50);

  TEST_ASSERT_EQUAL(50, v[0]);

  
}

i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(buffs_works);
  RUN_TEST(vecs_works);

  return UNITY_END();
}
