#include <iostream>
#include <Windows.h>
#include "HookDLLLoader.hpp"
#include "HookMsgWindow.hpp"
#include "HookThread.hpp"
#include <thread>
#include <tray.hpp>
#include "resource.h"
#pragma comment(lib,"traypp.lib")
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    std::thread th(
        []()
        {
            Tray::Tray tray("My Tray", (WORD)IDI_ICON1);
            tray.addEntry(Tray::Button("Exit", [&] {
                tray.exit();
                }));
            tray.run();
            exit(0);
        }
    );
    th.detach();

    HookDLLLoader::load();
    HookMsgWindow window;

    HookDLLLoader::initialize(window.hwnd_msg, window.msgid);
    
    HookThread::create();

    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
}
