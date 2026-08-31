export module quaternionsdemo:app;
import std;
import shared;
import :animationhelper;

struct BasicVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};
static_assert(sizeof(BasicVertex) == 32);

struct PerFrameConstants
{
	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;
	float gFogStart;
	float gFogRange;
	std::uint32_t gLightCount;
	std::uint32_t gUseTexture;
	std::uint32_t gAlphaClip;
	std::uint32_t gFogEnabled;
	DirectX::XMFLOAT3 gPadding;
	DirectX::XMFLOAT4 gFogColor;
};
static_assert(sizeof(PerFrameConstants) == 256);

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldInvTranspose;
	DirectX::XMFLOAT4X4 gWorldViewProj;
	DirectX::XMFLOAT4X4 gTexTransform;
	Material gMaterial;
};
static_assert(sizeof(PerObjectConstants) == 320);

export class QuatApp : public D3DApp
{
public:
	QuatApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance, L"Quaternion Demo" }
	{
		mCam.Pitch(DirectX::XMConvertToRadians(25.0f));
		mCam.SetPosition(0.0f, 8.0f, -20.0f);

		DirectX::XMStoreFloat4x4(&mGridWorld, DirectX::XMMatrixIdentity());

		auto boxScale = DirectX::XMMatrixScaling(3.0f, 1.0f, 3.0f);
		auto boxOffset = DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f);
		DirectX::XMStoreFloat4x4(&mBoxWorld, DirectX::XMMatrixMultiply(boxScale, boxOffset));

		for (auto i = 0u; i < 5; ++i)
		{
			DirectX::XMStoreFloat4x4(
				&mCylWorld[i * 2 + 0],
				DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
			DirectX::XMStoreFloat4x4(
				&mCylWorld[i * 2 + 1],
				DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

			DirectX::XMStoreFloat4x4(
				&mSphereWorld[i * 2 + 0],
				DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
			DirectX::XMStoreFloat4x4(
				&mSphereWorld[i * 2 + 1],
				DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
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

		mCylinderMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		mSphereMat.Ambient = DirectX::XMFLOAT4(0.6f, 0.8f, 0.9f, 1.0f);
		mSphereMat.Diffuse = DirectX::XMFLOAT4(0.6f, 0.8f, 0.9f, 1.0f);
		mSphereMat.Specular = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		auto q0 = DirectX::XMQuaternionRotationAxis(
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
			DirectX::XMConvertToRadians(30.0f));
		auto q1 = DirectX::XMQuaternionRotationAxis(
			DirectX::XMVectorSet(1.0f, 1.0f, 2.0f, 0.0f),
			DirectX::XMConvertToRadians(45.0f));
		auto q2 = DirectX::XMQuaternionRotationAxis(
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
			DirectX::XMConvertToRadians(-30.0f));
		auto q3 = DirectX::XMQuaternionRotationAxis(
			DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
			DirectX::XMConvertToRadians(70.0f));

		mSkullAnimation.Keyframes.resize(5);
		mSkullAnimation.Keyframes[0].TimePos = 0.0f;
		mSkullAnimation.Keyframes[0].Translation = DirectX::XMFLOAT3(-7.0f, 0.0f, 0.0f);
		mSkullAnimation.Keyframes[0].Scale = DirectX::XMFLOAT3(0.25f, 0.25f, 0.25f);
		DirectX::XMStoreFloat4(&mSkullAnimation.Keyframes[0].RotationQuat, q0);

		mSkullAnimation.Keyframes[1].TimePos = 2.0f;
		mSkullAnimation.Keyframes[1].Translation = DirectX::XMFLOAT3(0.0f, 2.0f, 10.0f);
		mSkullAnimation.Keyframes[1].Scale = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
		DirectX::XMStoreFloat4(&mSkullAnimation.Keyframes[1].RotationQuat, q1);

		mSkullAnimation.Keyframes[2].TimePos = 4.0f;
		mSkullAnimation.Keyframes[2].Translation = DirectX::XMFLOAT3(7.0f, 0.0f, 0.0f);
		mSkullAnimation.Keyframes[2].Scale = DirectX::XMFLOAT3(0.25f, 0.25f, 0.25f);
		DirectX::XMStoreFloat4(&mSkullAnimation.Keyframes[2].RotationQuat, q2);

		mSkullAnimation.Keyframes[3].TimePos = 6.0f;
		mSkullAnimation.Keyframes[3].Translation = DirectX::XMFLOAT3(0.0f, 1.0f, -10.0f);
		mSkullAnimation.Keyframes[3].Scale = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
		DirectX::XMStoreFloat4(&mSkullAnimation.Keyframes[3].RotationQuat, q3);

		mSkullAnimation.Keyframes[4].TimePos = 8.0f;
		mSkullAnimation.Keyframes[4].Translation = DirectX::XMFLOAT3(-7.0f, 0.0f, 0.0f);
		mSkullAnimation.Keyframes[4].Scale = DirectX::XMFLOAT3(0.25f, 0.25f, 0.25f);
		DirectX::XMStoreFloat4(&mSkullAnimation.Keyframes[4].RotationQuat, q0);

		Init();
	}

	~QuatApp() override = default;

	void Init() override
	{
		D3DApp::Init();

		HR(DirectX::CreateDDSTextureFromFile(
			md3dDevice.get(), L"Textures/floor.dds", nullptr, &mFloorTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(
			md3dDevice.get(), L"Textures/stone.dds", nullptr, &mStoneTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(
			md3dDevice.get(), L"Textures/bricks.dds", nullptr, &mBrickTexSRV));

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
		if (Win32::GetAsyncKeyState('W') & 0x8000)
			mCam.Walk(10.0f * dt);
		if (Win32::GetAsyncKeyState('S') & 0x8000)
			mCam.Walk(-10.0f * dt);
		if (Win32::GetAsyncKeyState('A') & 0x8000)
			mCam.Strafe(-10.0f * dt);
		if (Win32::GetAsyncKeyState('D') & 0x8000)
			mCam.Strafe(10.0f * dt);

		mAnimTimePos += dt;
		if (mAnimTimePos >= mSkullAnimation.GetEndTime())
			mAnimTimePos = 0.0f;

		mSkullAnimation.Interpolate(mAnimTimePos, mSkullWorld);
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(),
			reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(),
			D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL },
			1.0f,
			0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mPixelShader.get(), nullptr, 0);

		auto stride = static_cast<std::uint32_t>(sizeof(BasicVertex));
		auto offset = 0u;

		mCam.UpdateViewMatrix();
		auto viewProj = DirectX::XMMATRIX{ mCam.ViewProj() };

		auto perFrameConstants = PerFrameConstants{
			.gDirLights = { mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = 3,
			.gUseTexture = 1,
			.gAlphaClip = 0,
			.gFogEnabled = 0,
			.gFogColor = DirectX::XMFLOAT4{ DirectX::Colors::Silver },
		};
		md3dImmediateContext->UpdateSubresource(
			mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mShapesVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);
		md3dImmediateContext->PSSetSamplers(
			0, 1, mSamplerState.GetAddressOf());

		auto constantBuffersVS = std::array{ mPerObject.get() };
		auto constantBuffersPS = std::array{ mPerFrame.get(), mPerObject.get() };
		md3dImmediateContext->VSSetConstantBuffers(
			1,
			static_cast<std::uint32_t>(constantBuffersVS.size()),
			constantBuffersVS.data());
		md3dImmediateContext->PSSetConstantBuffers(
			0,
			static_cast<std::uint32_t>(constantBuffersPS.size()),
			constantBuffersPS.data());

		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mGridWorld) };
			auto perObjectConstants = BuildPerObjectConstants(
				world,
				viewProj,
				DirectX::XMMatrixScaling(6.0f, 8.0f, 1.0f),
				mGridMat);
			md3dImmediateContext->UpdateSubresource(
				mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			md3dImmediateContext->PSSetShaderResources(
				0, 1, mFloorTexSRV.GetAddressOf());
			md3dImmediateContext->DrawIndexed(
				mGridIndexCount, mGridIndexOffset, mGridVertexOffset);
		}

		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mBoxWorld) };
			auto perObjectConstants = BuildPerObjectConstants(
				world,
				viewProj,
				DirectX::XMMatrixIdentity(),
				mBoxMat);
			md3dImmediateContext->UpdateSubresource(
				mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			md3dImmediateContext->PSSetShaderResources(
				0, 1, mStoneTexSRV.GetAddressOf());
			md3dImmediateContext->DrawIndexed(
				mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
		}

		for (auto i = 0u; i < 10; ++i)
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mCylWorld[i]) };
			auto perObjectConstants = BuildPerObjectConstants(
				world,
				viewProj,
				DirectX::XMMatrixIdentity(),
				mCylinderMat);
			md3dImmediateContext->UpdateSubresource(
				mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			md3dImmediateContext->PSSetShaderResources(
				0, 1, mBrickTexSRV.GetAddressOf());
			md3dImmediateContext->DrawIndexed(
				mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		for (auto i = 0u; i < 10; ++i)
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mSphereWorld[i]) };
			auto perObjectConstants = BuildPerObjectConstants(
				world,
				viewProj,
				DirectX::XMMatrixIdentity(),
				mSphereMat);
			md3dImmediateContext->UpdateSubresource(
				mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			md3dImmediateContext->PSSetShaderResources(
				0, 1, mStoneTexSRV.GetAddressOf());
			md3dImmediateContext->DrawIndexed(
				mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		{
			md3dImmediateContext->IASetVertexBuffers(
				0, 1, mSkullVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(
				mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

			perFrameConstants.gUseTexture = 0;
			md3dImmediateContext->UpdateSubresource(
				mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);
			auto nullSrv = std::array<D3D11::ID3D11ShaderResourceView*, 1>{};
			md3dImmediateContext->PSSetShaderResources(
				0,
				static_cast<std::uint32_t>(nullSrv.size()),
				nullSrv.data());

			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mSkullWorld) };
			auto perObjectConstants = BuildPerObjectConstants(
				world,
				viewProj,
				DirectX::XMMatrixIdentity(),
				mSkullMat);
			md3dImmediateContext->UpdateSubresource(
				mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);
		}

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
			auto dx = DirectX::XMConvertToRadians(
				0.25f * static_cast<float>(x - mLastMousePos.x));
			auto dy = DirectX::XMConvertToRadians(
				0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.Pitch(dy);
			mCam.RotateY(dx);
		}

		mLastMousePos = { x, y };
	}

private:
	static auto BuildPerObjectConstants(
		DirectX::FXMMATRIX world,
		DirectX::CXMMATRIX viewProj,
		DirectX::CXMMATRIX texTransform,
		const Material& material) -> PerObjectConstants
	{
		auto constants = PerObjectConstants{ .gMaterial = material };
		DirectX::XMStoreFloat4x4(&constants.gWorld, world);
		DirectX::XMStoreFloat4x4(
			&constants.gWorldInvTranspose,
			MathHelper::InverseTranspose(world));
		DirectX::XMStoreFloat4x4(
			&constants.gWorldViewProj,
			world * viewProj);
		DirectX::XMStoreFloat4x4(&constants.gTexTransform, texTransform);
		return constants;
	}

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

		mBoxVertexOffset = 0;
		mGridVertexOffset = static_cast<int>(box.Vertices.size());
		mSphereVertexOffset =
			mGridVertexOffset + static_cast<int>(grid.Vertices.size());
		mCylinderVertexOffset =
			mSphereVertexOffset + static_cast<int>(sphere.Vertices.size());

		mBoxIndexCount = static_cast<std::uint32_t>(box.Indices.size());
		mGridIndexCount = static_cast<std::uint32_t>(grid.Indices.size());
		mSphereIndexCount = static_cast<std::uint32_t>(sphere.Indices.size());
		mCylinderIndexCount = static_cast<std::uint32_t>(cylinder.Indices.size());

		mBoxIndexOffset = 0;
		mGridIndexOffset = mBoxIndexCount;
		mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
		mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

		auto totalVertexCount = static_cast<std::uint32_t>(
			box.Vertices.size() +
			grid.Vertices.size() +
			sphere.Vertices.size() +
			cylinder.Vertices.size());
		auto totalIndexCount =
			mBoxIndexCount +
			mGridIndexCount +
			mSphereIndexCount +
			mCylinderIndexCount;

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
			.ByteWidth = static_cast<std::uint32_t>(
				sizeof(BasicVertex) * totalVertexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = vertices.data(),
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

		auto indices = std::vector<std::uint32_t>{};
		indices.reserve(totalIndexCount);
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
		indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
		indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
		indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(
				sizeof(std::uint32_t) * totalIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = indices.data(),
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
	}

	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream{ "Models/skull.txt" };
		if (not fin)
			throw std::runtime_error{ "Models/skull.txt not found." };

		auto vertexCount = 0u;
		auto triangleCount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vertexCount;
		fin >> ignore >> triangleCount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<BasicVertex>(vertexCount);
		for (auto& vertex : vertices)
		{
			fin >> vertex.Pos.x >> vertex.Pos.y >> vertex.Pos.z;
			fin >> vertex.Normal.x >> vertex.Normal.y >> vertex.Normal.z;
		}

		fin >> ignore >> ignore >> ignore;

		mSkullIndexCount = 3 * triangleCount;
		auto indices = std::vector<std::uint32_t>(mSkullIndexCount);
		for (auto i = 0u; i < triangleCount; ++i)
		{
			fin >>
				indices[i * 3 + 0] >>
				indices[i * 3 + 1] >>
				indices[i * 3 + 2];
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(
				sizeof(BasicVertex) * vertexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = vertices.data(),
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(
				sizeof(std::uint32_t) * mSkullIndexCount),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = indices.data(),
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(
				L"Shaders/BasicVS.cso",
				&vertexShaderBytecode),
			"Failed to read vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				vertexShaderBytecode->GetBufferPointer(),
				vertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mVertexShader),
			"Failed to create vertex shader.");
		BuildInputLayout(vertexShaderBytecode.get());

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		HR(
			D3D::D3DReadFileToBlob(
				L"Shaders/BasicPS.cso",
				&pixelShaderBytecode),
			"Failed to read pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(),
				pixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mPixelShader),
			"Failed to create pixel shader.");

		auto perFrameDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(PerFrameConstants)),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(
			md3dDevice->CreateBuffer(&perFrameDesc, nullptr, &mPerFrame),
			"Failed to create per-frame constant buffer.");

		auto perObjectDesc = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(PerObjectConstants)),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(
			md3dDevice->CreateBuffer(&perObjectDesc, nullptr, &mPerObject),
			"Failed to create per-object constant buffer.");

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
		HR(
			md3dDevice->CreateSamplerState(&samplerDesc, &mSamplerState),
			"Failed to create sampler state.");
	}

	void BuildInputLayout(D3D::ID3DBlob* vertexShaderBytecode)
	{
		auto inputElements = std::array{
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass =
					D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass =
					D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 24,
				.InputSlotClass =
					D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
		};
		HR(
			md3dDevice->CreateInputLayout(
				inputElements.data(),
				static_cast<std::uint32_t>(inputElements.size()),
				vertexShaderBytecode->GetBufferPointer(),
				vertexShaderBytecode->GetBufferSize(),
				&mInputLayout),
			"Failed to create input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mShapesVB;
	ComPtr<D3D11::ID3D11Buffer> mShapesIB;

	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;

	ComPtr<D3D11::ID3D11ShaderResourceView> mFloorTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickTexSRV;

	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	DirectionalLight mDirLights[3];
	Material mGridMat;
	Material mBoxMat;
	Material mCylinderMat;
	Material mSphereMat;
	Material mSkullMat;

	DirectX::XMFLOAT4X4 mSphereWorld[10];
	DirectX::XMFLOAT4X4 mCylWorld[10];
	DirectX::XMFLOAT4X4 mBoxWorld;
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mSkullWorld;

	int mBoxVertexOffset = 0;
	int mGridVertexOffset = 0;
	int mSphereVertexOffset = 0;
	int mCylinderVertexOffset = 0;

	std::uint32_t mBoxIndexOffset = 0;
	std::uint32_t mGridIndexOffset = 0;
	std::uint32_t mSphereIndexOffset = 0;
	std::uint32_t mCylinderIndexOffset = 0;

	std::uint32_t mBoxIndexCount = 0;
	std::uint32_t mGridIndexCount = 0;
	std::uint32_t mSphereIndexCount = 0;
	std::uint32_t mCylinderIndexCount = 0;
	std::uint32_t mSkullIndexCount = 0;

	Camera mCam;

	float mAnimTimePos = 0.0f;
	BoneAnimation mSkullAnimation;

	Win32::POINT mLastMousePos{};
};
