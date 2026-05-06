

#include <string.h>

#include "constants.h"
#include "core_types.h"
#include "intdefs.h"
#include "log.h"
#include "memory/alloc.h"
#include "memory/arena.h"
#include "memory/virt.h"
#include "unity.h"

void setUp(void) {
}

void tearDown(void) {}


struct Stuff {
  char buf[255];

  struct Point {
    f32 x;
    f32 y;
  } points[20];

  i64 counter;
};
alias(Stuff);

void arena_heap_exclusive(void) {

 ArenaHeap* ah = arena_heap_new(4, MEGABYTES(2));  
 TEST_ASSERT_NOT_NULL(ah);

 
 Stuff* s = arena_heap_zalloc(ah, mlayout_new(Stuff));
 TEST_ASSERT_NOT_NULL(s);

 *s = make(Stuff, .buf = {}, .points = {}, .counter = 5);

 arena_heap_destroy(ah);

}

void arena_heap_from_vmem(void) {
  VirtMem* vm = nullptr;

  TEST_ASSERT_EQUAL(OK, vmem_init(&vm, 4));

  TEST_ASSERT_NOT_NULL(vm);

  ArenaHeap* ah = arena_heap_in_vmem(vm, MEGABYTES(2), true);

  for (i32 i = 0; i < 50; i++) {
    char* b1 = arena_heap_zalloc(ah, mlayout_bytes(1024));
    char* b2 = arena_heap_zalloc(ah, mlayout_bytes(2048));

    TEST_ASSERT_NOT_NULL(b1);
    TEST_ASSERT_NOT_NULL(b2);

    strncpy(b1, "ayooo", sizeof("ayooo"));

    TEST_ASSERT_EQUAL_STRING(b1, "ayooo");


  }

  const ArenaHeapStats stats = arena_heap_stats(ah);
  println("TOTAL ALLOCATED IN BYTES : %li", stats.total_used);

  arena_heap_destroy(ah);
}



/// we can put a global allocator in an [Allocator]
/// struct and everything works just fine
// void global_allocator_trait(void) {
//   const Allocator g = global_allocator();

//   TEST_ASSERT_NULL(g.ctx);
//   TEST_ASSERT_NOT_NULL(g.vtable);

//   TEST_ASSERT_NOT_NULL(g.vtable->allocate);

//   u8* mem = allocator_allocate(g, 64, alignof(u8[64]));
//   TEST_ASSERT_NOT_NULL(mem);

//   TEST_ASSERT_NOT_NULL(g.vtable->free);

//   allocator_free(g, mem);
// }

i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(arena_heap_exclusive);
  RUN_TEST(arena_heap_from_vmem);

  return UNITY_END();
}
