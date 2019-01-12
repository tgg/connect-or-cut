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
#include <strsafe.h>
#include "inject.h"
#pragma warning(push)
#pragma warning(disable : 4091)
#include <ImageHlp.h>
#pragma warning(pop)
#pragma comment(lib, "Imagehlp.lib")

#ifdef UNICODE
#define LoadLib "LoadLibraryW"
#else
#define LoadLib "LoadLibraryA"
#endif

// Yup, it's duplicated from error.c to avoid moving this into
// another shared library.
static void MessageBoxError(DWORD dwLastError, LPTSTR lpszFunction)
{
	LPVOID lpError;
	LPVOID lpDisplayBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpError, 0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(
		LMEM_ZEROINIT,
		(_tcslen((LPTSTR)lpError) + _tcslen(lpszFunction) + 14) * sizeof(TCHAR));
	StringCchPrintf(
		(LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		_T("%s failed: %s"),
		lpszFunction, lpError);

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, _T("connect-or-cut"), MB_OK | MB_ICONERROR);

	LocalFree(lpError);
	LocalFree(lpDisplayBuf);
}

static BOOL CheckNtImage(PCSTR lpszLibraryPath, BOOL *is64Bit, BOOL *isConsole)
{
	if (!lpszLibraryPath || !is64Bit || !isConsole)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	BOOL ret = FALSE;
	PLOADED_IMAGE pImage = ImageLoad(lpszLibraryPath, NULL);

	if (pImage != NULL)
	{
		IMAGE_NT_HEADERS *pNtHdr = pImage->FileHeader;

		if (pNtHdr->Signature == IMAGE_NT_SIGNATURE)
		{
			ret = TRUE;

			if (pNtHdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
				*is64Bit = FALSE;
			else if (pNtHdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
				*is64Bit = TRUE;
			else // unsupported
				ret = FALSE;

			if (pNtHdr->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
				*isConsole = TRUE;
			else if (pNtHdr->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
				*isConsole = FALSE;
			else // unknown?
				ret = FALSE;
		}

		ImageUnload(pImage);
	}

	return ret;
}

BOOL LoadProcessLibrary(HANDLE hProcess, LPTSTR lpszLibraryPath)
{
	if (hProcess == NULL || lpszLibraryPath == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
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

BOOL CoCPath(LPTSTR szCoCPath)
{
	TCHAR szThisPath[MAX_PATH] = { 0 };

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
	return TRUE;
}

BOOL CoCExePath(LPTSTR lpPath, BOOL is64Bit)
{
	if (!CoCPath(lpPath))
	{
		return FALSE;
	}

	LPCTSTR lpExe = is64Bit ? _T("coc.exe ") : _T("coc32.exe ");
	SIZE_T szLen = is64Bit ? 8 : 10;
	_tcsncat(lpPath, lpExe, szLen);

	return TRUE;
}

BOOL CoCLibPath(LPTSTR lpPath, BOOL is64Bit)
{
	if (!CoCPath(lpPath))
	{
		return FALSE;
	}

	LPCTSTR lpLib = is64Bit ? _T("connect-or-cut.dll") : _T("connect-or-cut32.dll");
	SIZE_T szLen = is64Bit ? 19 : 21;
	_tcsncat(lpPath, lpLib, szLen);

	return TRUE;
}

BOOL LoadCoCLibrary(HANDLE hProcess, BOOL is64Bit)
{
	TCHAR szCoCPath[MAX_PATH] = { 0 };

	if (!CoCLibPath(szCoCPath, is64Bit))
	{
		return FALSE;
	}

	if (!LoadProcessLibrary(hProcess, szCoCPath))
	{
		return FALSE;
	}

	return TRUE;
}

static inline PSTR ExeWithoutQuotes(PSTR lpExe)
{
	PSTR exe = lpExe;

	if (exe)
	{
		if (exe[0] == '"')
		{
			exe++;
			size_t len = strlen(exe);
			if (len > 0 && exe[len - 1] == '"')
			{
				exe[len - 1] = '\0';
			}
		}
	}

	return exe;
}

//
// Creates the process specified in lpArgs, using passed function.
//
// Before doing so, ensure we will be able to inject connect-or-cut
// by comparing our bitness with the one of the process to run. If they differ
// then we rerun the appropriate coc.exe (or coc32.exe). If they don't then
// we inject using regular mechanism.
//
BOOL CreateProcessThenInject(stCreateProcess *lpArgs, BOOL *isConsole)
{
	if (lpArgs == NULL || lpArgs->fn == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	BOOL isThisWow64 = FALSE;
	if (!IsWow64Process(GetCurrentProcess(), &isThisWow64))
	{
		return FALSE;
	}

	PSTR exe = GetImageName(lpArgs);
	BOOL is64Bit = FALSE;

	if (!CheckNtImage(ExeWithoutQuotes(exe), &is64Bit, isConsole))
	{
		DWORD dwLastError = GetLastError();
		LocalFree(exe);
		SetLastError(dwLastError);
		return FALSE;
	}

	LocalFree(exe);
	if (isThisWow64 != !is64Bit)
	{
		// Need to rerun with coc.exe (or coc32.exe), so we'll alter the command-line
		TCHAR szCoCPath[MAX_PATH] = { 0 };
		if (!CoCExePath(szCoCPath, is64Bit))
		{
			return FALSE;
		}

		_tcscat(szCoCPath, lpArgs->lpCommandLine);

		// Store to restore later
		LPTSTR lpCommandLine = lpArgs->lpCommandLine;
		LPCTSTR lpApplicationName = lpArgs->lpApplicationName;
		lpArgs->lpCommandLine = szCoCPath;
		lpArgs->lpApplicationName = NULL;
		BOOL bCreateProcess = (*lpArgs->fn)(lpArgs->lpApplicationName, lpArgs->lpCommandLine, lpArgs->lpProcessAttributes,
			lpArgs->lpThreadAttributes, lpArgs->bInheritHandles, lpArgs->dwCreationFlags,
			lpArgs->lpEnvironment, lpArgs->lpCurrentDirectory, lpArgs->lpStartupInfo, lpArgs->lpProcessInformation);
		lpArgs->lpCommandLine = lpCommandLine;
		lpArgs->lpApplicationName = lpApplicationName;

		return bCreateProcess;
	}
	else
	{
		BOOL bNeedsResume = !(((lpArgs->dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED));
		lpArgs->dwCreationFlags |= CREATE_SUSPENDED;
		BOOL bCreateProcess = (*lpArgs->fn)(lpArgs->lpApplicationName, lpArgs->lpCommandLine, lpArgs->lpProcessAttributes,
			lpArgs->lpThreadAttributes, lpArgs->bInheritHandles, lpArgs->dwCreationFlags,
			lpArgs->lpEnvironment, lpArgs->lpCurrentDirectory, lpArgs->lpStartupInfo, lpArgs->lpProcessInformation);
		if (bNeedsResume)
		{
			lpArgs->dwCreationFlags &= ~CREATE_SUSPENDED;
		}

		if (bCreateProcess)
		{
			if (!LoadCoCLibrary(lpArgs->lpProcessInformation->hProcess, is64Bit))
			{
				// Injection failed. In that case we report the error and let the
				// process continue.
				MessageBoxError(GetLastError(), _T("LoadCoCLibrary"));
			}

			if (bNeedsResume && ResumeThread(lpArgs->lpProcessInformation->hThread) == -1)
			{
				// Unable to resume the main thread. We terminate the process to
				// avoid it from being in a hung state.
				DWORD dwLastError = GetLastError();
				TerminateProcess(lpArgs->lpProcessInformation->hProcess, 0xdeadbeef);
				WaitForSingleObject(lpArgs->lpProcessInformation->hProcess, INFINITE);
				SetLastError(dwLastError);
				return FALSE;
			}

		}

		return bCreateProcess;
	}
}

static TCHAR* MoveBeforeFirstArgument(TCHAR *lpCommandLine)
{
	TCHAR* p = lpCommandLine;
	BOOL bInQuotes = FALSE;
	size_t uiBackslashes = 0;

	while (*p != _T('\0'))
	{
		if (*p == _T('\\'))
		{
			uiBackslashes++;
		}
		else if (*p == _T('"'))
		{
			if (uiBackslashes % 2 == 0)
			{
				bInQuotes = !bInQuotes;
			}

			uiBackslashes = 0;
		}
		else if (*p == _T(' '))
		{
			uiBackslashes = 0;

			if (!bInQuotes)
				break;
		}
		else
		{
			uiBackslashes = 0;
		}

		p++;
	}

	return p;
}

static PSTR StringFromUnicode(LPCTSTR lpSource, size_t dstLen)
{
	size_t converted;
	PSTR dst = (PSTR) LocalAlloc(LMEM_ZEROINIT, dstLen);
	wcstombs_s(&converted, dst, dstLen, lpSource, _TRUNCATE);
	return dst;
}

PSTR GetImageName(stCreateProcess *lpArgs)
{
	if (lpArgs == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	if (lpArgs->lpApplicationName)
	{
		return StringFromUnicode(lpArgs->lpApplicationName, (_tcslen(lpArgs->lpApplicationName) + 1) * sizeof(TCHAR));
	}
	else
	{
		LPTSTR lpFirstArgument = MoveBeforeFirstArgument(lpArgs->lpCommandLine);
		return StringFromUnicode(lpArgs->lpCommandLine, lpFirstArgument + 1 - lpArgs->lpCommandLine);
	}
}