#pragma once


#if !defined(LIBNV_NO_USE_SHORT_NAMES)
#define typeof __typeof
#define pexpect ptr_nonnull
#define noreturn _Noreturn
#define noexecpt NOTHROW

#define INFO LOG_INFO
#define WARN LOG_WARN
#define LOGERR LOG_ERROR

#endif

