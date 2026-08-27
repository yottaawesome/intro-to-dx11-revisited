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
		: D3DApp(hInstance), mWalkCamMode(false)
	{
		mMainWndCaption = L"Particles Demo";
		mEnable4xMsaa = false;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, 100.0f);

		mDirLights[0].Ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, -0.57735f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		mSky.emplace(md3dDevice.get(), L"Textures/grasscube1024.dds", 5000.0f);

		Terrain::InitInfo tii;
		tii.HeightMapFilename = L"Textures/terrain.raw";
		tii.LayerMapFilename0 = L"Textures/grass.dds";
		tii.LayerMapFilename1 = L"Textures/darkdirt.dds";
		tii.LayerMapFilename2 = L"Textures/stone.dds";
		tii.LayerMapFilename3 = L"Textures/lightdirt.dds";
		tii.LayerMapFilename4 = L"Textures/snow.dds";
		tii.BlendMapFilename = L"Textures/blend.dds";
		tii.HeightScale = 50.0f;
		tii.HeightmapWidth = 2049;
		tii.HeightmapHeight = 2049;
		tii.CellSpacing = 0.5f;

		mTerrain.emplace(md3dDevice.get(), md3dImmediateContext.get(), tii);
		mRenderStates.emplace(md3dDevice.get());

		mRandomTexSRV = d3dHelper::CreateRandomTexture1DSRV(md3dDevice.get());

		std::vector<std::wstring> flares;
		flares.push_back(L"Textures\\flare0.dds");
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

		//md3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
		//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

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

	DirectionalLight mDirLights[3];

	Camera mCam;

	bool mWalkCamMode;

	Win32::POINT mLastMousePos = {};
};