export module terraindemo:terrain;
import std;
import shared;

export class Terrain
{
	struct PerFrameConstants
	{
		DirectionalLight gDirLights[3];

		DirectX::XMFLOAT3 gEyePosW;
		int gLightCount;

		bool32 gFogEnabled;
		float gFogStart;
		float gFogRange;
		// When distance is minimum, the tessellation is maximum.
		// When distance is maximum, the tessellation is minimum.
		float gMinDist;

		float gMaxDist;
		// Exponents for power of 2 tessellation.  The tessellation
		// range is [2^(gMinTess), 2^(gMaxTess)].  Since the maximum
		// tessellation is 64, this means gMaxTess can be at most 6
		// since 2^6 = 64.
		float gMinTess;
		float gMaxTess;
		float gTexelCellSpaceU;

		DirectX::XMFLOAT4 gFogColor;

		float gTexelCellSpaceV;
		float gWorldCellSpace;
		DirectX::XMFLOAT2 padding1;

		DirectX::XMFLOAT2 gTexScale;
		DirectX::XMFLOAT2 padding2;
		DirectX::XMFLOAT4 gWorldFrustumPlanes[6];
	};

	struct PerObjectConstants
	{
		// Terrain coordinate specified directly 
		// at center of world space.

		DirectX::XMFLOAT4X4 gViewProj;
		Material gMaterial;
	};

public:
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT2 Tex;
		DirectX::XMFLOAT2 BoundsY;
	};

	struct InitInfo
	{
		std::wstring HeightMapFilename;
		std::wstring LayerMapFilename0;
		std::wstring LayerMapFilename1;
		std::wstring LayerMapFilename2;
		std::wstring LayerMapFilename3;
		std::wstring LayerMapFilename4;
		std::wstring BlendMapFilename;
		float HeightScale;
		std::uint32_t HeightmapWidth;
		std::uint32_t HeightmapHeight;
		float CellSpacing;
	};

