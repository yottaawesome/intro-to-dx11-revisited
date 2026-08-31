export module ssaodemo:ssao;
import std;
import shared;
import :sharedvertices;

class Ssao
{
public:
	struct PerFrameConstants
	{
		DirectX::XMFLOAT4X4 gViewToTexSpace;
		DirectX::XMFLOAT4 gOffsetVectors[14];
		DirectX::XMFLOAT4 gFrustumCorners[4];
		float gOcclusionRadius;
		float gOcclusionFadeStart;
		float gOcclusionFadeEnd;
		float gSurfaceEpsilon;
	};
	static_assert(sizeof(PerFrameConstants) == 368);

	struct BlurPerFrameConstants
	{
		float gTexelWidth;
		float gTexelHeight;
		DirectX::XMFLOAT2 gPadding;
	};
	static_assert(sizeof(BlurPerFrameConstants) == 16);

	Ssao(
		ComPtr<D3D11::ID3D11Device>& device, 
		ComPtr<D3D11::ID3D11DeviceContext>& dc, 
		int width, 
		int height, 
		float fovy, 
		float farZ, 
		const ComPtr<D3D11::ID3D11InputLayout>& basic32InputLayout
	) : md3dDevice(device), mDC(dc), mBasic32InputLayout(basic32InputLayout)
	{
		OnSize(width, height, fovy, farZ);

		BuildFullScreenQuad();
		BuildOffsetVectors();
		BuildRandomVectorTexture();
		BuildShaders();
		BuildSamplerStates();
	}

	Ssao(const Ssao& rhs) = delete;
	Ssao& operator=(const Ssao& rhs) = delete;

	auto NormalDepthSRV() ->D3D11::ID3D11ShaderResourceView*
	{
		return mNormalDepthSRV.get();
	}

	auto AmbientSRV() ->D3D11::ID3D11ShaderResourceView*
	{
		return mAmbientSRV0.get();
	}

	///<summary>
	/// Call when the backbuffer is resized.  
	///</summary>
	void OnSize(int width, int height, float fovy, float farZ)
	{
		mRenderTargetWidth = static_cast<std::uint32_t>(std::max(width, 1));
		mRenderTargetHeight = static_cast<std::uint32_t>(std::max(height, 1));
		mAmbientMapWidth = std::max(mRenderTargetWidth / 2, 1u);
		mAmbientMapHeight = std::max(mRenderTargetHeight / 2, 1u);

		mRenderTargetViewport = D3D11::D3D11_VIEWPORT{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(mRenderTargetWidth),
			.Height = static_cast<float>(mRenderTargetHeight),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f,
		};

		// We render to ambient map at half the resolution.
		mAmbientMapViewport = D3D11::D3D11_VIEWPORT{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(mAmbientMapWidth),
			.Height = static_cast<float>(mAmbientMapHeight),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f,
		};

		BuildFrustumFarCorners(fovy, farZ);
		BuildTextureViews();
	}

	///<summary>
	/// Changes the render target to the NormalDepth render target.  Pass the 
	/// main depth buffer as the depth buffer to use when we render to the
	/// NormalDepth map.  This pass lays down the scene depth so that there in
	/// no overdraw in the subsequent rendering pass.
	///</summary>
	void SetNormalDepthRenderTarget(D3D11::ID3D11DepthStencilView* dsv)
	{
		auto nullResources = std::array<D3D11::ID3D11ShaderResourceView*, 16>{};
		mDC->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(nullResources.size()),
			nullResources.data());

		D3D11::ID3D11RenderTargetView* renderTargets[1] = { mNormalDepthRTV.get() };
		mDC->OMSetRenderTargets(1, renderTargets, dsv);
		mDC->RSSetViewports(1, &mRenderTargetViewport);

