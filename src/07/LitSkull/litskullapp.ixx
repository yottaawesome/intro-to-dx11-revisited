export module litskull:litskullapp;
import std;
import shared;
import :vertex;

struct PosNormal
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};

struct PerFrameConstants
{
	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;

	float gFogStart;
	float gFogRange;
	int gLightCount = 0;
	DirectX::XMFLOAT4 gFogColor;
	float pad[2];
};

struct PerObjectConstants
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
	LitSkullApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance }
	{
		mMainWndCaption = L"LitSkull Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		auto I = DirectX::XMMATRIX{DirectX::XMMatrixIdentity()};
		DirectX::XMStoreFloat4x4(&mGridWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

		auto boxScale = DirectX::XMMATRIX{DirectX::XMMatrixScaling(3.0f, 1.0f, 3.0f)};
		auto boxOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f)};
		DirectX::XMStoreFloat4x4(&mBoxWorld, DirectX::XMMatrixMultiply(boxScale, boxOffset));

		auto skullScale = DirectX::XMMATRIX{DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f)};
		auto skullOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f)};
		DirectX::XMStoreFloat4x4(&mSkullWorld, DirectX::XMMatrixMultiply(skullScale, skullOffset));

		for (int i = 0; i < 5; ++i)
		{
			DirectX::XMStoreFloat4x4(&mCylWorld[i * 2 + 0], DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
			DirectX::XMStoreFloat4x4(&mCylWorld[i * 2 + 1], DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

			DirectX::XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
			DirectX::XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
		}

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

		mGridMat.Ambient = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		mGridMat.Diffuse = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		mGridMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

		mCylinderMat.Ambient = DirectX::XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
		mCylinderMat.Diffuse = DirectX::XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
		mCylinderMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		mSphereMat.Ambient = DirectX::XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
		mSphereMat.Diffuse = DirectX::XMFLOAT4(0.2f, 0.4f, 0.6f, 1.0f);
		mSphereMat.Specular = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		Init();
	}

	void Init()
	{
		D3DApp::Init();

		// Must init Effects first since InputLayouts depend on shader signatures.
		//Effects::InitAll(md3dDevice);
		//InputLayouts::InitAll(md3dDevice.get());

		BuildShapeGeometryBuffers();
		BuildSkullGeometryBuffers();
		BuildShaders();
	}

	void OnResize()
	{
		D3DApp::OnResize();
		auto P = DirectX::XMMATRIX{DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f)};
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt)
	{
		// Convert Spherical to Cartesian coordinates.
		auto x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		auto z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		auto y = mRadius * std::cosf(mPhi);

		mEyePosW = DirectX::XMFLOAT3(x, y, z);

		// Build the view matrix.
		auto pos = DirectX::XMVECTOR{DirectX::XMVectorSet(x, y, z, 1.0f)};
		auto target = DirectX::XMVectorZero();
		auto up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		auto V = DirectX::XMMATRIX{DirectX::XMMatrixLookAtLH(pos, target, up)};
		DirectX::XMStoreFloat4x4(&mView, V);

		//
		// Switch the number of lights based on key presses.
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
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::LightSteelBlue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mPosNormal.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto stride = static_cast<std::uint32_t>(sizeof(PosNormal));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mProj) };
		auto viewProj = DirectX::XMMATRIX{ view * proj };

		// Set per frame constants.
		md3dImmediateContext->VSSetShader(mColorVS.get(), 0, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), 0, 0);
		auto perframe = PerFrameConstants{
			.gDirLights = {},
			.gEyePosW = mEyePosW,
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = static_cast<std::int32_t>(mLightCount),
			.gFogColor = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f),
		};
		for (auto i = 0u; i < mLightCount; ++i)
			perframe.gDirLights[i] = mDirLights[i];
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, 0, &perframe, 0, 0);
		
		auto shapesVertexBuffers = std::array{ mShapesVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(shapesVertexBuffers.size()), shapesVertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Draw the grid.
		auto perObject = PerObjectConstants{.gMaterial = mGridMat};
		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&mGridWorld);
		DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		DirectX::XMMATRIX worldViewProj = world * viewProj;
		DirectX::XMStoreFloat4x4(&perObject.gWorld, world);
		DirectX::XMStoreFloat4x4(&perObject.gWorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObject.gWorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&perObject.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, 0, &perObject, 0, 0);

		auto gridVSConstantBuffers = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(gridVSConstantBuffers.size()), gridVSConstantBuffers.data());
		auto gridPSConstantBuffers = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(gridPSConstantBuffers.size()), gridPSConstantBuffers.data());

		md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		//	// Draw the box.
		world = XMLoadFloat4x4(&mBoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;
		auto boxObject = PerObjectConstants{ .gMaterial = mBoxMat };
		DirectX::XMStoreFloat4x4(&boxObject.gWorld, world);
		DirectX::XMStoreFloat4x4(&boxObject.gWorldInvTranspose, worldInvTranspose);
		DirectX::XMStoreFloat4x4(&boxObject.gWorldViewProj, worldViewProj);
		DirectX::XMStoreFloat4x4(&boxObject.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, 0, &boxObject, 0, 0);

		md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

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

		HR(mSwapChain->Present(0, 0));
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
			auto dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			auto dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = std::clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			auto dx = 0.01f * static_cast<float>(x - mLastMousePos.x);
			auto dy = 0.01f * static_cast<float>(y - mLastMousePos.y);

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
		auto vertices = std::vector<PosNormal>(totalVertexCount);

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
			.ByteWidth = sizeof(PosNormal) * totalVertexCount,
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

		auto vertices = std::vector<PosNormal>(vcount);
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
			.ByteWidth = sizeof(PosNormal) * vcount,
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

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_VS.cso", &vertexShaderBytecode), "Failed to read vertex shader file.");

		auto hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		HR(hr, "Failed to create vertex shader.");

		BuildInputLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"FX/Basic_PS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");

		HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mColorPS), "Failed to create pixel shader.");
		pixelShaderBytecode.reset();

		auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameCbd, 0, &mPerFrameCB), "Failed to create constant buffer.");

		auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mPerObjectCB), "Failed to create constant buffer.");
	}

	void BuildInputLayout(D3D::ID3DBlob* vertexShaderBytecode)
	{
		// Create the vertex input layout.
		auto vertexDesc = std::array{
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

		auto hr = md3dDevice->CreateInputLayout(
			vertexDesc.data(),
			static_cast<std::uint32_t>(vertexDesc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mPosNormal
		);
		HR(hr, "Failed to create input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mShapesVB;
	ComPtr<D3D11::ID3D11Buffer> mShapesIB;

	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;
	ComPtr<D3D11::ID3D11InputLayout> mPosNormal;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;

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

