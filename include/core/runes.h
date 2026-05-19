#pragma once

#include <stdint.h>

#include "core_types.h"
#include "intdefs.h"
#include "core/sslice.h"

/// A Set of unique strings (called 'Runes'), hashed with FNV-1A,
/// Each rune has a unique index and a pointer to the table where it lives in memory,
/// this is to allow for fast comparison (between runes from the same parent),
/// Memory for each entry is kept in an [Arena] with a VirtMem backing
typedef struct RuneTable RuneTable;

struct Rune {
  i32 id;
  const RuneTable* parent;
};
typedef struct Rune Rune;

static constexpr const Rune RUNE_NONE = make(Rune, .id = -1, .parent = nullptr);


PURE_FUNC
static inline bool rune_is_none(Rune self) {
  return self.id <= RUNE_NONE.id || is_null(self.parent);
}

PURE_FUNC
static inline bool rune_is_ok(Rune self) {
  return !rune_is_none(self);
}


PURE_FUNC
static inline bool rune_eq(Rune left, Rune right) { return left.id == right.id && left.parent == right.parent; }

static constexpr const i32 RUNE_MAX_SIZE = INT16_MAX;


RuneTable* runetab_new(i32 entry_len, i32 name_storage_in_mb);

PURE_FUNC
METHOD
f32 runetab_load_factor(const RuneTable* self);

/// Resizes entry array, returns new length of array. Does nothing if new_entry_len <= current entry count
METHOD
i32 runetab_resize(RuneTable* self, i32 new_entry_len);

METHOD
Rune runetab_add(RuneTable* self, sslice name);

PURE_FUNC
METHOD
bool runetab_has(const RuneTable* self, Rune rune);

PURE_FUNC
METHOD
bool runetab_has_str(const RuneTable* self, const char* string, i32 string_len);

METHOD
PURE_FUNC
Rune runetab_lookup_rune(const RuneTable* self, sslice name);

METHOD
PURE_FUNC
sslice runetab_lookup(const RuneTable* self, Rune rune);

METHOD
void runetab_destroy(RuneTable* self);

PURE_FUNC static inline bool rune_strongeq(Rune left, Rune right) {
  if (is_null(left.parent) || is_null(right.parent) || !rune_eq(left, right)) {
    return false;
  }
  const sslice l = runetab_lookup(left.parent, left);
  const sslice r = runetab_lookup(right.parent, right);
  return sslice_eq(l, r);
}
