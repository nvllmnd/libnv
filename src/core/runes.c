#include "core/runes.h"

#include <stdint.h>
#include <string.h>

#include "algo.h"
#include "attributes.h"
#include "core/buffer.h"
#include "core/log.h"
#include "core_types.h"
#include "intdefs.h"
#include "memory/alloc.h"
#include "memory/layout.h"
#include "sslice.h"

typedef enum RuneType { Rune__Empty = 0, Rune__Inline, Rune__Large, Rune__TypeCount } RuneType;

static constexpr const i32 RUNE_INLINE_MAX = 32;

typedef i16 PfixSize;

struct PrefixRune {
  PfixSize len;
  char name[];
};
alias(PrefixRune);

#define asprune(_ptr) prefix_offset(_ptr, PrefixRune)

static constexpr const i32 RUNE_MIN_SIZE = sizeof(PrefixRune);

static constexpr const f32 LOAD_FACTOR = 0.75;

METHOD
static inline const char* prune_string(const PrefixRune* self) {
  assert(self);
  assert(self->len >= RUNE_MIN_SIZE);
  return &self->name[0];
}

i32 rune_min_size(void) { return RUNE_MIN_SIZE; }

struct RuneEntry {
  RuneType type;

  union {
    struct RuneInline {
      /// NOTE: same size of [PfixRune], to keep consistent with other prefix strings, and allow for doing a
      /// prefix length lookup on any const char* pointer in this RuneTable
      PfixSize len;
      char name[RUNE_INLINE_MAX + 1];  // + 1 for null terminal
    } inl;
    struct LargeRuneSlice {
      PfixSize len;
      /// byte index to start of prefixed rune
      i32 slot;
    } lrune;
  };

  u64 hash;
};
alias(RuneEntry);
alias(RuneInline);
alias(LargeRuneSlice);

static_assert(sizeof(PrefixRune) == sizeof_field(RuneInline, len),
              "Size of RuneInline metadata is not the same size of PrefixRune! This could cause issues if "
              "runetab_lookup_prune on an entry that is not of type Rune__Large!");

static void runetab_rehash_entries(RuneTable* self, const Vec(RuneEntry) old_entries, const char* old_prunes,
                                   i32 old_prunes_len);

METHOD
PURE_FUNC
static inline bool rentry_is_empty(const RuneEntry* self) {
  assert(self);
  return self->type == Rune__Empty;
}

// NOTE: Yes this struct is tiny, and yes we are fine with allocating it as an opaque pointer. Most allocators own or
// allocate into contiguous virtual memory, so the cost of allocations are cheap, so allocating this tiny guy is ok. If
// we are really obsessed with this tiny struct residing in memory that isnt dynamicall allocated/managed, then use
// StaticAlloc to allocate the oqaque pointer plus Since we allocate into contiguous virtual memory, there is a great
// chance this RuneTable struct will live right next to the entries it contains in memory
struct RuneTable {
  // struct {
  //   Vec(RuneEntry) elems;

  //   /// Cache entry length so we dont have to deref entries vec pointer to get length
  //   i32 len;

  //   /// Cache entry capacity so we dont have to deref entries vec pointer to get capacity
  //   i32 cap;
  // } entries;

  Vec(RuneEntry) entries;
  i32 entries_count;
  struct {
    char* elems;

    /// number of prefixed large runes in table
    i32 count;
    /// capacity of array used (for prefixed large runes) in bytes
    i32 cap;
    /// length of array used (for prefixed large runes) in bytes
    i32 len;
  } prunes;
};

// static const char* RUNETYPE_STRINGS[Rune__TypeCount] = {
//   STRINGIFY(Rune__Empty) " :: Rune is empty and not currently in use.",
//   STRINGIFY(Rune__Inline) " :: Rune small enough to fit inside of a RuneEntry, as opposed to living in a separate buffer for larger runes.",
//   STRINGIFY(Rune__Large) " :: Rune too large to fit inline inside of a RuneEntry, and lives in a separate buffer for larger runes." };