		// Clear view space normal to (0,0,-1) and clear depth to be very far away.  
		float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
		mDC->ClearRenderTargetView(mNormalDepthRTV.get(), clearColor);
	}

	///<summary>
	/// Changes the render target to the Ambient render target and draws a fullscreen
	/// quad to kick off the pixel shader to compute the AmbientMap.  No depth/stencil
	/// buffer is bound because this fullscreen pass does not need depth testing.
	///</summary>
	void ComputeSsao(const Camera& camera)
	{
		auto nullResources = std::array<D3D11::ID3D11ShaderResourceView*, 16>{};
		mDC->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(nullResources.size()),
			nullResources.data());

		// Bind the ambient map as the render target.  Observe that this pass does not bind 
		// a depth/stencil buffer--it does not need it, and without one, no depth test is
		// performed, which is what we want.
		D3D11::ID3D11RenderTargetView* renderTargets[1] = { mAmbientRTV0.get() };
		mDC->OMSetRenderTargets(1, renderTargets, 0);
		mDC->ClearRenderTargetView(mAmbientRTV0.get(), reinterpret_cast<const float*>(&DirectX::Colors::Black));
		mDC->RSSetViewports(1, &mAmbientMapViewport);

		// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
		static const DirectX::XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		DirectX::XMMATRIX P = camera.Proj();
		DirectX::XMMATRIX PT = DirectX::XMMatrixMultiply(P, T);

		auto perFrameConstants = PerFrameConstants{
			.gOcclusionRadius = 0.5f,
			.gOcclusionFadeStart = 0.2f,
			.gOcclusionFadeEnd = 2.0f,
			.gSurfaceEpsilon = 0.05f,
		};
		DirectX::XMStoreFloat4x4(&perFrameConstants.gViewToTexSpace, PT);
		std::copy(std::begin(mOffsets), std::end(mOffsets), std::begin(perFrameConstants.gOffsetVectors));
		std::copy(std::begin(mFrustumFarCorner), std::end(mFrustumFarCorner), std::begin(perFrameConstants.gFrustumCorners));
		mDC->UpdateSubresource(mPerFrameCB.get(), 0, nullptr, &perFrameConstants, 0, 0);

		mDC->VSSetShader(mSsaoVertexShader.get(), nullptr, 0);
		mDC->HSSetShader(nullptr, nullptr, 0);
		mDC->DSSetShader(nullptr, nullptr, 0);
		mDC->GSSetShader(nullptr, nullptr, 0);
		mDC->PSSetShader(mSsaoPixelShader.get(), nullptr, 0);

		mDC->VSSetConstantBuffers(0, 1, mPerFrameCB.GetAddressOf());
		mDC->PSSetConstantBuffers(0, 1, mPerFrameCB.GetAddressOf());

		auto resources = std::array{
			mNormalDepthSRV.get(),
			mRandomVectorSRV.get(),
		};
		mDC->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(resources.size()),
			resources.data());

		auto samplers = std::array{
			mNormalDepthSampler.get(),
			mRandomVectorSampler.get(),
		};
		mDC->PSSetSamplers(
			0,
			static_cast<std::uint32_t>(samplers.size()),
			samplers.data());

		UINT stride = sizeof(Vertices::Basic32);
		UINT offset = 0;

		mDC->IASetInputLayout(mBasic32InputLayout.get());
		mDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mDC->IASetVertexBuffers(0, 1, mScreenQuadVB.GetAddressOf(), &stride, &offset);
		mDC->IASetIndexBuffer(mScreenQuadIB.get(), DXGI_FORMAT_R16_UINT, 0);

		mDC->DrawIndexed(6, 0, 0);

		auto nullPassResources = std::array<D3D11::ID3D11ShaderResourceView*, 2>{};
		mDC->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(nullPassResources.size()),
			nullPassResources.data());

		mDC->OMSetRenderTargets(0, nullptr, nullptr);
	}

	///<summary>
	/// Blurs the ambient map to smooth out the noise caused by only taking a
	/// few random samples per pixel.  We use an edge preserving blur so that 
	/// we do not blur across discontinuities--we want edges to remain edges.
	///</summary>
	void BlurAmbientMap(int blurCount)
	{
		for (int i = 0; i < blurCount; ++i)
		{
			// Ping-pong the two ambient map textures as we apply
			// horizontal and vertical blur passes.
			BlurAmbientMap(mAmbientSRV0.get(), mAmbientRTV1.get(), true);
			BlurAmbientMap(mAmbientSRV1.get(), mAmbientRTV0.get(), false);
		}

		mDC->OMSetRenderTargets(0, nullptr, nullptr);
	}

