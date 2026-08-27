export module particlesystemdemo:particlesystem;
import std;
import shared;

class ParticleSystem
{
public:
	enum class BlendMode
	{
		Opaque,
		Additive
	};

	struct PerFrameConstants
	{
		DirectX::XMFLOAT3 gEyePosW;
		float PadA;
		DirectX::XMFLOAT3 gEmitPosW;
		float PadB;
		DirectX::XMFLOAT3 gEmitDirW;
		float PadC;
		float gGameTime;
		float gTimeStep;
		DirectX::XMFLOAT2 PadD;
		DirectX::XMFLOAT4X4 gViewProj;
	};
	static_assert(sizeof(PerFrameConstants) == 128);

	struct Vertex
	{
		DirectX::XMFLOAT3 InitialPos;
		DirectX::XMFLOAT3 InitialVel;
		DirectX::XMFLOAT2 Size;
		float Age;
		std::uint32_t Type;
	};
	static_assert(sizeof(Vertex) == 40);

	ParticleSystem(const ParticleSystem& rhs) = delete;
	ParticleSystem& operator=(const ParticleSystem& rhs) = delete;

	ParticleSystem(
		D3D11::ID3D11Device* device,
		const ComPtr<D3D11::ID3D11ShaderResourceView>& texArraySRV,
		const ComPtr<D3D11::ID3D11ShaderResourceView>& randomTexSRV,
		std::uint32_t maxParticles,
		const std::filesystem::path& streamOutVertexShaderFile,
		const std::filesystem::path& streamOutGeometryShaderFile,
		const std::filesystem::path& drawVertexShaderFile,
		const std::filesystem::path& drawGeometryShaderFile,
		const std::filesystem::path& drawPixelShaderFile,
		BlendMode blendMode
	)
	{
		Init(
			device,
			texArraySRV,
			randomTexSRV,
			maxParticles,
			streamOutVertexShaderFile,
			streamOutGeometryShaderFile,
			drawVertexShaderFile,
			drawGeometryShaderFile,
			drawPixelShaderFile,
			blendMode);
	}

	// Time elapsed since the system was reset.
	auto GetAge()const -> float
	{
		return mAge;
	}

	void SetEyePos(const DirectX::XMFLOAT3& eyePosW)
	{
		mEyePosW = eyePosW;
	}

	void SetEmitPos(const DirectX::XMFLOAT3& emitPosW)
	{
		mEmitPosW = emitPosW;
	}

	void SetEmitDir(const DirectX::XMFLOAT3& emitDirW)
	{
		mEmitDirW = emitDirW;
	}

	void Reset()
	{
		mFirstRun = true;
		mAge = 0.0f;
	}

	void Update(float dt, float gameTime)
	{
		mGameTime = gameTime;
		mTimeStep = dt;

		mAge += dt;
	}

	void Draw(D3D11::ID3D11DeviceContext* dc, const Camera& cam)
	{
		DirectX::XMMATRIX VP = cam.ViewProj();

		auto previousDepthStencilState = ComPtr<D3D11::ID3D11DepthStencilState>{};
		auto previousStencilRef = 0u;
		dc->OMGetDepthStencilState(&previousDepthStencilState, &previousStencilRef);

		auto previousBlendState = ComPtr<D3D11::ID3D11BlendState>{};
		auto previousBlendFactor = std::array<float, 4>{};
		auto previousSampleMask = 0u;
		dc->OMGetBlendState(&previousBlendState, previousBlendFactor.data(), &previousSampleMask);

		//
		// Set constants.
		//
		auto pfc = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gEmitPosW = mEmitPosW,
			.gEmitDirW = mEmitDirW,
			.gGameTime = mGameTime,
			.gTimeStep = mTimeStep,
		};
		DirectX::XMStoreFloat4x4(&pfc.gViewProj, VP);
		dc->UpdateSubresource(mPerFrameConstants.get(), 0, nullptr, &pfc, 0, 0);

		//
		// Set IA stage.
		//
		dc->IASetInputLayout(mInputLayout.get());
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

		auto stride = static_cast<std::uint32_t>(sizeof(Vertex));
		auto offset = 0u;

