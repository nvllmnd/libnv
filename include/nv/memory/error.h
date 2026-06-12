// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <errno.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"

static constexpr const error OK = 0;

typedef enum ApiError : error {
  ApiError__ParameterValueNotPowerOf2 = -500,
  ApiError__InvalidParameter,
  ApiError__NullParameter,
  ApiError__ValueOutOfRange,
} ApiError;

typedef enum NvError : error {

  /// Ok, No Error!
  Error__Ok = 0,

  /// mmap errors...
  Error__FailedMemMap = -100,
  Error__VMapCannotBeResized,
  Error__VMapInvalidRemapFlags,
  Error__VMapInvalidMapFlags,
  Error__FailedToPreloadOOM,
  Error__VMapCannotLockToRAM,
  Error__CannotUnlockRAM,
  Error__VMapCannotShrink,
  Error__VMapCannotGrow,
  Error__VMapCannotBeMoved,
  Error__VMapCannotExpandInPlace,
  Error__FailedMemUnmap,
  Error__NotEnoughPhysicalRAMAavailable,
  Error__VMemLimitReached,
  Error__MAdviseWillNeedFailed,
  Error__FailedRemap,
  Error__CannotExpandInPlace,
  Error__InvalidAllocationSize,
  Error__BufferNeedsResize = -300,
  Error__VirtMemOutOfMemory = -500,
  /// Not enough space/cannot allocate memory (POSIX.1-2001).
  Error__OOM = -ENOMEM,
  Error__ValTooLargeFoDataType = -EOVERFLOW,
  Error__ResourceTempUnavail = -EAGAIN,

} NvError;

CONST_FUNC
RETURNS_NON_NULL
static inline const char* error_string(NvError err) {
  switch (err) {
    case Error__Ok: {
      return STRINGIFY(Error__Ok) " :: Ok! no error.";
    } break;
    case Error__FailedMemMap: {
      return STRINGIFY(Error__FailedMemMap) " :: mmap returned MAP_FAILED";
    } break;
    case Error__FailedMemUnmap: {
      return STRINGIFY(Error__FailedMemUnmap) " :: munmap returned -1, check errno for more info";
    } break;
    case Error__BufferNeedsResize: {
      return STRINGIFY(Error__BufferNeedsResize) " :: Buff must be resized in order to append a given layout!";
    } break;
    case Error__VirtMemOutOfMemory: {
      return STRINGIFY(Error__VirtMeOutOfMemory) " :: Virtual Memory owned by VirtMem does not have enough memory for a given MemLayout!";
    } break;
    case Error__OOM: {
      return STRINGIFY(Error__OOM) " :: General/Unspecified Out of Memory Error.";
    } break;
    case Error__ValTooLargeFoDataType: {
      return STRINGIFY(Error__ValTooLargeForDataType) " :: Alias for ERRNO: EOVERFLOW";
    } break;
    case Error__ResourceTempUnavail: {
      return STRINGIFY(Error__ResourceTempUnavail) " :: Alias for ERRNO: EAGAIN";
    } break;
    case Error__VMapCannotBeResized:
      return STRINGIFY(Error__VMapCannotBeResized);
    case Error__VMapInvalidRemapFlags:
      return STRINGIFY(Error__VMapInvalidRemapFlags);
    case Error__VMapInvalidMapFlags:
      return STRINGIFY(Error__VMapInvalidMapFlags);
    case Error__FailedToPreloadOOM:
      return STRINGIFY(mError__FailedToPreloadOOM);
    case Error__VMapCannotLockToRAM:
      return STRINGIFY(Error__VMapCannotLockToRAM);
    case Error__VMapCannotShrink:
      return STRINGIFY(Error__VMapCannotShrink);
    case Error__VMapCannotGrow:
      return STRINGIFY(Error__VMapCannotGrow);
    case Error__VMapCannotBeMoved:
      return STRINGIFY(Error__VMapCannotBeMoved);
    case Error__VMapCannotExpandInPlace:
      return STRINGIFY(Error__VMapCannotExpandInPlace);
    case Error__NotEnoughPhysicalRAMAavailable:
      return STRINGIFY(Error__NotEnoughPhysicalRAMAavailable);
    case Error__VMemLimitReached:
      return STRINGIFY(Error__VMemLimitReached);
      break;
    case Error__CannotUnlockRAM:
      return STRINGIFY(Error__CannotUnlockRAM);
    case Error__MAdviseWillNeedFailed:
      return STRINGIFY(Error__MAdviseWillNeedFailed);
      break;
    case Error__FailedRemap:
      return STRINGIFY(Error__FailedRemap);
    case Error__CannotExpandInPlace:
      return STRINGIFY(Error__CannotExpandInPlace);
      break;
    case Error__InvalidAllocationSize:
      return STRINGIFY(Error__InvalidAllocationSize) " :: Size null, negative, or larger than INT64_MAX!";
      break;
  }

  return "Invalid NvError Value!";
}