private:
	void BlurAmbientMap(D3D11::ID3D11ShaderResourceView* inputSRV, D3D11::ID3D11RenderTargetView* outputRTV, bool horzBlur)
	{
		D3D11::ID3D11RenderTargetView* renderTargets[1] = { outputRTV };
		mDC->OMSetRenderTargets(1, renderTargets, 0);
		mDC->ClearRenderTargetView(outputRTV, reinterpret_cast<const float*>(&DirectX::Colors::Black));
		mDC->RSSetViewports(1, &mAmbientMapViewport);

		auto perFrameConstants = BlurPerFrameConstants{
			.gTexelWidth = 1.0f / static_cast<float>(mAmbientMapWidth),
			.gTexelHeight = 1.0f / static_cast<float>(mAmbientMapHeight),
			.gPadding = { 0.0f, 0.0f },
		};
		mDC->UpdateSubresource(
			mBlurPerFrameCB.get(), 0, nullptr, &perFrameConstants, 0, 0);

		mDC->VSSetShader(mBlurVertexShader.get(), nullptr, 0);
		mDC->HSSetShader(nullptr, nullptr, 0);
		mDC->DSSetShader(nullptr, nullptr, 0);
		mDC->GSSetShader(nullptr, nullptr, 0);
		mDC->PSSetShader(
			horzBlur ? mHorizontalBlurPixelShader.get() : mVerticalBlurPixelShader.get(),
			nullptr,
			0);
		mDC->PSSetConstantBuffers(0, 1, mBlurPerFrameCB.GetAddressOf());

		auto resources = std::array{
			mNormalDepthSRV.get(),
			inputSRV,
		};
		mDC->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(resources.size()),
			resources.data());

		auto samplers = std::array{
			mBlurSampler.get(),
			mBlurSampler.get(),
		};
		mDC->PSSetSamplers(
			0,
			static_cast<std::uint32_t>(samplers.size()),
			samplers.data());

		UINT stride = sizeof(Vertices::Basic32);
		UINT offset = 0;

		mDC->IASetInputLayout(mBasic32InputLayout.get());
		mDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mDC->IASetVertexBuffers(0, 1, mScreenQuadVB.GetAddressOf(), &stride, &offset);
		mDC->IASetIndexBuffer(mScreenQuadIB.get(), DXGI_FORMAT_R16_UINT, 0);

		mDC->DrawIndexed(6, 0, 0);

		auto nullResources = std::array<D3D11::ID3D11ShaderResourceView*, 2>{};
		mDC->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(nullResources.size()),
			nullResources.data());
	}

	void BuildFrustumFarCorners(float fovy, float farZ)
	{
		float aspect = (float)mRenderTargetWidth / (float)mRenderTargetHeight;

		float halfHeight = farZ * std::tanf(0.5f * fovy);
		float halfWidth = aspect * halfHeight;

		mFrustumFarCorner[0] = DirectX::XMFLOAT4(-halfWidth, -halfHeight, farZ, 0.0f);
		mFrustumFarCorner[1] = DirectX::XMFLOAT4(-halfWidth, +halfHeight, farZ, 0.0f);
		mFrustumFarCorner[2] = DirectX::XMFLOAT4(+halfWidth, +halfHeight, farZ, 0.0f);
		mFrustumFarCorner[3] = DirectX::XMFLOAT4(+halfWidth, -halfHeight, farZ, 0.0f);
	}

	void BuildFullScreenQuad()
	{
		auto v = std::array<Vertices::Basic32, 4>{};

		v[0].Pos = DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f);
		v[1].Pos = DirectX::XMFLOAT3(-1.0f, +1.0f, 0.0f);
		v[2].Pos = DirectX::XMFLOAT3(+1.0f, +1.0f, 0.0f);
		v[3].Pos = DirectX::XMFLOAT3(+1.0f, -1.0f, 0.0f);

		// Store far plane frustum corner indices in Normal.x slot.
		v[0].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		v[1].Normal = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		v[2].Normal = DirectX::XMFLOAT3(2.0f, 0.0f, 0.0f);
		v[3].Normal = DirectX::XMFLOAT3(3.0f, 0.0f, 0.0f);

		v[0].Tex = DirectX::XMFLOAT2(0.0f, 1.0f);
		v[1].Tex = DirectX::XMFLOAT2(0.0f, 0.0f);
		v[2].Tex = DirectX::XMFLOAT2(1.0f, 0.0f);
		v[3].Tex = DirectX::XMFLOAT2(1.0f, 1.0f);

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Vertices::Basic32) * static_cast<std::uint32_t>(v.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = v.data() };

		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

		auto indices = std::array<std::uint16_t, 6>{
			0, 1, 2,
			0, 2, 3
		};
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(std::uint16_t) * static_cast<std::uint32_t>(indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};

		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = indices.data() };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
	}

	void BuildShaders()
	{
		auto ssaoVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(L"Shaders/SsaoVS.cso", &ssaoVertexShaderBytecode),
			"Failed to read SSAO vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				ssaoVertexShaderBytecode->GetBufferPointer(),
				ssaoVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mSsaoVertexShader),
			"Failed to create SSAO vertex shader.");

		auto ssaoPixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(L"Shaders/SsaoPS.cso", &ssaoPixelShaderBytecode),
			"Failed to read SSAO pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				ssaoPixelShaderBytecode->GetBufferPointer(),
				ssaoPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mSsaoPixelShader),
			"Failed to create SSAO pixel shader.");

		auto blurVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(L"Shaders/SsaoBlurVS.cso", &blurVertexShaderBytecode),
			"Failed to read SSAO blur vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				blurVertexShaderBytecode->GetBufferPointer(),
				blurVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mBlurVertexShader),
			"Failed to create SSAO blur vertex shader.");

		auto horizontalBlurPixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(
				L"Shaders/SsaoBlurHorzPS.cso",
				&horizontalBlurPixelShaderBytecode),
			"Failed to read horizontal SSAO blur pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				horizontalBlurPixelShaderBytecode->GetBufferPointer(),
				horizontalBlurPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mHorizontalBlurPixelShader),
			"Failed to create horizontal SSAO blur pixel shader.");

		auto verticalBlurPixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(
				L"Shaders/SsaoBlurVertPS.cso",
				&verticalBlurPixelShaderBytecode),
			"Failed to read vertical SSAO blur pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				verticalBlurPixelShaderBytecode->GetBufferPointer(),
				verticalBlurPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mVerticalBlurPixelShader),
			"Failed to create vertical SSAO blur pixel shader.");

		auto createConstantBuffer = [this](
			std::uint32_t byteWidth,
			ComPtr<D3D11::ID3D11Buffer>& constantBuffer,
			const char* errorMessage)
		{
			auto bufferDesc = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = byteWidth,
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&bufferDesc, nullptr, &constantBuffer), errorMessage);
		};

		createConstantBuffer(
			sizeof(PerFrameConstants),
			mPerFrameCB,
			"Failed to create SSAO constant buffer.");
		createConstantBuffer(
			sizeof(BlurPerFrameConstants),
			mBlurPerFrameCB,
			"Failed to create SSAO blur constant buffer.");
	}

	void BuildSamplerStates()
	{
		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS,
			.BorderColor = { 0.0f, 0.0f, 0.0f, 1e5f },
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(
			md3dDevice->CreateSamplerState(&samplerDesc, &mNormalDepthSampler),
			"Failed to create SSAO normal-depth sampler state.");

		samplerDesc.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.BorderColor[3] = 0.0f;
		HR(
			md3dDevice->CreateSamplerState(&samplerDesc, &mRandomVectorSampler),
			"Failed to create SSAO random-vector sampler state.");

		samplerDesc.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
		HR(
			md3dDevice->CreateSamplerState(&samplerDesc, &mBlurSampler),
			"Failed to create SSAO blur sampler state.");
	}

	void BuildTextureViews()
	{
		ReleaseTextureViews();

		auto texDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = mRenderTargetWidth,
			.Height = mRenderTargetHeight,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT,
			.SampleDesc = { 1, 0 },
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG{D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET},
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};

		// view saves a reference.
		auto normalDepthTex = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &normalDepthTex));
		HR(md3dDevice->CreateShaderResourceView(normalDepthTex.get(), 0, &mNormalDepthSRV));
		HR(md3dDevice->CreateRenderTargetView(normalDepthTex.get(), 0, &mNormalDepthRTV));

		// Render ambient map at half resolution.
		texDesc.Width = mAmbientMapWidth;
		texDesc.Height = mAmbientMapHeight;
		texDesc.Format = DXGI_FORMAT_R16_FLOAT;
		// view saves a reference.
		auto ambientTex0 = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &ambientTex0));
		HR(md3dDevice->CreateShaderResourceView(ambientTex0.get(), 0, &mAmbientSRV0));
		HR(md3dDevice->CreateRenderTargetView(ambientTex0.get(), 0, &mAmbientRTV0));

		// view saves a reference.
		auto ambientTex1 = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &ambientTex1));
		HR(md3dDevice->CreateShaderResourceView(ambientTex1.get(), 0, &mAmbientSRV1));
		HR(md3dDevice->CreateRenderTargetView(ambientTex1.get(), 0, &mAmbientRTV1));
	}

	void ReleaseTextureViews()
	{
		mNormalDepthRTV.reset();
		mNormalDepthSRV.reset();
		mAmbientRTV0.reset();
		mAmbientSRV0.reset();
		mAmbientRTV1.reset();
		mAmbientSRV1.reset();
	}

	void BuildRandomVectorTexture()
	{
		auto texDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = 256,
			.Height = 256,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = { 1, 0 },
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto initData = D3D11::D3D11_SUBRESOURCE_DATA{ .SysMemPitch = 256 * sizeof(DirectX::PackedVector::XMCOLOR) };

		auto color = std::array<DirectX::PackedVector::XMCOLOR, 256 * 256>{};
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				auto v = DirectX::XMFLOAT3(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());
				color[i * 256 + j] = DirectX::PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
			}
		}

		initData.pSysMem = color.data();

		// view saves a reference.
		auto tex = ComPtr<D3D11::ID3D11Texture2D>();
		HR(md3dDevice->CreateTexture2D(&texDesc, &initData, &tex));

		HR(md3dDevice->CreateShaderResourceView(tex.get(), 0, &mRandomVectorSRV));
	}

	void BuildOffsetVectors()
	{
		// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
		// and the 6 center points along each cube face.  We always alternate the points on 
		// opposites sides of the cubes.  This way we still get the vectors spread out even
		// if we choose to use less than 14 samples.

		// 8 cube corners
		mOffsets[0] = DirectX::XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
		mOffsets[1] = DirectX::XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

		mOffsets[2] = DirectX::XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
		mOffsets[3] = DirectX::XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

		mOffsets[4] = DirectX::XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
		mOffsets[5] = DirectX::XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

		mOffsets[6] = DirectX::XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
		mOffsets[7] = DirectX::XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

		// 6 centers of cube faces
		mOffsets[8] = DirectX::XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
		mOffsets[9] = DirectX::XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

		mOffsets[10] = DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
		mOffsets[11] = DirectX::XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

		mOffsets[12] = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
		mOffsets[13] = DirectX::XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

		for (int i = 0; i < 14; ++i)
		{
			// Create random lengths in [0.25, 1.0].
			float s = MathHelper::RandF(0.25f, 1.0f);
			DirectX::XMVECTOR v = s * DirectX::XMVector4Normalize(DirectX::XMLoadFloat4(&mOffsets[i]));
			DirectX::XMStoreFloat4(&mOffsets[i], v);
		}
	}


