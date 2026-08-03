export module treebillboard:app;
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

namespace Basic32Vertex
{
	// Basic 32-byte vertex structure.
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Tex;
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
}

namespace TreeSpriteVertex
{
	// tree
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT2 Size;
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
		DirectX::XMFLOAT4X4 gViewProj;
		Material gMaterial;
	};
}

export class TreeBillboardApp : public D3DApp
{
public:
	TreeBillboardApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"Tree Billboard Demo";
		mEnable4xMsaa = true;

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

		mTreeMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mTreeMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mTreeMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

		Init();
	}

	void Init()
	{
		D3DApp::Init();

		mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

		// Must init Effects first since InputLayouts depend on shader signatures.

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/grass.dds", nullptr, &mGrassMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/water2.dds", nullptr, &mWavesMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/WireFence.dds", nullptr, &mBoxMapSRV));

		std::vector<std::wstring> treeFilenames;
		treeFilenames.push_back(L"Textures/tree0.dds");
		treeFilenames.push_back(L"Textures/tree1.dds");
		treeFilenames.push_back(L"Textures/tree2.dds");
		treeFilenames.push_back(L"Textures/tree3.dds");

		mTreeTextureMapArraySRV = d3dHelper::CreateConvertedTexture2DArraySRV(
			md3dDevice.get(),
			treeFilenames,
			DXGI::DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM
		);

		BuildLandGeometryBuffers();
		BuildWaveGeometryBuffers();
		BuildCrateGeometryBuffers();
		BuildTreeSpritesBuffer();
		BuildShaders();

		mRenderStates = { md3dDevice.get() };
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
		auto x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		auto z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		auto y = mRadius * std::cosf(mPhi);

		mEyePosW = DirectX::XMFLOAT3(x, y, z);

		// Build the view matrix.
		auto pos = DirectX::XMVECTOR{DirectX::XMVectorSet(x, y, z, 1.0f)};
		auto target = DirectX::XMVECTOR{DirectX::XMVectorZero()};
		auto up = DirectX::XMVECTOR{DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)};

		auto V = DirectX::XMMATRIX{DirectX::XMMatrixLookAtLH(pos, target, up)};
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

		auto v = reinterpret_cast<Basic32Vertex::Vertex*>(mappedData.pData);
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
		if (Win32::GetAsyncKeyState('R') & 0x8000)
			mAlphaToCoverageOn = true;
		if (Win32::GetAsyncKeyState('T') & 0x8000)
			mAlphaToCoverageOn = false;
	}

	void DrawScene()
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(),
			static_cast<D3D11::D3D11_CLEAR_FLAG>(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL),
			1.0f,
			0
		);

		auto blendFactor = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f };

		auto view = DirectX::XMMATRIX{XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{XMLoadFloat4x4(&mProj)};
		auto viewProj = DirectX::XMMATRIX{view * proj};

		// Draw the tree sprites
		DrawTreeSprites(viewProj);

		//
		// DrawTreeSprites() changes InputLayout and PrimitiveTopology, so change it based on 
		// the geometry we draw next.
		//
		
		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mColorVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		auto stride = static_cast<std::uint32_t>(sizeof(Basic32Vertex::Vertex));
		auto offset = 0u;

		//
		// Draw the box.
		//
		md3dImmediateContext->IASetVertexBuffers(0, 1, mBoxVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mBoxIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per frame constants for the rest of the objects.
		auto perFrameConstants = Basic32Vertex::PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = 3,
			.gUseTexture = mRenderOptions == RenderOptions::Textures or mRenderOptions == RenderOptions::TexturesAndFog,
			.gAlphaClip = mRenderOptions == RenderOptions::Textures or mRenderOptions == RenderOptions::TexturesAndFog,
			.gFogEnabled = mRenderOptions == RenderOptions::TexturesAndFog,
			.gFogColor = DirectX::XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f)
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perFrameConstants.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrameCB32.get(), 0, nullptr, &perFrameConstants, 0, 0);
		md3dImmediateContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		// Set per object constants.
		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&mBoxWorld);
		DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		DirectX::XMMATRIX worldViewProj = world * viewProj;
		auto perObjectConstants = Basic32Vertex::PerObjectConstants{
			.gMaterial = mBoxMat,
		};
		DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
		DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->PSSetShaderResources(0, 1, mBoxMapSRV.GetAddressOf());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB32.get(), 0, nullptr, &perObjectConstants, 0, 0);
		auto boxConstantBuffersVS = std::array{ mPerObjectCB32.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(boxConstantBuffersVS.size()), boxConstantBuffersVS.data());
		auto boxConstantBuffersPS = std::array{ mPerFrameCB32.get(), mPerObjectCB32.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(boxConstantBuffersPS.size()), boxConstantBuffersPS.data());

		md3dImmediateContext->OMSetBlendState(mRenderStates.AlphaToCoverageBS.get(), blendFactor.data(), 0xffffffff);
		md3dImmediateContext->RSSetState(mRenderStates.NoCullRS.get());
		md3dImmediateContext->DrawIndexed(36, 0, 0);

		// Restore default render state.
		md3dImmediateContext->RSSetState(0);

		//
		// Draw the hills and water with texture and fog (no alpha clipping needed).
		//

		//
		// Draw the hills.
		//
		md3dImmediateContext->IASetVertexBuffers(0, 1, mLandVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mLandIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&mLandWorld);
			DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
			DirectX::XMMATRIX worldViewProj = world * viewProj;

			auto landPerObject = Basic32Vertex::PerObjectConstants{
				.gMaterial = mLandMat,
			};
			DirectX::XMStoreFloat4x4(&landPerObject.gWorld, world);
			DirectX::XMStoreFloat4x4(&landPerObject.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&landPerObject.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&landPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mGrassTexTransform));
			md3dImmediateContext->PSSetShaderResources(0, 1, mGrassMapSRV.GetAddressOf());
			md3dImmediateContext->UpdateSubresource(mPerObjectCB32.get(), 0, nullptr, &landPerObject, 0, 0);
			md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);
		}

		//
		// Draw the waves.
		//
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mWavesVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(mWavesIB.get(), DXGI_FORMAT_R32_UINT, 0);

			//	// Set per object constants.
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&mWavesWorld);
			DirectX::XMMATRIX  worldInvTranspose = MathHelper::InverseTranspose(world);
			DirectX::XMMATRIX worldViewProj = world * viewProj;
			auto wavesPerObject = Basic32Vertex::PerObjectConstants{
				.gMaterial = mWavesMat,
			};
			DirectX::XMStoreFloat4x4(&wavesPerObject.gWorld, world);
			DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&wavesPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mWaterTexTransform));
			md3dImmediateContext->PSSetShaderResources(0, 1, mWavesMapSRV.GetAddressOf());
			md3dImmediateContext->UpdateSubresource(mPerObjectCB32.get(), 0, nullptr, &wavesPerObject, 0, 0);
			md3dImmediateContext->OMSetBlendState(mRenderStates.TransparentBS.get(), blendFactor.data(), 0xffffffff);
			md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);
		}

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

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}


