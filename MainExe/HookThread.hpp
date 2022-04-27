#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include <winrt/base.h>
#include "HookDLLLoader.hpp"
class HookThread
{
	inline static DWORD WINAPI ThreadProc(
        _In_ LPVOID lpParameter
    ) {

        MSG msg;
        PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
        if (!HookDLLLoader::sethooks())
            ExitProcess(0);

        std::cout << "Hook done" << std::endl;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }
public:
    inline static void create(){
        HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
        if (hThread == NULL)throw std::string("CreateThread failed");
    }
};

