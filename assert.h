#pragma once
#include <stdio.h>
#include <Windows.h>

//#define __DEBUG__

#define ERROR_PREFIX	L"[!] "
#define DEBUG_PREFIX	L"[D] "
#define INFO_PREFIX		L"[i] "

#define ASSERT(cond, msg)											\
	if (!cond) {													\
		wprintf(ERROR_PREFIX L"%s (%d)\n", msg, GetLastError());	\
		return FALSE;												\
	}

#define ASSERT_NORET(cond, msg)										\
	if (!cond) {													\
		wprintf(ERROR_PREFIX L"%s (%d)\n", msg, GetLastError());	\
		return;														\
	}

#define ASSERT_VAL(cond, failval, msg)								\
	if (cond == failval) {											\
		wprintf(ERROR_PREFIX L"%s (%d)\n", msg, GetLastError());	\
		return FALSE;												\
	}

#define ASSERT_VAL_NORET(cond, failval, msg)						\
	if (cond == failval) {											\
		wprintf(ERROR_PREFIX L"%s (%d)\n", msg, GetLastError());	\
		return;														\
	}

#define ASSERT_IGNORE_ERROR(cond, ignoreval, msg)					\
	if (!cond && GetLastError() != ignoreval) {						\
		wprintf(ERROR_PREFIX L"%s (%d)\n", msg, GetLastError());	\
		return FALSE;												\
	}

#ifdef __DEBUG__
#define DEBUG(msg0, msg1)	wprintf(DEBUG_PREFIX L"%s: %s\n", msg0, msg1);
#else
#define DEBUG(msg0, msg1)	msg0; msg1;
#endif