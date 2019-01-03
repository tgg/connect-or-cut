/* coc -- Injects connect-or-cut DLL into a new Windows process.
*
* Copyright Ⓒ 2017, 2018  Thomas Girard <thomas.g.girard@free.fr>
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
#include <sdkddkver.h>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

#include "inject.h"

extern void ErrorExit(LPTSTR lpszFunction);

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

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 1)
	{
		_tprintf(_T("Usage: %s COMMAND [ARGS]\n"), argv[0]);
		return 1;
	}

 	TCHAR *lpThisCommandLine = GetCommandLine();

	size_t lpThisLen = _tcslen(lpThisCommandLine) + 1;
	HANDLE hHeap = GetProcessHeap();
	if (hHeap == NULL)
	{
		ErrorExit(_T("GetProcessHeap"));
	}

	TCHAR *lpThatCommandLine = HeapAlloc(hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, lpThisLen * sizeof(TCHAR));
	if (lpThatCommandLine == NULL)
	{
		ErrorExit(_T("HeapAlloc"));
	}

	_tcsncpy(lpThatCommandLine, lpThisCommandLine, lpThisLen);

	TCHAR *lpOtherCommandLine = MoveBeforeFirstArgument(lpThatCommandLine);

	*lpOtherCommandLine = _T('\0');
	lpOtherCommandLine++;

	// Weird
	if (*lpOtherCommandLine == _T(' '))
		lpOtherCommandLine++;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	stCreateProcess cpArgs = {
		.fn = CreateProcess,
		.lpApplicationName = NULL,
		.lpCommandLine = lpOtherCommandLine,
		.lpProcessAttributes = NULL,
		.lpThreadAttributes = NULL,
		.bInheritHandles = TRUE,
		.dwCreationFlags = 0,
		.lpEnvironment = NULL,
		.lpCurrentDirectory = NULL,
		.lpStartupInfo = &si,
		.lpProcessInformation = &pi
	};

	BOOL bWaitForCompletion;

	if (!CreateProcessThenInject(&cpArgs, &bWaitForCompletion))
	{
		ErrorExit(_T("CreateProcess"));
	}

	HeapFree(hHeap, HEAP_NO_SERIALIZE, lpThatCommandLine);
	lpThatCommandLine = NULL;

	if (!bWaitForCompletion)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return 0;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwExitCode;
	GetExitCodeProcess(pi.hProcess, &dwExitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return dwExitCode;
}