// typedef enum NvError {
//     Error__NoSuchFileOrDir = -ENOENT,
//     Error__OpNotPermitted = -EPERM,
//     Error__OK = 0,

//  	1 	Operation not permitted
// ENOENT 	2 	No such file or directory
// ESRCH 	3 	No such process
// EINTR 	4 	Interrupted system call
// EIO 	5 	Input/output error
// ENXIO 	6 	No such device or address
// E2BIG 	7 	Argument list too long
// ENOEXEC 	8 	Exec format error
// EBADF 	9 	Bad file descriptor
// ECHILD 	10 	No child processes
// EAGAIN 	11 	Resource temporarily unavailable
// ENOMEM 	12 	Cannot allocate memory
// EACCES 	13 	Permission denied
// EFAULT 	14 	Bad address
// ENOTBLK 	15 	Block device required
// EBUSY 	16 	Device or resource busy
// EEXIST 	17 	File exists
// EXDEV 	18 	Invalid cross-device link
// ENODEV 	19 	No such device
// ENOTDIR 	20 	Not a directory
// EISDIR 	21 	Is a directory
// EINVAL 	22 	Invalid argument
// ENFILE 	23 	Too many open files in system
// EMFILE 	24 	Too many open files
// ENOTTY 	25 	Inappropriate ioctl for device
// ETXTBSY 	26 	Text file busy
// EFBIG 	27 	File too large
// ENOSPC 	28 	No space left on device
// ESPIPE 	29 	Illegal seek
// EROFS 	30 	Read-only file system
// EMLINK 	31 	Too many links
// EPIPE 	32 	Broken pipe
// EDOM 	33 	Numerical argument out of domain
// ERANGE 	34 	Numerical result out of range
// EDEADLK 	35 	Resource deadlock avoided
// ENAMETOOLONG 	36 	File name too long
// ENOLCK 	37 	No locks available
// ENOSYS 	38 	Function not implemented
// ENOTEMPTY 	39 	Directory not empty
// ELOOP 	40 	Too many levels of symbolic links
// ENOMSG 	42 	No message of desired type
// EIDRM 	43 	Identifier removed
// ECHRNG 	44 	Channel number out of range
// EL2NSYNC 	45 	Level 2 not synchronized
// EL3HLT 	46 	Level 3 halted
// EL3RST 	47 	Level 3 reset
// ELNRNG 	48 	Link number out of range
// EUNATCH 	49 	Protocol driver not attached
// ENOCSI 	50 	No CSI structure available
// EL2HLT 	51 	Level 2 halted
// EBADE 	52 	Invalid exchange
// EBADR 	53 	Invalid request descriptor
// EXFULL 	54 	Exchange full
// ENOANO 	55 	No anode
// EBADRQC 	56 	Invalid request code
// EBADSLT 	57 	Invalid slot
// EBFONT 	59 	Bad font file format
// ENOSTR 	60 	Device not a stream
// ENODATA 	61 	No data available
// ETIME 	62 	Timer expired
// ENOSR 	63 	Out of streams resources
// ENONET 	64 	Machine is not on the network
// ENOPKG 	65 	Package not installed
// EREMOTE 	66 	Object is remote
// ENOLINK 	67 	Link has been severed
// EADV 	68 	Advertise error
// ESRMNT 	69 	Srmount error
// ECOMM 	70 	Communication error on send
// EPROTO 	71 	Protocol error
// EMULTIHOP 	72 	Multihop attempted
// EDOTDOT 	73 	RFS specific error
// EBADMSG 	74 	Bad message
// EOVERFLOW 	75 	Value too large for defined data type
// ENOTUNIQ 	76 	Name not unique on network
// EBADFD 	77 	File descriptor in bad state
// EREMCHG 	78 	Remote address changed
// ELIBACC 	79 	Can not access a needed shared library
// ELIBBAD 	80 	Accessing a corrupted shared library
// ELIBSCN 	81 	.lib section in a.out corrupted
// ELIBMAX 	82 	Attempting to link in too many shared libraries
// ELIBEXEC 	83 	Cannot exec a shared library directly
// EILSEQ 	84 	Invalid or incomplete multibyte or wide character
// ERESTART 	85 	Interrupted system call should be restarted
// ESTRPIPE 	86 	Streams pipe error
// EUSERS 	87 	Too many users
// ENOTSOCK 	88 	Socket operation on non-socket
// EDESTADDRREQ 	89 	Destination address required
// EMSGSIZE 	90 	Message too long
// EPROTOTYPE 	91 	Protocol wrong type for socket
// ENOPROTOOPT 	92 	Protocol not available
// EPROTONOSUPPORT 	93 	Protocol not supported
// ESOCKTNOSUPPORT 	94 	Socket type not supported
// EOPNOTSUPP 	95 	Operation not supported
// EPFNOSUPPORT 	96 	Protocol family not supported
// EAFNOSUPPORT 	97 	Address family not supported by protocol
// EADDRINUSE 	98 	Address already in use
// EADDRNOTAVAIL 	99 	Cannot assign requested address
// ENETDOWN 	100 	Network is down
// ENETUNREACH 	101 	Network is unreachable
// ENETRESET 	102 	Network dropped connection on reset
// ECONNABORTED 	103 	Software caused connection abort
// ECONNRESET 	104 	Connection reset by peer
// ENOBUFS 	105 	No buffer space available
// EISCONN 	106 	Transport endpoint is already connected
// ENOTCONN 	107 	Transport endpoint is not connected
// ESHUTDOWN 	108 	Cannot send after transport endpoint shutdown
// ETOOMANYREFS 	109 	Too many references: cannot splice
// ETIMEDOUT 	110 	Connection timed out
// ECONNREFUSED 	111 	Connection refused
// EHOSTDOWN 	112 	Host is down
// EHOSTUNREACH 	113 	No route to host
// EALREADY 	114 	Operation already in progress
// EINPROGRESS 	115 	Operation now in progress
// ESTALE 	116 	Stale file handle
// EUCLEAN 	117 	Structure needs cleaning
// ENOTNAM 	118 	Not a Xenix named type file
// ENAVAIL 	119 	No Xenix semaphores available
// EISNAM 	120 	Is a named type file
// EREMOTEIO 	121 	Remote I/O error
// EDQUOT 	122 	Disk quota exceeded
// ENOMEDIUM 	123 	No medium found
// EMEDIUMTYPE 	124 	Wrong medium type
// ECANCELED 	125 	Operation canceled
// ENOKEY 	126 	Required key not available
// EKEYEXPIRED 	127 	Key has expired
// EKEYREVOKED 	128 	Key has been revoked
// EKEYREJECTED 	129 	Key was rejected by service
// EOWNERDEAD 	130 	Owner died
// ENOTRECOVERABLE 	131 	State not recoverable
// ERFKILL 	132 	Operation not possible due to RF-kill
// EHWPOISON 	133 	Memory page has hardware error
// ENOTSUP 	134 	Not supported parameter or option
// ENOMEDIUM 	135 	Missing media
// EILSEQ 	138 	Invalid multibyte sequence
// EOVERFLOW 	139 	Value too large
// ECANCELED 	140 	Asynchronous operation stopped before normal completion
// ENOTRECOVERABLE 	141 	State not recoverable
// EOWNERDEAD 	142 	Previous owner died
// ESTRPIPE 	143 	Streams pipe error

// } NvError;
