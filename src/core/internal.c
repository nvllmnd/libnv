#include "internal/types.h"
#include "nv/core_types.h"
#include "nv/memory/virt.h"

static NvAlloc* ALLOC = {};

NvError nvalloc_init(const i64 size) {
  if LIKELY (is_null(ALLOC)) {
    bailerr(vmem_init(&ALLOC, size));
  }
  return OK;
}

NvAlloc* nvallocator(void) {
  if (is_null(ALLOC)) {
    bailerr_with(nvalloc_init(DEFAULT_NVALLOC_SIZE), nullptr);
  }
  assert(ALLOC);
  return ALLOC;
}
