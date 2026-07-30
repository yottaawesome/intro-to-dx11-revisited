export module lighting;
import std;
import shared;
import :waves;

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};

struct PerFrameConstants
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	SpotLight gSpotLight;
	DirectX::XMFLOAT3 gEyePosW;
	float pad = 0.0f;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTranspose;
	DirectX::XMFLOAT4X4 WorldViewProj;
	Material Mat;
};

export class LightingApp : public D3DApp
{
public:
	LightingApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"Lighting Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		auto I = DirectX::XMMATRIX{DirectX::XMMatrixIdentity()};
		DirectX::XMStoreFloat4x4(&mLandWorld, I);
		DirectX::XMStoreFloat4x4(&mWavesWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

		auto wavesOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(0.0f, -3.0f, 0.0f)};
		DirectX::XMStoreFloat4x4(&mWavesWorld, wavesOffset);

		// Directional light.
		mDirLight.Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLight.Diffuse = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLight.Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLight.Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

		// Point light--position is changed every frame to animate in UpdateScene function.
		mPointLight.Ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		mPointLight.Diffuse = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
		mPointLight.Specular = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
		mPointLight.Att = DirectX::XMFLOAT3(0.0f, 0.1f, 0.0f);
		mPointLight.Range = 25.0f;

		// Spot light--position and direction changed every frame to animate in UpdateScene function.
		mSpotLight.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mSpotLight.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
		mSpotLight.Specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mSpotLight.Att = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		mSpotLight.Spot = 96.0f;
		mSpotLight.Range = 10000.0f;

		mLandMat.Ambient = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		mLandMat.Diffuse = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		mLandMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

		mWavesMat.Ambient = DirectX::XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
		mWavesMat.Diffuse = DirectX::XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
		mWavesMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		mWaves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

		BuildLandGeometryBuffers();
		BuildWaveGeometryBuffers();
		BuildShaders();
	}

	void OnResize() override
	{
		D3DApp::OnResize();
		auto P = DirectX::XMMATRIX{DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f)};
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt) override
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

		auto* v = reinterpret_cast<Vertex*>(mappedData.pData);
		for (auto i = 0u; i < mWaves.VertexCount(); ++i)
		{
			v[i].Pos = mWaves[i];
			v[i].Normal = mWaves.Normal(i);
		}

		md3dImmediateContext->Unmap(mWavesVB.get(), 0);

		//
		// Animate the lights.
		// Circle light over the land surface.
		mPointLight.Position.x = 70.0f * std::cosf(0.2f * mTimer.TotalTime());
		mPointLight.Position.z = 70.0f * std::sinf(0.2f * mTimer.TotalTime());
		mPointLight.Position.y = std::max(GetHillHeight(mPointLight.Position.x,
			mPointLight.Position.z), -3.0f) + 10.0f;


		// The spotlight takes on the camera position and is aimed in the
		// same direction the camera is looking.  In this way, it looks
		// like we are holding a flashlight.
		mSpotLight.Position = mEyePosW;
		DirectX::XMStoreFloat3(&mSpotLight.Direction, DirectX::XMVector3Normalize(target - pos));
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::LightSteelBlue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11::D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto stride = static_cast<std::uint32_t>(sizeof(Vertex));
		auto offset = 0u;

		DirectX::XMMATRIX view = XMLoadFloat4x4(&mView);
		DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProj);
		DirectX::XMMATRIX viewProj = view * proj;

		//
		// Set per frame constants.
		auto perFrame = PerFrameConstants{
			.gDirLight = mDirLight,
			.gPointLight = mPointLight,
			.gSpotLight = mSpotLight,
			.gEyePosW = mEyePosW,
		};
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, 0, &perFrame, 0, 0);
		md3dImmediateContext->VSSetShader(mColorVS.get(), 0, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), 0, 0);

		//
		// Draw the hills.
		auto vertexBuffers = std::array{ mLandVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mLandIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto landObject = PerObjectConstants{.Mat = mLandMat };
		auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mLandWorld)};
		auto worldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(world)};
		auto worldViewProj = DirectX::XMMATRIX{world * view * proj};
		DirectX::XMStoreFloat4x4(&landObject.World, world);
		DirectX::XMStoreFloat4x4(&landObject.WorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&landObject.WorldViewProj, worldViewProj);
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, 0, &landObject, 0, 0);

		auto vsConstantBuffers = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, 0, &perFrame, 0, 0);
		auto psConstantBuffers = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers.size()), psConstantBuffers.data());

		md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);

		//
		// Draw the waves.
		auto waveVertexBuffers = std::array{ mWavesVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(waveVertexBuffers.size()), waveVertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mWavesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto wavesObject = PerObjectConstants{ .Mat = mWavesMat };
		world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mWavesWorld)};
		worldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(world)};
		worldViewProj = DirectX::XMMATRIX{world * view * proj};
		DirectX::XMStoreFloat4x4(&wavesObject.World, world);
		DirectX::XMStoreFloat4x4(&wavesObject.WorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&wavesObject.WorldViewProj, worldViewProj);
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, 0, &wavesObject, 0, 0);

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
			// Make each pixel correspond to 0.2 unit in the scene.
			float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

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

		auto vertices = std::vector<Vertex>(grid.Vertices.size());
		for (auto i = 0ull; i < grid.Vertices.size(); ++i)
		{
			auto p = DirectX::XMFLOAT3{grid.Vertices[i].Position};

			p.y = GetHillHeight(p.x, p.z);

			vertices[i].Pos = p;
			vertices[i].Normal = GetHillNormal(p.x, p.z);
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Vertex) * grid.Vertices.size()),
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
			.ByteWidth = sizeof(Vertex) * mWaves.VertexCount(),
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
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
	}

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Lighting_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");

		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");

		BuildVertexLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Lighting_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");

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

		auto cbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&cbd, 0, &mPerObjectCB), "Failed to create constant buffer.");
	}

	void BuildVertexLayout(D3D::ID3DBlob* vertexShaderBytecode)
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
			}
		};

		auto hr = md3dDevice->CreateInputLayout(
			vertexDesc.data(),
			static_cast<std::uint32_t>(vertexDesc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mInputLayout
		);
		HR(hr, "Failed to create input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mLandVB;
	ComPtr<D3D11::ID3D11Buffer> mLandIB;
	ComPtr<D3D11::ID3D11Buffer> mWavesVB;
	ComPtr<D3D11::ID3D11Buffer> mWavesIB;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;

	DirectionalLight mDirLight;
	PointLight mPointLight;
	SpotLight mSpotLight;
	Material mLandMat;
	Material mWavesMat;

	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;

	ComPtr<D3D11::ID3D11RasterizerState> mWireframeRS;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;

	std::uint32_t mLandIndexCount = 0;

	std::uint32_t mGridIndexCount = 0;

	Waves mWaves;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	DirectX::XMFLOAT3 mEyePosW;

	float mTheta = 1.5f * MathHelper::Pi;
	float mPhi = 0.1f * MathHelper::Pi;
	float mRadius = 80.f;

	Win32::POINT mLastMousePos{};
};
