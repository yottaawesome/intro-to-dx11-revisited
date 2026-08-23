export module cubemap:app;
import std;
import shared;
import :sky;

struct BasicVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

struct PerFrameConstants
{
	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;
	float gFogStart;
	float gFogRange;
	std::uint32_t gLightCount;
	bool32 gUseTexture;
	bool32 gAlphaClip;
	bool32 gFogEnabled;
	bool32 gReflectionEnabled;
	DirectX::XMFLOAT2 gPadding;
	DirectX::XMFLOAT4 gFogColor;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};

export class CubeMapApp : public D3DApp
{
public:
	CubeMapApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance, L"CubeMap Demo" }
	{
		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&mGridWorld, I);

		DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(3.0f, 1.0f, 3.0f);
		DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f);
		DirectX::XMStoreFloat4x4(&mBoxWorld, DirectX::XMMatrixMultiply(boxScale, boxOffset));

		DirectX::XMMATRIX skullScale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);
		DirectX::XMMATRIX skullOffset = DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f);
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

		mGridMat.Ambient = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mGridMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mCylinderMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSphereMat.Ambient = DirectX::XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Diffuse = DirectX::XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Specular = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mSkullMat.Reflect = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		// Must init Effects first since InputLayouts depend on shader signatures.
		mSky.emplace(md3dDevice.get(), L"Textures/grasscube1024.dds", 5000.0f);
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/floor.dds", nullptr, &mFloorTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/stone.dds", nullptr, &mStoneTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/bricks.dds", nullptr, &mBrickTexSRV));

		BuildShapeGeometryBuffers();
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

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mPixelShader.get(), nullptr, 0);

		auto stride = static_cast<std::uint32_t>(sizeof(BasicVertex));
		auto offset = 0u;

		mCam.UpdateViewMatrix();

		auto view = DirectX::XMMATRIX{ mCam.View() };
		auto proj = DirectX::XMMATRIX{ mCam.Proj() };
		auto viewProj = DirectX::XMMATRIX{ mCam.ViewProj() };

		// Set per frame constants.
		auto perFrameConstants = PerFrameConstants{
			.gDirLights = { mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = false,
			.gPadding = DirectX::XMFLOAT2{},
			.gFogColor = DirectX::XMFLOAT4(DirectX::Colors::Silver),
		};
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		//
		// Draw the grid, cylinders, and box without any cubemap reflection.
		// 
		md3dImmediateContext->IASetVertexBuffers(0, 1, mShapesVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);
		md3dImmediateContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		// Draw the grid.
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mGridWorld) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mGridMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(6.0f, 8.0f, 1.0f));
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mFloorTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			auto perObjectConstantBuffersVS = std::array{ mPerObject.get() };
			auto perObjectConstantBuffersPS = std::array{ mPerFrame.get(), mPerObject.get() };
			md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(perObjectConstantBuffersVS.size()), perObjectConstantBuffersVS.data());
			md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(perObjectConstantBuffersPS.size()), perObjectConstantBuffersPS.data());
			md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);
		}

		// Draw the box.
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mBoxWorld) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mBoxMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mStoneTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
		}

		//	Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mCylWorld[i]) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * view * proj };
			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mCylinderMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mBrickTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		//
		// Draw the spheres with cubemap reflection.
		//
		// Draw the spheres.
		perFrameConstants.gReflectionEnabled = true;
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);
		for (int i = 0; i < 10; ++i)
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mSphereWorld[i]) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };
			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mSphereMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mStoneTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		// Draw the skull.
		{
			perFrameConstants.gReflectionEnabled = true;
			perFrameConstants.gUseTexture = false;
			md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

			md3dImmediateContext->IASetVertexBuffers(0, 1, mSkullVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mSkullWorld) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mSkullMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

			auto nullSrv = std::array<D3D11::ID3D11ShaderResourceView*, 1>{ };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(nullSrv.size()), nullSrv.data());
			md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);
		}

		mSky->Draw(md3dImmediateContext.get(), mCam);

		// restore default states, as the SkyFX changes them in the effect file.
		md3dImmediateContext->RSSetState(0);
		md3dImmediateContext->OMSetDepthStencilState(0, 0);

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

		mLastMousePos = { x, y };
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
			box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size());
		auto totalIndexCount = static_cast<std::uint32_t>(
			mBoxIndexCount + mGridIndexCount + mSphereIndexCount + mCylinderIndexCount);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		auto vertices = std::vector<BasicVertex>(totalVertexCount);

		auto k = 0u;
		for (auto i = 0ull; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
			vertices[k].Tex = box.Vertices[i].TexC;
		}

		for (auto i = 0ull; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Normal = grid.Vertices[i].Normal;
			vertices[k].Tex = grid.Vertices[i].TexC;
		}

		for (auto i = 0ull; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Normal = sphere.Vertices[i].Normal;
			vertices[k].Tex = sphere.Vertices[i].TexC;
		}

		for (auto i = 0ull; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Normal = cylinder.Vertices[i].Normal;
			vertices[k].Tex = cylinder.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(BasicVertex) * totalVertexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &vertices[0]};
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
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * totalIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
	}

	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream("Models/skull.txt");
		if (not fin)
			throw std::runtime_error{"Models/skull.txt not found."};

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<BasicVertex>(vcount);
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
			.ByteWidth = static_cast<std::uint32_t>(sizeof(BasicVertex) * vcount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &vertices[0]};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * mSkullIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
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

		// constant buffers basic32
		auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerFrameConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrameCbd, 0, &mPerFrame), "Failed to create constant buffer.");

		auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mPerObject), "Failed to create constant buffer.");

		// samplers
		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_ANISOTROPIC,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 4,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER,
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(md3dDevice->CreateSamplerState(&samplerDesc, &mSamplerState), "Failed to create sampler state.");
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
			}
		};
		HR(md3dDevice->CreateInputLayout(
			basic32Desc.data(),
			static_cast<std::uint32_t>(basic32Desc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mInputLayout),
			"Failed to create basic32 input layout.");
	}

private:
	std::optional<Sky> mSky;

	ComPtr<D3D11::ID3D11Buffer> mShapesVB;
	ComPtr<D3D11::ID3D11Buffer> mShapesIB;
	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;
	ComPtr<D3D11::ID3D11Buffer> mSkySphereVB;
	ComPtr<D3D11::ID3D11Buffer> mSkySphereIB;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	ComPtr<D3D11::ID3D11ShaderResourceView> mFloorTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickTexSRV;

	DirectionalLight mDirLights[3];
	std::uint32_t mLightCount = 3;
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

	std::uint32_t mSkullIndexCount;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};