// static inline const PrefixRune* runetab_lookup_prune(const RuneTable* self, const RuneEntry* entry) {
//   assert(self);
//   assert(entry);
//   if LIKELY (entry->type == Rune__Large) {
//     const i32 slot = entry->lrune.slot;

//     assert(slot < self->prunes.len);
//     return pcast(PrefixRune, &self->prunes.elems[slot]);
//   }
//   LOG_DBG(FILE_FMT
//           "Looking up PrefixRune with a RuneEntry that is of type %s. Returned pointer appears to point to a %s "
//           "struct, when its actually a %s struct. This is most likely fine.",
//           FILE_FMT_ARGS(RuneTable, (entry->type == Rune__Inline ? RUNETYPE_STRINGS[entry->type] : "UNKNOWN RUNETYPE")),
//           STRINGIFY(PrefixRune), STRINGIFY(RuneInline));
//   return pcast(PrefixRune, &entry->inl.name[0]);
// }

// static inline const char* runetab_entry_string(const RuneTable* self, const RuneEntry* entry) {
//   assert(self);
//   assert(entry);
//   if (entry->type == Rune__Inline) {
//     return &entry->inl.name[0];
//   }
//   assert(entry->lrune.slot < self->prunes.len);
//   const auto prune = (const PrefixRune*)&self->prunes.elems[entry->lrune.slot];
//   return &prune->name[0];
// }

// static inline i32 runetab_entry_len(const RuneTable* self, const RuneEntry* entry) {
//   assert(self);
//   assert(entry);
//   if (entry->type == Rune__Inline) {
//     return entry->inl.len;
//   }

//   assert(entry->lrune.slot < self->prunes.len);

//   const i32 len = entry->lrune.len;
//   const PrefixRune* prune = (const PrefixRune*)&self->prunes.elems[entry->lrune.slot];

//   if UNLIKELY (prune->len != len) {
//     LOG_DBG("Cached Prefix Rune Length out of sync with Length in PrefixRune memory! Cached: %d, Actual: %d", len,
//             prune->len);
//     return prune->len;
//   }

//   return len;
// }

PARAMS_NONNULL(1)
static inline void runetab_cleanup(RuneTable** self, Allocator alloc) {
  assert(self);

  RuneTable* const s = *self;
  if LIKELY (is_not_null(s)) {
    if LIKELY (is_not_null(s->entries)) {
      allocator_free(alloc, s->entries);
      s->entries = nullptr;
    }
    if LIKELY (is_not_null(s->prunes.elems)) {
      allocator_free(alloc, s->prunes.elems);
      s->prunes.elems = nullptr;
    }
    allocator_free(alloc, s);
    *self = nullptr;
  }
}

RuneTable* runetab_new(i32 capacity, Allocator alloc) {
  assert(capacity > 0);
  assert(allocator_is_ok(alloc));

  RuneTable* self = allocator_allocate(alloc, mlayout_new(RuneTable));
  if UNLIKELY (is_null(self)) {
    LOG_DBG(FILE_FMT
            "Given allocator failed to allocate RuneTable header of size: %d bytes. Ensure that allocator has enough "
            "space before trying to create a new RuneTable with it!",
            FILE_FMT_ARGS(RuneTable, (i32)sizeof(RuneTable)));

    assert(self);

    return nullptr;
  }
  self->entries = nullptr;
  self->prunes.elems = nullptr;

  const i32 entry_cap_bytes = sizeof(RuneEntry) * capacity;
  Vec(RuneEntry) const entries = vec_new(RuneEntry, capacity, alloc);

  if UNLIKELY (is_null(entries)) {
    LOG_DBG(FILE_FMT
            "Given allocator failed to allocate RuneEntries with capacity of size: %d bytes. Ensure that allocator has "
            "enough "
            "space before trying to resize or create a new RuneEntry Vec with it!",
            FILE_FMT_ARGS(RuneTable, entry_cap_bytes));
    runetab_cleanup(&self, alloc);

    assert(entries);

    return nullptr;
  }

  self->entries = entries;

  const i32 lrunes_cap = capacity * RUNE_INLINE_MAX;
  const MemLayout lrunes_layout = make(MemLayout, .size = lrunes_cap, .align = 1);

  char* const lrunes = allocator_allocate(alloc, lrunes_layout);

  if UNLIKELY (is_null(lrunes)) {
    LOG_DBG(FILE_FMT
            "Given allocator failed to allocate LargeRune Entries with capacity of size: %d bytes. Ensure that "
            "allocator has enough "
            "space before trying to resize or create a buffer for PrefixRunes!",
            FILE_FMT_ARGS(RuneTable, (i32)lrunes_layout.size));

    runetab_cleanup(&self, alloc);

    assert(lrunes);
    return nullptr;
  }

  self->prunes.elems = lrunes;
  self->prunes.count = 0;
  self->prunes.len = 0;
  self->prunes.cap = lrunes_layout.size;

  return self;
}

