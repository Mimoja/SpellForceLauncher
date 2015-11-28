#include "stdio.h"
#include "windows.h"
#include <iostream>

//#define absolutePath
//#define inject

using namespace std;

#ifdef inject 
char * dllPath = "injection.dll";
#endif

#ifdef absolutePath
LPWSTR gameBinary = L"G:\\SteamLibrary\\steamapps\\common\\Spellforce Platinum Edition\\SpellForce.exe";
LPWSTR workingDir = L"G:\\SteamLibrary\\steamapps\\common\\Spellforce Platinum Edition";
#else
LPWSTR gameBinary = L"SpellForce.exe";
LPWSTR workingDir = L".";
#endif
LPWSTR spellforceArgs = L" -window";

int setDebugPrivilege();

int main()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	printf("Starting Spellforce\n");

	if (!CreateProcess(gameBinary,
		spellforceArgs,      
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,  // Create the process sleeping CREATE_SUSPENDED
		NULL,
		workingDir,
		&si,
		&pi)
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return -1;
	}

	wprintf(L"Sucessfully started Spellforce with pid %d\n", pi.dwProcessId);

#ifdef inject 
	if (setDebugPrivilege()) {
		printf("setDebugPrivilege failed (%d).\n", GetLastError());
		return -1;
	}
	// Alloc memory INSIDE of the child process
	printf("Locating memory for dll\n");
	LPVOID AllocAdresse = (LPVOID)VirtualAllocEx(pi.hProcess, NULL, strlen(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (AllocAdresse == NULL) {
		printf("Error: the memory could not be allocated inside the chosen process.\n");
	}


	// Write dll file there
	printf("Copying the dll\n");
	if (!WriteProcessMemory(pi.hProcess, AllocAdresse, (void*)dllPath, sizeof(dllPath), 0)) {
		printf("Could not load dll!\n");
		return -1;
	}

	printf("Preparing dll jump adress\n");
	HMODULE hModule = GetModuleHandle(L"kernel32.dll");
	FARPROC  loadLibraryAddress = GetProcAddress(hModule, "LoadLibraryA");
	if (loadLibraryAddress == NULL) {
		printf("Could not get jump adress! (%d).\n", GetLastError());
		return -1;
	}
	printf("Injecting DLL thread\n");
	HANDLE hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, AllocAdresse, NULL, NULL);
	if (hRemoteThread == NULL) {
		printf("Error: the remote thread could not be created.\n");
	}
	else {
		printf("Success: the remote thread was successfully created.\n");
	}
	CloseHandle(hRemoteThread);

#endif

	//unpause
	printf("Resuming Spellforce\n");
	if (ResumeThread(pi.hThread) == -1) {
		printf("Could not resume Spellforce! (%d).\n", GetLastError());
		return -1;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	// Fullscreen after 2 seconds
	Sleep(2000);

	// Get window handle
	HWND spellForceWindow = FindWindow(NULL,L"SpellForce");
	if (spellForceWindow == NULL) {
		printf("Error: Could not find Spellforce Window (%d).\n", GetLastError());
	}
	// change style
	LONG lStyle = GetWindowLong(spellForceWindow, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU);
	SetWindowLong(spellForceWindow, GWL_STYLE, lStyle);

	RECT rect, rcClient;
	GetWindowRect(spellForceWindow, &rect);
	GetClientRect(spellForceWindow, &rcClient);

	// move window
	SetWindowPos(spellForceWindow, HWND_TOP,0, 0, rcClient.right - rcClient.left, rcClient.bottom- rcClient.top, SWP_NOSIZE);
	// Wait for program exit
	WaitForSingleObject(pi.hThread, INFINITE);

#ifdef inject 
	WaitForSingleObject(hRemoteThread, INFINITE);
	//clear the memory
	VirtualFreeEx(pi.hProcess, AllocAdresse, sizeof(dllPath), MEM_DECOMMIT);
#endif
    return 0;
}

int setDebugPrivilege()
{
	printf("Setting debug priviledge\n");
	HANDLE hProcess = GetCurrentProcess(), hToken;
	TOKEN_PRIVILEGES priv;
	LUID luid;

	OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)){
		return -1;
	}
	priv.PrivilegeCount = 1;
	priv.Privileges[0].Luid = luid;
	priv.Privileges[0].Attributes = 1 ? SE_PRIVILEGE_ENABLED : 0;
	AdjustTokenPrivileges(hToken, FALSE, &priv, 0, 0, 0);
	return 0;
}
