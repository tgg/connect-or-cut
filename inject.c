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
#include "inject.h"

#ifdef UNICODE
#define LoadLib "LoadLibraryW"
#else
#define LoadLib "LoadLibraryA"
#endif

BOOL LoadProcessLibrary(HANDLE hProcess, LPTSTR lpszLibraryPath)
{
	if (hProcess == NULL || lpszLibraryPath == NULL)
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

	size_t iLibraryPathLen = _tcslen(lpszLibraryPath) + 1;
	SIZE_T dwSize = iLibraryPathLen * sizeof(TCHAR);

	LPVOID lpPath = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (lpPath == NULL)
	{
		// Could not allocate memory in the process. GetLastError() will contain
		// relevant error.
		return FALSE;
	}

	if (!WriteProcessMemory(hProcess, lpPath, lpszLibraryPath, dwSize, NULL))
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

	if (!VirtualFreeEx(hProcess, lpPath, 0, MEM_RELEASE))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL LoadCoCLibrary(stCreateProcess *lpArgs, BOOL bResumeThread)
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

	LPTSTR lpszLastBackslash = _tcsrchr(szThisPath, _T('\\'));
	if (lpszLastBackslash == NULL)
	{
		SetLastError(ERROR_BAD_PATHNAME);
		return FALSE;
	}

	_tcsncpy(szCoCPath, szThisPath, lpszLastBackslash + 1 - szThisPath);

	// If the process being launched differs in bitness from us, then rerun it with
	// correct coc.exe (or coc32.exe)
	BOOL isOtherWow64 = FALSE;
	if (!IsWow64Process(lpArgs->lpProcessInformation->hProcess, &isOtherWow64))
	{
		return FALSE;
	}

	BOOL isThisWow64 = FALSE;
	if (!IsWow64Process(GetCurrentProcess(), &isThisWow64))
	{
		return FALSE;
	}

	if (isOtherWow64 != isThisWow64)
	{
		// We won't be able to inject here. So kill this suspended process and
		// try again with coc.exe (or coc32.exe)
		// TODO: TerminateProcess is asynchronous; we don't wait for completion but we 
		// probably should.
		if (!TerminateProcess(lpArgs->lpProcessInformation->hProcess, 0))
		{
			return FALSE;
		}

		CloseHandle(lpArgs->lpProcessInformation->hProcess);
		CloseHandle(lpArgs->lpProcessInformation->hThread);

		TCHAR *lpCoc = isOtherWow64 ? _T("coc32.exe ") : _T("coc.exe ");
		SIZE_T szLen = isOtherWow64 ? 10 : 8;

		_tcsncat(szCoCPath, lpCoc, szLen);
		_tcscat(szCoCPath, lpArgs->lpCommandLine);

		ZeroMemory(lpArgs->lpStartupInfo, sizeof(*lpArgs->lpStartupInfo));
		ZeroMemory(lpArgs->lpProcessInformation, sizeof(*lpArgs->lpProcessInformation));
		// Store to restore later
		LPTSTR lpCommandLine = lpArgs->lpCommandLine;
		LPCTSTR lpApplicationName = lpArgs->lpApplicationName;
		lpArgs->lpCommandLine = szCoCPath;
		lpArgs->lpApplicationName = NULL;

		if (!IndirectCreateProcess(lpArgs))
		{
			return FALSE;
		}
		else
		{
			lpArgs->lpCommandLine = lpCommandLine;
			lpArgs->lpApplicationName = lpApplicationName;
		}
	}
	else
	{
		if (!isOtherWow64)
		{
			_tcsncat(szCoCPath, _T("connect-or-cut.dll"), 19);
		}
		else
		{
			_tcsncat(szCoCPath, _T("connect-or-cut32.dll"), 21);
		}

		if (!LoadProcessLibrary(lpArgs->lpProcessInformation->hProcess, szCoCPath))
		{
			return FALSE;
		}

		if (bResumeThread && ResumeThread(lpArgs->lpProcessInformation->hThread) == -1)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL IndirectCreateProcess(stCreateProcess *lpArgs)
{
	if (lpArgs == NULL || lpArgs->fn == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	return (*lpArgs->fn)(lpArgs->lpApplicationName, lpArgs->lpCommandLine, lpArgs->lpProcessAttributes,
		lpArgs->lpThreadAttributes, lpArgs->bInheritHandles, lpArgs->dwCreationFlags,
		lpArgs->lpEnvironment, lpArgs->lpCurrentDirectory, lpArgs->lpStartupInfo, lpArgs->lpProcessInformation);
}