RuneTable* runetab_resize(RuneTable* self, i32 new_capacity, Allocator alloc) {
  assert(self);
  assert(new_capacity > 0);
  assert(allocator_is_ok(alloc));

  const i32 curr_cap = vec_capacity(self->entries);
  // dont do anything for shrinking, as that would cause us to have to rehash everything, so shrinking is not
  // desireable
  if (new_capacity <= curr_cap) {
    return self;
  }

  Vec(RuneEntry) const old_entries = self->entries;

  char* const old_prunes = self->prunes.elems;

  const i32 old_prunes_len = self->prunes.len;

  assert(old_entries);
  assert(old_prunes);

  // NOTE: Reallocate the RuneTable header too, so that it resides next to its entries in memory (instead of next to the
  // first entries, at worst case where allocator_free is a noop)

  RuneTable* next_self = allocator_allocate(alloc, mlayout_new(RuneTable));

  if UNLIKELY (is_null(next_self)) {
    LOG_DBG(FILE_FMT
            "Given allocator failed to allocate RuneTable header of size: %d bytes. Ensure that allocator has enough "
            "space before trying to resize it!",
            FILE_FMT_ARGS(RuneTable, (i32)sizeof(RuneTable)));

    return nullptr;
  }

  const i32 entries_new_cap_bytes = sizeof(RuneEntry) * new_capacity;

  Vec(RuneEntry) entries = vec_new(RuneEntry, new_capacity, alloc);

  // sanity check!
  assert(vec_capacity(entries) == entries_new_cap_bytes);

  // make sure entries are zeroed so we can properly pick up Rune__Empty entries when rehashing
  vec_clear_zeroed_cap(entries);
  vec_grow_to_cap(entries);

  if UNLIKELY (is_null(entries)) {
    LOG_DBG(FILE_FMT
            "Given allocator failed to allocate RuneEntries with capacity of size: %d bytes. Ensure that allocator has "
            "enough "
            "space before trying to resize or create a new RuneEntry Vec with it!",
            FILE_FMT_ARGS(RuneTable, entries_new_cap_bytes));

    runetab_cleanup(&next_self, alloc);

    return nullptr;
  }

  next_self->entries = entries;
  next_self->entries_count = self->entries_count;

  const i32 prunes_new_cap = old_prunes_len * (new_capacity / 4);
  char* prunes = allocator_allocate(alloc, make(MemLayout, .size = prunes_new_cap, .align = 1));

  if UNLIKELY (is_null(prunes)) {
    LOG_DBG(FILE_FMT
            "Given allocator failed to allocate LargeRune Entries with capacity of size: %d bytes. Ensure that "
            "allocator has enough "
            "space before trying to resize or create a buffer for PrefixRunes!",
            FILE_FMT_ARGS(RuneTable, (i32)prunes_new_cap));

    runetab_cleanup(&next_self, alloc);

    return nullptr;
  }

  runetab_rehash_entries(next_self, old_entries, old_prunes, old_prunes_len);

  memcpy(prunes, old_prunes, old_prunes_len);

  next_self->prunes.elems = prunes;
  next_self->prunes.count = self->prunes.count;

  allocator_free(alloc, old_entries);
  allocator_free(alloc, old_prunes);
  allocator_free(alloc, self);

  next_self->prunes.cap = prunes_new_cap;
  next_self->prunes.len = old_prunes_len;

  return next_self;
}

