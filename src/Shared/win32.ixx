module;

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d11.h>
#include <crtdbg.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <comdef.h>
#include <directxtk/DDSTextureLoader.h>
#include <directxtk/WICTextureLoader.h>

export module shared:win32;

export template<auto X>
struct ConstValue
{
	constexpr operator decltype(X)() noexcept
	{
		return X;
	}
	static constexpr auto operator()() noexcept -> decltype(X)
	{
		return X;
	}
};

export namespace Win32
{
	constexpr auto IdiApplication = ConstValue<IDI_APPLICATION>{};
	constexpr auto IdcArrow = ConstValue<IDC_ARROW>{};
	constexpr auto NullBrush = NULL_BRUSH;
	constexpr auto CrtAllocMemDf = _CRTDBG_ALLOC_MEM_DF;
	constexpr auto CrtLeakCheckDf = _CRTDBG_LEAK_CHECK_DF;
	constexpr auto MbOK = MB_OK;

	enum PM : unsigned long
	{
		Remove = PM_REMOVE,
	};

	enum WindowStyles : unsigned long
	{
		OverlappedWindow = WS_OVERLAPPEDWINDOW,
	};

	enum CS
	{
		HRedraw = CS_HREDRAW,
		VRedraw = CS_VREDRAW,
	};

	enum CW
	{
		UseDefault = CW_USEDEFAULT,
	};

	enum SW
	{
		Show = SW_SHOW,
	};

	enum MK
	{
		LButton = MK_LBUTTON,
		RButton = MK_RBUTTON,
		MButton = MK_MBUTTON
	};

	constexpr auto CpUtf8 = CP_UTF8;
	constexpr auto WcNoBestFitChars = WC_NO_BEST_FIT_CHARS;

	// Enable run-time debug checks for debug builds. No effect in release builds.
	constexpr auto SetDebugBuildFlag(int flag) noexcept
	{
#ifdef _DEBUG
		_CrtSetDbgFlag(flag);
#endif
	}

	using
		::HINSTANCE,
		::LPWSTR,
		::WNDCLASS,
		::HWND,
		::HANDLE,
		::LONG,
		::WPARAM,
		::UINT,
		::BYTE,
		::DWORD,
		::HRESULT,
		::HWND,
		::LPCWSTR,
		::LPARAM,
		::MINMAXINFO,
		::GUID,
		::HICON,
		::RECT,
		::LARGE_INTEGER,
		::LRESULT,
		::HBRUSH,
		::HICON,
		::POINT,
		::MSG,
		::_com_error,
		::GetAsyncKeyState,
		::WideCharToMultiByte,
		::MultiByteToWideChar,
		::OutputDebugStringA,
		::PeekMessageW,
		::TranslateMessage,
		::DispatchMessageW,
		::DefWindowProcW,
		::PostQuitMessage,
		::LoadIconW,
		::GetStockObject,
		::GetModuleHandleW,
		::LoadCursorW,
		::ReleaseCapture,
		::SetCapture,
		::AdjustWindowRect,
		::CreateWindowExW,
		::RegisterClassW,
		::ShowWindow,
		::UpdateWindow,
		::MessageBoxA,
		::MessageBoxW,
		::SetWindowTextW,
		::QueryPerformanceFrequency,
		::QueryPerformanceCounter
		;

