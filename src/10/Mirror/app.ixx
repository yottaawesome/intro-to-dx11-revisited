export module mirrordemo:app;
import std;
import shared;
import :renderstates;

// Basic 32-byte vertex structure.
struct Basic32
{
	DirectX::XMFLOAT3 Pos = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 Normal = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT2 Tex = {0.0f, 0.0f};
};

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

struct PerFrameConstants
{
	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;
	float gFogStart;
	float gFogRange;
	int gLightCount;
	int gUseTexture;
	int gAlphaClip;
	int gFogEnabled;
	int pad[3];
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

export class MirrorApp : public D3DApp
{
public:
	MirrorApp(Win32::HINSTANCE hInstance)
		: D3DApp{hInstance}
	{
		mMainWndCaption = L"Mirror Demo";
		mEnable4xMsaa = false;

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&mRoomWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

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

		mRoomMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mRoomMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mRoomMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

		// Reflected material is transparent so it blends into mirror.
		mMirrorMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mMirrorMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
		mMirrorMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

		mShadowMat.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mShadowMat.Diffuse = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
		mShadowMat.Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

		Init();
	}

	void Init()
	{
		D3DApp::Init();

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/checkboard.dds", nullptr, &mFloorDiffuseMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/brick01.dds", nullptr, &mWallDiffuseMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(), L"Textures/ice.dds", nullptr, &mMirrorDiffuseMapSRV));

		BuildRoomGeometryBuffers();
		BuildSkullGeometryBuffers();
		BuildShaders();