		// On the first pass, use the initialization VB.  Otherwise, use
		// the VB that contains the current particle list.
		if (mFirstRun)
			dc->IASetVertexBuffers(0, 1, mInitVB.GetAddressOf(), &stride, &offset);
		else
			dc->IASetVertexBuffers(0, 1, mDrawVB.GetAddressOf(), &stride, &offset);

		//
		// Draw the current particle list using stream-out only to update them.  
		// The updated vertices are streamed-out to the target VB. 
		//
		dc->VSSetShader(mStreamOutVertexShader.get(), nullptr, 0);
		dc->GSSetShader(mStreamOutGeometryShader.get(), nullptr, 0);
		dc->PSSetShader(nullptr, nullptr, 0);
		dc->GSSetConstantBuffers(0, 1, mPerFrameConstants.GetAddressOf());
		dc->GSSetShaderResources(1, 1, mRandomTexSRV.GetAddressOf());
		dc->GSSetSamplers(0, 1, mLinearSampler.GetAddressOf());
		dc->OMSetDepthStencilState(mDisableDepthDSS.get(), 0);
		dc->SOSetTargets(1, mStreamOutVB.GetAddressOf(), &offset);

		if (mFirstRun)
		{
			dc->Draw(1, 0);
			mFirstRun = false;
		}
		else
		{
			dc->DrawAuto();
		}

		// done streaming-out--unbind the vertex buffer
		D3D11::ID3D11Buffer* bufferArray[1] = { 0 };
		dc->SOSetTargets(1, bufferArray, &offset);

		// ping-pong the vertex buffers
		std::swap(mDrawVB, mStreamOutVB);

		//
		// Draw the updated particle system we just streamed-out. 
		//
		dc->IASetVertexBuffers(0, 1, mDrawVB.GetAddressOf(), &stride, &offset);

		dc->VSSetShader(mDrawVertexShader.get(), nullptr, 0);
		dc->GSSetShader(mDrawGeometryShader.get(), nullptr, 0);
		dc->PSSetShader(mDrawPixelShader.get(), nullptr, 0);
		dc->VSSetConstantBuffers(0, 1, mPerFrameConstants.GetAddressOf());
		dc->GSSetConstantBuffers(0, 1, mPerFrameConstants.GetAddressOf());
		dc->PSSetConstantBuffers(0, 1, mPerFrameConstants.GetAddressOf());
		dc->PSSetShaderResources(0, 1, mTexArraySRV.GetAddressOf());
		dc->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());
		dc->OMSetDepthStencilState(mNoDepthWritesDSS.get(), 0);

		auto blendFactor = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };
		auto blendState = mBlendMode == BlendMode::Additive ? mAdditiveBlendState.get() : nullptr;
		dc->OMSetBlendState(blendState, blendFactor.data(), std::numeric_limits<std::uint32_t>::max());
		dc->DrawAuto();

		dc->OMSetDepthStencilState(previousDepthStencilState.get(), previousStencilRef);
		dc->OMSetBlendState(previousBlendState.get(), previousBlendFactor.data(), previousSampleMask);
	}

