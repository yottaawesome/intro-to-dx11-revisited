export module treebillboard:app;
import std;
import shared;
import :waves;
import :renderstates;

// Basic 32-byte vertex structure.
struct Basic32
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

struct TreePointSprite
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 Size;
};

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

struct PerFrameConstantsBasic32
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

static_assert(sizeof(PerFrameConstantsBasic32) == 256);

struct PerObjectConstantsBasic32
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};

struct PerFrameConstantsTree
{
	DirectX::XMFLOAT4X4 gViewProj;
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

struct PerObjectConstantsTree
{
	DirectX::XMFLOAT4X4 gViewProj;
	Material gMaterial;
};

class TreeBillboardApp : public D3DApp
{
public:
	TreeBillboardApp(HINSTANCE hInstance);
	~TreeBillboardApp();

	void Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(Win32::WPARAM btnState, int x, int y);
	void OnMouseUp(Win32::WPARAM btnState, int x, int y);
	void OnMouseMove(Win32::WPARAM btnState, int x, int y);

private:
	auto GetHillHeight(float x, float z)const->float;
	auto GetHillNormal(float x, float z)const->DirectX::XMFLOAT3;
	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();
	void BuildCrateGeometryBuffers();
	void BuildTreeSpritesBuffer();
	void DrawTreeSprites(const DirectX::XMMATRIX& viewProj);

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

		// tree point sprite shaders
		auto treeVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/TreeSprite_VS.cso", &treeVertexShaderBytecode), "Failed to read tree vertex shader file.");
		hr = md3dDevice->CreateVertexShader(treeVertexShaderBytecode->GetBufferPointer(), treeVertexShaderBytecode->GetBufferSize(), 0, &mTreeVS);
		HR(hr, "Failed to create vertex shader.");
		auto treePixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/TreeSprite_PS.cso", &treePixelShaderBytecode), "Failed to read tree pixel shader file.");
		HR(md3dDevice->CreatePixelShader(treePixelShaderBytecode->GetBufferPointer(), treePixelShaderBytecode->GetBufferSize(), 0, &mTreePS), "Failed to create pixel shader.");
		treePixelShaderBytecode.reset();

		// Build layouts
		BuildInputLayout(vertexShaderBytecode.get(), treeVertexShaderBytecode.get());
		vertexShaderBytecode.reset();
		treeVertexShaderBytecode.reset();

		// constant buffers basic32
		auto perFrameCbd32 = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstantsBasic32),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameCbd32, 0, &mPerFrameCB32), "Failed to create constant buffer.");

		auto perObjectCbd32 = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstantsBasic32),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd32, 0, &mPerObjectCB32), "Failed to create constant buffer.");

		// constant buffers tree
		auto perFrameTreeCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstantsTree),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameTreeCbd, 0, &mPerFrameTreeCB), "Failed to create constant buffer.");

		auto perObjectTreeCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstantsTree),
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

	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB32;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB32;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameTreeCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectTreeCB;

	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	ComPtr<D3D11::ID3D11ShaderResourceView> mGrassMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWavesMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBoxMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mTreeTextureMapArraySRV;

	Waves mWaves;

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

	UINT mLandIndexCount;

	static const UINT TreeCount = 16;

	bool mAlphaToCoverageOn;

	DirectX::XMFLOAT2 mWaterTexOffset;

	RenderOptions mRenderOptions;

	DirectX::XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	Win32::POINT mLastMousePos{};
};