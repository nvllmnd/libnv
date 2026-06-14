// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "nv/memory/arena.h"
#include "nv/memory/vmem.h"

struct ArenaBlock {
  VArena mem;
  i64 size;
  ATTR_COUNTED_BY(size)
  byte data[];
};
alias(ArenaBlock);

// static inline i64 ab_available(const ArenaBlock* self) {
//   assert(self);
//   return va_available( &self->mem);
// }

// static inline i64 ab_used_bytes(const ArenaBlock* self) {
//   assert(self);
//   return va_used_bytes( &self->mem);
// }

// static inline ArenaBlock* ab_take(VArena arena) {
//   // ArenaBlock* self = va_allocate(&arena, )
// }


// Arena arena_new(i64 first_block_size) {
//   assert(first_block_size > 0);
//   VArena a = va_new(first_block_size);
//   if (va_isnone(&a)) {
//     LOG_ERROR("Failed to create new Arena!");
//     return ARENA_NONE;
//   }
//   ArenaBlock
//   Arena self = (Arena){.begin }
// }

/// @brief takes ownership of given VMem
Arena arena_from(VMem* vm);

/// @brief returns the bytes available for allocation in the current block
i64 arena_available(const Arena* self) METHOD PURE_FUNC;

/// @brief same as [arena_available], but iterates through each block, totaling each ones available bytes
i64 arena_total_avail(const Arena* self) METHOD PURE_FUNC;

/// @brief returns the bytes used for allocation in the current block
i64 arena_used_bytes(const Arena* self) METHOD PURE_FUNC;

/// @brief same as [arena_used_bytes], but iterates through each block, totaling each ones used bytes
i64 arena_total_used(const Arena* self) METHOD PURE_FUNC;

void* arena_allocate(Arena* self, Layout layout) METHOD;

void* arena_zallocate(Arena* self, Layout layout) METHOD;
bool arena_resize(Arena* self, void* ptr, Layout old, Layout new) METHOD;
void* arena_reallocate(Arena* self, void* ptr, Layout old, Layout new) METHOD;

char* arena_strdup(Arena* self, const char* str) PARAMS_NONNULL(1, 2);
char* arena_strndup(Arena* self, const char* str, i64 len) PARAMS_NONNULL(1, 2);

char* arena_vfstring(Arena* self, i64* lenout, const char* fmt, va_list args) METHOD;
char* arena_fstring(Arena* self, i64* lenout, const char* fmt, ...) METHOD HEDLEY_PRINTF_FORMAT(3, 4);

sslice arena_vfslice(Arena* self, const char* fmt, va_list args) METHOD;

sslice arena_fslice(Arena* self, const char* fmt, ...) METHOD HEDLEY_PRINTF_FORMAT(2, 3);

ArenaTag arena_checkpoint(const Arena* self);
i64 arena_reset_to(Arena* self, ArenaTag tag);
