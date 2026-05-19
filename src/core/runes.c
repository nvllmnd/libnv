#include "nv/core/runes.h"

#include <stdint.h>

#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/buffer.h"
#include "nv/core/constants.h"
#include "nv/core/log.h"
#include "nv/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/arena.h"
#include "nv/memory/layout.h"
#include "nv/core/sslice.h"

typedef enum RuneType { Rune__Empty = 0, Rune__Used, Rune__TypeCount } RuneType;

static constexpr const f32 LOAD_FACTOR = 0.75;

struct RuneEntry {
  RuneType type;
  sslice name;

  u64 hash;
};
alias(RuneEntry);

static void runetab_rehash_entries(RuneTable* self, const Vec(RuneEntry) old_entries);

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
  Vec(RuneEntry) entries;
  i32 entries_count;

  Arena* alloc;
};

RuneTable* runetab_new(i32 entry_len, i32 name_storage_in_mb) {
  assert(entry_len > 0);
  assert(name_storage_in_mb > 2);

  Arena* arena = arena_new(name_storage_in_mb, KILOBYTES(1));
  if UNLIKELY (is_null(arena)) {
    LOG_DBG(FILE_FMT " :: Failed to create new Arena of size %dMB and init capacity: %d",
            FILE_FMT_ARGS(RuneTable, name_storage_in_mb, KILOBYTES(1)));
    return nullptr;
  }

  RuneTable* self = arena_alloc(arena, mlayout_new(RuneTable));
  if UNLIKELY (is_null(self)) {
    LOG_DBG(FILE_FMT "Arena Allocator failed to allocate RuneTable of size: %d bytes!",
            FILE_FMT_ARGS(RuneTable, (i32)sizeof(RuneTable)));

    arena_destroy(arena);
    assert(self);

    return nullptr;
  }

  self->alloc = arena;
  self->entries = vec_new(RuneEntry, entry_len, arena_allocator(self->alloc));
  self->entries_count = 0;

  if UNLIKELY (is_null(self->entries)) {
    LOG_DBG(FILE_FMT " :: Arena allocator failed to allocate RuneEntry array of size: %d bytes!",
            FILE_FMT_ARGS(RuneTable, (i32)sizeof(RuneEntry) * entry_len));
    arena_destroy(self->alloc);

    assert(self->entries);

    return nullptr;
  }

  return self;
}

i32 runetab_resize(RuneTable* self, i32 new_entry_len) {
  assert(self);
  assert(new_entry_len > 0);

  const i32 curr_cap = vec_capacity(self->entries);
  // dont do anything for shrinking, as that would cause us to have to rehash everything, so shrinking is not
  // desireable
  if (new_entry_len <= curr_cap) {
    return self->entries_count;
  }

  Vec(RuneEntry) const old_entries = self->entries;

  // NOTE: Reallocate the RuneTable header too, so that it resides next to its entries in memory (instead of next to the
  // first entries, at worst case where allocator_free is a noop)

  const f32 next_load_factor = (f32)new_entry_len / self->entries_count;

  // make sure next load factor is large enough, otherwise this resize is pretty useless!
  if (next_load_factor > LOAD_FACTOR) {
    new_entry_len *= 4;
  }

  self->entries = vec_new(RuneEntry, new_entry_len, arena_allocator(self->alloc));

  if UNLIKELY (is_null(self->entries)) {
    LOG_DBG(FILE_FMT " :: Unable to resize RuneTable to %d entries!",
            FILE_FMT_ARGS(RuneTable, new_entry_len));


    return self->entries_count;
  }

  // make sure entries are zeroed so we can properly pick up Rune__Empty entries when rehashing
  vec_clear_zeroed_cap(self->entries);
  vec_grow_to_cap(self->entries);


  runetab_rehash_entries(self, old_entries);


  return new_entry_len;
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
  assert(self);
  assert(name.begin);
  assert(name.len > 0);

  const f32 load = runetab_load_factor(self);

  if (load >= LOAD_FACTOR) {
    LOG_DBG(FILE_FMT
            " :: Failed to add entry: %.*s. Load factor too high! resize Table in order to add more entries! Current "
            "load factor: %f",
            FILE_FMT_ARGS(RuneTable, name.len, name.begin, load));

    return RUNE_NONE;
  }

  const Rune r = runetab_lookup_rune(self, name);
  if (rune_is_ok(r)) {
    return r;
  }

  const u64 hash = fnv_hash64(name.begin, name.len);
  const i32 len = vec_len(self->entries);
  const u64 mask = len - 1;

  const i32 index = cast(i32, hash & mask);

  RuneEntry* entry = &self->entries[index];

  for (i32 i = clamp(index + 1, 0, len - 1); i < len; i++) {
    entry = &self->entries[i];

    // we only have to worry about insertion here, since we do a full lookup above with the call to runetab_lookup_rune
    // so we know if we got here, we did not find an entry at this hashed index

    if (entry->type == Rune__Empty) {
      const char* s = arena_strndup(self->alloc, name.begin, name.len);
      const sslice sl = sslice_new(s, name.len);
      entry->name = sl;
      entry->type = Rune__Used;
      entry->hash = hash;
      self->entries_count += 1;
      return make(Rune, .id = i, .parent = self);
    }

    if UNLIKELY (i == index) {
      return RUNE_NONE;
    }

    if (i + 1 >= len) {
      i = 0;
    }
  }

  LOG_DBG("Something went wrong when trying to add rune: %.*s to RuneTable!", name.len, name.begin);
  assert(false);

  UNREACHABLE();
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
  return !rune_is_none(rune);

}

