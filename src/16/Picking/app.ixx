export module picking:app;
import std;
import shared;
import :renderstates;

struct BasicVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

struct PerFrameConstants
{
	DirectionalLight DirLights[3];
	DirectX::XMFLOAT3 EyePos;
	float FogStart;
	float FogRange;
	std::uint32_t LightCount;
	bool32 UseTexture;
	bool32 AlphaClip;
	bool32 FogEnabled;
	DirectX::XMFLOAT3 Padding;
	DirectX::XMFLOAT4 FogColor;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTranspose;
	DirectX::XMFLOAT4X4 WorldViewProj;
	DirectX::XMFLOAT4X4 TexTransform;
	Material Material;
};

export class PickingApp : public D3DApp
{
public:
	PickingApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance, L"Picking Demo" }
	{
		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		DirectX::XMMATRIX MeshScale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);
		DirectX::XMMATRIX MeshOffset = DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&mMeshWorld, DirectX::XMMatrixMultiply(MeshScale, MeshOffset));

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

		mMeshMat.Ambient = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mMeshMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mMeshMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		mPickedTriangleMat.Ambient = DirectX::XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
		mPickedTriangleMat.Diffuse = DirectX::XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
		mPickedTriangleMat.Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();
		mRenderStates.emplace(md3dDevice.get());
		BuildShaders();
		BuildMeshGeometryBuffers();
	}

	void OnResize() override
	{
		D3DApp::OnResize();
		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
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
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mPixelShader.get(), nullptr, 0);

		auto stride = static_cast<std::uint32_t>(sizeof(BasicVertex));
		auto offset = 0u;

		mCam.UpdateViewMatrix();

		auto view = DirectX::XMMATRIX{mCam.View()};
		auto proj = DirectX::XMMATRIX{mCam.Proj()};
		auto viewProj = DirectX::XMMATRIX{mCam.ViewProj()};

		// Set per frame constants.
		auto perFrame = PerFrameConstants{
			.EyePos = mCam.GetPosition(),
			.FogStart = 15.0f,
			.FogRange = 175.0f,
			.LightCount = mLightCount,
			.UseTexture = false,
			.AlphaClip = false,
			.FogEnabled = false,
			.FogColor = DirectX::XMFLOAT4{ DirectX::Colors::Silver },
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perFrame.DirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrame, 0, 0);

		// Draw the Mesh.
		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(mRenderStates->WireframeRS.get());

		md3dImmediateContext->IASetVertexBuffers(0, 1, mMeshVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mMeshIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mMeshWorld) };
		auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
		auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

		// Set per object constants.
		auto perObject = PerObjectConstants{
			.Material = mMeshMat
		};
		DirectX::XMStoreFloat4x4(&perObject.World, world);
		DirectX::XMStoreFloat4x4(&perObject.WorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObject.WorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&perObject.TexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObject, 0, 0);

		auto vsConstants = std::array{ mPerObject.get() };
		auto psConstants = std::array{ mPerFrame.get(), mPerObject.get() };
		md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());

		md3dImmediateContext->DrawIndexed(mMeshIndexCount, 0, 0);

		// Restore default
		md3dImmediateContext->RSSetState(0);

		// Draw just the picked triangle again with a different material to highlight it.

		if (mPickedTriangle != -1)
		{
			// Change depth test from < to <= so that if we draw the same triangle twice, it will still pass
			// the depth test.  This is because we redraw the picked triangle with a different material
			// to highlight it.  
			md3dImmediateContext->OMSetDepthStencilState(mRenderStates->LessEqualDSS.get(), 0);

			perObject.Material = mPickedTriangleMat;
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObject, 0, 0);
			md3dImmediateContext->DrawIndexed(3, 3 * mPickedTriangle, 0);

			// restore default
			md3dImmediateContext->OMSetDepthStencilState(0, 0);
		}

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override
	{
		if ((btnState & Win32::MK::LButton) != 0)
		{
			mLastMousePos.x = x;
			mLastMousePos.y = y;

			Win32::SetCapture(mhMainWnd);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			Pick(x, y);
		}
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
	void BuildMeshGeometryBuffers()
	{
		auto fin = std::ifstream{ "Models/car.txt" };

		if (not fin)
			throw std::runtime_error{ "Models/car.txt not found." };

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
		mMeshVertices.resize(vcount);
		for (auto i = 0u; i < vcount; ++i)
		{
			fin >> mMeshVertices[i].Pos.x >> mMeshVertices[i].Pos.y >> mMeshVertices[i].Pos.z;
			fin >> mMeshVertices[i].Normal.x >> mMeshVertices[i].Normal.y >> mMeshVertices[i].Normal.z;

			auto P = DirectX::XMLoadFloat3(&mMeshVertices[i].Pos);

			vMin = DirectX::XMVectorMin(vMin, P);
			vMax = DirectX::XMVectorMax(vMax, P);
		}

		DirectX::XMStoreFloat3(&mMeshBox.Center, 0.5f * (vMin + vMax));
		DirectX::XMStoreFloat3(&mMeshBox.Extents, 0.5f * (vMax - vMin));

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		mMeshIndexCount = 3 * tcount;
		mMeshIndices.resize(mMeshIndexCount);
		for (auto i = 0u; i < tcount; ++i)
		{
			fin >> mMeshIndices[i * 3 + 0] >> mMeshIndices[i * 3 + 1] >> mMeshIndices[i * 3 + 2];
		}

		fin.close();

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(BasicVertex) * vcount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &mMeshVertices[0] };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mMeshVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(UINT) * mMeshIndexCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &mMeshIndices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mMeshIB));
	}

	void Pick(int sx, int sy)
	{
		auto P = DirectX::XMMATRIX{ mCam.Proj() };
		auto P2 = DirectX::XMFLOAT4X4{};
		DirectX::XMStoreFloat4x4(&P2, P);
		// Compute picking ray in view space.
		auto vx = (+2.0f * sx / mClientWidth - 1.0f) / P2(0, 0);
		auto vy = (-2.0f * sy / mClientHeight + 1.0f) / P2(1, 1);

		// Ray definition in view space.
		auto rayOrigin = DirectX::XMVECTOR{DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)};
		auto rayDir = DirectX::XMVECTOR{DirectX::XMVectorSet(vx, vy, 1.0f, 0.0f)};

		// Tranform ray to local space of Mesh.
		auto V = DirectX::XMMATRIX{ mCam.View() };
		auto detV = DirectX::XMMatrixDeterminant(V);
		auto invView = DirectX::XMMATRIX{ DirectX::XMMatrixInverse(&detV, V) };

		auto W = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mMeshWorld) };
		auto detW = DirectX::XMMatrixDeterminant(W);
		auto invWorld = DirectX::XMMATRIX{ DirectX::XMMatrixInverse(&detW, W) };

		auto toLocal = DirectX::XMMATRIX{ DirectX::XMMatrixMultiply(invView, invWorld) };

		rayOrigin = DirectX::XMVector3TransformCoord(rayOrigin, toLocal);
		rayDir = DirectX::XMVector3TransformNormal(rayDir, toLocal);

		// Make the ray direction unit length for the intersection tests.
		rayDir = DirectX::XMVector3Normalize(rayDir);

		// If we hit the bounding box of the Mesh, then we might have picked a Mesh triangle,
		// so do the ray/triangle tests.
		//
		// If we did not hit the bounding box, then it is impossible that we hit 
		// the Mesh, so do not waste effort doing ray/triangle tests.

		// Assume we have not picked anything yet, so init to -1.
		mPickedTriangle = -1;
		auto tmin = 0.0f;
		if (mMeshBox.Intersects(rayOrigin, rayDir, tmin))
		{
			// Find the nearest ray/triangle intersection.
			tmin = MathHelper::Infinity;
			for (auto i = 0u; i < mMeshIndices.size() / 3; ++i)
			{
				// Indices for this triangle.
				auto i0 = mMeshIndices[i * 3 + 0];
				auto i1 = mMeshIndices[i * 3 + 1];
				auto i2 = mMeshIndices[i * 3 + 2];

				// Vertices for this triangle.
				auto v0 = DirectX::XMVECTOR{ DirectX::XMLoadFloat3(&mMeshVertices[i0].Pos) };
				auto v1 = DirectX::XMVECTOR{ DirectX::XMLoadFloat3(&mMeshVertices[i1].Pos) };
				auto v2 = DirectX::XMVECTOR{ DirectX::XMLoadFloat3(&mMeshVertices[i2].Pos) };

				// We have to iterate over all the triangles in order to find the nearest intersection.
				auto t = 0.0f;
				auto hit = DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, t);
				if (hit and t < tmin)
				{
					// This is the new nearest picked triangle.
					tmin = t;
					mPickedTriangle = i;
				}
			}
		}

	}

	void BuildShaders()
	{
		{
			auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicVS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mVertexShader);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader), "Failed to create pixel shader.");
			BuildInputLayout(vertexShaderBytecode.get());
		}

		// constant buffers
		{
			auto perFrameDesc = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(PerFrameConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perFrameDesc, 0, &mPerFrame), "Failed to create per frame constant buffer.");
		}

		{
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
	ComPtr<D3D11::ID3D11Buffer> mMeshVB;
	ComPtr<D3D11::ID3D11Buffer> mMeshIB;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;
	std::optional<RenderStates> mRenderStates;

	// Keep system memory copies of the Mesh geometry for picking.
	std::vector<BasicVertex> mMeshVertices;
	std::vector<std::uint32_t> mMeshIndices;

	DirectX::BoundingBox mMeshBox;

	DirectionalLight mDirLights[3];
	Material mMeshMat;
	Material mPickedTriangleMat;
	std::uint32_t mLightCount = 3;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mMeshWorld;

	std::uint32_t mMeshIndexCount;

	std::uint32_t mPickedTriangle;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};