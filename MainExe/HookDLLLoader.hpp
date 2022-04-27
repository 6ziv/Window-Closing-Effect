#pragma once
#include <Windows.h>
#include <string>
class HookDLLLoader
{
public:
	typedef VOID(CALLBACK* INITIALIZE)(HWND, UINT);
    typedef BOOL(CALLBACK* SETHOOKS)();
	inline static HMODULE hMod;
	inline static INITIALIZE initialize;
    inline static SETHOOKS sethooks;
	inline static FARPROC hookProc;
    inline static FARPROC hook2Proc;
	inline static void load()
	{
        hMod = LoadLibraryA("HookDLL.dll");
        if (hMod == NULL) {
            DWORD err = GetLastError();
            throw(std::string("LoadLibraryA failed")+std::to_string(err));
        }
        FARPROC hProc = GetProcAddress(hMod, "CBTProc");
        if (hProc == NULL) {
            DWORD err = GetLastError();
            throw(std::string("GetProcAddress failed") + std::to_string(err));
        }
        
        FARPROC hProc2 = GetProcAddress(hMod, "Initialize");
        if (hProc2 == NULL) {
            DWORD err = GetLastError();
            throw(std::string("GetProcAddress failed") + std::to_string(err));
        }

        FARPROC hProc3 = GetProcAddress(hMod, "MsgProc");
        if (hProc3 == NULL) {
            DWORD err = GetLastError();
            throw(std::string("GetProcAddress failed") + std::to_string(err));
        }

        FARPROC hProc4 = GetProcAddress(hMod, "setHooks");
        if (hProc4 == NULL) {
            DWORD err = GetLastError();
            throw(std::string("GetProcAddress failed") + std::to_string(err));
        }
        hookProc = hProc;
        initialize = (INITIALIZE)(hProc2);
        hook2Proc = hProc3;
        sethooks = (SETHOOKS)(hProc4);
	}
};

