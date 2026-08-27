export module particlesystemdemo:particlesystem;
import std;
import shared;

class ParticleSystem
{
public:
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

	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		std::uint32_t Type;
	};

	ParticleSystem(const ParticleSystem& rhs) = delete;
	ParticleSystem& operator=(const ParticleSystem& rhs) = delete;

	ParticleSystem(
		D3D11::ID3D11Device* device,
		ComPtr<D3D11::ID3D11ShaderResourceView>& texArraySRV,
		ComPtr<D3D11::ID3D11ShaderResourceView>& randomTexSRV,
		std::uint32_t maxParticles,
		const std::wstring& vertexShaderFile,
		const std::wstring& pixelShaderFile,
		std::optional<std::wstring> geometryShaderFile
	)
	{
		Init(device, texArraySRV, randomTexSRV, maxParticles, vertexShaderFile, pixelShaderFile, geometryShaderFile);
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

		//
		// Set constants.
		//
		/*mFX->SetViewProj(VP);
		mFX->SetGameTime(mGameTime);
		mFX->SetTimeStep(mTimeStep);
		mFX->SetEyePosW(mEyePosW);
		mFX->SetEmitPosW(mEmitPosW);
		mFX->SetEmitDirW(mEmitDirW);
		mFX->SetTexArray(mTexArraySRV);
		mFX->SetRandomTex(mRandomTexSRV);*/
		auto pfc = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gEmitPosW = mEmitPosW,
			.gEmitDirW = mEmitDirW,
			.gGameTime = mGameTime,
			.gTimeStep = mTimeStep,
		};
		DirectX::XMStoreFloat4x4(&pfc.gViewProj, VP);

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
			dc->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
		else
			dc->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

		//
		// Draw the current particle list using stream-out only to update them.  
		// The updated vertices are streamed-out to the target VB. 
		//
		dc->SOSetTargets(1, &mStreamOutVB, &offset);

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
		dc->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);
		dc->DrawAuto();
	}

private:
	void Init(
		D3D11::ID3D11Device* device,
		ComPtr<D3D11::ID3D11ShaderResourceView>& texArraySRV,
		ComPtr<D3D11::ID3D11ShaderResourceView>& randomTexSRV,
		std::uint32_t maxParticles,
		const std::wstring& vertexShaderFile,
		const std::wstring& pixelShaderFile,
		std::optional<std::wstring> geometryShaderFile
	)
	{
		mMaxParticles = maxParticles;
		mTexArraySRV = texArraySRV;
		mRandomTexSRV = randomTexSRV;

		BuildVB(device);
		BuildShaders(device, vertexShaderFile, pixelShaderFile, geometryShaderFile);
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
		std::optional<std::wstring> vertexShaderFile, 
		std::optional<std::wstring> pixelShaderFile, 
		std::optional<std::wstring> geometryShaderFile
	)
	{
		// Vertex shader
		auto vsBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/TerrainVS.cso", &vsBytecode), "Failed to read vertex shader file.");
			auto hr = device->CreateVertexShader(
				vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), 0, &mVertexShader);
			HR(hr, "Failed to create vertex shader.");
		}

		// Pixel shader
		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/TerrainPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			auto hr = device->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader);
			HR(hr, "Failed to create pixel shader.");
		}

		BuildInputLayout(device, vsBytecode.get());

		// Create per frame constant buffer
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

	void BuildInputLayout(D3D11::ID3D11Device* device, D3D::ID3DBlob* vsBytecode)
	{
		auto inputDesc = std::array<D3D11::D3D11_INPUT_ELEMENT_DESC, 6>{
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

		HR(device->CreateInputLayout(inputDesc.data(), static_cast<std::uint32_t>(inputDesc.size()), vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &mInputLayout), "Failed to create input layout.");
	}

private:
	std::uint32_t mMaxParticles = 0;
	bool mFirstRun = true;

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
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;

	ComPtr<D3D11::ID3D11ShaderResourceView> mTexArraySRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mRandomTexSRV;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
};
