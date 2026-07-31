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
	DirectX::XMFLOAT4 gFogColor;
	int pad;
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
	BlendApp(Win32::HINSTANCE hInstance);
	~BlendApp();

	void Init()
	{
		D3DApp::Init();

		mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

		// Must init Effects first since InputLayouts depend on shader signatures.
		//Effects::InitAll(md3dDevice);
		//InputLayouts::InitAll(md3dDevice);
		//RenderStates::InitAll(md3dDevice);

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

		auto blendFactor = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto viewProj = view * proj;

		// Set per frame constants.
		//Effects::BasicFX->SetDirLights(mDirLights);
		//Effects::BasicFX->SetEyePosW(mEyePosW);
		//Effects::BasicFX->SetFogColor(Colors::Silver);
		//Effects::BasicFX->SetFogStart(15.0f);
		//Effects::BasicFX->SetFogRange(175.0f);

		//ID3DX11EffectTechnique* boxTech;
		//ID3DX11EffectTechnique* landAndWavesTech;

		//switch (mRenderOptions)
		//{
		//case RenderOptions::Lighting:
		//	boxTech = Effects::BasicFX->Light3Tech;
		//	landAndWavesTech = Effects::BasicFX->Light3Tech;
		//	break;
		//case RenderOptions::Textures:
		//	boxTech = Effects::BasicFX->Light3TexAlphaClipTech;
		//	landAndWavesTech = Effects::BasicFX->Light3TexTech;
		//	break;
		//case RenderOptions::TexturesAndFog:
		//	boxTech = Effects::BasicFX->Light3TexAlphaClipFogTech;
		//	landAndWavesTech = Effects::BasicFX->Light3TexFogTech;
		//	break;
		//}

		//D3DX11_TECHNIQUE_DESC techDesc;

		////
		//// Draw the box with alpha clipping.
		//// 

		//boxTech->GetDesc(&techDesc);
		//for (UINT p = 0; p < techDesc.Passes; ++p)
		//{
		//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
		//	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

		//	// Set per object constants.
		//	XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);
		//	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		//	XMMATRIX worldViewProj = world * view * proj;

		//	Effects::BasicFX->SetWorld(world);
		//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//	Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		//	Effects::BasicFX->SetMaterial(mBoxMat);
		//	Effects::BasicFX->SetDiffuseMap(mBoxMapSRV);

		//	md3dImmediateContext->RSSetState(RenderStates::NoCullRS);
		//	boxTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//	md3dImmediateContext->DrawIndexed(36, 0, 0);

		//	// Restore default render state.
		//	md3dImmediateContext->RSSetState(0);
		//}

		////
		//// Draw the hills and water with texture and fog (no alpha clipping needed).
		////

		//landAndWavesTech->GetDesc(&techDesc);
		//for (UINT p = 0; p < techDesc.Passes; ++p)
		//{
		//	//
		//	// Draw the hills.
		//	//
		//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
		//	md3dImmediateContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

		//	// Set per object constants.
		//	XMMATRIX world = XMLoadFloat4x4(&mLandWorld);
		//	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		//	XMMATRIX worldViewProj = world * view * proj;

		//	Effects::BasicFX->SetWorld(world);
		//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//	Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&mGrassTexTransform));
		//	Effects::BasicFX->SetMaterial(mLandMat);
		//	Effects::BasicFX->SetDiffuseMap(mGrassMapSRV);

		//	landAndWavesTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//	md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);

		//	//
		//	// Draw the waves.
		//	//
		//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
		//	md3dImmediateContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);

		//	// Set per object constants.
		//	world = XMLoadFloat4x4(&mWavesWorld);
		//	worldInvTranspose = MathHelper::InverseTranspose(world);
		//	worldViewProj = world * view * proj;

		//	Effects::BasicFX->SetWorld(world);
		//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//	Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&mWaterTexTransform));
		//	Effects::BasicFX->SetMaterial(mWavesMat);
		//	Effects::BasicFX->SetDiffuseMap(mWavesMapSRV);

		//	md3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		//	landAndWavesTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//	md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

		//	// Restore default blend state
		//	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
		//}

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
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");

		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");

		BuildInputLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");

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
	RenderStates mRenderStates;

	ComPtr<D3D11::ID3D11ShaderResourceView> mGrassMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWavesMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBoxMapSRV;

	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform;
	DirectX::XMFLOAT4X4 mWaterTexTransform;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;
	DirectX::XMFLOAT4X4 mBoxWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	std::uint32_t mLandIndexCount;

	DirectX::XMFLOAT2 mWaterTexOffset;

	RenderOptions mRenderOptions;

	DirectX::XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	Win32::POINT mLastMousePos{};
};
