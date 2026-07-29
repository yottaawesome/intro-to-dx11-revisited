export module lighting;
import std;
import shared;
import :waves;

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTranspose;
	DirectX::XMFLOAT4X4 WorldViewProj;
	Material Mat;
};

export class WavesDemo : public D3DApp
{
public:
	WavesDemo(Win32::HINSTANCE hInstance);

	void Init() override;
	void OnResize() override;
	void UpdateScene(float dt) override;
	void DrawScene() override
	{
		// todo
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

	void BuildWavesGeometryBuffers()
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
		auto hr = D3D::D3DReadFileToBlob(L"FX/color_VS.cso", &vertexShaderBytecode);
		HR(hr, "Failed to read vertex shader file.");

		hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");

		BuildVertexLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/color_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");

		HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mColorPS), "Failed to create pixel shader.");
		pixelShaderBytecode.reset();

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
				.SemanticName = "COLOR",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
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

	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;

	ComPtr<D3D11::ID3D11RasterizerState> mWireframeRS;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;

	UINT mLandIndexCount = 0;

	std::uint32_t mGridIndexCount = 0;

	Waves mWaves;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	float mTheta = 1.5f * MathHelper::Pi;
	float mPhi = 0.1f * MathHelper::Pi;
	float mRadius = 80.f;

	Win32::POINT mLastMousePos{};
};