private:
	void Init(
		D3D11::ID3D11Device* device,
		const ComPtr<D3D11::ID3D11ShaderResourceView>& texArraySRV,
		const ComPtr<D3D11::ID3D11ShaderResourceView>& randomTexSRV,
		std::uint32_t maxParticles,
		const std::filesystem::path& streamOutVertexShaderFile,
		const std::filesystem::path& streamOutGeometryShaderFile,
		const std::filesystem::path& drawVertexShaderFile,
		const std::filesystem::path& drawGeometryShaderFile,
		const std::filesystem::path& drawPixelShaderFile,
		BlendMode blendMode
	)
	{
		mMaxParticles = maxParticles;
		mTexArraySRV = texArraySRV;
		mRandomTexSRV = randomTexSRV;
		mBlendMode = blendMode;

		BuildVB(device);
		BuildShaders(
			device,
			streamOutVertexShaderFile,
			streamOutGeometryShaderFile,
			drawVertexShaderFile,
			drawGeometryShaderFile,
			drawPixelShaderFile);
		BuildRenderStates(device);
	}

	void BuildVB(D3D11::ID3D11Device* device)
	{
		//
		// Create the buffer to kick-off the particle system.
		//
		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Vertex) * 1,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		
		// The initial particle emitter has type 0 and age 0.  The rest
		// of the particle attributes do not apply to an emitter.
		Vertex p{};

		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &p };

		HR(device->CreateBuffer(&vbd, &vinitData, &mInitVB));

		//
		// Create the ping-pong buffers for stream-out and drawing.
		//
		vbd.ByteWidth = sizeof(Vertex) * mMaxParticles;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

		HR(device->CreateBuffer(&vbd, 0, &mDrawVB));
		HR(device->CreateBuffer(&vbd, 0, &mStreamOutVB));
	}

	void BuildShaders(
		D3D11::ID3D11Device* device,
		const std::filesystem::path& streamOutVertexShaderFile,
		const std::filesystem::path& streamOutGeometryShaderFile,
		const std::filesystem::path& drawVertexShaderFile,
		const std::filesystem::path& drawGeometryShaderFile,
		const std::filesystem::path& drawPixelShaderFile
	)
	{
		auto streamOutVSBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(streamOutVertexShaderFile.wstring().c_str(), &streamOutVSBytecode), "Failed to read stream-out vertex shader file.");
		HR(device->CreateVertexShader(
			streamOutVSBytecode->GetBufferPointer(),
			streamOutVSBytecode->GetBufferSize(),
			nullptr,
			&mStreamOutVertexShader),
			"Failed to create stream-out vertex shader.");

		auto streamOutGSBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(streamOutGeometryShaderFile.wstring().c_str(), &streamOutGSBytecode), "Failed to read stream-out geometry shader file.");
		auto streamOutDeclaration = std::array{
			D3D11::D3D11_SO_DECLARATION_ENTRY{ 0, "POSITION", 0, 0, 3, 0 },
			D3D11::D3D11_SO_DECLARATION_ENTRY{ 0, "VELOCITY", 0, 0, 3, 0 },
			D3D11::D3D11_SO_DECLARATION_ENTRY{ 0, "SIZE", 0, 0, 2, 0 },
			D3D11::D3D11_SO_DECLARATION_ENTRY{ 0, "AGE", 0, 0, 1, 0 },
			D3D11::D3D11_SO_DECLARATION_ENTRY{ 0, "TYPE", 0, 0, 1, 0 }
		};
		auto streamOutStrides = std::array{ static_cast<std::uint32_t>(sizeof(Vertex)) };
		HR(device->CreateGeometryShaderWithStreamOutput(
			streamOutGSBytecode->GetBufferPointer(),
			streamOutGSBytecode->GetBufferSize(),
			streamOutDeclaration.data(),
			static_cast<std::uint32_t>(streamOutDeclaration.size()),
			streamOutStrides.data(),
			static_cast<std::uint32_t>(streamOutStrides.size()),
			D3D11::SoNoRasterizedStream,
			nullptr,
			&mStreamOutGeometryShader),
			"Failed to create stream-out geometry shader.");

		auto drawVSBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(drawVertexShaderFile.wstring().c_str(), &drawVSBytecode), "Failed to read draw vertex shader file.");
		HR(device->CreateVertexShader(
			drawVSBytecode->GetBufferPointer(),
			drawVSBytecode->GetBufferSize(),
			nullptr,
			&mDrawVertexShader),
			"Failed to create draw vertex shader.");

		auto drawGSBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(drawGeometryShaderFile.wstring().c_str(), &drawGSBytecode), "Failed to read draw geometry shader file.");
		HR(device->CreateGeometryShader(
			drawGSBytecode->GetBufferPointer(),
			drawGSBytecode->GetBufferSize(),
			nullptr,
			&mDrawGeometryShader),
			"Failed to create draw geometry shader.");

		auto drawPSBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(drawPixelShaderFile.wstring().c_str(), &drawPSBytecode), "Failed to read draw pixel shader file.");
		HR(device->CreatePixelShader(
			drawPSBytecode->GetBufferPointer(),
			drawPSBytecode->GetBufferSize(),
			nullptr,
			&mDrawPixelShader),
			"Failed to create draw pixel shader.");

		BuildInputLayout(device, streamOutVSBytecode.get());

		auto perFrameConstantsDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(PerFrameConstants)),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		HR(device->CreateBuffer(&perFrameConstantsDesc, nullptr, &mPerFrameConstants), "Failed to create per frame constant buffer.");
	}

	void BuildRenderStates(D3D11::ID3D11Device* device)
	{
		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max()
		};
		HR(device->CreateSamplerState(&samplerDesc, &mLinearSampler), "Failed to create particle sampler state.");

		auto disableDepthDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = false,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS,
			.StencilEnable = false
		};
		HR(device->CreateDepthStencilState(&disableDepthDesc, &mDisableDepthDSS), "Failed to create stream-out depth-stencil state.");

		auto noDepthWritesDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS,
			.StencilEnable = false
		};
		HR(device->CreateDepthStencilState(&noDepthWritesDesc, &mNoDepthWritesDSS), "Failed to create particle draw depth-stencil state.");

		auto additiveBlendDesc = D3D11::D3D11_BLEND_DESC{
			.AlphaToCoverageEnable = false,
			.IndependentBlendEnable = false,
			.RenderTarget = {{
				.BlendEnable = true,
				.SrcBlend = D3D11::D3D11_BLEND::D3D11_BLEND_SRC_ALPHA,
				.DestBlend = D3D11::D3D11_BLEND::D3D11_BLEND_ONE,
				.BlendOp = D3D11::D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11::D3D11_BLEND::D3D11_BLEND_ZERO,
				.DestBlendAlpha = D3D11::D3D11_BLEND::D3D11_BLEND_ZERO,
				.BlendOpAlpha = D3D11::D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11::D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL
			}}
		};
		HR(device->CreateBlendState(&additiveBlendDesc, &mAdditiveBlendState), "Failed to create additive particle blend state.");
	}

	void BuildInputLayout(D3D11::ID3D11Device* device, D3D::ID3DBlob* vsBytecode)
	{
		auto inputDesc = std::array{
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
				.SemanticName = "VELOCITY",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "SIZE",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 24,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "AGE",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 32,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TYPE",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32_UINT,
				.InputSlot = 0,
				.AlignedByteOffset = 36,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};

		HR(device->CreateInputLayout(inputDesc.data(), static_cast<std::uint32_t>(inputDesc.size()), vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &mInputLayout), "Failed to create input layout.");
	}

