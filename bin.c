#include <stdio.h>
#include <objbase.h>

#include "assert.h"
#include "bin.h"

WCHAR	szRecycleBinPath[MAX_PATH];
WCHAR	szRecordFilePath[MAX_PATH];
HANDLE	hRecordFileWrite;

BOOL
SetRecordPointer(
	VOID
)
{
	BY_HANDLE_FILE_INFORMATION	info;
	DWORD	dwFileSzLow, dwFileSzHigh;

	// Retrieve file information
	ASSERT(GetFileInformationByHandle(hRecordFileWrite, &info),
		L"GetFileInformationByHandle");

	// Get the file size
	dwFileSzLow = info.nFileSizeLow;
	dwFileSzHigh = info.nFileSizeHigh;

	// Set the file pointer
	ASSERT_VAL(
		SetFilePointer(hRecordFileWrite, dwFileSzLow,
			&dwFileSzHigh, FILE_BEGIN),
		INVALID_SET_FILE_POINTER,
		L"SetFilePointer"
	);

	// Everything is OK!
	return TRUE;
}

BOOL
InitRecycleBin(
	VOID
)
{
	WCHAR	szWorkingDir[MAX_PATH];
	LPWSTR	szRootDir;

	// Get the root directory name (e.g. C:, D:)
	ASSERT(GetCurrentDirectoryW(MAX_PATH, szWorkingDir), L"GetCurrentDirectoryW");
	szWorkingDir[2] = L'\0';
	szRootDir = szWorkingDir;
	DEBUG(L"Partition letter", szRootDir);

	// Make the recycle bin's path
	swprintf(szRecycleBinPath, MAX_PATH, L"%s\\%s", szRootDir, RECYCLE_BIN_NAME);

	// Create the directory, if it doesn't exist
	ASSERT_IGNORE_ERROR(
		CreateDirectoryW(szRecycleBinPath, NULL),
		ERROR_ALREADY_EXISTS, L"CreateDirectoryW"
	);
	ASSERT(
		SetFileAttributesW(
			szRecycleBinPath,
			FILE_ATTRIBUTE_HIDDEN
		), L"SetFileAttributesW"
	);

	// Create and/or open the record file
	// First, make the path
	swprintf(szRecordFilePath, MAX_PATH, L"%s\\%s", szRecycleBinPath, RECORD_FILE_NAME);
	DEBUG(L"Record File Path", szRecordFilePath);

	// Secondly, create the file
	hRecordFileWrite = CreateFileW(
		szRecordFilePath, GENERIC_WRITE,
		FILE_SHARE_READ, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN, NULL);
	ASSERT_VAL(hRecordFileWrite, INVALID_HANDLE_VALUE, L"CreateFileW");

	// Finally, set the file pointer to be at the end of the file
	ASSERT(SetRecordPointer(), L"SetRecordPointer");

	// Okay
	// Everything is set
	return TRUE;
}

VOID
CloseRecycleBin(
	VOID
)
{
	// Close the handle for the open record file
	CloseHandle(hRecordFileWrite);
}

BOOL
WriteRecycleBinEntry(
	LPWSTR	szRelativeFilePath,
	LPWSTR	szGuid,
	LPWSTR	szGuidPath
)
{
	RECORD_ENTRY	re;
	WCHAR			szGlobalFilePath[MAX_PATH];

	// Initialize the struct to zero
	RtlFillMemory(&re, sizeof(re), 0);

	// Get global path of the file
	ASSERT(GetFullPathNameW(szRelativeFilePath,
		MAX_PATH, szGlobalFilePath, NULL),
		L"GetFullPathNameW"
	);

	// Copy the path and GUID to the struct
	RtlMoveMemory(&re.szOriginalPath, szGlobalFilePath, MAX_PATH * sizeof(WCHAR));
	RtlMoveMemory(&re.szGUID, szGuid, GUID_LEN * sizeof(WCHAR));
	RtlMoveMemory(&re.szGuidPath, szGuidPath, MAX_PATH * sizeof(WCHAR));

	// Set the entry to be valid
	re.bIsValid = TRUE;

	// Get the time of deletion
	GetLocalTime(&re.stDeletion);

	// Write the struct into the file
	ASSERT(WriteFile(hRecordFileWrite, &re,
		sizeof(re), NULL, NULL),
		L"WriteFile"
	);

	// Everything is OK!
	return TRUE;
}

BOOL
CopyFileAndDirectory(
	LPWSTR	szSource,
	LPWSTR	szDestination
)
{
	COPYFILE2_EXTENDED_PARAMETERS cep;

	if (GetFileAttributesW(szSource) & FILE_ATTRIBUTE_DIRECTORY)
	{
		cep.dwSize = sizeof(cep);
		cep.dwCopyFlags = COPY_FILE_DIRECTORY;
		cep.pfCancel = FALSE;
		cep.pProgressRoutine = NULL;
		cep.pvCallbackContext = NULL;

		ASSERT(SUCCEEDED(CopyFile2(szSource, szDestination, &cep)), L"CopyFile2");
	}
	else
		ASSERT(CopyFileW(szSource, szDestination, FALSE), L"CopyFileW");
	return TRUE;
}

BOOL
DeleteFileAndDirectory(
	LPWSTR	szPath
)
{
	if (GetFileAttributesW(szPath) & FILE_ATTRIBUTE_DIRECTORY)
	{
		ASSERT(RemoveDirectoryW(szPath), L"RemoveDirectoryW");
	}
	else
	{
		ASSERT(DeleteFileW(szPath), L"DeleteFileW");
	}
	return TRUE;
}

