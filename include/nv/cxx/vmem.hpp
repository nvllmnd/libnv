#pragma once

#include "nv/core/intdefs.h"
#include "nv/cxx/opt.hpp"

#include "nv/cxx/result.hpp"

#include "nv/cxx/ptr.hpp"

namespace nv::vmem {

enum class RemapMode {
  ResizeInPlace,
  Relocate,
  DontUnmap,
};

struct VmapMode {
  enum : i32 {
    PrivAnon = 1,
    NoReserve = 1 << 1,
    Lock = 1 << 2,
    Prefault = 1 << 3,

    Default = PrivAnon,
    WithNoReserve = PrivAnon | NoReserve,
    WithLock = PrivAnon | Lock,
    WithPrefault = PrivAnon | Prefault,
    WithReserveLock = PrivAnon | NoReserve | Lock,
    WithReservePrefault = PrivAnon | NoReserve | Prefault,
  } flags = Default;

  using Type = decltype(VmapMode::flags);

  constexpr VmapMode(VmapMode::Type mode) noexcept : flags(mode) {}
  constexpr VmapMode(const VmapMode& mode) noexcept = default;
  constexpr VmapMode& operator=(const VmapMode::Type& other) noexcept {
    if (this->flags != other) [[likely]] {
      this->flags = other;
    }
    return *this;
  }

  constexpr i32 get() const noexcept { return this->flags; }
};

struct VmemError {
  enum : u64 {
    None = 0,
    FailedMemoryMap = 1,
    CantResizeInPlace = 1 << 1,
    RelocateFailed = 1 << 2,
    InvalidSize = 1 << 3,
    PtrNotPageAligned = 1 << 4,
    InvalidUnmapSize = 1 << 5,
    MisAlignedUnmapPtr = 1 << 6,
    InnerSystemError = 1 << 7,
  } flags = None;

  using Type = decltype(VmemError::flags);

  constexpr VmemError(VmemError::Type err) noexcept : flags(err) {}
  constexpr VmemError(const VmemError& err) noexcept = default;
  constexpr VmemError& operator=(const VmemError::Type& other) noexcept {
    if (this->flags != other) [[likely]] {
      this->flags = other;
    }
    return *this;
  }

  constexpr i32 get() const noexcept { return this->flags; }
};

struct Vmem;

using Vptr = nv::ptr::NonNull<Vmem>;

using OptVptr = nv::opt::Opt<Vptr>;
using Vresult = nv::result::Result<Vptr, VmemError>;

Vresult map(isize size, VmapMode mode) noexcept;

inline Vresult map(isize size) noexcept { return nv::vmem::map(size, VmapMode::Default); }

OptVptr remap(Vptr self, isize new_size, RemapMode mode) noexcept;
void unmap(Vptr self) noexcept;

}  // namespace nv::vmem
