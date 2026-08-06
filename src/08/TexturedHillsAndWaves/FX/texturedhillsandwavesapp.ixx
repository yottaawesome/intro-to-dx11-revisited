export module texturedhillsandwaves:app;
import std;
import shared;
import :waves;

// Basic 32-byte vertex structure.
struct Basic32
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
	int gPadding;
	DirectX::XMFLOAT4 gFogColor;
};

static_assert(sizeof(PerFrameConstants) == 240);

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};

export class TexturedHillsAndWavesApp : public D3DApp
{
public:
	TexturedHillsAndWavesApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"TexturedHillsAndWaves Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		auto I = DirectX::XMMATRIX{DirectX::XMMatrixIdentity()};
		DirectX::XMStoreFloat4x4(&mLandWorld, I);
		DirectX::XMStoreFloat4x4(&mWavesWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

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
		mWavesMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mWavesMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		mWaves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

		// Must init Effects first since InputLayouts depend on shader signatures.

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/grass.dds", nullptr, &mGrassMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/water2.dds", nullptr, &mWavesMapSRV));

		BuildLandGeometryBuffers();
		BuildWaveGeometryBuffers();
		BuildShaders();
	}

	void OnResize() override
	{
		D3DApp::OnResize();
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt) override
	{
		// Convert Spherical to Cartesian coordinates.
		auto x = float{mRadius * std::sinf(mPhi) * std::cosf(mTheta)};
		auto z = float{mRadius * std::sinf(mPhi) * std::sinf(mTheta)};
		auto y = float{mRadius * std::cosf(mPhi)};

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
		if ((mTimer.TotalTime() - t_base) >= 0.25f)
		{
			t_base += 0.25f;
			auto i = static_cast<std::uint32_t>(5 + std::rand() % (mWaves.RowCount() - 10));
			auto j = static_cast<std::uint32_t>(5 + std::rand() % (mWaves.ColumnCount() - 10));
			auto r = MathHelper::RandF(1.0f, 2.0f);
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
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::LightSteelBlue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		md3dImmediateContext->VSSetShader(mColorVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), nullptr, 0);
		auto samplers = std::array{ mSamplerState.get() };
		md3dImmediateContext->PSSetSamplers(0, static_cast<std::uint32_t>(samplers.size()), samplers.data());

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto viewProj = view * proj;

		// Set per frame constants.
		auto perframe = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = true,
			.gFogColor = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f),
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perframe.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, nullptr, &perframe, 0, 0);

		// Draw the hills.
		auto vertexBuffers = std::array{ mLandVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mLandIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mLandWorld)};
		auto worldInvTranspose = MathHelper::InverseTranspose(world);
		auto worldViewProj = world * view * proj;
		auto perObject = PerObjectConstants{
			.gMaterial = mLandMat,
		};
		DirectX::XMStoreFloat4x4(&perObject.gWorld, world);
		DirectX::XMStoreFloat4x4(&perObject.gWorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObject.gWorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&perObject.gTexTransform, DirectX::XMLoadFloat4x4(&mGrassTexTransform));
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &perObject, 0, 0);
		auto vsConstantBuffers = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());
		auto psConstantBuffers = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers.size()), psConstantBuffers.data());
		auto shaderResourceViews = std::array{ mGrassMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(shaderResourceViews.size()), shaderResourceViews.data());
		md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);

		//
		// Draw the waves.
		//
		auto vertexBuffers2 = std::array{ mWavesVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(vertexBuffers2.size()), vertexBuffers2.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mWavesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		world = XMLoadFloat4x4(&mWavesWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;
		auto wavesPerObject = PerObjectConstants{
			.gMaterial = mWavesMat,
		};
		DirectX::XMStoreFloat4x4(&wavesPerObject.gWorld, world);
		DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&wavesPerObject.gWorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&wavesPerObject.gTexTransform, DirectX::XMLoadFloat4x4(&mWaterTexTransform));
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &wavesPerObject, 0, 0);
		auto vsConstantBuffers2 = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers2.size()), vsConstantBuffers2.data());
		auto psConstantBuffers2 = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers2.size()), psConstantBuffers2.data());
		auto shaderResourceViews2 = std::array{ mWavesMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(shaderResourceViews2.size()), shaderResourceViews2.data());
		md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;
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

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = std::clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			auto dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
			auto dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = std::clamp(mRadius, 50.0f, 500.0f);
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
		auto unitNormal = DirectX::XMVECTOR{DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&n))};
		DirectX::XMStoreFloat3(&n, unitNormal);
		return n;
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
		for (auto i = 0ull; i < grid.Vertices.size(); ++i)
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
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &vertices[0]
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

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
			.ByteWidth = sizeof(Basic32) * mWaves.VertexCount(),
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

private:
	ComPtr<D3D11::ID3D11Buffer> mLandVB;
	ComPtr<D3D11::ID3D11Buffer> mLandIB;

	ComPtr<D3D11::ID3D11Buffer> mWavesVB;
	ComPtr<D3D11::ID3D11Buffer> mWavesIB;
	ComPtr<D3D11::ID3D11InputLayout> mBasic32;
	ComPtr<D3D11::ID3D11ShaderResourceView> mDiffuseMapSRV;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	ComPtr<D3D11::ID3D11ShaderResourceView> mGrassMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWavesMapSRV;

	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform;
	DirectX::XMFLOAT4X4 mWaterTexTransform;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	std::uint32_t mLandIndexCount = 0;
	DirectX::XMFLOAT2 mWaterTexOffset{};

	DirectX::XMFLOAT3 mEyePosW;

	float mTheta = 1.3f * MathHelper::Pi;
	float mPhi = 0.4f * MathHelper::Pi;
	float mRadius = 80.0f;
	int mLightCount = 3;

	Win32::POINT mLastMousePos{};
};