#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "stdio.h"


BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ){
	FILE* file;
	fopen_s(&file, "temp.txt", "a+");

	switch (ul_reason_for_call){
	case DLL_PROCESS_ATTACH:
		fprintf(file, "DLL attach function called.\n");
		break;
	case DLL_PROCESS_DETACH:
		fprintf(file, "DLL detach function called.\n");
		break;
	case DLL_THREAD_ATTACH:
		fprintf(file, "DLL thread attach function called.\n");
		break;
	case DLL_THREAD_DETACH:
		fprintf(file, "DLL thread detach function called.\n");
		break;
	}
	fclose(file);
	return TRUE;
}

