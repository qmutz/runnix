/*
 *	Nickolas Pylarinos
 *	runnix.cpp
 *
 *	runnix is a console utility for running
 *	linux apps without opening the WSL
 *
 *	AKNOWLEDGEMENTS:
 *		Many thanks to GitHub user "0xbadfca11"
 *		for his code related to getting the WslLaunch function
 *		from the wslapi.dll.
 */

/*
	TODO:
	1) Implement settings xml creator and add the setting to select custom DISTRIBUTION_NAME
	2) find a way to run the executable and EXIT IMMEDIATELY!	-- think it works though!
	3) add support for running script files, too!
	4) support running executables from other drives too, not just C:\!
*/

/* includes */
#include <SDKDDKVer.h>
#include <process.h>
#include <windows.h>
#include <atlbase.h>
#include <wslapi.h>
#include <iostream>
#include <string>

/* defines */
#define SW_VERSION			"0.3"
#define DISTRIBUTION_NAME	L"Ubuntu"
#define WIN32_LEAN_AND_MEAN
#define _ATL_NO_AUTOMATIC_NAMESPACE

/* using */
using namespace std;

void print_usage(void)
{
	printf("Usage: runnix [command] [options]\n\n");
	printf("command: linux executable or script stored outside WSL\n");
	printf("options:\n\n\t-h	Print usage\n");
}


/**
 *	s2ws
 *
 *	convert std::string to PCWSTR
 *	taken from https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
 */
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

/**
 *	get_unix_path
 *
 *	gets path in ntfs form and converts it to unix form with
 *	the appropriate prefix to make it usable for bash
 */
string get_unix_path(string path)
{
	char windows_drive = '\0';
	string drive_prefix = "/mnt/";
	string temp_path  = "";
	
	/* copy contents of path to tmp_path */
	temp_path = path;

	/* identify Windows drive */
	windows_drive = temp_path[0];

	/* set drive prefix */
	drive_prefix += tolower(windows_drive);

	/* apply correct prefix to tmp_path */
	temp_path = drive_prefix + temp_path.substr(2, temp_path.length());

	/* Replace every '\' with '/' */
	for (unsigned i = 0; i < temp_path.length(); i++)
	{
		if (temp_path[i] == '\\')
			temp_path[i] = '/';
	}

	return temp_path;
}

/**
 *	WslExecute
 *
 *	Execute a command inside the WSL
 */
ULONG WslExecute(PCWSTR command)
{
	ULONG res = 1;

	if (auto WslLaunchPtr = AtlGetProcAddressFn(LoadLibraryExW(L"wslapi", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32), WslLaunch))
	{
		HANDLE	proc	= nullptr,
				stdIn	= GetStdHandle(STD_INPUT_HANDLE),
				stdOut	= GetStdHandle(STD_OUTPUT_HANDLE),
				stdErr	= GetStdHandle(STD_ERROR_HANDLE);

		res = WslLaunchPtr(DISTRIBUTION_NAME, command, FALSE, stdIn, stdOut, stdErr, &proc);

		//printf("res = %i => proc: %i\n", res, (int)proc);
		
		/* close handle */
		CloseHandle(proc);
	}

	return res;
}

int main(int argc, char * argv[])
{
	//printf("runnix v%s | by npyl\n\n", SW_VERSION);

	if (argc < 2) {
		printf("runnix: too few arguments\n");
		return 1;
	}

	string executable;
	int res = 0;

	/* Get unix path */
	executable = get_unix_path(argv[1]);

	/* Run executable */
	res = WslExecute(s2ws(executable).c_str());

	/* Check result */
	if (res != S_OK)
	{
		printf("runnix: couldn't run %s. Error: ", executable.c_str(), res);
		perror(NULL);
		printf("\n");
		return 1;
	}
	
	getchar();
	return 0;
}