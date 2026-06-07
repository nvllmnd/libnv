#pragma once
#include "nv/core/attributes.h"
#include "nv/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/error.h"

struct FileChunk {
  const char* name;
  i64 offset;
  i64 size;
  u8* begin;
  u8* end; 
};
alias(FileChunk);

struct FileChunkList {
  i64 capacity;
  i64 len;

  ATTR_COUNTED_BY(capacity)
  FileChunk data[];
};

alias(FileChunkList);


struct FileMap {
  i64 capacity;
  /// @brief total of mapped files currently loaded into storage
  i64 count;

  FileChunkList* list;
  u8* storage_begin;
  u8* storage_end;

  ATTR_COUNTED_BY(capacity)
  u8 storage[];

};
alias(FileMap);
  

/// @brief initializes ((max_entries * sizeof(FileChunk)) + sizeof(FileChunkList)  + capacity_bytes) bytes
METHOD
RETURNS_ERROR
NvError fmap_init(FileMap** self, i64 max_entries, i64 capacity_bytes);

/// @brief reads file at filepath into this FileMap
METHOD
const char* fmap_load_file_const(FileMap* self, const char* filepath, i32 pathlen); 

METHOD
char* fmap_load_file(FileMap* self, const char* filepath, i32 pathlen);