	constexpr auto Failed(HRESULT hr) noexcept -> bool
	{
		return FAILED(hr);
	}
	constexpr auto Succeeded(HRESULT hr) noexcept -> bool
	{
		return SUCCEEDED(hr);
	}
	enum Size
	{
		Minimized = SIZE_MINIMIZED,
		Maximized = SIZE_MAXIMIZED,
		Restored = SIZE_RESTORED
	};
	enum WA
	{
		Inactive = WA_INACTIVE,
		Active = WA_ACTIVE,
		ClickActive = WA_CLICKACTIVE
	};
	enum WindowMessages
	{
		Activate = WM_ACTIVATE,
		Size = WM_SIZE,
		EnterSizeMove = WM_ENTERSIZEMOVE,
		ExitSizeMove = WM_EXITSIZEMOVE,
		Destroy = WM_DESTROY,
		MenuChar = WM_MENUCHAR,
		GetMinMaxInfo = WM_GETMINMAXINFO,
		LButtonDown = WM_LBUTTONDOWN,
		MButtonDown = WM_MBUTTONDOWN,
		RButtonDown = WM_RBUTTONDOWN,
		LButtonUp = WM_LBUTTONUP,
		MButtonUp = WM_MBUTTONUP,
		RButtonUp = WM_RBUTTONUP,
		MouseMove = WM_MOUSEMOVE,
		KeyUp = WM_KEYUP,
		Quit = WM_QUIT
	};
	enum MNC//MenuChar
	{
		Close = MNC_CLOSE,
	};
	constexpr auto GetXLParam(auto lParam) noexcept -> auto
	{
		return GET_X_LPARAM(lParam);
	}
	constexpr auto GetYLParam(auto lParam) noexcept -> auto
	{
		return GET_Y_LPARAM(lParam);
	}
	constexpr auto MakeLResult(auto a, auto b) noexcept -> auto
	{
		return MAKELRESULT(a, b);
	}
	constexpr auto LoWord(auto dw) noexcept -> auto
	{
		return LOWORD(dw);
	}
	constexpr auto HiWord(auto dw) noexcept -> auto
	{
		return HIWORD(dw);
	}
}

export
{
	using 
		::DirectX::operator*,
		::DirectX::operator-,
		::DirectX::operator+
		;
}

export namespace DirectX
{
	constexpr auto Pi = DirectX::XM_PI;
	constexpr auto TwoPi = DirectX::XM_2PI;

	using
		::DirectX::XMFLOAT4,
		::DirectX::XMMATRIX,
		::DirectX::CXMMATRIX,
		::DirectX::FXMVECTOR,
		::DirectX::CXMVECTOR,
		::DirectX::XMVECTORF32,
		::DirectX::XMFLOAT3,
		::DirectX::XMFLOAT2,
		::DirectX::XMFLOAT4X4,
		::DirectX::XMVECTOR,
		::DirectX::XMLoadFloat3,
		::DirectX::XMLoadFloat4x4,
		::DirectX::XMVectorSet,
		::DirectX::XMMatrixIdentity,
		::DirectX::XMMatrixMultiply,
		::DirectX::XMMatrixLookAtLH,
		::DirectX::XMMatrixSet,
		::DirectX::XMMatrixLookToLH,
		::DirectX::XMLoadFloat4x4,
		::DirectX::XMConvertToRadians,
		::DirectX::XMMatrixPerspectiveFovLH,
		::DirectX::XMMatrixScaling,
		::DirectX::XMMatrixTranslation,
		::DirectX::XMStoreFloat4x4,
		::DirectX::XMStoreFloat3,
		::DirectX::XMLoadFloat4,
		::DirectX::XMPlaneNormalize,
		::DirectX::XMVectorGetX,
		::DirectX::XMMatrixReflect,
		::DirectX::XMVector3Greater,
		::DirectX::XMVector3Normalize,
		::DirectX::XMVector3Dot,
		::DirectX::XMVector3Cross,
		::DirectX::XMVector3LengthSq,
		::DirectX::XMVector3TransformNormal,
		::DirectX::XMVector3Less,
		::DirectX::XMVector3TransformCoord,
		::DirectX::XMMatrixShadow,
		::DirectX::XMMatrixRotationRollPitchYaw,
		::DirectX::XMMatrixRotationY,
		::DirectX::XMVectorZero,
		::DirectX::XMVectorSet,
		::DirectX::XMMatrixDeterminant,
		::DirectX::XMMatrixTranspose,
		::DirectX::XMMatrixInverse,
		::DirectX::XMStoreFloat4
		;

	using
		::DirectX::CreateDDSTextureFromFile,
		::DirectX::CreateWICTextureFromFile
		;

	namespace PackedVector
	{
		using
			::DirectX::PackedVector::XMCOLOR,
			::DirectX::PackedVector::XMHALF4,
			::DirectX::PackedVector::XMCOLOR,
			::DirectX::PackedVector::XMStoreColor
			;
	}

