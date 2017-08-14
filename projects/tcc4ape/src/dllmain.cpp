#include <Windows.h>
#include "Tcc4APE.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

static bool searchChanged = false;

static int AddDllPath()
{
	/*
		This part is super crucial: Since we are hosted by another application, windows will search
		for our dependencies (dll's like scilexer and libtcc) in our host's folder - vstplugins folder
		is usually not inside that, so we add another path based off DirectoryPath.
	*/
	if(!searchChanged) {
		///SetDllDirectoryA(TCC4Ape::GetDirectoryPath().c_str());
		searchChanged = true;
	}
	return 0xbadc0de;
}

static int dummy = AddDllPath();