f32 runetab_load_factor(const RuneTable* self) {

  assert(self);
  assert(self->entries);

  const i32 len = vec_len(self->entries);
  const f32 count = self->entries_count == 0 ? 1. : (f32)self->entries_count;
    
  return len / count;

}

METHOD
Rune runetab_add(RuneTable* self, sslice name) {
  const u64 hash = fnv_hash64(name.begin, name.len);
  const i32 len = vec_len(self->entries);
  const u64 mask = len - 1;

  const i32 index = cast(i32, hash & mask);
  RuneEntry* const start_entry = &self->entries[index];
  RuneEntry* entry = start_entry;

  const f32 load = runetab_load_factor(self);

  if (load >= LOAD_FACTOR) {
    LOG_DBG(FILE_FMT
            " :: Failed to add entry: %.*s. Load factor too high! resize Table in order to add more entries! Current "
            "load factor: %f",
            FILE_FMT_ARGS(RuneTable, name.len, name.begin, load));

    return RUNE_NONE;
  }

  i32 i = index;

  while (!rentry_is_empty(entry)) {
    entry++;
    i++;
    if (i >= len || entry > (&self->entries[len])) {
      entry = &self->entries[0];
      i = 0;
      continue;
    }
    if (entry == start_entry) {
      LOG_DBG(FILE_FMT " :: Failed to add entry: %.*s", FILE_FMT_ARGS(RuneTable, name.len, name.begin));
      return RUNE_NONE;
    }
  }

  assert(entry->type == Rune__Empty);

  if (name.len <= RUNE_INLINE_MAX) {
    entry->type = Rune__Inline;
    strncpy(entry->inl.name, name.begin, name.len);
    entry->inl.name[RUNE_INLINE_MAX] = '\0';
    entry->inl.len = name.len;
  } else {
    entry->type = Rune__Large;
    entry->lrune.slot = i;
    entry->lrune.len = name.len;
  }
  self->entries_count += 1;

  return make(Rune, .id = i, .parent = self);
}

PURE_FUNC
METHOD
bool runetab_has(const RuneTable* self, Rune rune) {
  if (rune.parent != self) {
    return false;
  }
  const i32 elen = vec_len(self->entries);
  if (rune.id < 0 || rune.id >= elen) {
    return false;
  }
  const RuneEntry* entry = &self->entries[rune.id];

  return !rentry_is_empty(entry);
}

PURE_FUNC
METHOD
bool runetab_has_str(const RuneTable* self, const char* string, i32 string_len) {
  assert(self);
  assert(string);
  assert(string_len > 0);
  const sslice name = sslice_new(.begin = string, .len = string_len);
  const Rune rune = runetab_lookup_rune(self, name);
  if (rune_is_none(rune)) {
    return false;
  }

  return runetab_has(self, rune);
}

static inline sslice entry_slice(const RuneTable* table, const RuneEntry* entry) {
  if (entry->type == Rune__Inline) {
    return sslice_new(.begin = entry->inl.name, .len = entry->inl.len);
  } else if (entry->type == Rune__Large) {
    const PrefixRune* prune = (const PrefixRune*)&table->prunes.elems[entry->lrune.slot];
    return sslice_new(prune_string(prune), .len = prune->len);
  }
  return sslice_empty();
}

