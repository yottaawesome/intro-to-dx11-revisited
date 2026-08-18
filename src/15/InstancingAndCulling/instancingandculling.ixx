export module instancingandculling;
import std;
import shared;

struct PerFrameConstants
{
	DirectionalLight DirLights[3];
	DirectX::XMFLOAT3 EyePosW;
	float FogStart;
	float FogRange;
	std::uint32_t LightCount;
	std::uint32_t UseTexture;
	std::uint32_t AlphaClip;
	std::uint32_t FogEnabled;
	DirectX::XMFLOAT3 Padding;
	DirectX::XMFLOAT4 FogColor;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTranspose;
	DirectX::XMFLOAT4X4 ViewProj;
	DirectX::XMFLOAT4X4 TexTransform;
	Material Material;
};

// The shader's VertexIn combines both input slots, but the CPU buffers remain
// split like the original sample: mesh attributes are per-vertex in slot 0,
// while World and Color are per-instance in slot 1. SV_InstanceID is generated
// by the GPU and therefore has no corresponding CPU-side field.
struct BasicVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

struct InstancedData
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4 Color;
};

export class InstancingAndCullingApp : public D3DApp
{
public:
	InstancingAndCullingApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"Instancing and Culling Demo";

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		auto I = DirectX::XMMATRIX{ DirectX::XMMatrixIdentity() };
		auto skullScale = DirectX::XMMATRIX{ DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) };
		auto skullOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f) };
		DirectX::XMStoreFloat4x4(&mSkullWorld, DirectX::XMMatrixMultiply(skullScale, skullOffset));

		mDirLights[0].Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();
		BuildShaders();
		BuildSkullGeometryBuffers();
		BuildInstancedBuffer();
	}

	void OnResize() override
	{
		D3DApp::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

		// Build the frustum from the projection matrix in view space.
		DirectX::BoundingFrustum::CreateFromMatrix(mCamFrustum, mCam.Proj());
	}

	void UpdateScene(float dt) override
	{
		//
		// Control the camera.
		//
		if (Win32::GetAsyncKeyState('W') & 0x8000)
			mCam.Walk(10.0f * dt);

		if (Win32::GetAsyncKeyState('S') & 0x8000)
			mCam.Walk(-10.0f * dt);

		if (Win32::GetAsyncKeyState('A') & 0x8000)
			mCam.Strafe(-10.0f * dt);

		if (Win32::GetAsyncKeyState('D') & 0x8000)
			mCam.Strafe(10.0f * dt);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			mFrustumCullingEnabled = true;

		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mFrustumCullingEnabled = false;

		//
		// Perform frustum culling.
		//

		mCam.UpdateViewMatrix();
		mVisibleObjectCount = 0;

		if (mFrustumCullingEnabled)
		{
			auto detView = DirectX::XMVECTOR{ DirectX::XMMatrixDeterminant(mCam.View()) };
			auto invView = DirectX::XMMATRIX{ DirectX::XMMatrixInverse(&detView, mCam.View()) };

			auto mappedData = D3D11::D3D11_MAPPED_SUBRESOURCE{};
			md3dImmediateContext->Map(mInstancedBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

			auto dataView = reinterpret_cast<InstancedData*>(mappedData.pData);

			for (auto i = 0u; i < mInstancedData.size(); ++i)
			{
				auto W = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mInstancedData[i].World) };
				auto detW = DirectX::XMVECTOR{ DirectX::XMMatrixDeterminant(W) };
				auto invWorld = DirectX::XMMATRIX{ DirectX::XMMatrixInverse(&detW, W) };

				// View space to the object's local space.
				auto toLocal = DirectX::XMMatrixMultiply(invView, invWorld);

				// Decompose the matrix into its individual parts.
				auto scale = DirectX::XMVECTOR{};
				auto rotQuat = DirectX::XMVECTOR{};
				auto translation = DirectX::XMVECTOR{};
				DirectX::XMMatrixDecompose(&scale, &rotQuat, &translation, toLocal);

				// Transform the camera frustum from view space to the object's local space.
				auto localspaceFrustum = DirectX::BoundingFrustum{};
				mCamFrustum.Transform(localspaceFrustum, DirectX::XMVectorGetX(scale), rotQuat, translation);

				// Perform the box/frustum intersection test in local space.
				if (localspaceFrustum.Intersects(mSkullBox) != 0)
				{
					// Write the instance data to dynamic VB of the visible objects.
					dataView[mVisibleObjectCount++] = mInstancedData[i];
				}
			}

			md3dImmediateContext->Unmap(mInstancedBuffer.get(), 0);
		}
		else // No culling enabled, draw all objects.
		{
			auto mappedData = D3D11::D3D11_MAPPED_SUBRESOURCE{};
			md3dImmediateContext->Map(mInstancedBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			auto dataView = reinterpret_cast<InstancedData*>(mappedData.pData);
			for (auto i = 0u; i < mInstancedData.size(); ++i)
			{
				dataView[mVisibleObjectCount++] = mInstancedData[i];
			}
			md3dImmediateContext->Unmap(mInstancedBuffer.get(), 0);
		}

		auto outs = std::wostringstream{};
		outs.precision(6);
		outs << std::format(L"Instancing and Culling Demo: {} objects visible out of {}", mVisibleObjectCount, mInstancedData.size());
		mMainWndCaption = outs.str();
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL}, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mPixelShader.get(), nullptr, 0);

		auto stride = std::array<std::uint32_t, 2>{ sizeof(BasicVertex), sizeof(InstancedData) };
		auto offset = std::array{ 0u, 0u };

		auto vbs = std::array{ mSkullVB.get(), mInstancedBuffer.get() };

		auto view = DirectX::XMMATRIX{mCam.View()};
		auto proj = DirectX::XMMATRIX{mCam.Proj()};
		auto viewProj = DirectX::XMMATRIX{mCam.ViewProj()};

		// Set per frame constants.
		auto perFrameConstants = PerFrameConstants{
			.FogStart = 15.0f,
			.FogRange = 175.0f,
			.LightCount = mLightCount,
			.UseTexture = false,
			.AlphaClip = false,
			.FogEnabled = false,
			.FogColor = DirectX::XMFLOAT4{ DirectX::Colors::Silver },
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perFrameConstants.DirLights));
		perFrameConstants.EyePosW = mCam.GetPosition();
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		// Draw the skull.

		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(stride.size()), vbs.data(), stride.data(), offset.data());
		md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mSkullWorld)};
		auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };

		auto perObjectConstants = PerObjectConstants{
			.Material = mSkullMat
		};
		DirectX::XMStoreFloat4x4(&perObjectConstants.World, world);
		DirectX::XMStoreFloat4x4(&perObjectConstants.WorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObjectConstants.ViewProj, viewProj);
		md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

		auto vsConstants = std::array{ mPerObject.get() };
		auto psConstants = std::array{ mPerFrame.get(), mPerObject.get() };
		md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
		md3dImmediateContext->DrawIndexedInstanced(mSkullIndexCount, mVisibleObjectCount, 0, 0, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override
	{
		mLastMousePos = { x, y };
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
			auto dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			auto dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.Pitch(dy);
			mCam.RotateY(dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream("Models/skull.txt");
		if (not fin)
			throw std::runtime_error{ "Models/skull.txt not found." };

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vMinf3 = DirectX::XMFLOAT3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		auto vMaxf3 = DirectX::XMFLOAT3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		auto vMin = DirectX::XMLoadFloat3(&vMinf3);
		auto vMax = DirectX::XMLoadFloat3(&vMaxf3);
		auto vertices = std::vector<BasicVertex>(vcount);
		for (auto i = 0u; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&vertices[i].Pos);

			vMin = DirectX::XMVectorMin(vMin, P);
			vMax = DirectX::XMVectorMax(vMax, P);
		}

		DirectX::XMStoreFloat3(&mSkullBox.Center, 0.5f * (vMin + vMax));
		DirectX::XMStoreFloat3(&mSkullBox.Extents, 0.5f * (vMax - vMin));

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		mSkullIndexCount = 3 * tcount;
		auto indices = std::vector<std::uint32_t>(mSkullIndexCount);
		for (auto i = 0u; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(BasicVertex) * vcount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &vertices[0]
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(std::uint32_t) * mSkullIndexCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &indices[0]
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}

	void BuildInstancedBuffer()
	{
		const int n = 5;
		mInstancedData.resize(n * n * n);

		auto width = 200.0f;
		auto height = 200.0f;
		auto depth = 200.0f;

		auto x = -0.5f * width;
		auto y = -0.5f * height;
		auto z = -0.5f * depth;
		auto dx = width / (n - 1);
		auto dy = height / (n - 1);
		auto dz = depth / (n - 1);
		for (auto k = 0; k < n; ++k)
		{
			for (auto i = 0; i < n; ++i)
			{
				for (auto j = 0; j < n; ++j)
				{
					// Position instanced along a 3D grid.
					mInstancedData[k * n * n + i * n + j].World = DirectX::XMFLOAT4X4(
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						x + j * dx, y + i * dy, z + k * dz, 1.0f);

					// Random color.
					mInstancedData[k * n * n + i * n + j].Color.x = MathHelper::RandF(0.0f, 1.0f);
					mInstancedData[k * n * n + i * n + j].Color.y = MathHelper::RandF(0.0f, 1.0f);
					mInstancedData[k * n * n + i * n + j].Color.z = MathHelper::RandF(0.0f, 1.0f);
					mInstancedData[k * n * n + i * n + j].Color.w = 1.0f;
				}
			}
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(InstancedData) * mInstancedData.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11::D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&vbd, 0, &mInstancedBuffer));
	}

	void BuildShaders()
	{
		{
			auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/vs.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mVertexShader);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/ps.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader), "Failed to create pixel shader.");
			BuildInputLayout(vertexShaderBytecode.get());
		}

		// constant buffers basic32
		auto perFrameDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameDesc, 0, &mPerFrame), "Failed to create per frame constant buffer.");

		auto perObjectDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectDesc, 0, &mPerObject), "Failed to create per object constant buffer.");
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
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "WORLD",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "WORLD",
				.SemanticIndex = 1,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = 16,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "WORLD",
				.SemanticIndex = 2,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = 32,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "WORLD",
				.SemanticIndex = 3,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = 48,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "COLOR",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = 64,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			}
		};
		HR(md3dDevice->CreateInputLayout(
			basic32Desc.data(),
			static_cast<std::uint32_t>(basic32Desc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mInputLayout),
			"Failed to create instanced input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;
	ComPtr<D3D11::ID3D11Buffer> mInstancedBuffer;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;

	// Bounding box of the skull.
	DirectX::BoundingBox mSkullBox;
	DirectX::BoundingFrustum mCamFrustum;

	std::uint32_t mVisibleObjectCount = 0;

	// Keep a system memory copy of the world matrices for culling.
	std::vector<InstancedData> mInstancedData;

	bool mFrustumCullingEnabled = true;

	DirectionalLight mDirLights[3];
	std::uint32_t mLightCount = 3;
	Material mSkullMat;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mSkullWorld;

	std::uint32_t mSkullIndexCount = 0;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};