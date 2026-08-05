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
	BlurApp(Win32::HINSTANCE hInstance);

	void Init()override;
	void OnResize()override;
	void UpdateScene(float dt)override;
	void DrawScene()override;

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)override;
	void OnMouseUp(Win32::WPARAM btnState, int x, int y)override;
	void OnMouseMove(Win32::WPARAM btnState, int x, int y)override;

private:
	void UpdateWaves();
	void DrawWrapper();
	void DrawScreenQuad()
	{
		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();

		// 0 lights, true for texture, false for alpha clip, false for fog
		//ID3DX11EffectTechnique* texOnlyTech = Effects::BasicFX->Light0TexTech;

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
		auto srvs = std::array{ mBlur.GetBlurredOutput() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srvs.size()), srvs.data());

		auto vsConstantBuffers = std::array{ mPerObject.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());
		auto psConstantBuffers = std::array{ mPerFrame.get(), mPerObject.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers.size()), psConstantBuffers.data());

		md3dImmediateContext->DrawIndexed(6, 0, 0);
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
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");
		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
		HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mColorPS), "Failed to create pixel shader.");
		pixelShaderBytecode.reset();

		//
		// Compute Shader
		// TODO
		//

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

	Win32::POINT mLastMousePos{};
};