

#include "nv/core/algo.h"
#include "nv/core/fs.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/core/spad.h"
#include "nv/core/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/virt.h"
#include "unity.h"

VirtMem* VM = {};
Allocator ALLOC = {};

void setUp(void) {
  bailerr_withv(vmem_init(&VM, GB(2)));
  ALLOC = vmem_allocator(VM);
}

void tearDown(void) {
  vmem_destroy(VM);
  VM = nullptr;
  ALLOC = (Allocator){};
}


void small_files(void) {
  // File* file = file_read_mem("/home/nvllmnd/sauce/priv/libnv/tests/data/file1.txt", ALLOC);

  File* file = file_read_mem("./data/file1.txt", ALLOC);

  TEST_ASSERT_NOT_NULL(file);

}


i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(small_files);

  return UNITY_END();
}
