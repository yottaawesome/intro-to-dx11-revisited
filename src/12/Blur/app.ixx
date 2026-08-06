export module blur:app;
import std;
import shared;
import :renderstates;
import :blurfilter;
import :waves;

// Basic 32-byte vertex structure.
struct Basic32
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

struct PerFrameConstants
{
	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;
	float gFogStart;
	float gFogRange;
	int gLightCount;
	int gUseTexture;
	int gAlphaClip;
	int gFogEnabled;
	DirectX::XMFLOAT3 gPadding;
	DirectX::XMFLOAT4 gFogColor;
};

static_assert(sizeof(PerFrameConstants) == 256);

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};

export class BlurApp : public D3DApp
{
public:
	BlurApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"Blur Demo";
		mEnable4xMsaa = false;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&mLandWorld, I);
		DirectX::XMStoreFloat4x4(&mWavesWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

		DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f);
		DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(8.0f, 5.0f, -15.0f);
		DirectX::XMStoreFloat4x4(&mBoxWorld, boxScale * boxOffset);

		DirectX::XMMATRIX grassTexScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

		mDirLights[0].Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f);

		mLandMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mLandMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mLandMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

		mWavesMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mWavesMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
		mWavesMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

		Init();
	}

	void Init()override
	{
		D3DApp::Init();

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/grass.dds", nullptr, &mGrassMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/water2.dds", nullptr, &mWavesMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/WireFence.dds", nullptr, &mCrateSRV));

		mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

		BuildLandGeometryBuffers();
		BuildWaveGeometryBuffers();
		BuildCrateGeometryBuffers();
		BuildScreenQuadGeometryBuffers();
		BuildOffscreenViews();
		BuildShaders();

		mRenderStates = { md3dDevice.get() };
	}

	void OnResize()override
	{
		D3DApp::OnResize();
		// Recreate the resources that depend on the client area size.
		BuildOffscreenViews();
		mBlur.Init(md3dDevice.get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt)override
	{
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		float z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		float y = mRadius * std::cosf(mPhi);

		mEyePosW = DirectX::XMFLOAT3(x, y, z);

		// Build the view matrix.
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);
		DirectX::XMStoreFloat4x4(&mView, V);

		//
		// Every quarter second, generate a random wave.
		//
		static auto t_base = 0.0f;
		if ((mTimer.TotalTime() - t_base) >= 0.1f)
		{
			t_base += 0.1f;
			auto i = static_cast<std::uint32_t>(5 + std::rand() % (mWaves.RowCount() - 10));
			auto j = static_cast<std::uint32_t>(5 + std::rand() % (mWaves.ColumnCount() - 10));
			auto r = MathHelper::RandF(0.5f, 1.0f);
			mWaves.Disturb(i, j, r);
		}

		mWaves.Update(dt);

		//
		// Update the wave vertex buffer with the new solution.
		//

		D3D11::D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(md3dImmediateContext->Map(mWavesVB.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		Basic32* v = reinterpret_cast<Basic32*>(mappedData.pData);
		for (auto i = 0u; i < mWaves.VertexCount(); ++i)
		{
			v[i].Pos = mWaves[i];
			v[i].Normal = mWaves.Normal(i);

			// Derive tex-coords in [0,1] from position.
			v[i].Tex.x = 0.5f + mWaves[i].x / mWaves.Width();
			v[i].Tex.y = 0.5f - mWaves[i].z / mWaves.Depth();
		}

		md3dImmediateContext->Unmap(mWavesVB.get(), 0);

		//
		// Animate water texture coordinates.
		//

		// Tile water texture.
		DirectX::XMMATRIX wavesScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);

		// Translate texture over time.
		mWaterTexOffset.y += 0.05f * dt;
		mWaterTexOffset.x += 0.1f * dt;
		DirectX::XMMATRIX wavesOffset = DirectX::XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

		// Combine scale and translation.
		DirectX::XMStoreFloat4x4(&mWaterTexTransform, wavesScale * wavesOffset);

		//
		// Switch the render mode based in key input.
		//
		if (Win32::GetAsyncKeyState('1') & 0x8000)
			mRenderOptions = RenderOptions::Lighting;
		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mRenderOptions = RenderOptions::Textures;
		if (Win32::GetAsyncKeyState('3') & 0x8000)
			mRenderOptions = RenderOptions::TexturesAndFog;
	}

	void DrawScene()override
	{
		// Render to our offscreen texture.  Note that we can use the same depth/stencil buffer
		// we normally use since our offscreen texture matches the dimensions.  

		D3D11::ID3D11RenderTargetView* renderTargets[1] = { mOffscreenRTV.get()};
		md3dImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView.get());
		md3dImmediateContext->VSSetShader(mColorVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), nullptr, 0);

		md3dImmediateContext->ClearRenderTargetView(mOffscreenRTV.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		//
		// Draw the scene to the offscreen texture
		//

		DrawWrapper();

		//
		// Restore the back buffer.  The offscreen render target will serve as an input into
		// the compute shader for blurring, so we must unbind it from the OM stage before we
		// can use it as an input into the compute shader.
		//
		renderTargets[0] = mRenderTargetView.get();
		md3dImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView.get());

		mBlur.BlurInPlace(
			md3dImmediateContext.get(),
			mOffscreenSRV.get(),
			mOffscreenUAV.get(),
			mHorzBlurCS.get(),
			mVertBlurCS.get(),
			4);

		//
		// Draw fullscreen quad with texture of blurred scene on it.
		//

		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		DrawScreenQuad();

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)override
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		Win32::SetCapture(mhMainWnd);
	}

	void OnMouseUp(Win32::WPARAM btnState, int x, int y)override
	{
		Win32::ReleaseCapture();
	}

	void OnMouseMove(Win32::WPARAM btnState, int x, int y)override
	{
		if ((btnState & Win32::MK::LButton) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			auto dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			auto dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = std::clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			auto dx = 0.1f * static_cast<float>(x - mLastMousePos.x);
			auto dy = 0.1f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = std::clamp(mRadius, 20.0f, 500.0f);
		}

		mLastMousePos = { x, y };
	}

