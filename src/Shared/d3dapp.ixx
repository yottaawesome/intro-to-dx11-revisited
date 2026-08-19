export module shared:d3dapp;
import std;
import :win32;
import :gametimer;
import :d3dutil;
import :comptr;
import :build;

export class D3DApp
{
public:
	D3DApp(Win32::HINSTANCE hInstance)
		: mhAppInst(hInstance)
	{
		// Get a pointer to the application object so we can forward 
		// Windows messages to the object's window procedure through
		// the global window procedure.
		gd3dApp = this;
	}

	D3DApp(Win32::HINSTANCE hInstance, std::wstring mainWndCaption)
		: mhAppInst(hInstance), mMainWndCaption(std::move(mainWndCaption))
	{
		// Get a pointer to the application object so we can forward 
		// Windows messages to the object's window procedure through
		// the global window procedure.
		gd3dApp = this;
	}

	virtual ~D3DApp()
	{
		mRenderTargetView.reset();
		mDepthStencilView.reset();
		mSwapChain.reset();
		mDepthStencilBuffer.reset();
		// Restore all default settings.
		if (md3dImmediateContext)
		{
			md3dImmediateContext->ClearState();
			md3dImmediateContext.reset();
		}
		md3dDevice.reset();
	}

	auto AppInst()const -> Win32::HINSTANCE
	{
		return mhAppInst;
	}

	auto MainWnd()const -> Win32::HWND
	{
		return mhMainWnd;
	}

	auto AspectRatio()const -> float
	{
		return static_cast<float>(mClientWidth) / mClientHeight;
	}

	auto Run() -> int
	{
		auto msg = Win32::MSG{ };

		mTimer.Reset();

		while (msg.message != Win32::WindowMessages::Quit)
		{
			// If there are Window messages then process them.
			if (Win32::PeekMessageW(&msg, 0, 0, 0, Win32::PM::Remove))
			{
				Win32::TranslateMessage(&msg);
				Win32::DispatchMessageW(&msg);
			}
			// Otherwise, do animation/game stuff.
			else
			{
				mTimer.Tick();

				if (not mAppPaused)
				{
					CalculateFrameStats();
					UpdateScene(mTimer.DeltaTime());
					DrawScene();
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
				}
			}
		}

		return static_cast<int>(msg.wParam);
	}

	// Framework methods.  Derived client class overrides these methods to 
	// implement specific application requirements.

	virtual void Init()
	{
		InitMainWindow();
		InitDirect3D();
	}

	virtual void OnResize()
	{
		//assert(md3dImmediateContext);
		//assert(md3dDevice);
		//assert(mSwapChain);

		// Release the old views, as they hold references to the buffers we
		// will be destroying.  Also release the old depth/stencil buffer.
		mRenderTargetView.reset();
		mDepthStencilView.reset();
		mDepthStencilBuffer.reset();

		// Resize the swap chain and recreate the render target view.
		HR(mSwapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
		auto backBuffer = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(mSwapChain->GetBuffer(0, backBuffer.Uuid(), backBuffer.ReleaseAndGetAddressOfVoid()));
		HR(md3dDevice->CreateRenderTargetView(backBuffer.get(), 0, &mRenderTargetView));
		backBuffer.reset();

		// Create the depth/stencil buffer and view.
		auto depthStencilDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = static_cast<std::uint32_t>(mClientWidth),
			.Height = static_cast<std::uint32_t>(mClientHeight),
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
			// Use 4X MSAA? --must match swap chain MSAA values.
			.SampleDesc = {
				.Count = static_cast<std::uint32_t>(mEnable4xMsaa ? 4 : 1),
				.Quality = static_cast<std::uint32_t>(mEnable4xMsaa ? m4xMsaaQuality - 1 : 0)
			},
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		HR(md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer));
		HR(md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.get(), 0, &mDepthStencilView));

		// Bind the render target view and depth/stencil view to the pipeline.
		md3dImmediateContext->OMSetRenderTargets(1, mRenderTargetView.GetAddressOf(), mDepthStencilView.get());

