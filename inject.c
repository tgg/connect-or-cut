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
#include <SDKDDKVer.h>
#include <tchar.h>
#include <Windows.h>

#ifdef UNICODE
#define LoadLib "LoadLibraryW"
#else
#define LoadLib "LoadLibraryA"
#endif

BOOL LibraryInject(HANDLE hProcess, TCHAR *szPath)
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

	CloseHandle(hThread);

	return TRUE;
}