public:
	Terrain()
	{
		DirectX::XMStoreFloat4x4(&mWorld, DirectX::XMMatrixIdentity());

		mMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mMat.Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 64.0f);
		mMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	auto GetWidth()const -> float
	{
		// Total terrain width.
		return (mInfo.HeightmapWidth - 1) * mInfo.CellSpacing;
	}

	auto GetDepth()const -> float
	{
		// Total terrain depth.
		return (mInfo.HeightmapHeight - 1) * mInfo.CellSpacing;
	}

	auto GetHeight(float x, float z)const -> float
	{
		// Transform from terrain local space to "cell" space.
		auto c = (x + 0.5f * GetWidth()) / mInfo.CellSpacing;
		auto d = (z - 0.5f * GetDepth()) / -mInfo.CellSpacing;

		// Get the row and column we are in.
		auto row = (int)std::floorf(d);
		auto col = (int)std::floorf(c);

		// Grab the heights of the cell we are in.
		// A*--*B
		//  | /|
		//  |/ |
		// C*--*D
		auto A = mHeightmap[row * mInfo.HeightmapWidth + col];
		auto B = mHeightmap[row * mInfo.HeightmapWidth + col + 1];
		auto C = mHeightmap[(row + 1) * mInfo.HeightmapWidth + col];
		auto D = mHeightmap[(row + 1) * mInfo.HeightmapWidth + col + 1];

		// Where we are relative to the cell.
		auto s = c - (float)col;
		auto t = d - (float)row;

		// If upper triangle ABC.
		if (s + t <= 1.0f)
		{
			auto uy = B - A;
			auto vy = C - A;
			return A + s * uy + t * vy;
		}
		else // lower triangle DCB.
		{
			auto uy = C - D;
			auto vy = B - D;
			return D + (1.0f - s) * uy + (1.0f - t) * vy;
		}
	}

	auto GetWorld()const -> DirectX::XMMATRIX
	{
		return DirectX::XMLoadFloat4x4(&mWorld);
	}

	void SetWorld(DirectX::CXMMATRIX M)
	{
		DirectX::XMStoreFloat4x4(&mWorld, M);
	}

	void Init(D3D11::ID3D11Device* device, D3D11::ID3D11DeviceContext* dc, const InitInfo& initInfo)
	{
		mInfo = initInfo;

		// Divide heightmap into patches such that each patch has CellsPerPatch.
		mNumPatchVertRows = ((mInfo.HeightmapHeight - 1) / CellsPerPatch) + 1;
		mNumPatchVertCols = ((mInfo.HeightmapWidth - 1) / CellsPerPatch) + 1;

		mNumPatchVertices = mNumPatchVertRows * mNumPatchVertCols;
		mNumPatchQuadFaces = (mNumPatchVertRows - 1) * (mNumPatchVertCols - 1);

		LoadHeightmap();
		Smooth();
		CalcAllPatchBoundsY();

		BuildQuadPatchVB(device);
		BuildQuadPatchIB(device);
		BuildHeightmapSRV(device);

		std::vector<std::wstring> layerFilenames;
		layerFilenames.push_back(mInfo.LayerMapFilename0);
		layerFilenames.push_back(mInfo.LayerMapFilename1);
		layerFilenames.push_back(mInfo.LayerMapFilename2);
		layerFilenames.push_back(mInfo.LayerMapFilename3);
		layerFilenames.push_back(mInfo.LayerMapFilename4);
		mLayerMapArraySRV = d3dHelper::CreateTexture2DArraySRV(device, dc, layerFilenames);

		HR(DirectX::CreateDDSTextureFromFile(device,
			mInfo.BlendMapFilename.c_str(), nullptr, &mBlendMapSRV));

		BuildShaders(device);
	}

	void Draw(D3D11::ID3D11DeviceContext* dc, const Camera& cam, DirectionalLight lights[3])
	{
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
		dc->IASetInputLayout(mInputLayout.get());

		auto stride = static_cast<std::uint32_t>(sizeof(Vertex));
		auto offset = 0u;
		dc->IASetVertexBuffers(0, 1, mQuadPatchVB.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mQuadPatchIB.get(), DXGI_FORMAT_R16_UINT, 0);

		DirectX::XMMATRIX viewProj = cam.ViewProj();

		DirectX::XMFLOAT4 worldPlanes[6];
		ExtractFrustumPlanes(worldPlanes, viewProj);

		// Set per frame constants.
		auto perFrameConstants = PerFrameConstants{
			.gEyePosW = cam.GetPosition(),
			.gLightCount = 1,
			.gFogEnabled = false,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gMinDist = 20.0f,
			.gMaxDist = 500.0f,
			.gMinTess = 0.0f,
			.gMaxTess = 6.0f,
			.gTexelCellSpaceU = 1.0f / mInfo.HeightmapWidth,
			.gFogColor = DirectX::XMFLOAT4{DirectX::Colors::Silver},
			.gTexelCellSpaceV = 1.0f / mInfo.HeightmapHeight,
			.gWorldCellSpace = mInfo.CellSpacing,
			.gTexScale = DirectX::XMFLOAT2(50.0f, 50.0f),
			.gWorldFrustumPlanes = {
				worldPlanes[0],
				worldPlanes[1],
				worldPlanes[2],
				worldPlanes[3],
				worldPlanes[4],
				worldPlanes[5]
			}
		};
		perFrameConstants.gDirLights[0] = lights[0];
		perFrameConstants.gDirLights[1] = lights[1];
		perFrameConstants.gDirLights[2] = lights[2];
		dc->UpdateSubresource(mPerFrameConstants.get(), 0, nullptr, &perFrameConstants, 0, 0);

		// Terrain vertices are already expressed in world space.
		auto perObjectConstants = PerObjectConstants{
			.gMaterial = mMat
		};
		DirectX::XMStoreFloat4x4(&perObjectConstants.gViewProj, viewProj);
		dc->UpdateSubresource(mPerObjectConstants.get(), 0, nullptr, &perObjectConstants, 0, 0);

		dc->VSSetShader(mVertexShader.get(), nullptr, 0);
		dc->HSSetShader(mHullShader.get(), nullptr, 0);
		dc->DSSetShader(mDomainShader.get(), nullptr, 0);
		dc->PSSetShader(mPixelShader.get(), nullptr, 0);

		dc->HSSetConstantBuffers(0, 1, mPerFrameConstants.GetAddressOf());

		auto constantBuffers = std::array{ mPerFrameConstants.get(), mPerObjectConstants.get() };
		dc->DSSetConstantBuffers(0, static_cast<std::uint32_t>(constantBuffers.size()), constantBuffers.data());
		dc->PSSetConstantBuffers(0, static_cast<std::uint32_t>(constantBuffers.size()), constantBuffers.data());

		dc->VSSetShaderResources(2, 1, mHeightMapSRV.GetAddressOf());
		dc->DSSetShaderResources(2, 1, mHeightMapSRV.GetAddressOf());

		auto shaderResources = std::array{
			mLayerMapArraySRV.get(),
			mBlendMapSRV.get(),
			mHeightMapSRV.get()
		};
		dc->PSSetShaderResources(0, static_cast<std::uint32_t>(shaderResources.size()), shaderResources.data());

		dc->VSSetSamplers(1, 1, mSampleLinearMipPoint.GetAddressOf());
		dc->DSSetSamplers(1, 1, mSampleLinearMipPoint.GetAddressOf());

		auto samplers = std::array{ mLinearSampler.get(), mSampleLinearMipPoint.get() };
		dc->PSSetSamplers(0, static_cast<std::uint32_t>(samplers.size()), samplers.data());

		dc->DrawIndexed(mNumPatchQuadFaces * 4, 0, 0);

		// FX sets tessellation stages, but it does not disable them.  So do that here
		// to turn off tessellation.
		dc->HSSetShader(0, 0, 0);
		dc->DSSetShader(0, 0, 0);
	}

