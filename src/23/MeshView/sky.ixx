export module meshviewdemo:sky;
import std;
import shared;

class Sky
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
	};

	struct PerFrameConstants
	{
		DirectX::XMFLOAT4X4 WorldViewProj;
	};
	static_assert(sizeof(PerFrameConstants) == 64);

	Sky(const Sky& rhs) = delete;
	auto operator=(const Sky& rhs) -> Sky& = delete;

	Sky(D3D11::ID3D11Device* device, const std::wstring& cubemapFilename, float skySphereRadius)
	{
		HR(DirectX::CreateDDSTextureFromFile(device, cubemapFilename.c_str(), nullptr, &mCubeMapSRV));
		BuildGeometryBuffers(device, skySphereRadius);
		BuildShaders(device);
		BuildRenderStates(device);
	}

	auto CubeMapSRV() -> D3D11::ID3D11ShaderResourceView*
	{
		return mCubeMapSRV.get();
	}

	void Draw(D3D11::ID3D11DeviceContext* dc, const Camera& camera)
	{
		// center Sky about eye in world space
		auto eyePos = DirectX::XMFLOAT3{camera.GetPosition()};
		auto T = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z)};
		auto WVP = DirectX::XMMATRIX{DirectX::XMMatrixMultiply(T, camera.ViewProj())};

		dc->VSSetShader(mVertexShader.get(), nullptr, 0);
		dc->GSSetShader(nullptr, nullptr, 0);
		dc->PSSetShader(mPixelShader.get(), nullptr, 0);

		auto perFrameConstants = PerFrameConstants{};
		DirectX::XMStoreFloat4x4(&perFrameConstants.WorldViewProj, WVP);
		dc->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);
		dc->PSSetShaderResources(0, 1, mCubeMapSRV.GetAddressOf());
		dc->VSSetConstantBuffers(0, 1, mPerFrame.GetAddressOf());
		dc->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		auto previousRasterizerState = ComPtr<D3D11::ID3D11RasterizerState>{};
		dc->RSGetState(&previousRasterizerState);
		auto previousDepthStencilState = ComPtr<D3D11::ID3D11DepthStencilState>{};
		auto previousStencilRef = 0u;
		dc->OMGetDepthStencilState(&previousDepthStencilState, &previousStencilRef);
		dc->RSSetState(mNoCullRS.get());
		dc->OMSetDepthStencilState(mLessEqualDSS.get(), 0);

		auto stride = static_cast<std::uint32_t>(sizeof(DirectX::XMFLOAT3));
		auto offset = 0u;
		dc->IASetVertexBuffers(0, 1, mVB.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mIB.get(), DXGI_FORMAT_R16_UINT, 0);
		dc->IASetInputLayout(mInputLayout.get()); 
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dc->DrawIndexed(mIndexCount, 0, 0);

		dc->RSSetState(previousRasterizerState.get());
		dc->OMSetDepthStencilState(previousDepthStencilState.get(), previousStencilRef);
	}

private:
	void BuildRenderStates(D3D11::ID3D11Device* device)
	{
		auto rasterizerDesc = D3D11::D3D11_RASTERIZER_DESC{
			.FillMode = D3D11::D3D11_FILL_MODE::D3D11_FILL_SOLID,
			.CullMode = D3D11::D3D11_CULL_MODE::D3D11_CULL_NONE,
			.FrontCounterClockwise = false,
			.DepthClipEnable = true
		};
		HR(device->CreateRasterizerState(&rasterizerDesc, &mNoCullRS), "Failed to create sky rasterizer state.");

		auto depthStencilDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL,
			.StencilEnable = false
		};
		HR(device->CreateDepthStencilState(&depthStencilDesc, &mLessEqualDSS), "Failed to create sky depth-stencil state.");
	}

	void BuildShaders(D3D11::ID3D11Device* device)
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/SkyVS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
		auto hr = device->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mVertexShader);
		HR(hr, "Failed to create vertex shader.");
		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/SkyPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
		HR(device->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader), "Failed to create pixel shader.");
		BuildInputLayout(device, vertexShaderBytecode.get());

		// constant buffers basic32
		auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(device->CreateBuffer(&perFrameCbd, 0, &mPerFrame), "Failed to create constant buffer.");

		// samplers
		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 4,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(device->CreateSamplerState(&samplerDesc, &mSamplerState), "Failed to create sampler state.");
	}

	void BuildInputLayout(D3D11::ID3D11Device* device, D3D::ID3DBlob* vertexShaderBytecode)
	{
		auto skyVertex = std::array{
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};
		HR(device->CreateInputLayout(
			skyVertex.data(),
			static_cast<std::uint32_t>(skyVertex.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mInputLayout),
			"Failed to create sky input layout.");
	}

	void BuildGeometryBuffers(D3D11::ID3D11Device* device, float skySphereRadius)
	{
		auto sphere = GeometryGenerator::MeshData{};
		auto geoGen = GeometryGenerator{};
		geoGen.CreateSphere(skySphereRadius, 30, 30, sphere);

		auto vertices = std::vector<DirectX::XMFLOAT3>(sphere.Vertices.size());
		for (auto i = 0ull; i < sphere.Vertices.size(); ++i)
			vertices[i] = sphere.Vertices[i].Position;
		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(DirectX::XMFLOAT3) * vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
		HR(device->CreateBuffer(&vbd, &vinitData, &mVB));

		mIndexCount = static_cast<std::uint32_t>(sphere.Indices.size());
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint16_t) * mIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		auto indices16 = std::vector<std::uint16_t>{};
		indices16.assign(sphere.Indices.begin(), sphere.Indices.end());
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &indices16[0] };

		HR(device->CreateBuffer(&ibd, &iinitData, &mIB));
	}

	ComPtr<D3D11::ID3D11Buffer> mVB;
	ComPtr<D3D11::ID3D11Buffer> mIB;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11ShaderResourceView> mCubeMapSRV;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;
	ComPtr<D3D11::ID3D11RasterizerState> mNoCullRS;
	ComPtr<D3D11::ID3D11DepthStencilState> mLessEqualDSS;
	std::uint32_t mIndexCount;
};