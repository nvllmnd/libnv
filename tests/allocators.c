

#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/core/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/vmem.h"
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

void vmem_alloc_and_cleanup(void) {
  VArena vm = va_new(GB(2), false);

  TEST_ASSERT_TRUE(va_isok(&vm));

  for (i32 i = 0; i < 50; i++) {
    char* b1 = va_zallocate(&vm, mlayout_bytes(1024));
    char* b2 = va_zallocate(&vm, mlayout_bytes(2048));

    TEST_ASSERT_NOT_NULL(b1);
    TEST_ASSERT_NOT_NULL(b2);

    strncpy(b1, "ayooo", sizeof("ayooo"));

    TEST_ASSERT_EQUAL_STRING(b1, "ayooo");
  }

  va_destroy(&vm);
}

// void vmap_ex_lock_and_commit(void) {
//   VirtMem* vm = nullptr;
//   NvError err = vmem_init_ex(&vm, make(VirtMemOpts, .size_bytes = KILOBYTES(24), .access = VMap__DefaultAccess,
//                                        .mode = VMap__CommitAll | VMap__NoReserve | VMap__LockAll));
//   TEST_ASSERT_EQUAL(Error__Ok, err);

//   err = vmem_lock(vm, KILOBYTES(12));
//   TEST_ASSERT_EQUAL(Error__Ok, err);

//   err = vmem_unlock(vm, KILOBYTES(12));
//   TEST_ASSERT_EQUAL(Error__Ok, err);

//   err = vmem_destroy(vm);
//   TEST_ASSERT_EQUAL(Error__Ok, err);
// }

// void vmap_marker_and_remap(void) {
//   VirtMem* vm = nullptr;
//   NvError err = vmem_init_ex(&vm, make(VirtMemOpts, .access = VMap__DefaultAccess, .mode = VMap__NoReserve,
//                                        .commit_bytes = 0, .lock_bytes = 0, .size_bytes = MEGABYTES(100)));
//   TEST_ASSERT_EQUAL(Error__Ok, err);

//   for (i32 i = 0; i < 25; i++) {
//     i32* x = vmem_allocate(vm, mlayout_new(i32));
//     TEST_ASSERT_NOT_NULL(x);
//     *x = i * i;
//   }

//   const VMarker marker = vmem_checkpoint(vm);

//   TEST_ASSERT(marker >= 0);

//   static constexpr const i64 INTCOUNT = 25;

//   for (i32 i = 0; i < INTCOUNT; i++) {
//     i32* x = vmem_allocate(vm, mlayout_new(i32));
//     TEST_ASSERT_NOT_NULL(x);
//     *x = i * i;
//   }

//   static constexpr const i64 ALLOCSIZE = INTCOUNT * sizeof(i32);

//   const i64 size = vmem_reset_to(vm, marker);
//   TEST_ASSERT_EQUAL(size, ALLOCSIZE);

//   VAddrOffset offset = 0;
//   Stuff* x = vmem_alloc_offset_array(vm, Stuff, 8, &offset);

//   for (i32 i = 0; i < 8; i++) {
//     x->counter = i * i;
//   }

//   const i32 pre = x[2].counter;

//   err = vmem_remap(&vm, MEGABYTES(101), VRemap__ExpandInPlace);

//   if (err == Error__CannotExpandInPlace) {
//     LOG("Could not expand in place, trying to relocate!");
//     err = vmem_remap(&vm, MEGABYTES(300), VRemap__AllowRelocate);
//   }

//   TEST_ASSERT_EQUAL(Error__Ok, err);

//   vmem_update_ptr(vm, (void**)&x, offset);
//  TEST_ASSERT_EQUAL(x[2].counter, pre);
// }

i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(vmem_alloc_and_cleanup);

  return UNITY_END();
}