private:
	auto GetHillHeight(float x, float z)const->float
	{
		return 0.3f * (z * std::sinf(0.1f * x) + x * std::cosf(0.1f * z));
	}

	auto GetHillNormal(float x, float z)const->DirectX::XMFLOAT3
	{
		// n = (-df/dx, 1, -df/dz)
		auto n = DirectX::XMFLOAT3(
			-0.03f * z * std::cosf(0.1f * x) - 0.3f * std::cosf(0.1f * z),
			1.0f,
			-0.3f * std::sinf(0.1f * x) + 0.03f * x * std::sinf(0.1f * z));
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
		auto vertices = std::vector<Basic32Vertex::Vertex>(grid.Vertices.size());
		for (auto i = 0u; i < grid.Vertices.size(); ++i)
		{
			DirectX::XMFLOAT3 p = grid.Vertices[i].Position;

			p.y = GetHillHeight(p.x, p.z);

			vertices[i].Pos = p;
			vertices[i].Normal = GetHillNormal(p.x, p.z);
			vertices[i].Tex = grid.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32Vertex::Vertex) * grid.Vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
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
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &grid.Indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
	}

	void BuildWaveGeometryBuffers()
	{
		// Create the vertex buffer.  Note that we allocate space only, as
		// we will be updating the data every time step of the simulation.

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32Vertex::Vertex) * mWaves.VertexCount()),
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
		int k = 0;
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
			.ByteWidth = static_cast<std::uint32_t>(sizeof(UINT) * indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
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

		auto vertices = std::vector<Basic32Vertex::Vertex>(box.Vertices.size());

		for (auto i = 0u; i < box.Vertices.size(); ++i)
		{
			vertices[i].Pos = box.Vertices[i].Position;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].Tex = box.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic32Vertex::Vertex) * box.Vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(UINT) * box.Indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &box.Indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}

	void BuildTreeSpritesBuffer()
	{
		auto v = std::array<TreeSpriteVertex::Vertex, TreeCount>{};

		for (auto i = 0u; i < TreeCount; ++i)
		{
			float x = MathHelper::RandF(-35.0f, 35.0f);
			float z = MathHelper::RandF(-35.0f, 35.0f);
			float y = GetHillHeight(x, z);

			// Move tree slightly above land height.
			y += 10.0f;

			v[i].Pos = DirectX::XMFLOAT3(x, y, z);
			v[i].Size = DirectX::XMFLOAT2(24.0f, 24.0f);
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(TreeSpriteVertex::Vertex) * TreeCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = v.data() };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mTreeSpritesVB));
	}

	void DrawTreeSprites(const DirectX::XMMATRIX& viewProj)
	{
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		md3dImmediateContext->IASetInputLayout(mTreePointSprite.get());

		md3dImmediateContext->VSSetShader(mTreeVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mTreePS.get(), nullptr, 0);
		md3dImmediateContext->GSSetShader(mTreeGS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShaderResources(0, 1, mTreeTextureMapArraySRV.GetAddressOf());
		md3dImmediateContext->PSSetSamplers(0, 1, mTreeSamplerState.GetAddressOf());

		auto stride = static_cast<std::uint32_t>(sizeof(TreeSpriteVertex::Vertex));
		auto offset = 0u;

		auto perFrameTreeCB = TreeSpriteVertex::PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = 3,
			.gUseTexture = mRenderOptions == RenderOptions::Textures or mRenderOptions == RenderOptions::TexturesAndFog,
			.gAlphaClip = mRenderOptions == RenderOptions::Textures or mRenderOptions == RenderOptions::TexturesAndFog,
			.gFogEnabled = mRenderOptions == RenderOptions::TexturesAndFog,
			.gFogColor = DirectX::XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f)
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perFrameTreeCB.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrameTreeCB.get(), 0, nullptr, &perFrameTreeCB, 0, 0);

		auto perObjectTreeCBData = TreeSpriteVertex::PerObjectConstants{
			.gMaterial = mTreeMat,
		};
		DirectX::XMStoreFloat4x4(&perObjectTreeCBData.gViewProj, viewProj);
		md3dImmediateContext->UpdateSubresource(mPerObjectTreeCB.get(), 0, nullptr, &perObjectTreeCBData, 0, 0);

		auto vsConstantBuffers = std::array{ mPerObjectTreeCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());
		auto psConstantBuffers = std::array{ mPerFrameTreeCB.get(), mPerObjectTreeCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers.size()), psConstantBuffers.data());
		auto gsConstantBuffers = std::array{ mPerFrameTreeCB.get(), mPerObjectTreeCB.get() };
		md3dImmediateContext->GSSetConstantBuffers(0, static_cast<std::uint32_t>(gsConstantBuffers.size()), gsConstantBuffers.data());

		auto treeVBs = std::array{ mTreeSpritesVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(treeVBs.size()), treeVBs.data(), &stride, &offset);
		auto blendFactor = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };

		if (mAlphaToCoverageOn)
			md3dImmediateContext->OMSetBlendState(mRenderStates.AlphaToCoverageBS.get(), blendFactor.data(), 0xffffffff);
		md3dImmediateContext->Draw(TreeCount, 0);
		md3dImmediateContext->OMSetBlendState(0, blendFactor.data(), 0xffffffff);
	}

	void BuildShaders()
	{
		// basic32 shaders
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");
		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
		HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mColorPS), "Failed to create pixel shader.");
		pixelShaderBytecode.reset();

		//
		// Tree point sprite shaders
		// Vertex Shader
		auto treeVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/TreeSprite_VS.cso", &treeVertexShaderBytecode), "Failed to read tree vertex shader file.");
		hr = md3dDevice->CreateVertexShader(treeVertexShaderBytecode->GetBufferPointer(), treeVertexShaderBytecode->GetBufferSize(), 0, &mTreeVS);
		HR(hr, "Failed to create vertex shader.");
		// Pixel Shader
		auto treePixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/TreeSprite_PS.cso", &treePixelShaderBytecode), "Failed to read tree pixel shader file.");
		HR(md3dDevice->CreatePixelShader(treePixelShaderBytecode->GetBufferPointer(), treePixelShaderBytecode->GetBufferSize(), 0, &mTreePS), "Failed to create pixel shader.");
		treePixelShaderBytecode.reset();
		// Geometry Shader
		auto treeGeometryShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/TreeSprite_GS.cso", &treeGeometryShaderBytecode), "Failed to read tree geometry shader file.");
		HR(md3dDevice->CreateGeometryShader(treeGeometryShaderBytecode->GetBufferPointer(), treeGeometryShaderBytecode->GetBufferSize(), 0, &mTreeGS), "Failed to create geometry shader.");

		// Build layouts
		BuildInputLayout(vertexShaderBytecode.get(), treeVertexShaderBytecode.get());
		vertexShaderBytecode.reset();
		treeVertexShaderBytecode.reset();

		// constant buffers basic32
		auto perFrameCbd32 = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Basic32Vertex::PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameCbd32, 0, &mPerFrameCB32), "Failed to create constant buffer.");

		auto perObjectCbd32 = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Basic32Vertex::PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd32, 0, &mPerObjectCB32), "Failed to create constant buffer.");

		// constant buffers tree
		auto perFrameTreeCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(TreeSpriteVertex::PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameTreeCbd, 0, &mPerFrameTreeCB), "Failed to create constant buffer.");

		auto perObjectTreeCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(TreeSpriteVertex::PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectTreeCbd, 0, &mPerObjectTreeCB), "Failed to create constant buffer.");

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

		auto samplerDesc2 = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 4,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(md3dDevice->CreateSamplerState(&samplerDesc2, &mTreeSamplerState), "Failed to create sampler state.");
	}

	void BuildInputLayout(D3D::ID3DBlob* vertexShaderBytecode, D3D::ID3DBlob* treeShaderBytecode)
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

		// Create the tree point sprite input layout.
		auto treeVertexDesc = std::array{
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
				.SemanticName = "SIZE",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
		};
		HR(md3dDevice->CreateInputLayout(
			treeVertexDesc.data(),
			static_cast<std::uint32_t>(treeVertexDesc.size()),
			treeShaderBytecode->GetBufferPointer(),
			treeShaderBytecode->GetBufferSize(),
			&mTreePointSprite),
			"Failed to create tree input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mLandVB;
	ComPtr<D3D11::ID3D11Buffer> mLandIB;
	ComPtr<D3D11::ID3D11Buffer> mWavesVB;
	ComPtr<D3D11::ID3D11Buffer> mWavesIB;
	ComPtr<D3D11::ID3D11Buffer> mBoxVB;
	ComPtr<D3D11::ID3D11Buffer> mBoxIB;
	ComPtr<D3D11::ID3D11Buffer> mTreeSpritesVB;
	ComPtr<D3D11::ID3D11InputLayout> mBasic32;
	ComPtr<D3D11::ID3D11InputLayout> mTreePointSprite;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11VertexShader> mTreeVS;
	ComPtr<D3D11::ID3D11PixelShader> mTreePS;
	ComPtr<D3D11::ID3D11GeometryShader> mTreeGS;

	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB32;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB32;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameTreeCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectTreeCB;

	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;
	ComPtr<D3D11::ID3D11SamplerState> mTreeSamplerState;

	ComPtr<D3D11::ID3D11ShaderResourceView> mGrassMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWavesMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBoxMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mTreeTextureMapArraySRV;

	RenderStates mRenderStates;
	Waves mWaves;
	int mNumLights = 3;
	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;
	Material mTreeMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform;
	DirectX::XMFLOAT4X4 mWaterTexTransform;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;
	DirectX::XMFLOAT4X4 mBoxWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	std::uint32_t mLandIndexCount = 0;

	static constexpr auto TreeCount = 16u;

	bool mAlphaToCoverageOn = true;

	DirectX::XMFLOAT2 mWaterTexOffset = {0.0f, 0.0f};

	RenderOptions mRenderOptions = RenderOptions::TexturesAndFog;

	DirectX::XMFLOAT3 mEyePosW = {0.0f, 0.0f, 0.0f};

	float mTheta = 1.3f * MathHelper::Pi;
	float mPhi = 0.4f * MathHelper::Pi;
	float mRadius = 80.0f;

	Win32::POINT mLastMousePos{};
};