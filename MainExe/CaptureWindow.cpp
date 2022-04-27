#include "CaptureWindow.h"

#include <iostream>
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
#include <roerrorapi.h>
#include <shlobj_core.h>
#include <dwmapi.h>
#include <thread>
#pragma comment(lib,"Dwmapi.lib")
#pragma comment(lib,"windowsapp.lib")

#include <gl/glew.h>
#include <gl/wglew.h>
#include <gl/wglext.h>
void CaptureWindow::InitCaptureWindow() {
    // Init COM
    winrt::init_apartment(winrt::apartment_type::multi_threaded);


    winrt::check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(), nullptr, nullptr));

    const auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    {
        winrt::com_ptr<::IInspectable> inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put()));
        device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    }

    auto idxgiDevice2 = dxgiDevice.as<IDXGIDevice2>();
    winrt::com_ptr<IDXGIAdapter> adapter;
    winrt::check_hresult(idxgiDevice2->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
    winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

    d3dDevice->GetImmediateContext(&d3dContext);

    activationFactory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    glewInit();
    wglDXOpenDeviceNV = (decltype(wglDXOpenDeviceNV))wglGetProcAddress("wglDXOpenDeviceNV");

}
GLuint CaptureWindow::Capture(HWND hwndTarget)
{
    RECT rect{};
    DwmGetWindowAttribute(hwndTarget, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT));
    const auto size = winrt::Windows::Graphics::SizeInt32{ rect.right - rect.left, rect.bottom - rect.top };
    
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool =
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            device,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            size);

    auto interopFactory = activationFactory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem captureItem = { nullptr };

    auto hr = interopFactory->CreateForWindow(hwndTarget, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
        reinterpret_cast<void**>(winrt::put_abi(captureItem)));
    winrt::check_hresult(hr);

    auto isFrameArrived = false;
    winrt::com_ptr<ID3D11Texture2D> texture;

    m_framePool.FrameArrived([&](auto& framePool, auto&)
        {

            if (isFrameArrived) return;
            auto frame = framePool.TryGetNextFrame();

            struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
                IDirect3DDxgiInterfaceAccess : ::IUnknown
            {
                virtual HRESULT __stdcall GetInterface(GUID const& id, void** object) = 0;
            };

            auto access = frame.Surface().as<IDirect3DDxgiInterfaceAccess>();
            access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());
            isFrameArrived = true;
            return;
        });

    const auto session = m_framePool.CreateCaptureSession(captureItem);
    session.IsCursorCaptureEnabled(false);
    session.StartCapture();


    // Message pump
    MSG msg;
    clock_t timer = clock();
    while (!isFrameArrived)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
            DispatchMessage(&msg);

        if (clock() - timer > 20000)
        {
            // TODO: try to make here a better error handling
            throw std::string("Window capture timeout");
        }
    }

    session.Close();

    D3D11_TEXTURE2D_DESC capturedTextureDesc;
    texture->GetDesc(&capturedTextureDesc);

    capturedTextureDesc.Usage = D3D11_USAGE_STAGING;
    capturedTextureDesc.BindFlags = 0;
    capturedTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    capturedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    winrt::com_ptr<ID3D11Texture2D> userTexture = nullptr;
    winrt::check_hresult(d3dDevice->CreateTexture2D(&capturedTextureDesc, NULL, userTexture.put()));
    d3dContext->CopyResource(userTexture.get(), texture.get());
    if (!userTexture)throw std::string("Cannot get texture");

    std::cout << "wglDXOpenDeviceNV at:" << wglDXOpenDeviceNV << std::endl;
    HANDLE hGlDev = wglDXOpenDeviceNV(d3dDevice.get());
    GLuint glTex;
    glGenTextures(1, &glTex);
    GLuint glTexDest;
    glGenTextures(1, &glTexDest);
    HANDLE hGLTx = wglDXRegisterObjectNV(hGlDev, (void*)userTexture.get(), glTex, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
    BOOL lockOK = wglDXLockObjectsNV(hGlDev, 1, &hGLTx);

    glBindTexture(GL_TEXTURE_2D, glTexDest);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, size.Width, size.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glCopyImageSubData(
        glTex, GL_TEXTURE_2D, 0, 0, 0, 0,
        glTexDest, GL_TEXTURE_2D, 0, 0, 0, 0,
        size.Width, size.Height, 1
    );
    wglDXUnlockObjectsNV(hGlDev, 1, &hGLTx);
    wglDXUnregisterObjectNV(hGlDev, hGLTx);
    glDeleteTextures(1, &glTex);
    wglDXCloseDeviceNV(hGlDev);

    return glTexDest;
}