private:
	ComPtr<D3D11::ID3D11Device> md3dDevice;
	ComPtr<D3D11::ID3D11DeviceContext> mDC;

	ComPtr<D3D11::ID3D11Buffer> mScreenQuadVB;
	ComPtr<D3D11::ID3D11Buffer> mScreenQuadIB;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mBlurPerFrameCB;

	ComPtr<D3D11::ID3D11VertexShader> mSsaoVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mSsaoPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mBlurVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mHorizontalBlurPixelShader;
	ComPtr<D3D11::ID3D11PixelShader> mVerticalBlurPixelShader;

	ComPtr<D3D11::ID3D11SamplerState> mNormalDepthSampler;
	ComPtr<D3D11::ID3D11SamplerState> mRandomVectorSampler;
	ComPtr<D3D11::ID3D11SamplerState> mBlurSampler;

	ComPtr<D3D11::ID3D11ShaderResourceView> mRandomVectorSRV;

	ComPtr<D3D11::ID3D11RenderTargetView> mNormalDepthRTV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mNormalDepthSRV;

	// Need two for ping-ponging during blur.
	ComPtr<D3D11::ID3D11RenderTargetView> mAmbientRTV0;
	ComPtr<D3D11::ID3D11ShaderResourceView> mAmbientSRV0;

	ComPtr<D3D11::ID3D11RenderTargetView> mAmbientRTV1;
	ComPtr<D3D11::ID3D11ShaderResourceView> mAmbientSRV1;

	ComPtr<D3D11::ID3D11InputLayout> mBasic32InputLayout;


	std::uint32_t mRenderTargetWidth = 1;
	std::uint32_t mRenderTargetHeight = 1;
	std::uint32_t mAmbientMapWidth = 1;
	std::uint32_t mAmbientMapHeight = 1;

	DirectX::XMFLOAT4 mFrustumFarCorner[4]{};

	DirectX::XMFLOAT4 mOffsets[14]{};

	D3D11::D3D11_VIEWPORT mRenderTargetViewport{};
	D3D11::D3D11_VIEWPORT mAmbientMapViewport{};
};