#pragma once
#include <windows.h>

#define RECYCLE_BIN_NAME	L"$BetterRecycleBin"
#define RECORD_FILE_NAME	L"$Record.txt"

#define FIND_TYPE_GUID		0
#define FIND_TYPE_ORIG_NAME	1

#define GUID_LEN	40

typedef struct
{
	BOOL		bIsValid;
	WCHAR		szOriginalPath[MAX_PATH];
	WCHAR		szGuidPath[MAX_PATH];
	WCHAR		szGUID[GUID_LEN];
	SYSTEMTIME	stDeletion;
}	RECORD_ENTRY, *PRECORD_ENTRY;

typedef BOOL(*FIND_ACTION)(LPWSTR, LPWSTR);

BOOL
InitRecycleBin(
	VOID
);

VOID
CloseRecycleBin(
	VOID
);

VOID
RecycleFile(
	LPWSTR	szFilePath,
	LPWSTR	szParentDirPath
);

VOID
RestoreFile(
	LPWSTR	szFind,
	DWORD	dwType
);

VOID
PurgeFile(
	LPWSTR	szFind,
	DWORD	dwType
);

VOID
PrintFile(
	VOID
);