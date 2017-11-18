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
	1) Implement option to select custom DISTRIBUTION_NAME	-- same as (3)
	2) find a way to run the executable and EXIT IMMEDIATELY!	-- think it works though!
	3) add support for running script files, too!	-- Will be added when we implement correct arguments parsing mechanism
*/

/* includes */
#include <SDKDDKVer.h>
#include <process.h>
#include <windows.h>
#include <atlbase.h>
#include <wslapi.h>
#include <iostream>
#include <vector>
#include <string>

/* defines */
#define SW_VERSION			"0.4"
#define DISTRIBUTION_NAME	L"Ubuntu"

#define WIN32_LEAN_AND_MEAN
#define _ATL_NO_AUTOMATIC_NAMESPACE

/* using */
using namespace std;

/**
 *	print_usage
 *	
 *	lets not document this...
 */
void print_usage(void)
{
	printf("runnix v%s | by npyl\n\n", SW_VERSION);
	printf("Usage: runnix --path [executable] --args1 [args1] --args2 [args2] --options [options]\n\n");
	printf("executable:\tlinux executable or script stored outside WSL\n");
	printf("args1:\t\tstuff you can pass to bash such as DISPLAY=:0.0 before executable\n");
	printf("args2:\t\tthe arguments to be passed to your executable\n");
	printf("options:\tspecific flags to be passed to runnix\n\n");
	printf("valid options are:\n");
	printf("-h\tprint usage\n");
	printf("-s\ttells runnix to treat [executable] as script\n");
	printf("-d\t[distribution] allows you to set the distro you want to run your executable in.\n");
	printf("\t[distribution] must be a unique name representing a distro e.g. Fabrikam.Distro.10.01 or Ubuntu.  Default is Ubuntu.\n");
}

/**
 *	parse_arguments
 *
 *	Custom **homemade** arguments parser for runnix
 *
 *	Usage:
 *	runnix --path ... --args1 ... --args2 ... --options ..."
 *
 *	Rules:
 *	--path is always index 1
 *	--options must be the last switch
 *	other switches dont have fixed index
 *	currently we support having args1 before or after args2
 */
void parse_arguments(string &executable, vector<string> &args1, vector<string> &args2, vector<string> &options, int argc, char ** argv)
{
	bool haveArgs1 = false;		int firstArgs1Index = 0;
	bool haveArgs2 = false;		int firstArgs2Index = 0;
	bool haveOptions = false;	int firstOptionIndex = 0;
	
	executable = argv[2];

	/* Search the arguments list for switches */
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "--args1") == 0)
		{
			haveArgs1 = true;
			firstArgs1Index = i + 1;
		}
		if (strcmp(argv[i], "--args2") == 0)
		{
			haveArgs2 = true;
			firstArgs2Index = i + 1;
		}
		if (strcmp(argv[i], "--options") == 0)
		{
			haveOptions = true;
			firstOptionIndex = i + 1;
		}
	}

	/* Get args1 */
	if (haveArgs1)
	{
		printf("haveArgs1\n");

		for (int i = firstArgs1Index; i < (argc - 1) && (argv[i][0] == '-'); i++)
		{
			args1.push_back(argv[i]);
		}
	}

	/* Get args2 */
	if (haveArgs2)
	{
		printf("haveArgs2\n");

		for (int i = firstArgs2Index; i < (argc - 1) && (argv[i][0] == '-'); i++)
		{
			args2.push_back(argv[i]);
		}
	}

	/* Get options */
	if (haveOptions)
	{
		for (int i = firstOptionIndex; i < argc - 1; i++)
			options.push_back(argv[i]);
	}
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
		
		/* close handle */
		CloseHandle(proc);
	}

	return res;
}

int main(int argc, char * argv[])
{
	if (argc < 2) {
		printf("runnix: too few arguments\n");
		print_usage();
		return 1;
	}

	string executable;
	vector<string> args1;
	vector<string> args2;
	vector<string> options;
	int res = 0;

	/* Parse arguments */
	parse_arguments(executable, args1, args2, options, argc, argv);

	/* Get unix path */
	executable = get_unix_path(executable);

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