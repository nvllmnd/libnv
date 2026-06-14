// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nv/core/attributes.h"
#include "nv/core/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/vmem.h"

// TODO: Finish this, or decide to get rid of it, im leaning more towards removing it, 

struct ArenaBlock;

struct nv_nodiscard_msg("Attempted to ignore Arena Marker! This is probably an error. DO NOT IGNORE!") ArenaTag {
  const struct ArenaBlock* block;
  i64 offset;
};
alias(ArenaTag);

struct nv_nodiscard_msg("Ignoring Arena, which owns dynamic resources. DO NOT IGNORE!") Arena {
  struct ArenaBlock* begin;
  struct ArenaBlock* end;

  i64 count;
};
alias(Arena);

static constexpr const Arena ARENA_NONE = {};

METHOD
static inline bool arena_none(const Arena* self) { return memcmp(self, &ARENA_NONE, sizeof(Arena)) == 0; }

METHOD
static inline bool arena_ok(const Arena* self) { return is_not_null(self) && is_not_null(self->begin); }

Arena arena_new(i64 first_block_size);

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
