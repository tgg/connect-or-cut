/* inject.c -- Injects connect-or-cut DLL into a Windows process.
*
* Copyright Ⓒ 2017  Thomas Girard <thomas.g.girard@free.fr>
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
*  * Neither the name of the author nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <SDKDDKVer.h>
#include <tchar.h>
#include <Windows.h>

#ifdef UNICODE
#define LoadLib "LoadLibraryW"
#else
#define LoadLib "LoadLibraryA"
#endif

BOOL LoadProcessLibrary(HANDLE hProcess, TCHAR *szLibraryPath)
{
	if (hProcess == NULL || szLibraryPath == NULL)
	{
		// Assume GetLastError() will provide explanation on these erroneous values.
		return FALSE;
	}

	LPTHREAD_START_ROUTINE lpLoadLibrary = (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle(_T("kernel32.dll")), LoadLib);

	if (lpLoadLibrary == NULL)
	{
		// GetLastError() will contain relevant error.
		return FALSE;
	}

	int iLibraryPathLen = lstrlen(szLibraryPath) + 1;
	SIZE_T dwSize = iLibraryPathLen * sizeof(TCHAR);

	LPVOID lpPath = VirtualAllocEx(hProcess, NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (lpPath == NULL)
	{
		// Could not allocate memory in the process. GetLastError() will contain
		// relevant error.
		return FALSE;
	}

	if (!WriteProcessMemory(hProcess, lpPath, szLibraryPath, dwSize, NULL))
	{
		// Fail gracefully: release allocated memory (ignoring error), then
		// restore initial error so that GetLastError() in the callee works.
		DWORD dwError = GetLastError();
		VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE);
		SetLastError(dwError);
		return FALSE;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, lpLoadLibrary, lpPath, 0, NULL);
	if (hThread == NULL)
	{
		DWORD dwError = GetLastError();
		VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE);
		SetLastError(dwError);
		return FALSE;
	}

	DWORD dwWaitForThread = WaitForSingleObject(hThread, INFINITE);
	if (dwWaitForThread != WAIT_OBJECT_0)
	{
		// The only valid status would be WAIT_FAILED according to doc
		DWORD dwError = GetLastError();
		// Here we don't know exactly whether the thread has completed or not,
		// so we forcibly kill it.
		TerminateThread(hThread, 0xdeadbeef);
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE);
		SetLastError(dwError);
		return FALSE;
	}

	DWORD dwExitCode;
	if (!GetExitCodeThread(hThread, &dwExitCode))
	{
		DWORD dwError = GetLastError();
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE);
		SetLastError(dwError);
		return FALSE;
	}

	if (dwExitCode == 0)
	{
		// The LoadLibrary call failed, but the thread is gone now, so
		// GetLastError() is no longer relevant.
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE);
		// We build significant error message, but with no explanation :-(
		SetLastError(TYPE_E_CANTLOADLIBRARY);
		return FALSE;
	}

	if (!CloseHandle(hThread))
	{
		DWORD dwError = GetLastError();
		VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE);
		SetLastError(dwError);
		return FALSE;
	}

	/*if (!VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE))
	{
		return FALSE;
	}*/

	return TRUE;
}

BOOL LoadCoCLibrary(HANDLE hProcess)
{
	TCHAR szThisPath[MAX_PATH] = { 0 };
	TCHAR szCoCPath[MAX_PATH] = { 0 };
	HMODULE self = GetModuleHandle(_T("connect-or-cut.dll"));

	if (self == NULL)
	{
		self = GetModuleHandle(_T("connect-or-cut32.dll"));
		// If self is NULL again, then we assume we are being
		// injected from coc.exe program.
	}

	if (GetModuleFileName(self, szThisPath, MAX_PATH) == 0)
	{
		return FALSE;
	}

	TCHAR* lastBackslash = _tcsrchr(szThisPath, _T('\\'));
	if (lastBackslash == NULL)
	{
		// TODO
		return FALSE;
	}

	_tcsncpy(szCoCPath, szThisPath, lastBackslash + 1 - szThisPath);

	// Which DLL to use depends on bitnesss. Return is TRUE iif process runs in 32 bits on Windows 64.
	BOOL isWow64 = FALSE;
	if (!IsWow64Process(hProcess, &isWow64))
	{
		// TODO
		return FALSE;
	}

	if (!isWow64)
	{
		_tcsncat(szCoCPath, _T("connect-or-cut.dll"), 19);
	}
	else
	{
		_tcsncat(szCoCPath, _T("connect-or-cut32.dll"), 21);
	}

	return LoadProcessLibrary(hProcess, szCoCPath);
}
