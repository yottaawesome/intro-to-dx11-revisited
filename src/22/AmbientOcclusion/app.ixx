export module ambientocclusiondemo:app;
import std;
import shared;
import :octree;

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 gWorldViewProj;
};

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
	float AmbientAccess;
};

export class AmbientOcclusionApp : public D3DApp
{
public:
	AmbientOcclusionApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance, L"Ambient Occlusion")
	{
		mCam.SetPosition(0.0f, 5.0f, -5.0f);
		mCam.LookAt(
			DirectX::XMFLOAT3(-4.0f, 4.0f, -4.0f),
			DirectX::XMFLOAT3(0.0f, 2.2f, 0.0f),
			DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));

		DirectX::XMMATRIX skullScale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);
		DirectX::XMMATRIX skullOffset = DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&mSkullWorld, DirectX::XMMatrixMultiply(skullScale, skullOffset));
		Init();
	}

	void Init() override
	{
		D3DApp::Init();
		BuildSkullGeometryBuffers();
		BuildShaders();
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

		auto stride = static_cast<std::uint32_t>(sizeof(Vertex));
		auto offset = 0u;

		mCam.UpdateViewMatrix();

		auto view = DirectX::XMMATRIX{mCam.View()};
		auto proj = DirectX::XMMATRIX{mCam.Proj()};
		auto viewProj = DirectX::XMMATRIX{mCam.ViewProj()};

		// Draw the skull.

		md3dImmediateContext->IASetVertexBuffers(0, 1, mSkullVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mSkullWorld)};
		auto worldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(world)};
		auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

		auto perObjectConstants = PerObjectConstants{};
		DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
		md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
		auto vsConstantBuffers = std::array{ mPerObject.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(vsConstantBuffers.size()), vsConstantBuffers.data());

		md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

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
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.Pitch(dy);
			mCam.RotateY(dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
	void BuildVertexAmbientOcclusion(
		std::vector<Vertex>& vertices,
		const std::vector<std::uint32_t>& indices
	)
	{
		auto vcount = static_cast<std::uint32_t>(vertices.size());
		auto tcount = static_cast<std::uint32_t>(indices.size() / 3);

		std::vector<DirectX::XMFLOAT3> positions(vcount);
		for (auto i = 0u; i < vcount; ++i)
			positions[i] = vertices[i].Position;

		Octree octree;
		octree.Build(positions, indices);

		// For each vertex, count how many triangles contain the vertex.
		std::vector<int> vertexSharedCount(vcount);
		for (auto i = 0u; i < tcount; ++i)
		{
			auto i0 = indices[i * 3 + 0];
			auto i1 = indices[i * 3 + 1];
			auto i2 = indices[i * 3 + 2];

			DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&vertices[i0].Position);
			DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&vertices[i1].Position);
			DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&vertices[i2].Position);

			DirectX::XMVECTOR edge0 = v1 - v0;
			DirectX::XMVECTOR edge1 = v2 - v0;

			DirectX::XMVECTOR normal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(edge0, edge1));

			DirectX::XMVECTOR centroid = (v0 + v1 + v2) / 3.0f;

			// Offset to avoid self intersection.
			centroid += 0.001f * normal;

			const int NumSampleRays = 32;
			float numUnoccluded = 0;
			for (int j = 0; j < NumSampleRays; ++j)
			{
				DirectX::XMVECTOR randomDir = MathHelper::RandHemisphereUnitVec3(normal);

				// TODO: Technically we should not count intersections that are far 
				// away as occluding the triangle, but this is OK for demo.
				if (!octree.RayOctreeIntersect(centroid, randomDir))
				{
					numUnoccluded++;
				}
			}

			float ambientAccess = numUnoccluded / NumSampleRays;

			// Average with vertices that share this face.
			vertices[i0].AmbientAccess += ambientAccess;
			vertices[i1].AmbientAccess += ambientAccess;
			vertices[i2].AmbientAccess += ambientAccess;

			vertexSharedCount[i0]++;
			vertexSharedCount[i1]++;
			vertexSharedCount[i2]++;
		}

		// Finish average by dividing by the number of samples we added.
		for (auto i = 0u; i < vcount; ++i)
		{
			vertices[i].AmbientAccess /= vertexSharedCount[i];
		}
	}

	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream{ "Models/skull.txt" };
		if (not fin)
			throw std::runtime_error{ "Models/skull.txt not found." };

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<Vertex>(vcount);
		for (auto i = 0u; i < vcount; ++i)
		{
			fin >> vertices[i].Position.x >> vertices[i].Position.y >> vertices[i].Position.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		}

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

		BuildVertexAmbientOcclusion(vertices, indices);

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Vertex) * vcount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
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
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/AmbientOcclusionVS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");
		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 0, &mVertexShader);
		HR(hr, "Failed to create vertex shader.");
		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/AmbientOcclusionPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
		HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader), "Failed to create pixel shader.");
		BuildInputLayout(vertexShaderBytecode.get());

		auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mPerObject), "Failed to create constant buffer.");
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
				.SemanticName = "AMBIENT",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 32,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};
		HR(md3dDevice->CreateInputLayout(
			basic32Desc.data(),
			static_cast<std::uint32_t>(basic32Desc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mInputLayout),
			"Failed to create input layout.");
	}

private:

	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;

	DirectX::XMFLOAT4X4 mSkullWorld;

	std::uint32_t mSkullIndexCount = 0;

	Camera mCam;

	Win32::POINT mLastMousePos{0, 0};
};