private:
	std::uint32_t mMaxParticles = 0;
	bool mFirstRun = true;
	BlendMode mBlendMode = BlendMode::Opaque;

	float mGameTime = 0.0f;
	float mTimeStep = 0.0f;
	float mAge = 0.0f;

	DirectX::XMFLOAT3 mEyePosW = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mEmitPosW = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mEmitDirW = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	ComPtr<D3D11::ID3D11Buffer> mInitVB;
	ComPtr<D3D11::ID3D11Buffer> mDrawVB;
	ComPtr<D3D11::ID3D11Buffer> mStreamOutVB;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameConstants;
	ComPtr<D3D11::ID3D11VertexShader> mStreamOutVertexShader;
	ComPtr<D3D11::ID3D11GeometryShader> mStreamOutGeometryShader;
	ComPtr<D3D11::ID3D11VertexShader> mDrawVertexShader;
	ComPtr<D3D11::ID3D11GeometryShader> mDrawGeometryShader;
	ComPtr<D3D11::ID3D11PixelShader> mDrawPixelShader;

	ComPtr<D3D11::ID3D11ShaderResourceView> mTexArraySRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mRandomTexSRV;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11SamplerState> mLinearSampler;
	ComPtr<D3D11::ID3D11DepthStencilState> mDisableDepthDSS;
	ComPtr<D3D11::ID3D11DepthStencilState> mNoDepthWritesDSS;
	ComPtr<D3D11::ID3D11BlendState> mAdditiveBlendState;
};
