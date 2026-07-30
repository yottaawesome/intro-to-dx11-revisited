export module litskull:litskullapp;
import std;
import shared;
import :vertex;

struct cbPerFrame
{
	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;

	float  gFogStart;
	float  gFogRange;
	DirectX::XMFLOAT4 gFogColor;
};

struct cbPerObject
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};

export class LitSkullApp : public D3DApp
{
public:
	LitSkullApp(HINSTANCE hInstance);
	~LitSkullApp();

	void Init()
	{
		D3DApp::Init();

		// Must init Effects first since InputLayouts depend on shader signatures.
		//Effects::InitAll(md3dDevice);
		InputLayouts::InitAll(md3dDevice.get());

		BuildShapeGeometryBuffers();
		BuildSkullGeometryBuffers();

		Init();
	}

	void OnResize()
	{
		D3DApp::OnResize();

		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt)
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

		//
		// Switch the number of lights based on key presses.
		//
		if (Win32::GetAsyncKeyState('0') & 0x8000)
			mLightCount = 0;
		if (Win32::GetAsyncKeyState('1') & 0x8000)
			mLightCount = 1;
		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mLightCount = 2;
		if (Win32::GetAsyncKeyState('3') & 0x8000)
			mLightCount = 3;
	}

	void DrawScene()
	{
		//md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
		//md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//md3dImmediateContext->IASetInputLayout(InputLayouts::PosNormal);
		//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//UINT stride = sizeof(Vertex::PosNormal);
		//UINT offset = 0;

		//XMMATRIX view = XMLoadFloat4x4(&mView);
		//XMMATRIX proj = XMLoadFloat4x4(&mProj);
		//XMMATRIX viewProj = view * proj;

		//// Set per frame constants.
		//Effects::BasicFX->SetDirLights(mDirLights);
		//Effects::BasicFX->SetEyePosW(mEyePosW);

		//// Figure out which technique to use.
		//ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light1Tech;
		//switch (mLightCount)
		//{
		//case 1:
		//	activeTech = Effects::BasicFX->Light1Tech;
		//	break;
		//case 2:
		//	activeTech = Effects::BasicFX->Light2Tech;
		//	break;
		//case 3:
		//	activeTech = Effects::BasicFX->Light3Tech;
		//	break;
		//}

		//D3DX11_TECHNIQUE_DESC techDesc;
		//activeTech->GetDesc(&techDesc);
		//for (UINT p = 0; p < techDesc.Passes; ++p)
		//{
		//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
		//	md3dImmediateContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

		//	// Draw the grid.
		//	XMMATRIX world = XMLoadFloat4x4(&mGridWorld);
		//	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		//	XMMATRIX worldViewProj = world * view * proj;

		//	Effects::BasicFX->SetWorld(world);
		//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//	Effects::BasicFX->SetMaterial(mGridMat);

		//	activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//	md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		//	// Draw the box.
		//	world = XMLoadFloat4x4(&mBoxWorld);
		//	worldInvTranspose = MathHelper::InverseTranspose(world);
		//	worldViewProj = world * view * proj;

		//	Effects::BasicFX->SetWorld(world);
		//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//	Effects::BasicFX->SetMaterial(mBoxMat);

		//	activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//	md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		//	// Draw the cylinders.
		//	for (int i = 0; i < 10; ++i)
		//	{
		//		world = XMLoadFloat4x4(&mCylWorld[i]);
		//		worldInvTranspose = MathHelper::InverseTranspose(world);
		//		worldViewProj = world * view * proj;

		//		Effects::BasicFX->SetWorld(world);
		//		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//		Effects::BasicFX->SetMaterial(mCylinderMat);

		//		activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//		md3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		//	}

		//	// Draw the spheres.
		//	for (int i = 0; i < 10; ++i)
		//	{
		//		world = XMLoadFloat4x4(&mSphereWorld[i]);
		//		worldInvTranspose = MathHelper::InverseTranspose(world);
		//		worldViewProj = world * view * proj;

		//		Effects::BasicFX->SetWorld(world);
		//		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//		Effects::BasicFX->SetMaterial(mSphereMat);

		//		activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//		md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		//	}

		//	// Draw the skull.

		//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		//	md3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		//	world = XMLoadFloat4x4(&mSkullWorld);
		//	worldInvTranspose = MathHelper::InverseTranspose(world);
		//	worldViewProj = world * view * proj;

		//	Effects::BasicFX->SetWorld(world);
		//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//	Effects::BasicFX->SetMaterial(mSkullMat);

		//	activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		//	md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);
		//}

		//HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;
		Win32::SetCapture(mhMainWnd);
	}

	void OnMouseUp(Win32::WPARAM btnState, int x, int y)
	{
		Win32::ReleaseCapture();
	}

	void OnMouseMove(Win32::WPARAM btnState, int x, int y)
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
			mRadius = std::clamp(mRadius, 3.0f, 200.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
	void BuildShapeGeometryBuffers()
	{
		auto box = GeometryGenerator::MeshData{};
		auto grid = GeometryGenerator::MeshData{};
		auto sphere = GeometryGenerator::MeshData{};
		auto cylinder = GeometryGenerator::MeshData{};

		auto geoGen = GeometryGenerator{};
		geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
		geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
		geoGen.CreateSphere(0.5f, 20, 20, sphere);
		geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

		// Cache the vertex offsets to each object in the concatenated vertex buffer.
		mBoxVertexOffset = 0;
		mGridVertexOffset = static_cast<std::uint32_t>(box.Vertices.size());
		mSphereVertexOffset = mGridVertexOffset + static_cast<std::uint32_t>(grid.Vertices.size());
		mCylinderVertexOffset = mSphereVertexOffset + static_cast<std::uint32_t>(sphere.Vertices.size());

		// Cache the index count of each object.
		mBoxIndexCount = static_cast<std::uint32_t>(box.Indices.size());
		mGridIndexCount = static_cast<std::uint32_t>(grid.Indices.size());
		mSphereIndexCount = static_cast<std::uint32_t>(sphere.Indices.size());
		mCylinderIndexCount = static_cast<std::uint32_t>(cylinder.Indices.size());

		// Cache the starting index for each object in the concatenated index buffer.
		mBoxIndexOffset = 0;
		mGridIndexOffset = mBoxIndexCount;
		mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
		mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

		auto totalVertexCount = static_cast<std::uint32_t>(
			box.Vertices.size() +
			grid.Vertices.size() +
			sphere.Vertices.size() +
			cylinder.Vertices.size()
		);

		auto totalIndexCount = static_cast<std::uint32_t>(
			mBoxIndexCount +
			mGridIndexCount +
			mSphereIndexCount +
			mCylinderIndexCount
		);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		auto vertices = std::vector<Vertex::PosNormal>(totalVertexCount);

		auto k = 0u;
		for (auto i = 0ull; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
		}

		for (auto i = 0ull; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Normal = grid.Vertices[i].Normal;
		}

		for (auto i = 0ull; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Normal = sphere.Vertices[i].Normal;
		}

		for (auto i = 0ull; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Normal = cylinder.Vertices[i].Normal;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Vertex::PosNormal) * totalVertexCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &vertices[0]
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		auto indices = std::vector<std::uint32_t>{};
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
		indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
		indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
		indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(std::uint32_t) * totalIndexCount,
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &indices[0]
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
	}

	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream("Models/skull.txt");

		if (not fin)
			throw std::runtime_error("Models/skull.txt not found.");

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<Vertex::PosNormal>(vcount);
		for (auto i = 0u; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
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

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Vertex::PosNormal) * vcount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &vertices[0]
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(std::uint32_t) * mSkullIndexCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = &indices[0]
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mShapesVB;
	ComPtr<D3D11::ID3D11Buffer> mShapesIB;

	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;

	DirectionalLight mDirLights[3];
	Material mGridMat;
	Material mBoxMat;
	Material mCylinderMat;
	Material mSphereMat;
	Material mSkullMat;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mSphereWorld[10];
	DirectX::XMFLOAT4X4 mCylWorld[10];
	DirectX::XMFLOAT4X4 mBoxWorld;
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mSkullWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	int mBoxVertexOffset;
	int mGridVertexOffset;
	int mSphereVertexOffset;
	int mCylinderVertexOffset;

	std::uint32_t mBoxIndexOffset;
	std::uint32_t mGridIndexOffset;
	std::uint32_t mSphereIndexOffset;
	std::uint32_t mCylinderIndexOffset;

	std::uint32_t mBoxIndexCount;
	std::uint32_t mGridIndexCount;
	std::uint32_t mSphereIndexCount;
	std::uint32_t mCylinderIndexCount;

	std::uint32_t mSkullIndexCount = 0;

	std::uint32_t mLightCount = 1;

	DirectX::XMFLOAT3 mEyePosW = {0.0f, 0.0f, 0.0f};

	float mTheta = 1.5f * MathHelper::Pi;
	float mPhi = 0.1f * MathHelper::Pi;
	float mRadius = 15.0f;

	Win32::POINT mLastMousePos{};
};