		mRenderStates = { md3dDevice.get() };
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
		auto target = DirectX::XMVECTOR{DirectX::XMVectorZero()};
		auto up = DirectX::XMVECTOR{DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)};

		auto V = DirectX::XMMATRIX{DirectX::XMMatrixLookAtLH(pos, target, up)};
		DirectX::XMStoreFloat4x4(&mView, V);

		//
		// Switch the render mode based in key input.
		//
		if (Win32::GetAsyncKeyState('1') & 0x8000)
			mRenderOptions = RenderOptions::Lighting;
		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mRenderOptions = RenderOptions::Textures;
		if (Win32::GetAsyncKeyState('3') & 0x8000)
			mRenderOptions = RenderOptions::TexturesAndFog;


		//
		// Allow user to move box.
		//
		if (Win32::GetAsyncKeyState('A') & 0x8000)
			mSkullTranslation.x -= 1.0f * dt;
		if (Win32::GetAsyncKeyState('D') & 0x8000)
			mSkullTranslation.x += 1.0f * dt;
		if (Win32::GetAsyncKeyState('W') & 0x8000)
			mSkullTranslation.y += 1.0f * dt;
		if (Win32::GetAsyncKeyState('S') & 0x8000)
			mSkullTranslation.y -= 1.0f * dt;

		// Don't let user move below ground plane.
		mSkullTranslation.y = std::max(mSkullTranslation.y, 0.0f);

		// Update the new world matrix.
		auto skullRotate = DirectX::XMMATRIX{DirectX::XMMatrixRotationY(0.5f * MathHelper::Pi)};
		auto skullScale = DirectX::XMMATRIX{DirectX::XMMatrixScaling(0.45f, 0.45f, 0.45f)};
		auto skullOffset = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z)};
		DirectX::XMStoreFloat4x4(&mSkullWorld, skullRotate * skullScale * skullOffset);
	}

	void DrawScene()
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Black));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mBasic32.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(mColorVS.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mColorPS.get(), nullptr, 0);
		auto samplers = std::array{ mSamplerState.get() };
		md3dImmediateContext->PSSetSamplers(0, static_cast<std::uint32_t>(samplers.size()), samplers.data());

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		auto stride = static_cast<std::uint32_t>(sizeof(Basic32));
		auto offset = 0u;

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto viewProj = DirectX::XMMATRIX{view * proj};

		// Set per frame constants.
		auto perFrame = PerFrameConstants{
			.gEyePosW = mEyePosW,
			.gFogStart = 2.0f,
			.gFogRange = 40.0f,
			.gLightCount = mLightCount,
			.gUseTexture = (mRenderOptions == RenderOptions::Lighting) ? 0 : 1,
			.gFogEnabled = (mRenderOptions == RenderOptions::TexturesAndFog) ? 1 : 0,
			.gFogColor = DirectX::XMFLOAT4(DirectX::Colors::Black),
		};
		std::copy(std::begin(mDirLights), std::end(mDirLights), std::begin(perFrame.gDirLights));
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, nullptr, &perFrame, 0, 0);

		// Skull doesn't have texture coordinates, so we can't texture it.
		//
		// Draw the floor and walls to the back buffer as normal.
		//
		auto roomVertexBuffers = std::array{ mRoomVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(roomVertexBuffers.size()), roomVertexBuffers.data(), &stride, &offset);

		// Set per object constants.
		auto roomWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mRoomWorld)};
		auto roomWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(roomWorld)};
		auto roomWorldViewProj = DirectX::XMMATRIX{roomWorld * viewProj};
		auto perObjectRoom = PerObjectConstants{
			.gMaterial = mRoomMat,
		};
		DirectX::XMStoreFloat4x4(&perObjectRoom.gWorld, roomWorld);
		DirectX::XMStoreFloat4x4(&perObjectRoom.gWorldInvTranspose, roomWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObjectRoom.gWorldViewProj, roomWorldViewProj);
		DirectX::XMStoreFloat4x4(&perObjectRoom.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &perObjectRoom, 0, 0);
		auto worldSRVs = std::array{ mFloorDiffuseMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(worldSRVs.size()), worldSRVs.data());
		auto roomConstantBuffersVS = std::array{ mPerObjectCB.get() };
		md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(roomConstantBuffersVS.size()), roomConstantBuffersVS.data());
		auto roomConstantBuffersPS = std::array{ mPerFrameCB.get(), mPerObjectCB.get() };
		md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(roomConstantBuffersPS.size()), roomConstantBuffersPS.data());

		// Floor
		md3dImmediateContext->Draw(6, 0);

		// Wall
		auto wallSRVs = std::array{ mWallDiffuseMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(wallSRVs.size()), wallSRVs.data());
		md3dImmediateContext->Draw(18, 6);

		//
		// Draw the skull to the back buffer as normal.
		//
		auto skullVBs = std::array{ mSkullVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(skullVBs.size()), skullVBs.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto skullWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mSkullWorld)};
		auto skullWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(skullWorld)};
		auto skullWorldViewProj = DirectX::XMMATRIX{skullWorld * viewProj};
		auto perObjectSkull = PerObjectConstants{
			.gMaterial = mSkullMat,
		};
		DirectX::XMStoreFloat4x4(&perObjectSkull.gWorld, skullWorld);
		DirectX::XMStoreFloat4x4(&perObjectSkull.gWorldInvTranspose, skullWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObjectSkull.gWorldViewProj, skullWorldViewProj);
		DirectX::XMStoreFloat4x4(&perObjectSkull.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &perObjectSkull, 0, 0);
		md3dImmediateContext->PSSetShaderResources(0, 0, nullptr);
		md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);
		

		//
		// Draw the mirror to stencil buffer only.
		//

		auto mirrorVBs = std::array{ mRoomVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(mirrorVBs.size()), mirrorVBs.data(), &stride, &offset);

		// Set per object constants.
		auto mirrorRoomWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mRoomWorld)};
		auto mirrorRoomWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(mirrorRoomWorld)};
		auto mirrorRoomWorldViewProj = DirectX::XMMATRIX{mirrorRoomWorld * viewProj};
		auto mirrorRoomPerObject = PerObjectConstants{
			.gMaterial = mMirrorMat,
		};
		DirectX::XMStoreFloat4x4(&mirrorRoomPerObject.gWorld, mirrorRoomWorld);
		DirectX::XMStoreFloat4x4(&mirrorRoomPerObject.gWorldInvTranspose, mirrorRoomWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&mirrorRoomPerObject.gWorldViewProj, mirrorRoomWorldViewProj);
		DirectX::XMStoreFloat4x4(&mirrorRoomPerObject.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &mirrorRoomPerObject, 0, 0);

		// Do not write to render target.
		md3dImmediateContext->OMSetBlendState(mRenderStates.NoRenderTargetWritesBS.get(), blendFactor, 0xffffffff);

		// Render visible mirror pixels to stencil buffer.
		// Do not write mirror depth to depth buffer at this point, otherwise it will occlude the reflection.
		md3dImmediateContext->OMSetDepthStencilState(mRenderStates.MarkMirrorDSS.get(), 1);

		md3dImmediateContext->Draw(6, 24);

		// Restore states.
		md3dImmediateContext->OMSetDepthStencilState(0, 0);
		md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);


		//
		// Draw the skull reflection.
		//
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(skullVBs.size()), skullVBs.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto mirrorPlane = DirectX::XMVECTOR{DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)}; // xy plane
		auto R = DirectX::XMMATRIX{DirectX::XMMatrixReflect(mirrorPlane)};
		auto mirrorSkullWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mSkullWorld) * R};
		auto mirrorSkullWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(mirrorSkullWorld)};
		auto mirrorSkullWorldViewProj = DirectX::XMMATRIX{mirrorSkullWorld * view * proj};
		auto mirrorSkullPerObject = PerObjectConstants{
			.gMaterial = mSkullMat,
		};
		DirectX::XMStoreFloat4x4(&mirrorSkullPerObject.gWorld, mirrorSkullWorld);
		DirectX::XMStoreFloat4x4(&mirrorSkullPerObject.gWorldInvTranspose, mirrorSkullWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&mirrorSkullPerObject.gWorldViewProj, mirrorSkullWorldViewProj);
		DirectX::XMStoreFloat4x4(&mirrorSkullPerObject.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &mirrorSkullPerObject, 0, 0);

		// Cache the old light directions, and reflect the light directions.
		DirectX::XMFLOAT3 oldLightDirections[3];
		for (int i = 0; i < 3; ++i)
		{
			oldLightDirections[i] = mDirLights[i].Direction;
			auto lightDir = DirectX::XMVECTOR{DirectX::XMLoadFloat3(&mDirLights[i].Direction)};
			auto reflectedLightDir = DirectX::XMVECTOR{ DirectX::XMVector3TransformNormal(lightDir, R)};
			DirectX::XMStoreFloat3(&mDirLights[i].Direction, reflectedLightDir);
		}
		perFrame.gLightCount = mLightCount;
		md3dImmediateContext->UpdateSubresource(mPerFrameCB.get(), 0, nullptr, &perFrame, 0, 0);

		// Cull clockwise triangles for reflection.
		md3dImmediateContext->RSSetState(mRenderStates.CullClockwiseRS.get());

		// Only draw reflection into visible mirror pixels as marked by the stencil buffer. 
		md3dImmediateContext->OMSetDepthStencilState(mRenderStates.DrawReflectionDSS.get(), 1);
		md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// Restore default states.
		md3dImmediateContext->RSSetState(0);
		md3dImmediateContext->OMSetDepthStencilState(0, 0);

		// Restore light directions.
		for (int i = 0; i < 3; ++i)
		{
			mDirLights[i].Direction = oldLightDirections[i];
		}

		//
		// Draw the mirror to the back buffer as usual but with transparency
		// blending so the reflection shows through.
		// 

		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(roomVertexBuffers.size()), roomVertexBuffers.data(), &stride, &offset);

		// Set per object constants.
		auto mirrorWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mRoomWorld)};
		auto mirrorWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(mirrorWorld)};
		auto mirrorWorldViewProj = DirectX::XMMATRIX{mirrorWorld * view * proj};
		auto mirrorPerObject = PerObjectConstants{
			.gMaterial = mMirrorMat,
		};
		DirectX::XMStoreFloat4x4(&mirrorPerObject.gWorld, mirrorWorld);
		DirectX::XMStoreFloat4x4(&mirrorPerObject.gWorldInvTranspose, mirrorWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&mirrorPerObject.gWorldViewProj, mirrorWorldViewProj);
		DirectX::XMStoreFloat4x4(&mirrorPerObject.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &mirrorPerObject, 0, 0);
		auto mirrorSRVs = std::array{ mMirrorDiffuseMapSRV.get() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(mirrorSRVs.size()), mirrorSRVs.data());

		// Mirror
		md3dImmediateContext->OMSetBlendState(mRenderStates.TransparentBS.get(), blendFactor, 0xffffffff);
		md3dImmediateContext->Draw(6, 24);

		//
		// Draw the skull shadow.
		//
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(skullVBs.size()), skullVBs.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto shadowPlane = DirectX::XMVECTOR{DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)}; // xz plane
		auto toMainLight = DirectX::XMVECTOR{-DirectX::XMLoadFloat3(&mDirLights[0].Direction)};
		auto S = DirectX::XMMATRIX{DirectX::XMMatrixShadow(shadowPlane, toMainLight)};
		auto shadowOffsetY = DirectX::XMMATRIX{DirectX::XMMatrixTranslation(0.0f, 0.001f, 0.0f)};

		// Set per object constants.
		auto skullShadowWorld = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mSkullWorld) * S * shadowOffsetY};
		auto skullShadowWorldInvTranspose = DirectX::XMMATRIX{MathHelper::InverseTranspose(skullShadowWorld)};
		auto skullShadowWorldViewProj = DirectX::XMMATRIX{skullShadowWorld * view * proj};
		auto perObjectShadow = PerObjectConstants{
			.gMaterial = mShadowMat,
		};
		DirectX::XMStoreFloat4x4(&perObjectShadow.gWorld, skullShadowWorld);
		DirectX::XMStoreFloat4x4(&perObjectShadow.gWorldInvTranspose, skullShadowWorldInvTranspose);
		DirectX::XMStoreFloat4x4(&perObjectShadow.gWorldViewProj, skullShadowWorldViewProj);
		DirectX::XMStoreFloat4x4(&perObjectShadow.gTexTransform, DirectX::XMMatrixIdentity());
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, nullptr, &perObjectShadow, 0, 0);
		md3dImmediateContext->OMSetDepthStencilState(mRenderStates.NoDoubleBlendDSS.get(), 0);
		md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// Restore default states.
		md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
		md3dImmediateContext->OMSetDepthStencilState(0, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)
	{
		mLastMousePos = { .x = x, .y = y };
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
			mRadius = std::clamp(mRadius, 3.0f, 50.0f);
		}
		mLastMousePos = { .x = x, .y = y };
	}

