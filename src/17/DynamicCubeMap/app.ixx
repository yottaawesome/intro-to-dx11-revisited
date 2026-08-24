export module dynamiccubemap:app;
import std;
import shared;
import :sky;

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
	bool32 gUseTexture;
	bool32 gAlphaClip;
	bool32 gFogEnabled;
	bool32 gReflectionEnabled;
	DirectX::XMFLOAT2 gPadding;
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

export class DynamicCubeMapApp : public D3DApp
{
public:
	DynamicCubeMapApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance, L"Dynamic CubeMap Demo" }
	{
		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&mGridWorld, I);

		DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(3.0f, 1.0f, 3.0f);
		DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f);
		DirectX::XMStoreFloat4x4(&mBoxWorld, DirectX::XMMatrixMultiply(boxScale, boxOffset));

		DirectX::XMMATRIX centerSphereScale = DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f);
		DirectX::XMMATRIX centerSphereOffset = DirectX::XMMatrixTranslation(0.0f, 2.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&mCenterSphereWorld, DirectX::XMMatrixMultiply(centerSphereScale, centerSphereOffset));

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

		mSphereMat.Ambient = DirectX::XMFLOAT4(0.6f, 0.8f, 1.0f, 1.0f);
		mSphereMat.Diffuse = DirectX::XMFLOAT4(0.6f, 0.8f, 1.0f, 1.0f);
		mSphereMat.Specular = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mSkullMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCenterSphereMat.Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mCenterSphereMat.Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mCenterSphereMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mCenterSphereMat.Reflect = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		mSky.emplace(md3dDevice.get(), L"Textures/sunsetcube1024.dds", 5000.0f);

		HR(DirectX::CreateDDSTextureFromFile(
			md3dDevice.get(), L"Textures/floor.dds", nullptr, &mFloorTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(
			md3dDevice.get(), L"Textures/stone.dds", nullptr, &mStoneTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(
			md3dDevice.get(), L"Textures/bricks.dds", nullptr, &mBrickTexSRV));

		BuildDynamicCubeMapViews();
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

		//
		// Animate the skull around the center sphere.
		//
		DirectX::XMMATRIX skullScale = DirectX::XMMatrixScaling(0.2f, 0.2f, 0.2f);
		DirectX::XMMATRIX skullOffset = DirectX::XMMatrixTranslation(3.0f, 2.0f, 0.0f);
		DirectX::XMMATRIX skullLocalRotate = DirectX::XMMatrixRotationY(2.0f * mTimer.TotalTime());
		DirectX::XMMATRIX skullGlobalRotate = DirectX::XMMatrixRotationY(0.5f * mTimer.TotalTime());
		DirectX::XMStoreFloat4x4(&mSkullWorld, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);
		mCam.UpdateViewMatrix();
	}

	void DrawScene() override
	{
		D3D11::ID3D11RenderTargetView* renderTargets[1];

		// Generate the cube map.
		md3dImmediateContext->RSSetViewports(1, &mCubeMapViewport);
		for (int i = 0; i < 6; ++i)
		{
			// Clear cube map face and depth buffer.
			md3dImmediateContext->ClearRenderTargetView(
				mDynamicCubeMapRTV[i].get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
			md3dImmediateContext->ClearDepthStencilView(
				mDynamicCubeMapDSV.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

			// Bind cube map face as render target.
			renderTargets[0] = mDynamicCubeMapRTV[i].get();
			md3dImmediateContext->OMSetRenderTargets(1, renderTargets, mDynamicCubeMapDSV.get());

			// Draw the scene with the exception of the center sphere to this cube map face.
			DrawScene(mCubeMapCamera[i], false);
		}

		// Restore old viewport and render targets.
		md3dImmediateContext->RSSetViewports(1, &mScreenViewport);
		renderTargets[0] = mRenderTargetView.get();
		md3dImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView.get());

		// Have hardware generate lower mipmap levels of cube map.
		md3dImmediateContext->GenerateMips(mDynamicCubeMapSRV.get());

		// Now draw the scene as normal, but with the center sphere.
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		DrawScene(mCam, true);

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
	void DrawScene(const Camera& camera, bool drawCenterSphere)
	{
		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mPixelShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		auto stride = static_cast<std::uint32_t>(sizeof(BasicVertex));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{ camera.View() };
		auto proj = DirectX::XMMATRIX{ camera.Proj() };
		auto viewProj = DirectX::XMMATRIX{ camera.ViewProj() };

		// Set per frame constants.
		auto perFrameConstants = PerFrameConstants{
			.gDirLights = { mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = false,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = drawCenterSphere,
			.gPadding = { 0.0f, 0.0f },
			.gFogColor = { 0.7f, 0.7f, 0.7f, 1.0f },
		};
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		//
		// Draw the skull.
		//
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mSkullVB.GetAddressOf(), &stride, &offset);
			md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

			auto world = DirectX::XMLoadFloat4x4(&mSkullWorld);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;

			auto perObjectConstants = PerObjectConstants{ .gMaterial = mSkullMat, };
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			auto srv = std::array<D3D11::ID3D11ShaderResourceView*, 2>{ nullptr, mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto constantBuffersVS = std::array{ mPerObject.get() };
			auto constantBuffersPS = std::array{ mPerFrame.get(), mPerObject.get() };
			md3dImmediateContext->VSSetConstantBuffers(
				1, static_cast<std::uint32_t>(constantBuffersVS.size()), constantBuffersVS.data());
			md3dImmediateContext->PSSetConstantBuffers(
				0, static_cast<std::uint32_t>(constantBuffersPS.size()), constantBuffersPS.data());

			md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);
		}

		md3dImmediateContext->IASetVertexBuffers(0, 1, mShapesVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);
		perFrameConstants.gUseTexture = true;
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		//
		// Draw the grid, cylinders, spheres and box without any cubemap reflection.
		// 
		// Draw the grid.
		{
			auto world = DirectX::XMLoadFloat4x4(&mGridWorld);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;
			
			auto perObjectConstants = PerObjectConstants{ .gMaterial = mGridMat, };
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(6.0f, 8.0f, 1.0f));
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mFloorTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);
		}

		//	Draw the box.
		{
			auto world = DirectX::XMLoadFloat4x4(&mBoxWorld);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;
			auto perObjectConstants = PerObjectConstants{ .gMaterial = mBoxMat, };
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mStoneTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
		}

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mCylWorld[i]) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };
			auto perObjectConstants = PerObjectConstants{ .gMaterial = mCylinderMat, };
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mBrickTexSRV.get(), mSky->CubeMapSRV() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		//	Draw the spheres.
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

		//
		// Draw the center sphere with the dynamic cube map.
		//
		if (drawCenterSphere)
		{
			perFrameConstants.gUseTexture = false;
			md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

			// Draw the center sphere.
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mCenterSphereWorld) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };
			auto perObjectConstants = PerObjectConstants{
				.gMaterial = mCenterSphereMat,
			};
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
			auto srv = std::array{ mStoneTexSRV.get(), mDynamicCubeMapSRV.get() };
			md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
			md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		mSky->Draw(md3dImmediateContext.get(), camera);

		// restore default states, as the SkyFX changes them in the effect file.
		md3dImmediateContext->RSSetState(0);
		md3dImmediateContext->OMSetDepthStencilState(0, 0);
	}
	
	void BuildCubeFaceCamera(float x, float y, float z)
	{
		// Generate the cube map about the given position.
		auto center = DirectX::XMFLOAT3{x, y, z};
		auto worldUp = DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f};

		// Look along each coordinate axis.
		auto targets = std::array{
			DirectX::XMFLOAT3{x + 1.0f, y, z}, // +X
			DirectX::XMFLOAT3{x - 1.0f, y, z}, // -X
			DirectX::XMFLOAT3{x, y + 1.0f, z}, // +Y
			DirectX::XMFLOAT3{x, y - 1.0f, z}, // -Y
			DirectX::XMFLOAT3{x, y, z + 1.0f}, // +Z
			DirectX::XMFLOAT3{x, y, z - 1.0f}  // -Z
		};

		// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
		// are looking down +Y or -Y, so we need a different "up" vector.
		auto ups = std::array{
			DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f},  // +X
			DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f},  // -X
			DirectX::XMFLOAT3{0.0f, 0.0f, -1.0f}, // +Y
			DirectX::XMFLOAT3{0.0f, 0.0f, +1.0f}, // -Y
			DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f},	 // +Z
			DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f}	 // -Z
		};

		for (auto i = 0u; i < targets.size(); ++i)
		{
			mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
			mCubeMapCamera[i].SetLens(0.5f * DirectX::Pi, 1.0f, 0.1f, 1000.0f);
			mCubeMapCamera[i].UpdateViewMatrix();
		}
	}
	
	void BuildDynamicCubeMapViews()
	{
		//
		// Cubemap is a special texture array with 6 elements.
		//
		auto texDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = CubeMapSize,
			.Height = CubeMapSize,
			.MipLevels = 0,
			.ArraySize = 6,
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc{.Count = 1, .Quality = 0},
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = 0,
			.MiscFlags = D3D11::D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11::D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE
		};

		auto cubeTex = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &cubeTex));

		//
		// Create a render target view to each cube map face 
		// (i.e., each element in the texture array).
		// 

		auto rtvDesc = D3D11::D3D11_RENDER_TARGET_VIEW_DESC{
			.Format = texDesc.Format,
			.ViewDimension = D3D11::D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DARRAY,
			.Texture2DArray = { .MipSlice = 0, .ArraySize = 1 }
		};

		for (int i = 0; i < 6; ++i)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			HR(md3dDevice->CreateRenderTargetView(cubeTex.get(), &rtvDesc, &mDynamicCubeMapRTV[i]));
		}

		//
		// Create a shader resource view to the cube map.
		//

		auto srvDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
			.Format = texDesc.Format,
			.ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBE,
			.TextureCube = { .MostDetailedMip = 0, .MipLevels = std::numeric_limits<std::uint32_t>::max() }
		};

		HR(md3dDevice->CreateShaderResourceView(cubeTex.get(), &srvDesc, &mDynamicCubeMapSRV));

		cubeTex.reset();

		//
		// We need a depth texture for rendering the scene into the cubemap
		// that has the same resolution as the cubemap faces.  
		//
		auto depthTexDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = CubeMapSize,
			.Height = CubeMapSize,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT,
			.SampleDesc{.Count = 1, .Quality = 0},
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};

		auto depthTex = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(md3dDevice->CreateTexture2D(&depthTexDesc, 0, &depthTex));

		// Create the depth stencil view for the entire cube
		auto dsvDesc = D3D11::D3D11_DEPTH_STENCIL_VIEW_DESC{
			.Format = depthTexDesc.Format,
			.ViewDimension = D3D11::D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D,
			.Flags = 0,
			.Texture2D = { .MipSlice = 0 }
		};
		HR(md3dDevice->CreateDepthStencilView(depthTex.get(), &dsvDesc, &mDynamicCubeMapDSV));

		depthTex.reset();

		//
		// Viewport for drawing into cubemap.
		// 

		mCubeMapViewport.TopLeftX = 0.0f;
		mCubeMapViewport.TopLeftY = 0.0f;
		mCubeMapViewport.Width = (float)CubeMapSize;
		mCubeMapViewport.Height = (float)CubeMapSize;
		mCubeMapViewport.MinDepth = 0.0f;
		mCubeMapViewport.MaxDepth = 1.0f;
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
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
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
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
	}

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
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
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
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &indices[0] };
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
		{
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
		}

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
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	ComPtr<D3D11::ID3D11ShaderResourceView> mFloorTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickTexSRV;
	ComPtr<D3D11::ID3D11DepthStencilView> mDynamicCubeMapDSV;
	ComPtr<D3D11::ID3D11RenderTargetView> mDynamicCubeMapRTV[6];
	ComPtr<D3D11::ID3D11ShaderResourceView> mDynamicCubeMapSRV;
	D3D11::D3D11_VIEWPORT mCubeMapViewport;

	static constexpr auto CubeMapSize = 256;

	DirectionalLight mDirLights[3];
	Material mGridMat;
	Material mBoxMat;
	Material mCylinderMat;
	Material mSphereMat;
	Material mSkullMat;
	Material mCenterSphereMat;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mSphereWorld[10];
	DirectX::XMFLOAT4X4 mCylWorld[10];
	DirectX::XMFLOAT4X4 mBoxWorld;
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mSkullWorld;
	DirectX::XMFLOAT4X4 mCenterSphereWorld;

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
	std::uint32_t mLightCount = 3;

	Camera mCam;
	Camera mCubeMapCamera[6];

	Win32::POINT mLastMousePos{};
};