private:
	void DrawWrapper()
	{
		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto blendFactor = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto viewProj = DirectX::XMMATRIX{view * proj};

		// Set per frame constants.
		auto perFrameConstants = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mNumLights,
			.gUseTexture = mRenderOptions == RenderOptions::Textures or mRenderOptions == RenderOptions::TexturesAndFog,
			.gAlphaClip = mRenderOptions == RenderOptions::Textures or mRenderOptions == RenderOptions::TexturesAndFog,
			.gFogEnabled = mRenderOptions == RenderOptions::TexturesAndFog,
			.gPadding = DirectX::XMFLOAT3{},
			.gFogColor = DirectX::XMFLOAT4{DirectX::Colors::Silver}
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perFrameConstants.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		//
		// Draw the box with alpha clipping.
		// 
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mBoxVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(mBoxIB.get(), DXGI_FORMAT_R32_UINT, 0);
			md3dImmediateContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

			// Set per object constants.
			auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mBoxWorld)};
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = DirectX::XMMATRIX{world * viewProj};
			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mBoxMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			auto boxSRVs = std::array{mCrateSRV.get()};
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(boxSRVs.size()), boxSRVs.data());
			auto boxConstantBuffersVS = std::array{ mPerObject.get() };
			md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(boxConstantBuffersVS.size()), boxConstantBuffersVS.data());
			auto boxConstantBuffersPS = std::array{ mPerFrame.get(), mPerObject.get() };
			md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(boxConstantBuffersPS.size()), boxConstantBuffersPS.data());

			md3dImmediateContext->RSSetState(mRenderStates.NoCullRS.get());
			md3dImmediateContext->DrawIndexed(36, 0, 0);
		}

		// Restore default render state.
		md3dImmediateContext->RSSetState(0);

		//
		// Draw the hills and water with texture and fog (no alpha clipping needed).
		//
		perFrameConstants.gAlphaClip = false;
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		//
		// Draw the hills.
		//
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mLandVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(mLandIB.get(), DXGI_FORMAT_R32_UINT, 0);

			// Set per object constants.
			auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mLandWorld)};
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = DirectX::XMMATRIX{world * view * proj};

			auto landPerObject = PerObjectConstants{
				.gMaterial = mLandMat,
			};
			DirectX::XMStoreFloat4x4(&landPerObject.gWorld, world);
			DirectX::XMStoreFloat4x4(&landPerObject.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&landPerObject.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&landPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mGrassTexTransform));
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &landPerObject, 0, 0);
			auto landSRVs = std::array{ mGrassMapSRV.get() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(landSRVs.size()), landSRVs.data());
			auto landConstantBuffersVS = std::array{ mPerObject.get() };
			auto landConstantBuffersPS = std::array{ mPerFrame.get(), mPerObject.get() };
			md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(landConstantBuffersVS.size()), landConstantBuffersVS.data());
			md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(landConstantBuffersPS.size()), landConstantBuffersPS.data());

			md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);
		}

		//
		//	Draw the waves.
		//
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mWavesVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(mWavesIB.get(), DXGI_FORMAT_R32_UINT, 0);

			// Set per object constants.
			auto world = DirectX::XMLoadFloat4x4(&mWavesWorld);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;
			auto wavesPerObject = PerObjectConstants{
				.gMaterial = mWavesMat,
			};
			DirectX::XMStoreFloat4x4(&wavesPerObject.gWorld, world);
			DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&wavesPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mWaterTexTransform));
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &wavesPerObject, 0, 0);

			auto wavesSRVs = std::array{ mWavesMapSRV.get() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(wavesSRVs.size()), wavesSRVs.data());
			auto wavesConstantBuffersVS = std::array{ mPerObject.get() };
			auto wavesConstantBuffersPS = std::array{ mPerFrame.get(), mPerObject.get() };
			md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(wavesConstantBuffersVS.size()), wavesConstantBuffersVS.data());
			md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(wavesConstantBuffersPS.size()), wavesConstantBuffersPS.data());

			md3dImmediateContext->OMSetBlendState(mRenderStates.TransparentBS.get(), blendFactor.data(), 0xffffffff);
			md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);
		}

		// Restore default blend state
		md3dImmediateContext->OMSetBlendState(0, blendFactor.data(), 0xffffffff);
	}

	void DrawScreenQuad()
	{
		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		auto identity = DirectX::XMMATRIX{DirectX::XMMatrixIdentity()};

		md3dImmediateContext->IASetVertexBuffers(0, 1, mScreenQuadVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mScreenQuadIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto perFrameConstants = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gLightCount = 0,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
		};
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		auto perObjectConstants = PerObjectConstants{
			.gWorld = d3dHelper::Identity4x4, 
			.gWorldInvTranspose = d3dHelper::Identity4x4,
			.gWorldViewProj = d3dHelper::Identity4x4,
			.gTexTransform = d3dHelper::Identity4x4,
		};
		md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
		auto srvs = std::array{ mOffscreenSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srvs.size()), srvs.data());

		auto vsConstantBuffers = std::array{ mPerObject.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());
		auto psConstantBuffers = std::array{ mPerFrame.get(), mPerObject.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers.size()), psConstantBuffers.data());

		md3dImmediateContext->DrawIndexed(6, 0, 0);

		auto nullSRVs = std::array<D3D11::ID3D11ShaderResourceView*, 1>{};
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(nullSRVs.size()), nullSRVs.data());
	}
	auto GetHillHeight(float x, float z)const -> float
	{
		return 0.3f * (z * std::sinf(0.1f * x) + x * std::cosf(0.1f * z));
	}
	auto GetHillNormal(float x, float z)const -> DirectX::XMFLOAT3
	{
		// n = (-df/dx, 1, -df/dz)
		auto n = DirectX::XMFLOAT3{
			-0.03f * z * std::cosf(0.1f * x) - 0.3f * std::cosf(0.1f * z),
			1.0f,
			-0.3f * std::sinf(0.1f * x) + 0.03f * x * std::sinf(0.1f * z)};
		auto unitNormal = DirectX::XMVECTOR{DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&n))};
		DirectX::XMStoreFloat3(&n, unitNormal);
		return n;
	}
	void BuildLandGeometryBuffers()
	{
		auto grid = GeometryGenerator::MeshData{};
		auto geoGen = GeometryGenerator{};

		geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

		mLandIndexCount = static_cast<std::uint32_t>(grid.Indices.size());

		//
		// Extract the vertex elements we are interested and apply the height function to
		// each vertex.  
		//
		auto vertices = std::vector<Basic32>(grid.Vertices.size());
		for (auto i = 0u; i < grid.Vertices.size(); ++i)
		{
			DirectX::XMFLOAT3 p = grid.Vertices[i].Position;
			p.y = GetHillHeight(p.x, p.z);
			vertices[i].Pos = p;
			vertices[i].Normal = GetHillNormal(p.x, p.z);
			vertices[i].Tex = grid.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32) * grid.Vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(UINT) * mLandIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &grid.Indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
	}

	void BuildWaveGeometryBuffers()
	{
		// Create the vertex buffer.  Note that we allocate space only, as
		// we will be updating the data every time step of the simulation.

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32) * mWaves.VertexCount()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11::D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
		};
		HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesVB));

		// Create the index buffer.  The index buffer is fixed, so we only 
		// need to create and set once.
		auto indices = std::vector<std::uint32_t>(3 * mWaves.TriangleCount()); // 3 indices per face

		// Iterate over each quad.
		auto m = mWaves.RowCount();
		auto n = mWaves.ColumnCount();
		auto k = 0;
		for (auto i = 0u; i < m - 1; ++i)
		{
			for (auto j = 0u; j < n - 1; ++j)
			{
				indices[k] = i * n + j;
				indices[k + 1] = i * n + j + 1;
				indices[k + 2] = (i + 1) * n + j;
				indices[k + 3] = (i + 1) * n + j;
				indices[k + 4] = i * n + j + 1;
				indices[k + 5] = (i + 1) * n + j + 1;
				k += 6; // next quad
			}
		}

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
	}

	void BuildCrateGeometryBuffers()
	{
		auto box = GeometryGenerator::MeshData{};
		auto geoGen = GeometryGenerator{};
		geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//
		auto vertices = std::vector<Basic32>(box.Vertices.size());
		for (auto i = 0u; i < box.Vertices.size(); ++i)
		{
			vertices[i].Pos = box.Vertices[i].Position;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].Tex = box.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32) * box.Vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &vertices[0]};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * box.Indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &box.Indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}

	void BuildScreenQuadGeometryBuffers()
	{
		auto quad = GeometryGenerator::MeshData{};
		auto geoGen = GeometryGenerator{};
		geoGen.CreateFullscreenQuad(quad);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//
		auto vertices = std::vector<Basic32>(quad.Vertices.size());
		for (auto i = 0u; i < quad.Vertices.size(); ++i)
		{
			vertices[i].Pos = quad.Vertices[i].Position;
			vertices[i].Normal = quad.Vertices[i].Normal;
			vertices[i].Tex = quad.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32) * quad.Vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * quad.Indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &quad.Indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
	}

	void BuildOffscreenViews()
	{
		// We call this function everytime the window is resized so that the render target is a quarter
		// the client area dimensions.  So Release the previous views before we create new ones.
		mOffscreenSRV.reset();
		mOffscreenRTV.reset();
		mOffscreenUAV.reset();

		auto texDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = static_cast<std::uint32_t>(mClientWidth),
			.Height = static_cast<std::uint32_t>(mClientHeight),
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc{ .Count = 1, .Quality = 0 },
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG{D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS},
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};


		auto offscreenTex = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &offscreenTex));

		// Null description means to create a view to all mipmap levels using 
		// the format the texture was created with.
		HR(md3dDevice->CreateShaderResourceView(offscreenTex.get(), 0, &mOffscreenSRV));
		HR(md3dDevice->CreateRenderTargetView(offscreenTex.get(), 0, &mOffscreenRTV));
		HR(md3dDevice->CreateUnorderedAccessView(offscreenTex.get(), 0, &mOffscreenUAV));

		// View saves a reference to the texture so we can release our reference.
		offscreenTex.reset();
	}

	void BuildShaders()
	{
		// basic32 shaders
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/Basic_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/Basic_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mColorPS), "Failed to create pixel shader.");
		}

		//
		// Compute Shader
		{
			auto horzBlurShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			auto vertBlurShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/Blur_HorzBlurCS.cso", &horzBlurShaderBytecode), "Failed to read compute shader file.");
			HR(D3D::D3DReadFileToBlob(L"Shaders/Blur_VertBlurCS.cso", &vertBlurShaderBytecode), "Failed to read compute shader file.");
			HR(md3dDevice->CreateComputeShader(
				horzBlurShaderBytecode->GetBufferPointer(),
				horzBlurShaderBytecode->GetBufferSize(),
				nullptr,
				&mHorzBlurCS),
				"Failed to create horizontal blur compute shader.");
			HR(md3dDevice->CreateComputeShader(
				vertBlurShaderBytecode->GetBufferPointer(),
				vertBlurShaderBytecode->GetBufferSize(),
				nullptr,
				&mVertBlurCS),
				"Failed to create vertical blur compute shader.");
		}

		// Build layouts
		BuildInputLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		// constant buffers basic32
		auto perFrameCbd32 = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameCbd32, 0, &mPerFrame), "Failed to create constant buffer.");

		auto perObjectCbd32 = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd32, 0, &mPerObject), "Failed to create constant buffer.");

		// samplers
		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_ANISOTROPIC,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 4,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(md3dDevice->CreateSamplerState(&samplerDesc, &mSamplerState), "Failed to create sampler state.");
	}

	void BuildInputLayout(D3D::ID3DBlob* vertexShaderBytecode)
	{
		// Create the vertex input layout.
		auto basic32Desc = std::array{
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 24,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};
		HR(md3dDevice->CreateInputLayout(
			basic32Desc.data(),
			static_cast<std::uint32_t>(basic32Desc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mBasic32),
			"Failed to create basic32 input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mLandVB;
	ComPtr<D3D11::ID3D11Buffer> mLandIB;
	ComPtr<D3D11::ID3D11Buffer> mWavesVB;
	ComPtr<D3D11::ID3D11Buffer> mWavesIB;
	ComPtr<D3D11::ID3D11Buffer> mBoxVB;
	ComPtr<D3D11::ID3D11Buffer> mBoxIB;
	ComPtr<D3D11::ID3D11Buffer> mScreenQuadVB;
	ComPtr<D3D11::ID3D11Buffer> mScreenQuadIB;
	ComPtr<D3D11::ID3D11InputLayout> mBasic32;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11ComputeShader> mHorzBlurCS;
	ComPtr<D3D11::ID3D11ComputeShader> mVertBlurCS;

	ComPtr<D3D11::ID3D11ShaderResourceView> mGrassMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWavesMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mCrateSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mOffscreenSRV;
	ComPtr<D3D11::ID3D11UnorderedAccessView> mOffscreenUAV;
	ComPtr<D3D11::ID3D11RenderTargetView> mOffscreenRTV;

	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	BlurFilter mBlur;
	RenderStates mRenderStates;
	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mWaterTexTransform = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mLandWorld = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mWavesWorld = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mBoxWorld = d3dHelper::Identity4x4;

	DirectX::XMFLOAT4X4 mView = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mProj = d3dHelper::Identity4x4;

	std::uint32_t mLandIndexCount = 0;
	std::uint32_t mWaveIndexCount = 0;

	DirectX::XMFLOAT2 mWaterTexOffset = {0.0f, 0.0f};

	RenderOptions mRenderOptions = RenderOptions::TexturesAndFog;

	DirectX::XMFLOAT3 mEyePosW = {0.0f, 0.0f, 0.0f};

	float mTheta = 1.3f * MathHelper::Pi;
	float mPhi = 0.4f * MathHelper::Pi;
	float mRadius = 80.0f;
	int mNumLights = 3;

	Win32::POINT mLastMousePos{};
};