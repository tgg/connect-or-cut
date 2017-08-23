/* coc -- Injects connect-or-cut DLL into a new Windows process.
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

#include <sdkddkver.h>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

extern BOOL LoadCoCLibrary(HANDLE);

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 0)
	{
		wprintf(L"Usage: %s COMMAND [ARGS]\n", argv[0]);
		return 1;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	// Build the full command-line, making sure to add "" around each argument.
	LPWSTR lpwsCommandLine = GetCommandLine();

	if (!CreateProcess(NULL, argv[1], NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
	{
		// TODO
	}

	LoadCoCLibrary(pi.hProcess);

	ResumeThread(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwExitCode;
	GetExitCodeProcess(pi.hProcess, &dwExitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return dwExitCode;
}
