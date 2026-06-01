

#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/constants.h"
#include "nv/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/arena.h"
#include "nv/memory/block_alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/virt.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

struct Stuff {
  char buf[4096];

  struct Point {
    f32 x;
    f32 y;
  } points[20];

  i64 counter;
};
alias(Stuff);


void arena_heap_exclusive(void) {
  Arena* ah = arena_new(MEGABYTES(4), MEGABYTES(2));
  TEST_ASSERT_NOT_NULL(ah);

  Stuff* s = arena_zalloc(ah, mlayout_new(Stuff));
  TEST_ASSERT_NOT_NULL(s);

  *s = make(Stuff, .buf = {}, .points = {}, .counter = 5);

  arena_destroy(ah);
}

void arena_heap_from_vmem(void) {
  VirtMem* vm = nullptr;

  TEST_ASSERT_EQUAL(OK, vmem_init(&vm, MEGABYTES(16)));

  TEST_ASSERT_NOT_NULL(vm);

  Arena* ah = arena_in_vmem(vm, MEGABYTES(2), true);

  for (i32 i = 0; i < 50; i++) {
    char* b1 = arena_zalloc(ah, mlayout_bytes(1024));
    char* b2 = arena_zalloc(ah, mlayout_bytes(2048));

    TEST_ASSERT_NOT_NULL(b1);
    TEST_ASSERT_NOT_NULL(b2);

    strncpy(b1, "ayooo", sizeof("ayooo"));

    TEST_ASSERT_EQUAL_STRING(b1, "ayooo");
  }

  const ArenaStats stats = arena_stats(ah);
  println("TOTAL ALLOCATED IN BYTES : %li", stats.total_used);

  arena_destroy(ah);
}


void vmap_ex_lock_and_commit(void) {
  
  VirtMem* vm = nullptr;
  MemError err = vmem_init_ex(&vm,  make(VirtMemOpts, .size_bytes = KILOBYTES(24), .access = VMap__DefaultAccess, .mode = VMap__CommitAll | VMap__NoReserve | VMap__LockAll));
  TEST_ASSERT_EQUAL(MemError__Ok, err);

  err = vmem_lock(vm, KILOBYTES(12));
  TEST_ASSERT_EQUAL(MemError__Ok, err);

  err = vmem_unlock(vm, KILOBYTES(12));
  TEST_ASSERT_EQUAL(MemError__Ok, err);

  err = vmem_destroy(vm);
  TEST_ASSERT_EQUAL(MemError__Ok, err);

  
}

void vmap_marker_and_remap(void) {

  VirtMem* vm = nullptr;
  MemError err = vmem_init_ex(&vm, make(VirtMemOpts, .access = VMap__DefaultAccess, .mode = VMap__NoReserve, .commit_bytes = 0, .lock_bytes = 0, .size_bytes = MEGABYTES(100)));
  TEST_ASSERT_EQUAL(MemError__Ok, err);

  for (i32 i = 0; i < 25; i++) {
    i32* x = vmem_allocate(vm, mlayout_new(i32));
    TEST_ASSERT_NOT_NULL(x);
    *x = i * i;
  }

  const VMarker marker = vmem_mark(vm);

  TEST_ASSERT(marker >= 0);


  for (i32 i = 0; i < 25; i++) {
    i32* x = vmem_allocate(vm, mlayout_new(i32));
    TEST_ASSERT_NOT_NULL(x);
    *x = i * i;
  }

  vmem_reset_to( vm, marker);
  

  VAddrOffset offset = 0;
  Stuff* x = vmem_alloc_offset_array(vm, Stuff, 8, &offset);

  
  for (i32 i = 0; i < 8; i++ ) {
    x->counter = i * i;
  }

  const i32 pre = x[2].counter;

  err = vmem_remap(&vm, MEGABYTES(101), VRemap__ExpandInPlace);

  if (err == MemError__CannotExpandInPlace) {
    LOG("Could not expand in place, trying to relocate!");
    err = vmem_remap(&vm, MEGABYTES(300), VRemap__AllowRelocate);
  }

  TEST_ASSERT_EQUAL(MemError__Ok, err);


  vmem_update_ptr(vm, (void**)&x, offset);


  TEST_ASSERT_EQUAL(x[2].counter, pre);

  
}


void block_allocator_works(void) {
  BlockAllocator* ba = ba_owned_new(MEGABYTES(24));
  TEST_ASSERT_NOT_NULL(ba);

  Stuff* ss[50] = {};

  for (i32 i = 0; i < 50; i++) {
    Stuff* s = ba_allocate(ba, mlayout_new(Stuff));
    TEST_ASSERT_NOT_NULL(s);
    *s = make(Stuff, .buf = {}, .points = {}, .counter =  69);
    ss[i] = s;
  }

  for (i32 i = 0; i < 50; i++) {
    ba_free(ba, ss[i]);
  }

  ba_destroy(ba);
}

void block_allocator_relcaims_memory(void) {
  BlockAllocator* ba = ba_owned_new(MEGABYTES(4));
  TEST_ASSERT_NOT_NULL(ba);

  Stuff* s =  ba_allocate(ba, mlayout_new(Stuff));
  TEST_ASSERT_NOT_NULL(s);

  ba_free(ba, s);

  
  struct Point* ps = ba_allocate(ba, mlayout_array(struct Point, 20));
  TEST_ASSERT_NOT_NULL(ps);

  
  struct Point* ps2 = ba_allocate(ba, mlayout_array(struct Point, 32));
  TEST_ASSERT_NOT_NULL(ps2);


  
  struct Point* ps3 = ba_allocate(ba, mlayout_array(struct Point, 10));
  TEST_ASSERT_NOT_NULL(ps3);

  
  struct Point* ps4 = ba_allocate(ba, mlayout_array(struct Point, 64));
  TEST_ASSERT_NOT_NULL(ps4);


  ba_free(ba, ps2);
  ps2 = nullptr;

  
  ba_free(ba, ps3);
  ps3 = nullptr;


  struct Point* ps5 = ba_allocate(ba, mlayout_array(struct Point, 200));
  TEST_ASSERT_NOT_NULL(ps5);

  
  struct Point* ps6 = ba_allocate(ba, mlayout_array(struct Point, 120));
  TEST_ASSERT_NOT_NULL(ps6);

  ba_free(ba, ps5);
  ps5 = nullptr;


  
  struct Point* ps7 = ba_allocate(ba, mlayout_array(struct Point, 10));
  TEST_ASSERT_NOT_NULL(ps7);

  
  ba_free(ba, ps7);
  ps7 = nullptr;


  ba_free(ba, ps4);
  ps4 = nullptr;

  ba_free(ba, ps6);
  ps6 = nullptr;


  ba_free(ba, ps);
  ps = nullptr;

  ba_destroy(ba);

  
}
i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(arena_heap_exclusive);
  RUN_TEST(arena_heap_from_vmem);
  RUN_TEST(block_allocator_works);
  RUN_TEST(block_allocator_relcaims_memory);
  RUN_TEST(vmap_ex_lock_and_commit);
  RUN_TEST(vmap_marker_and_remap);

  return UNITY_END();
}