VOID
RecycleFileRecursively(
	LPWSTR	szRootDir,
	LPWSTR	szParentGuid
)
{
	HANDLE	hFindFile;
	WCHAR	szFindString[MAX_PATH];
	WCHAR	szFoundGlobalPath[MAX_PATH];
	WIN32_FIND_DATAW	data;

	// Make string to search (e.g. "somedir\*")
	swprintf(szFindString, MAX_PATH, L"%s\\*", szRootDir);
	DEBUG(L"Find string", szFindString);

	// Iterate through files in the directory
	hFindFile = FindFirstFileW(szFindString, &data);
	ASSERT_VAL_NORET(hFindFile, INVALID_HANDLE_VALUE, L"FindFirstFileW");
	do
	{
		// Continue if the directory is . or ..
		if (!wcscmp(data.cFileName, L".") || !wcscmp(data.cFileName, L".."))
			continue;
		DEBUG(L"File found", data.cFileName);

		// Make the path from root dir and recursively call RecycleFile
		swprintf(szFoundGlobalPath, MAX_PATH, L"%s\\%s", szRootDir, data.cFileName);
		RecycleFile(szFoundGlobalPath, szParentGuid);
	} while (FindNextFileW(hFindFile, &data));

	// Close the find object
	FindClose(hFindFile);
}

VOID
RecycleFile(
	LPWSTR	szFilePath,
	LPWSTR	szParentDirGuid
)
{
	GUID	guid;
	WCHAR	szGuid[GUID_LEN];
	WCHAR	szGuidPath[MAX_PATH];
	INT		StringFromGUID2ReturnVal;

	// Generate GUID, the new file name for the deleted file
	if (!SUCCEEDED(CoCreateGuid(&guid)))
	{
		wprintf(L"[ERROR] CoCreateGuid\n");
		return;
	}
	
	// Check if all 40 chars are written
	StringFromGUID2ReturnVal = StringFromGUID2(&guid, szGuid, GUID_LEN);
	if (StringFromGUID2ReturnVal != GUID_LEN - 1)
		return;
	DEBUG(szFilePath, szGuid);

	// Copy the file to the bin
	if (szParentDirGuid)
		swprintf(szGuidPath, MAX_PATH, L"%s\\%s", szParentDirGuid, szGuid);
	else
		swprintf(szGuidPath, MAX_PATH, L"%s\\%s", szRecycleBinPath, szGuid);
	DEBUG(L"Final path", szGuidPath);
	ASSERT_NORET(
		CopyFileAndDirectory(szFilePath, szGuidPath),
		L"CopyFileAndDirectory - x"
	);
	
	// If the file is a directory, recursively recycle the file
	if (GetFileAttributesW(szFilePath) & FILE_ATTRIBUTE_DIRECTORY)
	{
		DEBUG(L"Directory", szFilePath);
		RecycleFileRecursively(szFilePath, szGuidPath);
	}
	else
		DEBUG(L"File", szFilePath);

	// Remove the original file
	ASSERT_NORET(
		DeleteFileAndDirectory(szFilePath),
		L"DeleteFileAndDirectory - x"
	);

	// Write the entry for the file deletion
	ASSERT_NORET(
		WriteRecycleBinEntry(szFilePath, szGuid, szGuidPath),
		L"WriteRecycleBinEntry - x"
	);

	// Tells the user
	wprintf(INFO_PREFIX L"%s is in the recycle bin as %s\n", szFilePath, szGuid);
}

VOID
FindFile(
	LPWSTR		szFind,
	DWORD		dwType,
	FIND_ACTION	fnFindAction
)
{

}

BOOL
RestoreAction(
	LPWSTR	szGuidPath,
	LPWSTR	szOrigPath
)
{
	return CopyFileAndDirectory(szGuidPath, szOrigPath)
		&& DeleteFileAndDirectory(szGuidPath);
}

BOOL
PurgeAction(
	LPWSTR	szGuidPath,
	LPWSTR	szOrigPath
)
{
	return DeleteFileAndDirectory(szGuidPath);
}

VOID
RestoreFile(
	LPWSTR	szFind,
	DWORD	dwType
)
{

}

VOID
PurgeFile(
	LPWSTR	szFind,
	DWORD	dwType
)
{

}

VOID
PrintFile(
	VOID
)
{
	RECORD_ENTRY	re;
	HANDLE	hRecordFileRead;
	DWORD	dwReadCount;

	// Open the log file
	hRecordFileRead = CreateFileW(
		szRecordFilePath, GENERIC_READ,
		FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_HIDDEN, NULL);
	ASSERT_VAL_NORET(hRecordFileRead,
		INVALID_HANDLE_VALUE,
		L"PrintFile - CreateFileW");

	// Read the entries and print
	wprintf(L"%-38s | %-10s | %s\n", L"GUID", L"Date", L"Original Path");
	do
	{
		ASSERT_NORET(
			ReadFile(hRecordFileRead, &re,
				sizeof(re), &dwReadCount, NULL),
			L"ReadFile"
		);
		if (dwReadCount <= 0)
			break;

		// only print when valid
		if (!re.bIsValid)
			continue;
		wprintf(L"%s | %04d-%02d-%02d | %s\n",
			re.szGUID, re.stDeletion.wYear, re.stDeletion.wMonth,
			re.stDeletion.wDay, re.szOriginalPath
		);
	} while (dwReadCount > 0);

	CloseHandle(hRecordFileRead);
}