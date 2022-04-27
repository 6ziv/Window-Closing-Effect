#pragma once
#include <Windows.h>
#include <dxgi.h>
#include <inspectable.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <gl/glew.h>
class CaptureWindow {
public:
	inline static winrt::com_ptr<ID3D11Device> d3dDevice;
	inline static winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device;
	inline static winrt::com_ptr<IDXGIFactory2> factory;
	inline static ID3D11DeviceContext* d3dContext = nullptr;
	inline static winrt::Windows::Foundation::IActivationFactory activationFactory;
	static void InitCaptureWindow();
	static GLuint Capture(HWND hwndTarget);
};