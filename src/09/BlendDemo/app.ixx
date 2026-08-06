export module blenddemo:app;
import std;
import shared;
import :waves;
import :renderstates;

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

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};

// Basic 32-byte vertex structure.
struct Basic32
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

export class BlendApp : public D3DApp
{
public:
	BlendApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance }
	{
		mMainWndCaption = L"Blend Demo";
		mEnable4xMsaa = false;

		auto I = DirectX::XMMATRIX{DirectX::XMMatrixIdentity()};
		DirectX::XMStoreFloat4x4(&mLandWorld, I);
		DirectX::XMStoreFloat4x4(&mWavesWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

		auto boxScale = DirectX::XMMATRIX{DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f)};
		auto boxOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(8.0f, 5.0f, -15.0f)};
		DirectX::XMStoreFloat4x4(&mBoxWorld, boxScale * boxOffset);

		auto grassTexScale = DirectX::XMMATRIX{DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f)};
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

	void Init()
	{
		D3DApp::Init();

		mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

		auto texResource = ComPtr<D3D11::ID3D11Resource>{};
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/grass.dds", &texResource, &mGrassMapSRV));
		// view saves reference

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/water2.dds", &texResource, &mWavesMapSRV));
		// view saves reference

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/WireFence.dds", &texResource, &mBoxMapSRV));
		// view saves reference

		mRenderStates = {md3dDevice.get()};
		BuildLandGeometryBuffers();
		BuildWaveGeometryBuffers();
		BuildCrateGeometryBuffers();
		BuildShaders();
	}

	void OnResize()
	{
		D3DApp::OnResize();
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt)
	{
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		float z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		float y = mRadius * std::cosf(mPhi);

		mEyePosW = DirectX::XMFLOAT3(x, y, z);

		// Build the view matrix.
		auto pos = DirectX::XMVectorSet(x, y, z, 1.0f);
		auto target = DirectX::XMVectorZero();
		auto up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		auto V = DirectX::XMMatrixLookAtLH(pos, target, up);
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

		auto mappedData = D3D11::D3D11_MAPPED_SUBRESOURCE{};
		HR(md3dImmediateContext->Map(mWavesVB.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		auto v = reinterpret_cast<Basic32*>(mappedData.pData);
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
		auto wavesScale = DirectX::XMMATRIX{DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f)};

		// Translate texture over time.
		mWaterTexOffset.y += 0.05f * dt;
		mWaterTexOffset.x += 0.1f * dt;
		auto wavesOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f)};

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

	void DrawScene()
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		md3dImmediateContext->VSSetShader(mColorVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), nullptr, 0);
		auto samplers = std::array{ mSamplerState.get() };
		md3dImmediateContext->PSSetSamplers(0, static_cast<std::uint32_t>(samplers.size()), samplers.data());

		auto blendFactor = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto viewProj = view * proj;

		// Set per frame constants.
		DirectX::Colors::Silver;
		auto perframe = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = (mRenderOptions == RenderOptions::Lighting) ? 0 : 1,
			.gAlphaClip = 1,
			.gFogEnabled = (mRenderOptions == RenderOptions::TexturesAndFog) ? 1 : 0,
		};
		DirectX::XMStoreFloat4(&perframe.gFogColor, DirectX::Colors::Silver);
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perframe.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, nullptr, &perframe, 0, 0);

		//
		// Draw the box with alpha clipping.
		//
		auto boxVertexBuffers = std::array{ mBoxVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(boxVertexBuffers.size()), boxVertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mBoxIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto boxWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mBoxWorld)};
		auto boxWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(boxWorld)};
		auto boxWorldViewProj = DirectX::XMMATRIX{boxWorld * viewProj};
		auto boxPerObject = PerObjectConstants{
			.gMaterial = mBoxMat,
		};
		DirectX::XMStoreFloat4x4(&boxPerObject.gWorld, boxWorld);
		DirectX::XMStoreFloat4x4(&boxPerObject.gWorldInvTranspose, boxWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&boxPerObject.gWorldViewProj, boxWorldViewProj);
		DirectX::XMStoreFloat4x4(&boxPerObject.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &boxPerObject, 0, 0);
		auto boxShaderResourceViews = std::array{ mBoxMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(boxShaderResourceViews.size()), boxShaderResourceViews.data());
		md3dImmediateContext->RSSetState(mRenderStates.NoCullRS.get());
		auto boxConstantBuffersVS = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(boxConstantBuffersVS.size()), boxConstantBuffersVS.data());
		auto boxConstantBuffersPS = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(boxConstantBuffersPS.size()), boxConstantBuffersPS.data());
		md3dImmediateContext->OMSetBlendState(mRenderStates.AlphaToCoverageBS.get(), nullptr, 0xffffffff);
		md3dImmediateContext->DrawIndexed(36, 0, 0);

		// Restore default render state.
		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

		//
		// Draw the hills and water with texture and fog (no alpha clipping needed).
		//
		// Draw the hills.
		//
		md3dImmediateContext->IASetVertexBuffers(0, 1, mLandVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mLandIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto landWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mLandWorld)};
		auto landWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(landWorld)};
		auto landWorldViewProj = DirectX::XMMATRIX{landWorld * viewProj};
		auto landPerObject = PerObjectConstants{
			.gMaterial = mLandMat,
		};
		DirectX::XMStoreFloat4x4(&landPerObject.gWorld, landWorld);
		DirectX::XMStoreFloat4x4(&landPerObject.gWorldInvTranspose, landWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&landPerObject.gWorldViewProj, landWorldViewProj);
		DirectX::XMStoreFloat4x4(&landPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mGrassTexTransform));
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &landPerObject, 0, 0);
		auto landShaderResourceViews = std::array{ mGrassMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(landShaderResourceViews.size()), landShaderResourceViews.data());
		md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);

		//
		// Draw the waves.
		//
		auto wavesVertexBuffers = std::array{ mWavesVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(wavesVertexBuffers.size()), wavesVertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mWavesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto wavesWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mWavesWorld)};
		auto wavesWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(wavesWorld)};
		auto wavesWorldViewProj = DirectX::XMMATRIX{wavesWorld * view * proj};

		auto wavesPerObject = PerObjectConstants{
			.gMaterial = mWavesMat,
		};
		DirectX::XMStoreFloat4x4(&wavesPerObject.gWorld, wavesWorld);
		DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldInvTranspose, wavesWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldViewProj, wavesWorldViewProj);
		DirectX::XMStoreFloat4x4(&wavesPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mWaterTexTransform));
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &wavesPerObject, 0, 0);
		md3dImmediateContext->PSSetShaderResources(0, 1, mWavesMapSRV.GetAddressOf());

		md3dImmediateContext->OMSetBlendState(mRenderStates.TransparentBS.get(), blendFactor.data(), 0xffffffff);
		md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

		// Restore default blend state
		md3dImmediateContext->OMSetBlendState(0, blendFactor.data(), 0xffffffff);

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		Win32::SetCapture(mhMainWnd);
	}

	void OnMouseUp(Win32::WPARAM btnState, int x, int y)
	{
		Win32::ReleaseCapture();
	}

	void OnMouseMove(Win32::WPARAM btnState, int x, int y)
	{
		if ((btnState & Win32::MK::LButton) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = std::clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			float dx = 0.1f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.1f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = std::clamp(mRadius, 20.0f, 500.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
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
			-0.3f * std::sinf(0.1f * x) + 0.03f * x * std::sinf(0.1f * z)
		};
		auto unitNormal = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&n));
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
		auto vertices = std::vector<Basic32>(grid.Vertices.size());
		for (auto i = 0u; i < grid.Vertices.size(); ++i)
		{
			auto p = DirectX::XMFLOAT3{grid.Vertices[i].Position};
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
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &vertices[0]
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * mLandIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &grid.Indices[0]
		};
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
			.MiscFlags = 0
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
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &indices[0]
		};
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
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &vertices[0]
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * box.Indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &box.Indices[0]
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/Basic_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");

		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");

		BuildInputLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/Basic_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");

		HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mColorPS), "Failed to create pixel shader.");
		pixelShaderBytecode.reset();

		auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameCbd, 0, &mPerFrameCB), "Failed to create constant buffer.");

		auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mPerObjectCB), "Failed to create constant buffer.");

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
		auto vertexDesc = std::array{
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
			vertexDesc.data(),
			static_cast<std::uint32_t>(vertexDesc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mBasic32),
			"Failed to create input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mLandVB;
	ComPtr<D3D11::ID3D11Buffer> mLandIB;
	ComPtr<D3D11::ID3D11Buffer> mWavesVB;
	ComPtr<D3D11::ID3D11Buffer> mWavesIB;
	ComPtr<D3D11::ID3D11Buffer> mBoxVB;
	ComPtr<D3D11::ID3D11Buffer> mBoxIB;
	ComPtr<D3D11::ID3D11InputLayout> mBasic32;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;
	int mLightCount = 3;
	RenderStates mRenderStates;

	ComPtr<D3D11::ID3D11ShaderResourceView> mGrassMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWavesMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBoxMapSRV;

	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4X4 mWaterTexTransform = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4X4 mLandWorld = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4X4 mWavesWorld = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4X4 mBoxWorld = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

	DirectX::XMFLOAT4X4 mView = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4X4 mProj = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

	std::uint32_t mLandIndexCount = 0;

	DirectX::XMFLOAT2 mWaterTexOffset = {0.0f, 0.0f};

	RenderOptions mRenderOptions = RenderOptions::TexturesAndFog;

	DirectX::XMFLOAT3 mEyePosW = {0.0f, 0.0f, 0.0f};

	float mTheta = 1.3f * MathHelper::Pi;
	float mPhi = 0.4f * MathHelper::Pi;
	float mRadius = 80.0f;

	Win32::POINT mLastMousePos{};
};
