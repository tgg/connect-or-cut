#include <sdkddkver.h>
#include <tchar.h>
#include <Windows.h>
#include <strsafe.h>

/*
 * This function is based on:
 *  https://msdn.microsoft.com/en-us/library/windows/desktop/ms680582(v=vs.85).aspx
 */
void ErrorExit(LPTSTR lpszFunction)
{
	LPVOID lpError;
	LPVOID lpDisplayBuf;
	DWORD dwLastError = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpError, 0, NULL);

	lpDisplayBuf = (LPVOID) LocalAlloc(
		LMEM_ZEROINIT,
		(_tcslen((LPTSTR) lpError) + _tcslen(lpszFunction) + 14) * sizeof(TCHAR));
	StringCchPrintf(
		(LPTSTR) lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		_T("%s failed: %s"),
		lpszFunction, lpError);

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, _T("connect-or-cut"), MB_OK | MB_ICONERROR);

	LocalFree(lpError);
	LocalFree(lpDisplayBuf);
	ExitProcess(dwLastError);
}