METHOD
PURE_FUNC
Rune runetab_lookup_rune(const RuneTable* self, sslice name) {
  const u64 hash = fnv_hash64(name.begin, name.len);
  const i32 elen = vec_len(self->entries);
  const u64 mask = elen - 1;

  const i32 index = cast(i32, hash & mask);

  for (i32 i = clamp(index, 0, elen - 1); i < elen; i++) {
    const RuneEntry* entry = &self->entries[i];

    entry = &self->entries[i];

    if (entry->type == Rune__Empty) {
      return RUNE_NONE;
    }

    if (entry->type > Rune__Empty && entry->hash == hash) {
      if LIKELY (sslice_eq(entry->name, name)) {
        return make(Rune, .id = i, .parent = self);
      }
      LOG_DBG("Looking up %.*s in RuneTable matches entry hash, but not the entry string value! %.*s", name.len,
              name.begin, entry->name.len, entry->name.begin);
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
sslice runetab_lookup(const RuneTable* self, Rune rune) {
  assert(self);
  assert(self->entries);
  assert(rune.id != RUNE_NONE.id || self->entries_count == 0);
  if (rune.parent != self) {
    return sslice_empty();
  }
  assert(rune.id < vec_len(self->entries));
  const RuneEntry entry = self->entries[rune.id];
  if (entry.type == Rune__Empty) {
    return sslice_empty();
  }
  return entry.name;
}

METHOD
void runetab_destroy(RuneTable* self) {
  assert(self);
  assert(self->entries);
  arena_destroy(self->alloc);
}

void runetab_rehash_entries(RuneTable* self, const Vec(RuneEntry) old_entries) {
  Vec(RuneEntry) entries = self->entries;

  const i32 entries_len = vec_len(entries);
  const u64 mask = entries_len - 1;

  vec_for(old_entries) {
    const RuneEntry entry = old_entries[i];

    const u64 hash = entry.hash;

    const i32 index = (hash & mask);
    assert(index < entries_len);

    {
      RuneEntry* next = &entries[index];
      if LIKELY (next->type == Rune__Empty) {
        *next = entry;
        continue;
      }

      LOG_DBG(FILE_FMT
              "Hash collision for entry: %.*s. Starting linear probe! If this is happening often (or really any more "
              "than very infrequently) then you may want to increase the capacity of that RuneTable's RuneEntry Vec!",
              FILE_FMT_ARGS(RuneTable, entry.name.len, entry.name.begin));

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
