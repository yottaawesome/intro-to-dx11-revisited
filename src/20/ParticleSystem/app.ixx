export module particlesystemdemo:app;
import std;
import shared;
import :sky;
import :terrain;
import :renderstates;
import :particlesystem;

export class ParticlesApp : public D3DApp
{
public:
	ParticlesApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance, L"Particles Demo")
	{
		mEnable4xMsaa = false;
		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		mSky.emplace(md3dDevice.get(), L"Textures/grasscube1024.dds", 5000.0f);

		auto tii = Terrain::InitInfo{
			.HeightMapFilename = L"Textures/terrain.raw",
			.LayerMapFilename0 = L"Textures/grass.dds",
			.LayerMapFilename1 = L"Textures/darkdirt.dds",
			.LayerMapFilename2 = L"Textures/stone.dds",
			.LayerMapFilename3 = L"Textures/lightdirt.dds",
			.LayerMapFilename4 = L"Textures/snow.dds",
			.BlendMapFilename = L"Textures/blend.dds",
			.HeightScale = 50.0f,
			.HeightmapWidth = 2049,
			.HeightmapHeight = 2049,
			.CellSpacing = 0.5f,
		};

		mTerrain.emplace(md3dDevice.get(), md3dImmediateContext.get(), tii);
		mRenderStates.emplace(md3dDevice.get());

		mRandomTexSRV = d3dHelper::CreateRandomTexture1DSRV(md3dDevice.get());

		auto flares = std::vector<std::wstring>{ L"Textures\\flare0.dds" };
		mFlareTexSRV = d3dHelper::CreateTexture2DArraySRV(md3dDevice.get(), md3dImmediateContext.get(), flares);

		mFire.emplace(
			md3dDevice.get(),
			mFlareTexSRV,
			mRandomTexSRV,
			500,
			L"Shaders/FireStreamOutVS.cso",
			L"Shaders/FireStreamOutGS.cso",
			L"Shaders/FireDrawVS.cso",
			L"Shaders/FireDrawGS.cso",
			L"Shaders/FireDrawPS.cso",
			ParticleSystem::BlendMode::Additive);
		mFire->SetEmitPos(DirectX::XMFLOAT3(0.0f, 1.0f, 120.0f));

		std::vector<std::wstring> raindrops;
		raindrops.push_back(L"Textures\\raindrop.dds");
		mRainTexSRV = d3dHelper::CreateTexture2DArraySRV(md3dDevice.get(), md3dImmediateContext.get(), raindrops);

		mRain.emplace(
			md3dDevice.get(),
			mRainTexSRV,
			mRandomTexSRV,
			10000,
			L"Shaders/RainStreamOutVS.cso",
			L"Shaders/RainStreamOutGS.cso",
			L"Shaders/RainDrawVS.cso",
			L"Shaders/RainDrawGS.cso",
			L"Shaders/RainDrawPS.cso",
			ParticleSystem::BlendMode::Opaque);
	}

	void OnResize() override
	{
		D3DApp::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 3000.0f);
	}

	void UpdateScene(float dt) override
	{
		//
		// Control the camera.
		//
		if (Win32::GetAsyncKeyState('W') & 0x8000)
			mCam.Walk(10.0f * dt);

		if (Win32::GetAsyncKeyState('S') & 0x8000)
			mCam.Walk(-10.0f * dt);

		if (Win32::GetAsyncKeyState('A') & 0x8000)
			mCam.Strafe(-10.0f * dt);

		if (Win32::GetAsyncKeyState('D') & 0x8000)
			mCam.Strafe(10.0f * dt);

		//
		// Walk/fly mode
		//
		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mWalkCamMode = true;
		if (Win32::GetAsyncKeyState('3') & 0x8000)
			mWalkCamMode = false;

		// 
		// Clamp camera to terrain surface in walk mode.
		//
		if (mWalkCamMode)
		{
			DirectX::XMFLOAT3 camPos = mCam.GetPosition();
			float y = mTerrain->GetHeight(camPos.x, camPos.z);
			mCam.SetPosition(camPos.x, y + 2.0f, camPos.z);
		}

		//
		// Reset particle systems.
		//
		if (Win32::GetAsyncKeyState('R') & 0x8000)
		{
			mFire->Reset();
			mRain->Reset();
		}

		mFire->Update(dt, mTimer.TotalTime());
		mRain->Update(dt, mTimer.TotalTime());

		mCam.UpdateViewMatrix();
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(mRenderStates->WireframeRS.get());

		mTerrain->Draw(md3dImmediateContext.get(), mCam, mDirLights);

		md3dImmediateContext->RSSetState(nullptr);

		mSky->Draw(md3dImmediateContext.get(), mCam);

		// Draw particle systems last so it is blended with scene.
		mFire->SetEyePos(mCam.GetPosition());
		mFire->Draw(md3dImmediateContext.get(), mCam);

		mRain->SetEyePos(mCam.GetPosition());
		mRain->SetEmitPos(mCam.GetPosition());
		mRain->Draw(md3dImmediateContext.get(), mCam);

		md3dImmediateContext->RSSetState(nullptr);

		HR(mSwapChain->Present(0, 0));
	}


	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override
	{
		mLastMousePos = { x, y };
		Win32::SetCapture(mhMainWnd);
	}

	void OnMouseUp(Win32::WPARAM btnState, int x, int y) override
	{
		Win32::ReleaseCapture();
	}

	void OnMouseMove(Win32::WPARAM btnState, int x, int y) override
	{
		if ((btnState & Win32::MK::LButton) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			auto dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			auto dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.Pitch(dy);
			mCam.RotateY(dx);
		}
		mLastMousePos = { x, y };
	}

private:
	std::optional<Sky> mSky;
	std::optional<Terrain> mTerrain;
	std::optional<RenderStates> mRenderStates;

	ComPtr<D3D11::ID3D11ShaderResourceView> mFlareTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mRainTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mRandomTexSRV;

	std::optional<ParticleSystem> mFire;
	std::optional<ParticleSystem> mRain;

	DirectionalLight mDirLights[3]{
		{
			.Ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f),
			.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f),
			.Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f)
		},
		{
			.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
			.Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f)
		},
		{
			.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
			.Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, -0.57735f)
		}
	};

	Camera mCam = Camera::Position{ 0.0f, 2.0f, 100.0f };

	bool mWalkCamMode = false;

	Win32::POINT mLastMousePos = {};
};