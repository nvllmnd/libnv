#pragma once


#include "nv/core/sslice.h"
#include "nv/core_types.h"
#include "nv/memory/error.h"
struct Rune {
  u64 hash;
  sslice name; 
};
alias(Rune);

static constexpr const Rune RUNE_NONE = (Rune){};

PURE_FUNC
static inline bool rune_is_none(Rune self) { return memcmp(&self, &RUNE_NONE, sizeof(Rune)) == 0; }

PURE_FUNC
static inline bool rune_is_ok(Rune self) { return !rune_is_none(self); }

PURE_FUNC
static inline bool rune_eq(Rune lhs, Rune rhs) {
  return lhs.hash == rhs.hash && sslice_eq(lhs.name, rhs.name);
}


typedef struct RuneSet RuneSet;

RETURNS_ERROR
METHOD
NvError rset_init(RuneSet** self, i32 entry_len, i32 storage_size);

PURE_FUNC
METHOD
f32 rset_load_factor(const RuneSet* self);


METHOD
i32 rset_grow(RuneSet* self, i32 new_entry_len);

METHOD
Rune rset_add(RuneSet* self, sslice name);

PURE_FUNC
METHOD
bool rset_has(const RuneSet* self, Rune rune);

PURE_FUNC
METHOD
bool rset_has_str(const RuneSet* self, const char* string, i32 string_len);

PURE_FUNC
METHOD
Rune rset_lookup(const RuneSet* self, sslice name);

/// looks up @param (sslice name) in global RuneTable,
/// If name exists in global RuneTable, @param (Rune* out) is filled in with the
/// Rune data pointing to the entry in the global RuneTable for that name.
/// If @param (Rune* out) is nullptr, this parameter is ignored.
/// @returns true if name exists in global RuneTable, false otherwise.
/// @param (Rune* out) is only touched when name exists in global RuneTable AND that parameter is non-null.
/// as such, it is safe to pass nullptr as this function second parameter, and it functions exaclty as if you were to
/// call [runetab_has_str] with @param (sslice name), contents as its parameters
bool rset_get(const RuneSet* self, sslice name, Rune* out);

void rset_destroy(RuneSet* self);

void rset_clear(RuneSet* self);

void rset_print_entries(RuneSet* self);
