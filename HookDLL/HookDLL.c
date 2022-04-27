#include <Windows.h>
#include <stdio.h>
static HANDLE hMapObject = NULL;
typedef struct
{
	HWND hWnd;
	UINT message;
	HHOOK hooks[2];
} SharedData;
SharedData* lpvMem;
HINSTANCE hModule;
BOOL WINAPI DllMain(HINSTANCE hinstDLL,  // DLL module handle
	DWORD fdwReason,              // reason called 
	LPVOID lpvReserved)           // reserved 
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hModule = hinstDLL;
		hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), TEXT("dllmemfilemap"));
		if (hMapObject == NULL)
			return FALSE;
		lpvMem = (SharedData*)MapViewOfFile(hMapObject, FILE_MAP_WRITE, 0, 0, 0);
		if (lpvMem == NULL)
			return FALSE;
		lpvMem->hooks[0] = NULL;
		lpvMem->hooks[1] = NULL;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		if (lpvMem->hooks[0])UnhookWindowsHookEx(lpvMem->hooks[0]);
		if (lpvMem->hooks[1])UnhookWindowsHookEx(lpvMem->hooks[1]);
		SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);//release dlls
		UnmapViewOfFile(lpvMem);
		CloseHandle(hMapObject);
		break;
	default:
		break;
	}
	return TRUE;
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);
}
__declspec(dllexport) VOID CALLBACK Initialize(HWND hWnd, UINT msg) {
	//MessageBoxA(0, "Init", "Init", MB_OK);
	lpvMem->hWnd = hWnd;
	lpvMem->message = msg;
}

__declspec(dllexport) LRESULT CALLBACK MsgProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	if (nCode < 0)
		return CallNextHookEx(lpvMem->hooks[1], nCode, wParam, lParam);
	if (nCode == HC_ACTION) {
		CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;
		if (cwp->message == WM_SHOWWINDOW && cwp->wParam == FALSE) {
			HWND hwnd = cwp->hwnd;
			if (
				IsWindow(hwnd) &&
				GetParent(hwnd) == NULL &&
				IsWindowVisible(hwnd) &&
				(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == 0
				) {
				char tmp[1024];
				char tmp2[1024];
				GetWindowTextA(hwnd, tmp, 1024);

				RECT rect;
				GetWindowRect(hwnd, &rect);
				sprintf_s(tmp2, 1024, "Hiding: %s %dx%d", tmp, rect.right - rect.left, rect.bottom - rect.top);
				OutputDebugStringA(tmp2);
				SendMessage(lpvMem->hWnd, lpvMem->message, (WPARAM)hwnd, (LPARAM)NULL);
				OutputDebugStringA("Event Fired");
			}
		}
	}
	return CallNextHookEx(lpvMem->hooks[1], nCode, wParam, lParam);
}
__declspec(dllexport) LRESULT CALLBACK CBTProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (nCode != HCBT_DESTROYWND)
		return CallNextHookEx(lpvMem->hooks[0], nCode, wParam, lParam);

	HWND hwnd = (HWND)(wParam);
	
	if (
		IsWindow(hwnd) && 
		GetParent(hwnd) == NULL && 
		IsWindowVisible(hwnd) && 
		(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == 0
		) {
		char tmp[1024];
		char tmp2[1024];
		GetWindowTextA(hwnd, tmp, 1024);

		RECT rect;
		GetWindowRect(hwnd, &rect);
		sprintf_s(tmp2, 1024, "Destroying: %s %dx%d", tmp, rect.right - rect.left, rect.bottom - rect.top);
		OutputDebugStringA(tmp2);
		SendMessage(lpvMem->hWnd, lpvMem->message, (WPARAM)hwnd, (LPARAM)NULL);
		OutputDebugStringA("Event Fired");
		return 0;
	}

	return CallNextHookEx(lpvMem->hooks[0], nCode, wParam, lParam);
}

__declspec(dllexport) BOOL CALLBACK setHooks()
{
	lpvMem->hooks[0] = SetWindowsHookExA(WH_CBT, CBTProc, hModule, 0);
	if (lpvMem->hooks[0] == NULL) {
		return FALSE;
	}

	lpvMem->hooks[1] = SetWindowsHookExA(WH_CALLWNDPROC, MsgProc, hModule, 0);
	if (lpvMem->hooks[1] == NULL) {
		UnhookWindowsHookEx(lpvMem->hooks[0]);
		lpvMem->hooks[0] = NULL;
		return FALSE;
	}
	return TRUE;
}