METHOD
PURE_FUNC
Rune runetab_lookup_rune(const RuneTable* self, sslice name) {
  const u64 hash = fnv_hash64(name.begin, name.len);
  const i32 elen = vec_len(self->entries);
  const u64 mask = elen - 1;

  const i32 index = cast(i32, hash & mask);

  // const RuneEntry* entry = &self->entries[index];
  // if (entry->type > Rune__Empty && entry->hash == hash) {
  //   return make(Rune, .id = index, .parent = self);
  // }

  for (i32 i = clamp(index, 0, elen - 1); i < elen; i++) {
    const RuneEntry* entry = &self->entries[i];

    entry = &self->entries[i];

    if (entry->type == Rune__Empty) {
      return RUNE_NONE;
    }

    if (entry->type > Rune__Empty && entry->hash == hash) {
      const sslice eslice = entry_slice(self, entry);
      if LIKELY (sslice_eq(eslice, name)) {
        return make(Rune, .id = i, .parent = self);
      }
      LOG_DBG("Looking up %.*s in RuneTable matches entry hash, but not the entry string value! %.*s", name.len,
              name.begin, eslice.len, eslice.begin);
      continue;
    }

    // we looped all the way around, but havent found this entry, this should only happen if given name does not exist
    // in this table
    if (i == index) {
      return RUNE_NONE;
    }

    if (i + 1 >= elen) {
      i = 0;
    }
  }

  return RUNE_NONE;
}

METHOD
PURE_FUNC
const char* runetab_lookup(const RuneTable* self, Rune rune);

METHOD
PURE_FUNC
sslice runetab_lookup_slice(const RuneTable* self, Rune rune);

METHOD
void runetab_destroy(RuneTable* self, Allocator alloc);

void runetab_rehash_entries(RuneTable* self, const Vec(RuneEntry) old_entries, const char* old_prunes,
                            i32 old_prunes_len) {
  Vec(RuneEntry) entries = self->entries;

  const i32 entries_cap = vec_capacity(entries);
  const u64 mask = entries_cap - 1;

  vec_for(old_entries) {
    const RuneEntry entry = old_entries[i];

    u64 hash = 0;
    const char* ename = nullptr;
    i32 elen = -1;

    if (entry.type == Rune__Inline) {
      ename = entry.inl.name;
      elen = entry.inl.len;
    } else {
      assert(entry.type == Rune__Large);
      assert(entry.lrune.slot < old_prunes_len);
      const PrefixRune* pfix = pcast(const PrefixRune, &old_prunes[entry.lrune.slot]);
      const char* pstring = prune_string(pfix);
      ename = pstring;
      elen = pfix->len;

      // hash = fnv_hash64(pstring, pfix->len);
    }

    // sanity checks
    assert(hash != 0);
    assert(ename);
    assert(elen >= 0);

    hash = fnv_hash64(ename, elen);

    const i32 index = (hash & mask);
    assert(index < entries_cap);

    {
      RuneEntry* next = &entries[index];
      if LIKELY (next->type == Rune__Empty) {
        *next = entry;
        continue;
      }

      LOG_DBG(FILE_FMT
              "Hash collision for entry: %.*s. Starting linear probe! If this is happening often (or really any more "
              "than very infrequently) then you may want to increase the capacity of that RuneTable's RuneEntry Vec!",
              FILE_FMT_ARGS(RuneTable, elen, ename));
      // linear probe through Entries to find an empty cell.
      // this should only happen rarely (if at all) in case of hash collision
      const i32 elen = vec_len(entries);
      for (i32 j = clamp(i + 1, 0, elen - 1); j < elen; j++) {
        if (j + 1 >= elen) {
          j = 0;
        }

        next = &entries[j];
        if (next->type == Rune__Empty) {
          LOG_DBG(FILE_FMT "Found Hash Collision Entry using linear probing at index: %d", FILE_FMT_ARGS(RuneTable, j));
          *next = entry;
          break;
        }

        // we have wrapped full around and not found an entry. This should never happen!
        if UNLIKELY (j == i) {
          log_fatal(FILE_FMT "Could not find a free entry in RuneTable while trying to rehash entries during resize!",
                    FILE_FMT_ARGS());
        }
      }
    }
  }
}
