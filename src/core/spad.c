#include "nv/core/spad.h"

#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/log.h"
#include "nv/core/sslice.h"
#include "nv/iter/string.h"
#include "nv/memory/alloc.h"
#include "nv/memory/virt.h"

StringPad spad_delim_new(struct VirtMem* vm, char delim) {
  if UNLIKELY (is_null(vm)) {
    LOG_FATAL("Tried to create a new StringPad with a null VirtMem pointer!");
  }

  const VirtMemView view = vmem_view(vm);

  const char* vstart = view.start;
  const char* begin = &vstart[view.used_bytes];
  // points to ending 'null character'
  const char* end = begin + 1;

  const VMarker marker = vmem_mark(vm);

  const i32 size = 0;
  return (StringPad){.vm = vm, .size = size, .mark = marker, .delim = delim, .begin = begin, .end = end};
}

i32 spad_clone_into(StringPad* self, char* buff_out, i32 buff_len) {
  assert(self);
  assert(buff_out);
  assert(buff_len >= 0);
  const i32 size = self->end - self->begin;

  const i32 len = min(buff_len, size);

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
  assert(self);
  assert(s);
  assert(len > 0);

  const i32 avail = vmem_available(self->vm);

  if UNLIKELY (avail <= 0) {
    ELOG_DBG("Not enough memory in backing VirtMem to append string %.*s of length: %d", len, s, len);
    return sslice_empty();
  }

  if (len > avail) {
    len = avail;
  }

  // make room for our delimiter, if any
  const i32 size = self->delim != '\0' ? len + 1 : len;

  char* str = punwrap(vmem_allocate(self->vm, mlayout_bytes(size)));

  strncpy(str, s, len);

  self->end += size;
  self->size += size;

  if (self->delim != '\0') {
    str[len] = self->delim;
  }

  return sslice_new(str, len);
}

sslice spad_append(StringPad* self, const char* s) {
  assert(self);
  assert(s);

  const i32 len = stringlen(s);
  return spad_nappend(self, s, len);
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

  i32 slen = 0;
  const char* str = vmem_vfstring(self->vm, &slen, fmt, args);

  // NOTE: Here we delete the top most byte, so that the null character that
  // vmem_fstring appends to our formatted string gets overwritten on the next call to this function,
  // otherwise there would be a bunch of null characters interleaved into the string we are building!
  vmem_delete_back(self->vm, 1);

  // const i32 len = vfstring_length(fmt, args);
  //
  // const i32 avail = vmem_available(self->vm) - 1;
  //
  // const i32 alloc_size = min(len, avail)
  //
  //     // NOTE: We not calling the vmem_fstring functions as they all append null character to the strings they
  //     allocate
  //
  //     char* str = punwrap(vmem_allocate(self->vm, mlayout_bytes(alloc_size)));

  self->end += slen;
  self->size += slen;

  //
  // vsnprintf(str, alloc_size + 1, fmt, args);

  return sslice_new(str, slen);
}

void spad_destroy(StringPad* self) {
  if LIKELY (self && is_not_null(self->vm)) {
    vmem_reset_to(self->vm, self->mark);
    memset(self, 0, sizeof(StringPad));
  }
}

char spad_putchar(StringPad* self, char c) {
  assert(self);

  if UNLIKELY (vmem_available(self->vm) < 1) {
    return -1;
  }

  char* ch = punwrap(vmem_allocate(self->vm, mlayout_bytes(1)));

  *ch = c;

  self->size+= 1;

  return c;
}

u8 spad_putbyte(StringPad* self, u8 byte) {
  assert(self);

  if UNLIKELY (vmem_available(self->vm) < 1) {
    return UINT8_MAX;
  }

  u8* by = punwrap(vmem_allocate(self->vm, mlayout_bytes(1)));
  *by = byte;

  self->size += 1;

  return byte;
}

void spad_clear(StringPad* self) {
  assert(self);

  self->end = self->begin + 1; 
  self->size = 0;
  

  vmem_reset_to(self->vm, self->mark);

}

void spad_clear_zeroed(StringPad* self) {
  assert(self);


  self->end = self->begin + 1; 
  self->size = 0;
  

  vmem_reset_zeroed(self->vm, self->mark);

  
}

