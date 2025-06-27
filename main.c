#include <stdio.h>
#include <Windows.h>

#include "assert.h"
#include "bin.h"

void ParseArgument(int argc, wchar_t* argv[])
{
	wchar_t* word = argv[1];
	wchar_t* option = argv[2];
	wchar_t* file = argv[2];
	wchar_t* findfile = argv[3];

	if (argc < 2)									wprintf(L"Insufficient arguments.\n");
	else if (!wcscmp(word, L"delete") && argc == 3)	RecycleFile(file, NULL);

	else if (!wcscmp(word, L"restore") && argc == 4)
	{
		if (!wcscmp(option, L"guid"))		RestoreFile(findfile, FIND_TYPE_GUID);
		else if (!wcscmp(option, L"name"))	RestoreFile(findfile, FIND_TYPE_ORIG_NAME);
	}
	else if (!wcscmp(word, L"purge") && argc == 4)
	{
		if (!wcscmp(option, L"guid"))		PurgeFile(findfile, FIND_TYPE_GUID);
		else if (!wcscmp(option, L"name"))	PurgeFile(findfile, FIND_TYPE_ORIG_NAME);
	}
	else if (!wcscmp(word, L"view"))	PrintFile();
	else								wprintf(L"Unknown option\n");
}

// options:
// delete <filename>
// restore <filename>
// purge <filename>
// view
int wmain(int argc, wchar_t* argv[])
{
	// Initialize the recycle bin related resources
	ASSERT_NORET(InitRecycleBin(), L"InitRecycleBin - x");

	// Parse the argument and do some actions
	ParseArgument(argc, argv);

	// Clean up the resources
	CloseRecycleBin();

	// Wait
	_wsystem(L"pause");

	return 0;
}