	namespace Colors
	{
		using
			::DirectX::Colors::White,
			::DirectX::Colors::Black,
			::DirectX::Colors::Red,
			::DirectX::Colors::Green,
			::DirectX::Colors::Blue,
			::DirectX::Colors::Yellow,
			::DirectX::Colors::Cyan,
			::DirectX::Colors::Magenta,
			::DirectX::Colors::Silver,
			::DirectX::Colors::LightSteelBlue
			;
	}
}

export namespace D3D
{
	using
		::D3D_FEATURE_LEVEL,
		::D3D_DRIVER_TYPE,
		::ID3DBlob,
		::D3DReadFileToBlob
		;
}

export namespace DXGI
{
	enum Usage : unsigned int
	{
		RenderTargetOutput = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	};

	using
		::IDXGISwapChain,
		::IDXGISwapChain1,
		::IDXGIDevice,
		::IDXGIFactory,
		::IDXGIFactory2,
		::IDXGIAdapter,
		::DXGI_FORMAT,
		::DXGI_SAMPLE_DESC,
		::DXGI_MODE_SCANLINE_ORDER,
		::DXGI_SWAP_EFFECT,
		::DXGI_MODE_SCALING,
		::DXGI_SWAP_CHAIN_DESC
		;
}

export namespace D3D11
{
	constexpr auto SdkVersion = D3D11_SDK_VERSION;
	using 
		::D3D11_DEPTH_STENCIL_DESC,
		::D3D11_DEPTH_WRITE_MASK,
		::D3D11_COMPARISON_FUNC,
		::D3D11_STENCIL_OP,
		::ID3D11DepthStencilState,
		::ID3D11ShaderResourceView,
		::ID3D11Device,
		::D3D11_SRV_DIMENSION,
		::ID3D11DeviceContext,
		::D3D11_TEXTURE2D_DESC,
		::ID3D11Resource,
		::ID3D11RenderTargetView,
		::ID3D11DepthStencilView,
		::D3D11_SUBRESOURCE_DATA,
		::D3D11_TEXTURE1D_DESC,
		::D3D11_BLEND_DESC,
		::D3D11_BLEND_OP,
		::D3D11_BLEND,
		::D3D11_COLOR_WRITE_ENABLE,
		::D3D11_BLEND_DESC1,
		::ID3D11Texture1D,
		::D3D11_INPUT_CLASSIFICATION,
		::D3D11_FILL_MODE,
		::D3D11_CULL_MODE,
		::ID3D11Texture2D,
		::D3D11_VIEWPORT,
		::D3D11_MAP,
		::D3D11_RASTERIZER_DESC,
		::D3D11_MAPPED_SUBRESOURCE,
		::D3D11_CLEAR_FLAG,
		::ID3D11RenderTargetView,
		::ID3D11DepthStencilView,
		::ID3D11RasterizerState,
		::ID3D11BlendState,
		::ID3D11ShaderResourceView,
		::D3D11_BIND_FLAG,
		::D3D11_PRIMITIVE_TOPOLOGY,
		::D3D11_SUBRESOURCE_DATA,
		::D3D11_SHADER_RESOURCE_VIEW_DESC,
		::D3D11_CREATE_DEVICE_FLAG,
		::ID3D11Buffer,
		::ID3D11VertexShader,
		::ID3D11PixelShader,
		::D3D11_USAGE,
		::D3D11_CPU_ACCESS_FLAG,
		::D3D11_INPUT_ELEMENT_DESC,
		::D3D11_RESOURCE_MISC_FLAG,
		::D3D11_FILTER,
		::ID3D11InputLayout,
		::D3D11_COMPARISON_FUNC,
		::D3D11_INPUT_ELEMENT_DESC,
		::D3D11_TEXTURE_ADDRESS_MODE,
		::D3D11_BUFFER_DESC,
		::D3D11_SAMPLER_DESC,
		::ID3D11SamplerState,
		::D3D11CreateDevice
		;
}
