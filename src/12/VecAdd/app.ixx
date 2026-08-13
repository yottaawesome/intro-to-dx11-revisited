export module vecadd:app;
import std;
import shared;
import :renderstates;

struct Data
{
	DirectX::XMFLOAT3 v1;
	DirectX::XMFLOAT2 v2;
};

// Basic 32-byte vertex structure.
struct Basic32
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

export class VecAddApp : public D3DApp
{
public:
	VecAddApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"Compute Shader Vec Add Demo";
		Init();
	}

	void Init()
	{
		D3DApp::Init();

		BuildShaders();
		mRenderStates = { md3dDevice.get() };
		BuildBuffersAndViews();
	}

	void OnResize()
	{
		D3DApp::OnResize();
	}

	void UpdateScene(float dt) {}
	void DrawScene()
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void DoComputeWork()
	{
		auto inputSRVs = std::array{ mInputASRV.get(), mInputBSRV.get() };
		auto outputUAVs = std::array{ mOutputUAV.get() };

		md3dImmediateContext->CSSetShader(mVecAddCS.get(), nullptr, 0);
		md3dImmediateContext->CSSetShaderResources(
			0,
			static_cast<std::uint32_t>(inputSRVs.size()),
			inputSRVs.data());
		md3dImmediateContext->CSSetUnorderedAccessViews(
			0,
			static_cast<std::uint32_t>(outputUAVs.size()),
			outputUAVs.data(),
			nullptr);

		constexpr auto threadsPerGroup = 32u;
		md3dImmediateContext->Dispatch((mNumElements + threadsPerGroup - 1) / threadsPerGroup, 1, 1);

		auto nullSRVs = std::array<D3D11::ID3D11ShaderResourceView*, 2>{};
		md3dImmediateContext->CSSetShaderResources(0, static_cast<std::uint32_t>(nullSRVs.size()), nullSRVs.data());

		// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
		// and a resource cannot be both an output and input at the same time.
		auto nullUAVs = std::array<D3D11::ID3D11UnorderedAccessView*, 1>{};
		md3dImmediateContext->CSSetUnorderedAccessViews(0, static_cast<std::uint32_t>(nullUAVs.size()), nullUAVs.data(), nullptr);

		// Disable compute shader.
		md3dImmediateContext->CSSetShader(nullptr, nullptr, 0);

		auto fout = std::ofstream{ "results.txt" };
		if (not fout)
			throw std::runtime_error{"Failed to open results.txt for writing."};

		// Copy the output buffer to system memory.
		md3dImmediateContext->CopyResource(mOutputDebugBuffer.get(), mOutputBuffer.get());

		// Map the data for reading.
		auto mappedData = D3D11::D3D11_MAPPED_SUBRESOURCE{};
		HR(md3dImmediateContext->Map(mOutputDebugBuffer.get(), 0, D3D11_MAP_READ, 0, &mappedData));

		auto dataView = static_cast<const Data*>(mappedData.pData);
		for (auto i = 0u; i < mNumElements; ++i)
		{
			fout << std::format(
				"({:.1f}, {:.1f}, {:.1f}, {:.1f}, {:.1f})\n", 
				dataView[i].v1.x, dataView[i].v1.y, dataView[i].v1.z, dataView[i].v2.x, dataView[i].v2.y);
		}

		md3dImmediateContext->Unmap(mOutputDebugBuffer.get(), 0);

		fout.close();
	}

private:
	void BuildBuffersAndViews()
	{
		auto dataA = std::vector<Data>(mNumElements);
		auto dataB = std::vector<Data>(mNumElements);
		for (auto i = 0u; i < mNumElements; ++i)
		{
			dataA[i].v1 = DirectX::XMFLOAT3(static_cast<float>(i), static_cast<float>(i), static_cast<float>(i));
			dataA[i].v2 = DirectX::XMFLOAT2(static_cast<float>(i), 0.0f);
			dataB[i].v1 = DirectX::XMFLOAT3(-static_cast<float>(i), static_cast<float>(i), 0.0f);
			dataB[i].v2 = DirectX::XMFLOAT2(0.0f, -static_cast<float>(i));
		}

		// Create a buffer to be bound as a shader input (D3D11_BIND_SHADER_RESOURCE).
		auto inputDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Data) * mNumElements),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = 0,
			.MiscFlags = D3D11::D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			.StructureByteStride = sizeof(Data),
		};

		auto vinitDataA = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &dataA[0]};
		auto bufferA = ComPtr<D3D11::ID3D11Buffer>{};
		HR(md3dDevice->CreateBuffer(&inputDesc, &vinitDataA, &bufferA));

		auto vinitDataB = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &dataB[0]};
		auto bufferB = ComPtr<D3D11::ID3D11Buffer>{};
		HR(md3dDevice->CreateBuffer(&inputDesc, &vinitDataB, &bufferB));

		// Create a read-write buffer the compute shader can write to (D3D11_BIND_UNORDERED_ACCESS).
		auto outputDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Data) * mNumElements),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS,
			.CPUAccessFlags = 0,
			.MiscFlags = D3D11::D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			.StructureByteStride = sizeof(Data),
		};
		HR(md3dDevice->CreateBuffer(&outputDesc, 0, &mOutputBuffer));

		// Create a system memory version of the buffer to read the results back from.
		outputDesc.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_STAGING;
		outputDesc.BindFlags = 0;
		outputDesc.CPUAccessFlags = D3D11::D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ;
		HR(md3dDevice->CreateBuffer(&outputDesc, 0, &mOutputDebugBuffer));

		auto srvDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFEREX,
			.BufferEx = {
				.FirstElement = 0,
				.NumElements = mNumElements,
				.Flags = 0,
			},
		};
		HR(md3dDevice->CreateShaderResourceView(bufferA.get(), &srvDesc, &mInputASRV));
		HR(md3dDevice->CreateShaderResourceView(bufferB.get(), &srvDesc, &mInputBSRV));

		auto uavDesc = D3D11::D3D11_UNORDERED_ACCESS_VIEW_DESC{
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D11::D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.FirstElement = 0,
				.NumElements = mNumElements,
				.Flags = 0,
			},
		};
		HR(md3dDevice->CreateUnorderedAccessView(mOutputBuffer.get(), &uavDesc, &mOutputUAV));
		// Views hold references to buffers, so we can release these.
	}

	void BuildShaders()
	{
		//
		// Compute Shader
		{
			auto vecAddBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/VecAdd.cso", &vecAddBytecode), "Failed to read compute shader file.");
			HR(md3dDevice->CreateComputeShader(
				vecAddBytecode->GetBufferPointer(),
				vecAddBytecode->GetBufferSize(),
				nullptr,
				&mVecAddCS),
				"Failed to create vector addition compute shader.");
		}
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mOutputBuffer;
	ComPtr<D3D11::ID3D11Buffer> mOutputDebugBuffer;
	ComPtr<D3D11::ID3D11ShaderResourceView> mInputASRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mInputBSRV;
	ComPtr<D3D11::ID3D11UnorderedAccessView> mOutputUAV;
	ComPtr<D3D11::ID3D11ComputeShader> mVecAddCS;
	RenderStates mRenderStates;
	std::uint32_t mNumElements = 32;
};
