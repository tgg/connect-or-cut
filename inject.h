#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef BOOL(WINAPI *pfnCreateProcess) (LPCTSTR lpApplicationName, 
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation);

typedef struct stCreateProcess {
	pfnCreateProcess fn;
	LPCTSTR lpApplicationName;
	LPTSTR lpCommandLine;
	LPSECURITY_ATTRIBUTES lpProcessAttributes;
	LPSECURITY_ATTRIBUTES lpThreadAttributes;
	BOOL bInheritHandles;
	DWORD dwCreationFlags;
	LPVOID lpEnvironment;
	LPCTSTR lpCurrentDirectory;
	LPSTARTUPINFO lpStartupInfo;
	LPPROCESS_INFORMATION lpProcessInformation;
} stCreateProcess;

BOOL CreateProcessThenInject(stCreateProcess *lpArgs, BOOL *isConsole);

BOOL LoadCoCLibrary(HANDLE hProcess, BOOL is64Bit);

PSTR GetImageName(stCreateProcess *lpArgs);