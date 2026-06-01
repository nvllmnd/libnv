#include "nv/core/spad.h"
#include <string.h>
#include "nv/core/algo.h"
#include "nv/core/log.h"
#include "nv/core/sslice.h"
#include "nv/iter/string.h"
#include "nv/memory/alloc.h"
#include "nv/memory/virt.h"


StringPad spad_new(struct VirtMem* vm, char delim) {
  if UNLIKELY (is_null(vm)) { 
    LOG_FATAL("Tried to create a new StringPad with a null VirtMem pointer!");
  }
  
  const VirtMemView view = vmem_view(vm);

  const char* vstart = view.start;
  const char* begin = &vstart[view.used_bytes];
  // NOTE: end is always +1 the last item
  const char* end = begin + 1;

  const i32 size = 0;
  return make(StringPad, .size = size, .delim = delim, .begin = begin, .end = end);
}


i32 spad_clone_into(StringPad* self, char* buff_out, i32 buff_len) {
  assert(self);
  assert(buff_out);

  const i32 len = min(buff_len - 1, self->size);

  strncpy(buff_out, self->begin, len);

  buff_out[len] = '\0';

  return len;

}

sslice spad_clone_string(StringPad* self, Allocator alloc) {
  assert(self);
  const i32 len = self->size + 1;
  char* str = allocator_allocate(alloc, mlayout_bytes(len));

  spad_clone_into(self, str, len);

  return sslice_new(str, len);
  
}

sslice spad_nappend(StringPad* self, const char* s, i32 len) {

  const i32 avail = vmem_available(self->vm);

  if UNLIKELY (avail <= 0) {
    ELOG_DBG("Not enough memory in backing VirtMem to append string %.*s of length: %d", len, s, len);
    return sslice_empty();
  }


  if (len > avail) {
    len = avail;
  }

  char* str = punwrap(vmem_allocate(self->vm, mlayout_bytes(len)));

  strncpy(str, s, len);

  self->end += len;
  self->size += len;

  return sslice_new(str, len);  
}

sslice spad_append(StringPad* self, const char* s) {
  assert(self);
  assert(s);

  const i32 len = stringlen(s);
  return spad_nappend(self, s,  len);


}


sslice spad_fappend(StringPad* self, const char* fmt, ...) {
  assert(self);
  assert(fmt);
  va_list args = {};
  va_start(args);

  const sslice sl = spad_vfappend(self, fmt, args);
  va_end(args);

  return sl;
}

sslice spad_vfappend(StringPad* self, const char* fmt, va_list args) {
  assert(self);


  const i32 len = vfstring_length(fmt, args);

  const i32 avail = vmem_available(self->vm);

  
  // alloc_size - 1 so next append overwrites the null character that vsnprintf applies
  const i32 alloc_size = min(len, avail) - 1;

  // NOTE: We not calling the vmem_fstring functions as they all append null character to the strings they allocate


  char* str = punwrap(vmem_allocate(self->vm, mlayout_bytes(alloc_size)));

  self->end += alloc_size;
  self->size += alloc_size;

  // 
  vsnprintf(str, alloc_size + 1, fmt, args);

  return sslice_new(str, alloc_size);
  
}


