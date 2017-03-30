#include <sdkddkver.h>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

#ifdef UNICODE
#define LoadLib "LoadLibraryW"
#else
#define LoadLib "LoadLibraryA"
#endif

//TCHAR szPath[MAX_PATH]; /* TODO: full path to connect-or-cut.dll */

BOOL coc_inject(HANDLE hProcess, TCHAR *szPath)
{
	LPTHREAD_START_ROUTINE lpLoadLibrary = (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), LoadLib);

	if (lpLoadLibrary == NULL)
	{
		return FALSE;
	}

	int iLen = lstrlen(szPath) + 1;
	int cbSize = iLen * sizeof(TCHAR);

	LPVOID lpPath = VirtualAllocEx(hProcess, NULL, cbSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (lpPath == NULL)
	{
		return FALSE;
	}

	if (!WriteProcessMemory(hProcess, lpPath, szPath, cbSize, NULL))
	{
		return FALSE;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, lpLoadLibrary, lpPath, 0, NULL);
	if (hThread == NULL)
	{
		return FALSE;
	}

	if (WaitForSingleObject(hThread, INFINITE) != WAIT_OBJECT_0)
	{
		return FALSE;
	}

	DWORD dwExitCode;
	if (!GetExitCodeThread(hThread, &dwExitCode))
	{
		return FALSE;
	}

	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3)
	{
		wprintf(L"Usage: %s [cmdline] [lib]\n", argv[0]);
		return 1;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, argv[1], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
	{
		// TODO
	}

	coc_inject(pi.hProcess, argv[2]);

	ResumeThread(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}