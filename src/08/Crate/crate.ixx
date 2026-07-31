export module crate;
import std;
import shared;

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
	int gLightCount = 0;
	int gUseTexture = 0;
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

export class CrateApp : public D3DApp
{
public:
	CrateApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance }
	{
		mMainWndCaption = L"Crate Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&mBoxWorld, I);
		DirectX::XMStoreFloat4x4(&mTexTransform, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

		mDirLights[0].Ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(1.4f, 1.4f, 1.4f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 16.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(-0.707f, 0.0f, 0.707f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/WoodCrate01.dds", nullptr, mDiffuseMapSRV.GetAddressOf()), "Failed to create texture from file");

		BuildGeometryBuffers();
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
	}
	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::LightSteelBlue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		md3dImmediateContext->VSSetShader(mColorVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), nullptr, 0);

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		DirectX::XMMATRIX view = XMLoadFloat4x4(&mView);
		DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProj);
		DirectX::XMMATRIX viewProj = view * proj;

		// Set per frame constants.
		auto perframe = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 0,
			.gFogRange = 0,
			.gLightCount = mLightCount,
			.gUseTexture = true,
			.gFogColor = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f),
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perframe.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, nullptr, &perframe, 0, 0);

		auto vertexBuffers = std::array{ mBoxVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mBoxIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Draw the box.
		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&mBoxWorld);
		DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		DirectX::XMMATRIX worldViewProj = world * viewProj;
		auto perObject = PerObjectConstants{
			.gMaterial = mBoxMat,
		};
		DirectX::XMStoreFloat4x4(&perObject.gWorld, world);
		DirectX::XMStoreFloat4x4(&perObject.gWorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObject.gWorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&perObject.gTexTransform, DirectX::XMLoadFloat4x4(&mTexTransform));
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &perObject, 0, 0);

		auto vsConstantBuffers = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());
		auto psConstantBuffers = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstantBuffers.size()), psConstantBuffers.data());
		auto shaderResourceViews = std::array{ mDiffuseMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(shaderResourceViews.size()), shaderResourceViews.data());

		md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

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
			// Make each pixel correspond to 0.01 unit in the scene.
			float dx = 0.01f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.01f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = std::clamp(mRadius, 1.0f, 15.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
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

		auto hr = md3dDevice->CreateInputLayout(
			vertexDesc.data(),
			static_cast<std::uint32_t>(vertexDesc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mBasic32
		);
		HR(hr, "Failed to create input layout.");
	}

	void BuildGeometryBuffers()
	{
		auto box = GeometryGenerator::MeshData{};

		auto geoGen = GeometryGenerator{};
		geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

		// Cache the vertex offsets to each object in the concatenated vertex buffer.
		mBoxVertexOffset = 0;

		// Cache the index count of each object.
		mBoxIndexCount = static_cast<std::uint32_t>(box.Indices.size());

		// Cache the starting index for each object in the concatenated index buffer.
		mBoxIndexOffset = 0;

		auto totalVertexCount = static_cast<std::uint32_t>(box.Vertices.size());
		auto totalIndexCount = static_cast<std::uint32_t>(mBoxIndexCount);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		auto vertices = std::vector<Basic32>(totalVertexCount);

		auto k = 0u;
		for (auto i = 0ull; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
			vertices[k].Tex = box.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Basic32) * totalVertexCount,
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

		auto indices = std::vector<std::uint32_t>{};
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(std::uint32_t) * totalIndexCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mBoxVB;
	ComPtr<D3D11::ID3D11Buffer> mBoxIB;
	ComPtr<D3D11::ID3D11InputLayout> mBasic32;
	ComPtr<D3D11::ID3D11ShaderResourceView> mDiffuseMapSRV;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;

	DirectionalLight mDirLights[3];
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mTexTransform;
	DirectX::XMFLOAT4X4 mBoxWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	int mBoxVertexOffset;
	std::uint32_t mBoxIndexOffset;
	std::uint32_t mBoxIndexCount;
	int mLightCount = 2;

	DirectX::XMFLOAT3 mEyePosW = { 0.0f, 0.0f, 0.0f };

	float mTheta = 1.3f * MathHelper::Pi;
	float mPhi = 0.4f * MathHelper::Pi;
	float mRadius = 2.5f;

	Win32::POINT mLastMousePos{};
};