private:
	void BuildRoomGeometryBuffers()
	{
		// Create and specify geometry.  For this sample we draw a floor
		// and a wall with a mirror on it.  We put the floor, wall, and
		// mirror geometry in one vertex buffer.
		//
		//   |--------------|
		//   |              |
		//   |----|----|----|
		//   |Wall|Mirr|Wall|
		//   |    | or |    |
		//   /--------------/
		//  /   Floor      /
		// /--------------/


		auto v = std::array<Basic32, 30>{
			// Floor: Observe we tile texture coordinates.
			Basic32{ .Pos = {-3.5f, 0.0f, -10.0f},	.Normal = {0.0f, 1.0f, 0.0f},	.Tex = {0.0f, 4.0f} },
			Basic32{ .Pos = {-3.5f, 0.0f, 0.0f},	.Normal = {0.0f, 1.0f, 0.0f},	.Tex = {0.0f, 0.0f} },
			Basic32{ .Pos = {7.5f, 0.0f, 0.0f},		.Normal = {0.0f, 1.0f, 0.0f},	.Tex = {4.0f, 0.0f} },

			Basic32{ .Pos = {-3.5f, 0.0f, -10.0f},	.Normal = {0.0f, 1.0f, 0.0f},	.Tex = {0.0f, 4.0f} },
			Basic32{ .Pos = {7.5f, 0.0f, 0.0f},		.Normal = {0.0f, 1.0f, 0.0f},	.Tex = {4.0f, 0.0f} },
			Basic32{ .Pos = {7.5f, 0.0f, -10.0f},	.Normal = {0.0f, 1.0f, 0.0f},	.Tex = {4.0f, 4.0f} },

			// Wall: Observe we tile texture coordinates, and that we leave a gap in the middle for the mirror.
			Basic32{ .Pos = {-3.5f, 0.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 2.0f}},
			Basic32{ .Pos = {-3.5f, 4.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 0.0f}},
			Basic32{ .Pos = {-2.5f, 4.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.5f, 0.0f}},

			Basic32{ .Pos = {-3.5f, 0.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 2.0f}},
			Basic32{ .Pos = {-2.5f, 4.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.5f, 0.0f}},
			Basic32{ .Pos = {-2.5f, 0.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.5f, 2.0f}},

			Basic32{ .Pos = {2.5f, 0.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 2.0f}},
			Basic32{ .Pos = {2.5f, 4.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 0.0f}},
			Basic32{ .Pos = {7.5f, 4.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {2.0f, 0.0f}},

			Basic32{ .Pos = {2.5f, 0.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 2.0f}},
			Basic32{ .Pos = {7.5f, 4.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {2.0f, 0.0f}},
			Basic32{ .Pos = {7.5f, 0.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {2.0f, 2.0f}},

			Basic32{ .Pos = {-3.5f, 4.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 1.0f}},
			Basic32{ .Pos = {-3.5f, 6.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 0.0f}},
			Basic32{ .Pos = {7.5f, 6.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {6.0f, 0.0f}},

			Basic32{ .Pos = {-3.5f, 4.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {0.0f, 1.0f}},
			Basic32{ .Pos = {7.5f, 6.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {6.0f, 0.0f}},
			Basic32{ .Pos = {7.5f, 4.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f},	.Tex = {6.0f, 1.0f}},

			// Mirror
			Basic32{ .Pos = {-2.5f, 0.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f}, .Tex = {0.0f, 1.0f}},
			Basic32{ .Pos = {-2.5f, 4.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f}, .Tex = {0.0f, 0.0f}},
			Basic32{ .Pos = {2.5f, 4.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f}, .Tex = {1.0f, 0.0f}},

			Basic32{ .Pos = {-2.5f, 0.0f, 0.0f},	.Normal = {0.0f, 0.0f, -1.0f}, .Tex = {0.0f, 1.0f}},
			Basic32{ .Pos = {2.5f, 4.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f}, .Tex = {1.0f, 0.0f}},
			Basic32{ .Pos = {2.5f, 0.0f, 0.0f},		.Normal = {0.0f, 0.0f, -1.0f}, .Tex = {1.0f, 1.0f}},
		};
		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Basic32) * 30,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = v.data() };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mRoomVB));
	}

	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream{"Models/skull.txt"};

		if (not fin)
			throw std::runtime_error{"Models/skull.txt not found."};

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<Basic32>(vcount);
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
			.ByteWidth = sizeof(Basic32) * vcount,
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
			vertexDesc.data(),
			static_cast<std::uint32_t>(vertexDesc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mBasic32),
			"Failed to create input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mRoomVB;

	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;
	ComPtr<D3D11::ID3D11InputLayout> mBasic32;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11Buffer> mPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11SamplerState> mSamplerState;

	ComPtr<D3D11::ID3D11ShaderResourceView> mFloorDiffuseMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mWallDiffuseMapSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mMirrorDiffuseMapSRV;

	int mLightCount = 3;
	RenderStates mRenderStates;

	DirectionalLight mDirLights[3];
	Material mRoomMat;
	Material mSkullMat;
	Material mMirrorMat;
	Material mShadowMat;

	DirectX::XMFLOAT4X4 mRoomWorld = DirectX::XMFLOAT4X4{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	DirectX::XMFLOAT4X4 mSkullWorld = DirectX::XMFLOAT4X4{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	std::uint32_t mSkullIndexCount = 0;
	DirectX::XMFLOAT3 mSkullTranslation = {0.0f, 1.0f, -5.0f};

	DirectX::XMFLOAT4X4 mView = DirectX::XMFLOAT4X4{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	DirectX::XMFLOAT4X4 mProj = DirectX::XMFLOAT4X4{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	RenderOptions mRenderOptions = RenderOptions::Textures;

	DirectX::XMFLOAT3 mEyePosW = {0.0f, 0.0f, 0.0f};

	float mTheta = 1.24f * MathHelper::Pi;
	float mPhi = 0.42f * MathHelper::Pi;
	float mRadius = 12.0f;

	Win32::POINT mLastMousePos{};
};