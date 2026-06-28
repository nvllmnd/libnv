#pragma once

// #include "nv/core/algo.h"
// #include "nv/core/sslice.h"
// struct StringKey {
//   u64 hash;
//   sslice name;
// };
// typedef struct StringKey StringKey;

// static constexpr const StringKey SKEY_NONE = zeroed(StringKey);

// RETURNS_ERROR
// ZError runetab_init(i32 entry_len, i32 storage_size);

// /// Resizes entry array, returns new length of array. Does nothing if new_entry_len <= current entry count
// i32 runetab_grow(i32 new_entry_len);

// Rune runetab_add(sslice name);

// PURE_FUNC
// bool runetab_has(Rune rune);

// PURE_FUNC
// bool runetab_has_str(const char* string, i32 string_len);

// PURE_FUNC
// Rune runetab_lookup(sslice name);

// /// looks up @param (sslice name) in global RuneTable,
// /// If name exists in global RuneTable, @param (Rune* out) is filled in with the
// /// Rune data pointing to the entry in the global RuneTable for that name.
// /// If @param (Rune* out) is nullptr, this parameter is ignored.
// /// @returns true if name exists in global RuneTable, false otherwise.
// /// @param (Rune* out) is only touched when name exists in global RuneTable AND that parameter is non-null.
// /// as such, it is safe to pass nullptr as this function second parameter, and it functions exaclty as if you were to
// /// call [runetab_has_str] with @param (sslice name), contents as its parameters
// bool runetab_get(sslice name, Rune* out);

// void runetab_destroy(void);

// void runetab_clear(void);

// void runetab_print_entries(void);
