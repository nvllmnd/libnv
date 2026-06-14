#pragma once

#include "nv/common.h"

#define PBlockData \
  i64 id;          \
  i64 next;        \
  bool alive

struct PBlockChunk {
  PBlockData;
  byte data[];
};
alias(PBlockChunk);

#define PBLOCK_HEADER_SIZE sizeof(__typeof(struct { PBlockData; }))

#define PBlock(T)     \
  struct {            \
    PBlockData;       \
    __typeof(T) data; \
  }

#define pb_sizeof(T) (sizeof(__typeof(T)) + sizeof(PBlockChunk))
#define pb_typeof(_pb) typeof_field(PBlock(__typeof((_pb))), data)

#define pb_as_bytes(_pb)                                                                            \
  ({                                                                                                \
    static_assert(sizeof(__typeof(*(_pb))) == sizeof(PBlockChunk) + sizeof(__typeof((_pb)->data))); \
    ((PBlockChunk*)((void*)(_pb)));                                                                 \
  })

#define MemPool(T)              \
  struct {                      \
    PBlock(__typeof(T)) * pool; \
    i64 next_avail;             \
    i64 capacity;               \
    i64 live_count;             \
    i64 dead_count;             \
  }

static constexpr const MemPool(byte) MPOOL_EMPTY = {};

#define mpool_is_none(_mp) (memcmp((_mp), &MPOOL_EMPTY, sizeof(MPOOL_EMPTY)) == 0)

#define mpool_is_ok(_mp) (is_not_null((_mp).pool))

#define mpool_new(T, _begin, _end)                                                                          \
  ({                                                                                                        \
    static_assert(sizeof(__typeof(*(_begin))) == sizeof(byte) && sizeof(__typeof(*(_end))) == sizeof(byte), \
                  "begin and end pointers for initializing MemPool must be pointers to bytes!");            \
    const i64 _size = (_end) - (_begin);                                                                    \
    MemPool(T) _mp = {};                                                                                    \
    if (_size > (i64)PBLOCK_HEADER_SIZE) {                                                                  \
      const i64 _cap = _size / (pb_sizeof(T));                                                              \
      auto _pool = (typeof_field(MemPool(T), pool))(void*)(_begin);                                         \
      _mp.capacity = _cap;                                                                                  \
      _mp.pool = _pool;                                                                                     \
      for (i32 i = 0; i < _cap; i++) {                                                                      \
        auto _ptr = &_mp.pool[i];                                                                           \
        _ptr->alive = false;                                                                                \
        _ptr->id = i;                                                                                       \
        const i64 _next = i + 1 < _cap ? i + 1 : -1;                                                        \
        _ptr->next = _next;                                                                                 \
      }                                                                                                     \
      _mp.next_avail = 0;                                                                                   \
      _mp.live_count = 0;                                                                                   \
      _mp.dead_count = _cap;                                                                                \
    }                                                                                                       \
    _mp;                                                                                                    \
  })

#define mpool_type(_mp) __typeof(pb_typeof((_mp).pool))

#define MPOOL_TRACK_FREE(_self) \
  do {                          \
    _self->live_count -= 1;     \
    _self->dead_count += 1;     \
  } while (0)

#define mpool_end(_mp)             \
  ({                               \
    auto _self = (_mp);            \
    (_self.pool + _self.capacity); \
  })

#define mpool_contains(_mp, _p)              \
  ({                                         \
    const auto _self = (_mp);                \
    const auto _ptr = (byte*)(_p);           \
    const auto _begin = (byte*)_self.pool;   \
    const auto _end = (byte*)mpool_end(_mp); \
    _ptr >= _begin && _ptr < _end;            \
  })

#define MPOOL_TRACK_ALLOC(_self) \
  do {                           \
    _self->live_count += 1;      \
    _self->dead_count -= 1;      \
  } while (0)

#define mpool_free(_mp, _elem)                                                         \
  ({                                                                                   \
    auto _self = (_mp);                                                                \
    if (is_not_null((_elem))) {                                                        \
      auto _pblock = ((PBlock(__typeof((_elem)))*)((byte*)(_elem) - PBLOCK_HEADER_SIZE)); \
                                                                                       \
      if (mpool_contains(*_mp, _pblock)) {                                                 \
        _pblock->alive = false;                                                           \
        _pblock->next = _self->next_avail;                                                \
        _self->next_avail = _pblock->id;                                                  \
        MPOOL_TRACK_FREE(_self);                                                       \
      }                                                                                \
    }                                                                                  \
  })

#define mpool_allocate(_mp)                                \
  ({                                                       \
    auto _self = (_mp);                                    \
    __typeof(_self->pool->data)* _res = nullptr;           \
    if (_self->next_avail >= 0 && _self->dead_count > 0) { \
      auto _ptr = &_self->pool[_self->next_avail];         \
      _ptr->alive = true;                                  \
      _self->next_avail = _ptr->next;                      \
      _res = &_ptr->data;                                  \
      MPOOL_TRACK_ALLOC(_self);                            \
    }                                                      \
    _res;                                                  \
  })

#define mpool_zallocate(_mp)                     \
  ({                                             \
    auto _ptr = mpool_allocate(_mp);             \
    if (is_not_null(_ptr)) {                     \
      memset(_ptr, 0, sizeof(mpool_type(*_mp))); \
    }                                            \
    _ptr;                                        \
  })

typedef MemPool(byte) MemPoolByte;
typedef MemPool(char) MemPoolChar;
typedef MemPool(i16) MemPoolInt16;
typedef MemPool(u16) MemPoolUInt16;
typedef MemPool(i32) MemPoolInt32;
typedef MemPool(u32) MemPoolUInt32;
typedef MemPool(i64) MemPoolInt64;
typedef MemPool(u64) MemPoolUInt64;
typedef MemPool(f32) MemPoolFloat32;
typedef MemPool(f64) MemPoolFloat64;
