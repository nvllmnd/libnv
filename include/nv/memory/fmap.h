#pragma once
#include "nv/core/attributes.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core/sslice.h"
#include "nv/core_types.h"
#include "nv/iter/array.h"
#include "nv/memory/alloc.h"

/// @brief based off the assumption that most hardware is using a 4k page size.
static constexpr const i64 FILEMAP_MIN_SIZE = KB(4);


/// @brief byte offset to the location of a loaded file in resulting memory mapped file [FileMap]
typedef i64 FMapChunkAddr;

struct FMapTable {
  /// @brief Number of loaded files
  i64 count;
  /// @brief cached size of offsets (count * sizeof(i64))
  i64 size_bytes;

  ATTR_COUNTED_BY(size_bytes)
  FMapChunkAddr offsets[];
};
alias(FMapTable);

typedef enum FMapFlags  {
    /// @brief resulting memory mapped file is readable
    FMap__Read = 1,
    /// @brief resulting memory mapped file is writable
    FMap__Write = 1 << 1,
    /// @brief resulting memory mapped file is readable AND writable
    FMap__ReadWrite = FMap__Read | FMap__Write,
    /// @brief Merge each file into a single, non-delimited memory mapped file
    /// @details mutually exclusive with [FMap__DelimEach] and [FMap__TrackEach]
    FMap__ConcatAsOne = 1 << 4,
    /// @brief use a caller defined delimiter between each file loaded into the single resulting memory mapped file
    FMap__DelimEach = 1 << 5,
    /// @brief use a table to track where each file is loaded into the single resulting memory mapped file
    /// @details the table uses the first n bytes of resulting memory mapped storage, where n is the number of loaded
    /// files * sizeof(i64)
    /// mutually exclusive with [FMap__ConcatAsOne]
    FMap__TrackEach = 1 << 6,
  } HEDLEY_FLAGS FMapFlags; 

/// @brief a large, memory-mapped file
/// @details
struct FileMap {
   /// @see [FMapFlags]
   FMapFlags flags;

  /// @brief nullptr when [FMap__TrackEach] bit is not set in flags OR when [FMap__ConcatAsOne] bit is set in flags
  /// @details you can use this to iterate over or find the location of each file in resulting memory mapped file
  FMapTable* table;

  /// @brief equal to the address of &storage[0] if [FMap__TrackEach] bit is not set in flags
  u8* storage_start;

  /// @brief -1 if [FMap__DelimEach] bit is not set in flags, if [FMap__DelimEach] bit is set and user does not specify
  /// this value it will be set to '\0'
  char delim;
  /// @brief cached count of bytes in this filemaps storage for counted_by attribute
  i64 size;

  ATTR_COUNTED_BY(size)
  u8 storage[];
};
alias(FileMap);


static constexpr const i32 LIBNV_FILE_EXT_MAX_LEN = 4;

typedef CharArray(LIBNV_FILE_EXT_MAX_LEN) FileExt;

struct FileMapOpts {
  sslice relpath;
  FileExt extension;
};
alias(FileMapOpts);

FileMap fmap_dir_readall();