private:
	void LoadHeightmap()
	{
		// A height for each vertex
		auto in = std::vector<unsigned char>(mInfo.HeightmapWidth * mInfo.HeightmapHeight);

		// Open the file.
		auto inFile = std::ifstream{mInfo.HeightMapFilename.c_str(), std::ios_base::binary};

		if (not inFile)
			throw std::runtime_error{ "LoadHeightmap() failed to open heightmap file." };

		// Read the RAW bytes.
		inFile.read((char*)&in[0], (std::streamsize)in.size());
		// Done with file.
		inFile.close();

		// Copy the array data into a float array and scale it.
		mHeightmap.resize(mInfo.HeightmapHeight * mInfo.HeightmapWidth, 0);
		for (auto i = 0u; i < mInfo.HeightmapHeight * mInfo.HeightmapWidth; ++i)
			mHeightmap[i] = (in[i] / 255.0f) * mInfo.HeightScale;
	}

	void Smooth()
	{
		auto dest = std::vector<float>(mHeightmap.size());

		for (auto i = 0u; i < mInfo.HeightmapHeight; ++i)
			for (auto j = 0u; j < mInfo.HeightmapWidth; ++j)
				dest[i * mInfo.HeightmapWidth + j] = Average(i, j);

		// Replace the old heightmap with the filtered one.
		mHeightmap = dest;
	}

	auto InBounds(int i, int j) -> bool
	{
		// True if ij are valid indices; false otherwise.
		return
			i >= 0		and i < (int)mInfo.HeightmapHeight 
			and j >= 0	and j < (int)mInfo.HeightmapWidth;
	}

	auto Average(int i, int j) -> float
	{
		// Function computes the average height of the ij element.
		// It averages itself with its eight neighbor pixels.  Note
		// that if a pixel is missing neighbor, we just don't include it
		// in the average--that is, edge pixels don't have a neighbor pixel.
		//
		// ----------
		// | 1| 2| 3|
		// ----------
		// |4 |ij| 6|
		// ----------
		// | 7| 8| 9|
		// ----------

		auto avg = 0.0f;
		auto num = 0.0f;

		// Use int to allow negatives.  If we use UINT, @ i=0, m=i-1=UINT_MAX
		// and no iterations of the outer for loop occur.
		for (auto m = i - 1; m <= i + 1; ++m)
		{
			for (auto n = j - 1; n <= j + 1; ++n)
			{
				if (InBounds(m, n))
				{
					avg += mHeightmap[m * mInfo.HeightmapWidth + n];
					num += 1.0f;
				}
			}
		}

		return avg / num;
	}

	void CalcAllPatchBoundsY()
	{
		mPatchBoundsY.resize(mNumPatchQuadFaces);
		// For each patch
		for (auto i = 0u; i < mNumPatchVertRows - 1; ++i)
			for (auto j = 0u; j < mNumPatchVertCols - 1; ++j)
				CalcPatchBoundsY(i, j);
	}

	void CalcPatchBoundsY(std::uint32_t i, std::uint32_t j)
	{
		// Scan the heightmap values this patch covers and compute the min/max height.

		auto x0 = j * CellsPerPatch;
		auto x1 = (j + 1) * CellsPerPatch;

		auto y0 = i * CellsPerPatch;
		auto y1 = (i + 1) * CellsPerPatch;

		auto minY = +std::numeric_limits<float>::infinity();
		auto maxY = -std::numeric_limits<float>::infinity();
		for (auto y = y0; y <= y1; ++y)
		{
			for (auto x = x0; x <= x1; ++x)
			{
				auto k = y * mInfo.HeightmapWidth + x;
				minY = std::min(minY, mHeightmap[k]);
				maxY = std::max(maxY, mHeightmap[k]);
			}
		}

		auto patchID = i * (mNumPatchVertCols - 1) + j;
		mPatchBoundsY[patchID] = DirectX::XMFLOAT2(minY, maxY);
	}

	void BuildQuadPatchVB(D3D11::ID3D11Device* device)
	{
		auto patchVertices = std::vector<Vertex>(mNumPatchVertRows * mNumPatchVertCols);

		auto halfWidth = 0.5f * GetWidth();
		auto halfDepth = 0.5f * GetDepth();

		auto patchWidth = GetWidth() / (mNumPatchVertCols - 1);
		auto patchDepth = GetDepth() / (mNumPatchVertRows - 1);
		auto du = 1.0f / (mNumPatchVertCols - 1);
		auto dv = 1.0f / (mNumPatchVertRows - 1);

		for (auto i = 0u; i < mNumPatchVertRows; ++i)
		{
			auto z = halfDepth - i * patchDepth;
			for (auto j = 0u; j < mNumPatchVertCols; ++j)
			{
				auto x = -halfWidth + j * patchWidth;

				patchVertices[i * mNumPatchVertCols + j].Pos = DirectX::XMFLOAT3(x, 0.0f, z);

				// Stretch texture over grid.
				patchVertices[i * mNumPatchVertCols + j].Tex.x = j * du;
				patchVertices[i * mNumPatchVertCols + j].Tex.y = i * dv;
			}
		}

		// Store axis-aligned bounding box y-bounds in upper-left patch corner.
		for (auto i = 0u; i < mNumPatchVertRows - 1; ++i)
		{
			for (auto j = 0u; j < mNumPatchVertCols - 1; ++j)
			{
				auto patchID = i * (mNumPatchVertCols - 1) + j;
				patchVertices[i * mNumPatchVertCols + j].BoundsY = mPatchBoundsY[patchID];
			}
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Vertex) * patchVertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};

		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &patchVertices[0],
			.SysMemPitch = 0,
			.SysMemSlicePitch = 0
		};
		HR(device->CreateBuffer(&vbd, &vinitData, &mQuadPatchVB));
	}

	void BuildQuadPatchIB(D3D11::ID3D11Device* device)
	{
		auto indices = std::vector<std::uint16_t>(mNumPatchQuadFaces * 4); // 4 indices per quad face

		// Iterate over each quad and compute indices.
		auto k = 0;
		for (auto i = 0u; i < mNumPatchVertRows - 1; ++i)
		{
			for (auto j = 0u; j < mNumPatchVertCols - 1; ++j)
			{
				// Top row of 2x2 quad patch
				indices[k] = i * mNumPatchVertCols + j;
				indices[k + 1] = i * mNumPatchVertCols + j + 1;

				// Bottom row of 2x2 quad patch
				indices[k + 2] = (i + 1) * mNumPatchVertCols + j;
				indices[k + 3] = (i + 1) * mNumPatchVertCols + j + 1;

				k += 4; // next quad
			}
		}

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint16_t) * indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &indices[0],
			.SysMemPitch = 0,
			.SysMemSlicePitch = 0
		};
		HR(device->CreateBuffer(&ibd, &iinitData, &mQuadPatchIB));
	}

	void BuildHeightmapSRV(D3D11::ID3D11Device* device)
	{
		auto texDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = mInfo.HeightmapWidth,
			.Height = mInfo.HeightmapHeight,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0},
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};		

		// HALF is defined in xnamath.h, for storing 16-bit float.
		auto hmap = std::vector<DirectX::PackedVector::HALF>(mHeightmap.size());
		std::transform(mHeightmap.begin(), mHeightmap.end(), hmap.begin(), DirectX::PackedVector::XMConvertFloatToHalf);

		auto data = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &hmap[0],
			.SysMemPitch = static_cast<std::uint32_t>(mInfo.HeightmapWidth * sizeof(DirectX::PackedVector::HALF)),
			.SysMemSlicePitch = 0
		};
		
		// SRV saves reference.
		auto hmapTex = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(device->CreateTexture2D(&texDesc, &data, &hmapTex));
		auto srvDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
			.Format = texDesc.Format,
			.ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = {.MostDetailedMip = 0, .MipLevels = std::numeric_limits<std::uint32_t>::max()}
		};
		HR(device->CreateShaderResourceView(hmapTex.get(), &srvDesc, &mHeightMapSRV));
	}

	void BuildShaders(D3D11::ID3D11Device* device)
	{
		// Vertex shader
		auto vsBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/TerrainVS.cso", &vsBytecode), "Failed to read vertex shader file.");
			auto hr = device->CreateVertexShader(
				vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), 0, &mVertexShader);
			HR(hr, "Failed to create vertex shader.");
		}

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/TerrainPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			auto hr = device->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader);
			HR(hr, "Failed to create pixel shader.");
		}

		// Hull shader
		auto hsBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/TerrainHS.cso", &hsBytecode), "Failed to read hull shader file.");
			auto hr = device->CreateHullShader(
				hsBytecode->GetBufferPointer(), hsBytecode->GetBufferSize(), 0, &mHullShader);
			HR(hr, "Failed to create hull shader.");
		}

		// Domain shader
		auto dsBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/TerrainDS.cso", &dsBytecode), "Failed to read domain shader file.");
			auto hr = device->CreateDomainShader(
				dsBytecode->GetBufferPointer(), dsBytecode->GetBufferSize(), 0, &mDomainShader);
			HR(hr, "Failed to create domain shader.");
		}

		BuildInputLayout(device, vsBytecode);

		// Build buffer for per frame constants.
		auto perFrameConstantsDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(PerFrameConstants)),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		HR(device->CreateBuffer(&perFrameConstantsDesc, nullptr, &mPerFrameConstants), "Failed to create per frame constant buffer.");

		// Build buffer for per object constants.
		auto perObjectConstantsDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(PerObjectConstants)),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		HR(device->CreateBuffer(&perObjectConstantsDesc, nullptr, &mPerObjectConstants), "Failed to create per object constant buffer.");

		auto samplerLinear = D3D11::D3D11_SAMPLER_DESC{
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
		HR(device->CreateSamplerState(&samplerLinear, &mLinearSampler), "Failed to create sampler state.");

		auto sampleLinearMipPoint = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 4,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(device->CreateSamplerState(&sampleLinearMipPoint, &mSampleLinearMipPoint), "Failed to create sampler state.");
	}

	void BuildInputLayout(D3D11::ID3D11Device* device, const ComPtr<D3D::ID3DBlob>& vsBytecode)
	{
		auto inputDesc = std::array<D3D11::D3D11_INPUT_ELEMENT_DESC, 3>{
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
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 1,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 20,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};
		HR(device->CreateInputLayout(
			inputDesc.data(), static_cast<std::uint32_t>(inputDesc.size()), vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &mInputLayout), "Failed to create input layout.");
	}

private:

	// Divide heightmap into patches such that each patch has CellsPerPatch cells
	// and CellsPerPatch+1 vertices.  Use 64 so that if we tessellate all the way 
	// to 64, we use all the data from the heightmap.  
	static constexpr auto CellsPerPatch = 64;

	ComPtr<D3D11::ID3D11Buffer> mQuadPatchVB;
	ComPtr<D3D11::ID3D11Buffer> mQuadPatchIB;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameConstants;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectConstants;

	ComPtr<D3D11::ID3D11ShaderResourceView> mLayerMapArraySRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBlendMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mHeightMapSRV;

	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mDomainShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;

	ComPtr<D3D11::ID3D11SamplerState> mLinearSampler;
	ComPtr<D3D11::ID3D11SamplerState> mSampleLinearMipPoint;

	InitInfo mInfo;

	std::uint32_t mNumPatchVertices = 0;
	std::uint32_t mNumPatchQuadFaces = 0;

	std::uint32_t mNumPatchVertRows = 0;
	std::uint32_t mNumPatchVertCols = 0;

	DirectX::XMFLOAT4X4 mWorld;

	Material mMat;

	std::vector<DirectX::XMFLOAT2> mPatchBoundsY;
	std::vector<float> mHeightmap;
};