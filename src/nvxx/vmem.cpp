#include "nvxx/vmem.hpp"
#include "nv/memory/vmem.h"
#include "nvxx/alloc.hpp"
#include "nvxx/opt.hpp"

namespace nv {

// TODO: Add error logging to the error path of allocation interface impl functions (here and elsewhere!)

AllocResult PageAlloc::alloc(Layout layout) noexcept {
  if (void* ptr = vmap_memory(layout.size, false)) [[likely]] {
    return ptr;
  }
  return nullptr;
}

bool PageAlloc::resize(void* ptr, Layout old_layout, Layout new_layout) noexcept {
  assert(ptr);
  if (vremap_memory(ptr, old_layout.size, new_layout.size, false)) {
    return true;
  }
  return false;
}

AllocResult PageAlloc::realloc(void* ptr, Layout old_layout, Layout new_layout) noexcept {
  assert(ptr);
  if (void* res = vremap_memory(ptr, old_layout.size, new_layout.size, true)) [[likely]] {
    return res;
  }
  return nullptr;
}

void PageAlloc::free(void* ptr, Layout layout) noexcept {
  assert(ptr);
  vunmap_memory(ptr, layout.size);
}

}  // namespace nv
