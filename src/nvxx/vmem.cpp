#include "nvxx/vmem.hpp"
#include "nv/memory/vmem.h"
#include "nvxx/alloc.hpp"
#include "nvxx/opt.hpp"

namespace nv {

// TODO: Add error logging to the error path of allocation interface impl functions (here and elsewhere!)

[[gnu::nonnull]]
bool Vmem::resize(void* ptr, isize old_size, isize new_size) noexcept {
  assert(ptr);
  if (vremap_memory(ptr, old_size, new_size, false)) {
    return true;
  }
  return false;
}

AllocResult Vmem::alloc(isize size, isize) noexcept { return vmap_memory(size, false); }

AllocResult Vmem::realloc(void* ptr, isize old_size, isize new_size) noexcept {
  assert(ptr);
  if (void* res = vremap_memory(ptr, old_size, new_size, true)) [[likely]] {
    return res;
  }
  return nullptr;
}

[[gnu::nonnull]]
void free(void* ptr, isize size) noexcept {
  assert(ptr);
  vunmap_memory(ptr, size);
}

}  // namespace nv