		// Set the viewport transform.
		mScreenViewport = {
			.TopLeftX = 0,
			.TopLeftY = 0,
			.Width = static_cast<float>(mClientWidth),
			.Height = static_cast<float>(mClientHeight),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f,
		};

		md3dImmediateContext->RSSetViewports(1, &mScreenViewport);
	}
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(Win32::WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(Win32::WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(Win32::WPARAM btnState, int x, int y) {}

	auto MsgProc(Win32::HWND hwnd, Win32::UINT msg, Win32::WPARAM wParam, Win32::LPARAM lParam) -> Win32::LRESULT
	{
		switch (msg)
		{
			// WM_ACTIVATE is sent when the window is activated or deactivated.  
			// We pause the game when the window is deactivated and unpause it 
			// when it becomes active.  
			case Win32::WindowMessages::Activate:
			{
				if (Win32::LoWord(wParam) == Win32::WA::Inactive)
				{
					mAppPaused = true;
					mTimer.Stop();
				}
				else
				{
					mAppPaused = false;
					mTimer.Start();
				}
				return 0;
			}

			// WM_SIZE is sent when the user resizes the window.  
			case Win32::WindowMessages::Size:
			{
				// Save the new client area dimensions.
				mClientWidth = Win32::LoWord(lParam);
				mClientHeight = Win32::HiWord(lParam);
				if (not md3dDevice)
					return 0;

				if (wParam == Win32::Size::Minimized)
				{
					mAppPaused = true;
					mMinimized = true;
					mMaximized = false;
				}
				else if (wParam == Win32::Size::Maximized)
				{
					mAppPaused = false;
					mMinimized = false;
					mMaximized = true;
					OnResize();
				}
				else if (wParam == Win32::Size::Restored)
				{
					// Restoring from minimized state?
					if (mMinimized)
					{
						mAppPaused = false;
						mMinimized = false;
						OnResize();
					}
					// Restoring from maximized state?
					else if (mMaximized)
					{
						mAppPaused = false;
						mMaximized = false;
						OnResize();
					}
					else if (mResizing)
					{
						// If user is dragging the resize bars, we do not resize 
						// the buffers here because as the user continuously 
						// drags the resize bars, a stream of WM_SIZE messages are
						// sent to the window, and it would be pointless (and slow)
						// to resize for each WM_SIZE message received from dragging
						// the resize bars.  So instead, we reset after the user is 
						// done resizing the window and releases the resize bars, which 
						// sends a WM_EXITSIZEMOVE message.
					}
					else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
					{
						OnResize();
					}
				}
				return 0;
			}
			
			// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
			case Win32::WindowMessages::EnterSizeMove:
			{
				mAppPaused = true;
				mResizing = true;
				mTimer.Stop();
				return 0;
			}

			// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
			// Here we reset everything based on the new window dimensions.
			case Win32::WindowMessages::ExitSizeMove:
			{
				mAppPaused = false;
				mResizing = false;
				mTimer.Start();
				OnResize();
				return 0;
			}

			// WM_DESTROY is sent when the window is being destroyed.
			case Win32::WindowMessages::Destroy:
			{
				Win32::PostQuitMessage(0);
				return 0;
			}

			// The WM_MENUCHAR message is sent when a menu is active and the user presses 
			// a key that does not correspond to any mnemonic or accelerator key. 
			case Win32::WindowMessages::MenuChar:
			{
				// Don't beep when we alt-enter.
				return Win32::MakeLResult(0, Win32::MNC::Close);
			}

			// Catch this message so to prevent the window from becoming too small.
			case Win32::WindowMessages::GetMinMaxInfo:
			{
				((Win32::MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
				((Win32::MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
				return 0;
			}

			case Win32::WindowMessages::LButtonDown:
			case Win32::WindowMessages::MButtonDown:
			case Win32::WindowMessages::RButtonDown:
			{
				OnMouseDown(wParam, Win32::GetXLParam(lParam), Win32::GetYLParam(lParam));
				return 0;
			}
			case Win32::WindowMessages::LButtonUp:
			case Win32::WindowMessages::MButtonUp:
			case Win32::WindowMessages::RButtonUp:
			{
				OnMouseUp(wParam, Win32::GetXLParam(lParam), Win32::GetYLParam(lParam));
				return 0;
			}
			case Win32::WindowMessages::MouseMove:
			{
				OnMouseMove(wParam, Win32::GetXLParam(lParam), Win32::GetYLParam(lParam));
				return 0;
			}
		}

		return Win32::DefWindowProcW(hwnd, msg, wParam, lParam);
	}

protected:
	// This is just used to forward Windows messages from a global window
	// procedure to our member function window procedure because we cannot
	// assign a member function to WNDCLASS::lpfnWndProc.
	static inline auto gd3dApp = static_cast<D3DApp*>(nullptr);

	static auto MainWndProc(Win32::HWND hwnd, Win32::UINT msg, Win32::WPARAM wParam, Win32::LPARAM lParam) -> Win32::LRESULT
	{
		// Forward hwnd on because we can get messages (e.g., WM_CREATE)
		// before CreateWindow returns, and thus before mhMainWnd is valid.
		return gd3dApp->MsgProc(hwnd, msg, wParam, lParam);
	}
	
	void InitMainWindow()
	{
		auto wc = Win32::WNDCLASS{
			.style = Win32::CS::HRedraw | Win32::CS::VRedraw,
			.lpfnWndProc = MainWndProc,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = mhAppInst,
			.hIcon = Win32::LoadIconW(0, Win32::IdiApplication()),
			.hCursor = Win32::LoadCursorW(0, Win32::IdcArrow()),
			.hbrBackground = (Win32::HBRUSH)Win32::GetStockObject(Win32::NullBrush),
			.lpszMenuName = 0,
			.lpszClassName = L"D3DWndClassName"
		};
		
		if (!Win32::RegisterClassW(&wc))
			throw std::runtime_error{ "RegisterClass Failed." };

		// Compute window rectangle dimensions based on requested client area dimensions.
		auto R = Win32::RECT{ 0, 0, mClientWidth, mClientHeight };
		Win32::AdjustWindowRect(&R, Win32::WindowStyles::OverlappedWindow, false);
		auto width = R.right - R.left;
		auto height = R.bottom - R.top;

		mhMainWnd = Win32::CreateWindowExW(
			0, 
			L"D3DWndClassName", 
			mMainWndCaption.c_str(),
			Win32::WindowStyles::OverlappedWindow, 
			Win32::CW::UseDefault,
			Win32::CW::UseDefault, 
			width, 
			height, 
			0, 
			0, 
			mhAppInst, 
			0);
		if (not mhMainWnd)
			throw std::runtime_error{ "CreateWindow Failed." };

		Win32::ShowWindow(mhMainWnd, Win32::SW::Show);
		Win32::UpdateWindow(mhMainWnd);
	}

	void InitDirect3D()
	{
		// Create the device and device context.
		auto createDeviceFlags = 0u;
		if constexpr (IsDebugBuild)
			createDeviceFlags |= D3D11::D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;

		auto featureLevel = D3D::D3D_FEATURE_LEVEL{};
		auto hr = D3D11::D3D11CreateDevice(
			nullptr,            // default adapter
			md3dDriverType,
			nullptr,            // no software device
			createDeviceFlags,
			nullptr,			// default feature level array
			0,              
			D3D11::SdkVersion,
			&md3dDevice,
			&featureLevel,
			&md3dImmediateContext);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "D3D11CreateDevice Failed." };
		if (featureLevel != D3D_FEATURE_LEVEL_11_0)
			throw std::runtime_error{ "Direct3D Feature Level 11 unsupported." };

		// Check 4X MSAA quality support for our back buffer format.
		// All Direct3D 11 capable devices support 4X MSAA for all render 
		// target formats, so we only need to check quality support.

		HR(md3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality));
		//assert(m4xMsaaQuality > 0);

		// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.

		auto sd = DXGI::DXGI_SWAP_CHAIN_DESC{
			.BufferDesc = {
				.Width = static_cast<std::uint32_t>(mClientWidth),
				.Height = static_cast<std::uint32_t>(mClientHeight),
				.RefreshRate = {
					.Numerator = 60,
					.Denominator = 1
				},
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ScanlineOrdering = DXGI::DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				.Scaling = DXGI::DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED
			},
			// Use 4X MSAA? 
			.SampleDesc = {
				.Count = static_cast<std::uint32_t>(mEnable4xMsaa ? 4 : 1),
				.Quality = static_cast<std::uint32_t>(mEnable4xMsaa ? m4xMsaaQuality - 1 : 0)
			},
			.BufferUsage = DXGI::Usage::RenderTargetOutput,
			.BufferCount = 1,
			.OutputWindow = mhMainWnd,
			.Windowed = true,
			// TODO: Use DXGI_SWAP_EFFECT_FLIP_DISCARD, but this requires using IDXGIFactory2 and CreateSwapChainForHwnd().
			.SwapEffect = DXGI::DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD,
			.Flags = 0
		};

		// To correctly create the swap chain, we must use the IDXGIFactory that was
		// used to create the device.  If we tried to use a different IDXGIFactory instance
		// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
		// This function is being called with a device from a different IDXGIFactory."

		auto dxgiDevice = ComPtr<DXGI::IDXGIDevice>{};
		HR(md3dDevice->QueryInterface(dxgiDevice.Uuid(), dxgiDevice.VoidAddress()));

		auto dxgiAdapter = ComPtr<DXGI::IDXGIAdapter>{};
		HR(dxgiDevice->GetParent(dxgiAdapter.Uuid(), dxgiAdapter.VoidAddress()));

		auto dxgiFactory = ComPtr<DXGI::IDXGIFactory>{};
		HR(dxgiAdapter->GetParent(dxgiFactory.Uuid(), dxgiFactory.VoidAddress()));

		HR(dxgiFactory->CreateSwapChain(md3dDevice.get(), &sd, mSwapChain.GetAddressOf()));

		// The remaining steps that need to be carried out for d3d creation
		// also need to be executed every time the window is resized.  So
		// just call the OnResize method here to avoid code duplication.

		OnResize();
	}

	void CalculateFrameStats()
	{
		// Code computes the average frames per second, and also the 
		// average time it takes to render one frame.  These stats 
		// are appended to the window caption bar.

		static auto frameCnt = 0;
		static auto timeElapsed = 0.0f;

		frameCnt++;

		// Compute averages over one second period.
		if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
		{
			auto fps = static_cast<float>(frameCnt); // fps = frameCnt / 1
			auto mspf = 1000.0f / fps;
			Win32::SetWindowTextW(mhMainWnd, std::format(L"{}    FPS: {}    Frame Time: {} (ms)", mMainWndCaption, fps, mspf).c_str());
			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}
	}

protected:
	Win32::HINSTANCE mhAppInst;
	Win32::HWND mhMainWnd=nullptr;
	bool mAppPaused = false;
	bool mMinimized = false;
	bool mMaximized = false;
	bool mResizing = false;
	std::uint32_t m4xMsaaQuality = 0;

	GameTimer mTimer;

	ComPtr<D3D11::ID3D11Device> md3dDevice;
	ComPtr<D3D11::ID3D11DeviceContext> md3dImmediateContext;
	ComPtr<DXGI::IDXGISwapChain> mSwapChain;
	ComPtr<D3D11::ID3D11Texture2D> mDepthStencilBuffer;
	ComPtr<D3D11::ID3D11RenderTargetView> mRenderTargetView;
	ComPtr<D3D11::ID3D11DepthStencilView> mDepthStencilView;
	D3D11::D3D11_VIEWPORT mScreenViewport{};

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption = L"D3D11 Application";
	D3D::D3D_DRIVER_TYPE md3dDriverType = D3D::D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE;
	int mClientWidth=800;
	int mClientHeight=600;
	bool mEnable4xMsaa=false;
};
