export module ssaodemo:app;
import std;
import shared;
import :sky;
import :renderstates;
import :shadowmap;
import :ssao;
import :sharedvertices;

enum RenderOptions
{
	RenderOptionsBasic = 0,
	RenderOptionsNormalMap = 1,
	RenderOptionsDisplacementMap = 2
};

struct BoundingSphere
{
	DirectX::XMFLOAT3 Center = { 0.0f, 0.0f, 0.0f };
	float Radius = 0.0f;
};

namespace Basic
{
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
		DirectX::XMFLOAT4X4 gWorldViewProjTex;
		DirectX::XMFLOAT4X4 gTexTransform;
		DirectX::XMFLOAT4X4 gShadowTransform;
		Material gMaterial;
	};

	static_assert(sizeof(PerFrameConstants) == 256);
	static_assert(sizeof(PerObjectConstants) == 448);
}

namespace Normal
{
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
		DirectX::XMFLOAT4X4 gWorldViewProjTex;
		DirectX::XMFLOAT4X4 gTexTransform;
		DirectX::XMFLOAT4X4 gShadowTransform;
		Material gMaterial;
	};

	static_assert(sizeof(PerFrameConstants) == 256);
	static_assert(sizeof(PerObjectConstants) == 448);
}

namespace Displacement
{
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
		float gHeightScale;
		float gMaxTessDistance;
		float gMinTessDistance;
		float gMinTessFactor;
		float gMaxTessFactor;
		DirectX::XMFLOAT3 gTessellationPadding;
	};

	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorld;
		DirectX::XMFLOAT4X4 gWorldInvTranspose;
		DirectX::XMFLOAT4X4 gViewProj;
		DirectX::XMFLOAT4X4 gWorldViewProj;
		DirectX::XMFLOAT4X4 gTexTransform;
		DirectX::XMFLOAT4X4 gShadowTransform;
		Material gMaterial;
	};

	static_assert(sizeof(PerFrameConstants) == 288);
	static_assert(sizeof(PerObjectConstants) == 448);
}

namespace Shadow
{
	struct PerFrameConstants
	{
		DirectX::XMFLOAT3 gEyePosW;
		float gHeightScale;
		float gMaxTessDistance;
		float gMinTessDistance;
		float gMinTessFactor;
		float gMaxTessFactor;
	};

	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorld;
		DirectX::XMFLOAT4X4 gWorldInvTranspose;
		DirectX::XMFLOAT4X4 gViewProj;
		DirectX::XMFLOAT4X4 gWorldViewProj;
		DirectX::XMFLOAT4X4 gTexTransform;
	};

	static_assert(sizeof(PerFrameConstants) == 32);
	static_assert(sizeof(PerObjectConstants) == 320);
}

namespace NormalDepth
{
	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorldView;
		DirectX::XMFLOAT4X4 gWorldInvTransposeView;
		DirectX::XMFLOAT4X4 gWorldViewProj;
		DirectX::XMFLOAT4X4 gTexTransform;
	};

	static_assert(sizeof(PerObjectConstants) == 256);
}

namespace DebugTexture
{
	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorldViewProj;
	};

	static_assert(sizeof(PerObjectConstants) == 64);
}

export class SsaoApp : public D3DApp
{
public:
	SsaoApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance, L"SSAO Demo" }
	{
		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		mSceneBounds.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		mSceneBounds.Radius =
			std::sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
		XMStoreFloat4x4(&mGridWorld, I);

		DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(3.0f, 1.0f, 3.0f);
		DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f);
		XMStoreFloat4x4(&mBoxWorld, DirectX::XMMatrixMultiply(boxScale, boxOffset));

		DirectX::XMMATRIX skullScale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);
		DirectX::XMMATRIX skullOffset = DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		XMStoreFloat4x4(&mSkullWorld, DirectX::XMMatrixMultiply(skullScale, skullOffset));

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

		mOriginalLightDir[0] = mDirLights[0].Direction;
		mOriginalLightDir[1] = mDirLights[1].Direction;
		mOriginalLightDir[2] = mDirLights[2].Direction;

		mGridMat.Ambient = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mGridMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mCylinderMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSphereMat.Ambient = DirectX::XMFLOAT4(0.6f, 0.8f, 0.9f, 1.0f);
		mSphereMat.Diffuse = DirectX::XMFLOAT4(0.6f, 0.8f, 0.9f, 1.0f);
		mSphereMat.Specular = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);

		mBoxMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mSkullMat.Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mSkullMat.Reflect = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

		Init();
	}

	void Init() override
	{
		D3DApp::Init();
		BuildShaders();

		mSky.emplace(md3dDevice.get(), L"Textures/desertcube1024.dds", 5000.0f);
		mSmap.emplace(md3dDevice.get(), SMapSize, SMapSize);
		mRenderStates.emplace(md3dDevice.get());

		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		mSsao.emplace(md3dDevice, md3dImmediateContext, mClientWidth, mClientHeight, mCam.GetFovY(), mCam.GetFarZ(), mBasic32InputLayout);

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(),
			L"Textures/floor.dds", nullptr, &mStoneTexSRV));

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(),
			L"Textures/bricks.dds", nullptr, &mBrickTexSRV));

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(),
			L"Textures/floor_nmap.dds", nullptr, &mStoneNormalTexSRV));

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice.get(),
			L"Textures/bricks_nmap.dds", nullptr, &mBrickNormalTexSRV));

		BuildShapeGeometryBuffers();
		BuildSkullGeometryBuffers();
		BuildScreenQuadGeometryBuffers();
	}

	void OnResize() override
	{
		D3DApp::OnResize();
		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		if (mSsao)
			mSsao->OnSize(mClientWidth, mClientHeight, mCam.GetFovY(), mCam.GetFarZ());
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
		// Switch the rendering effect based on key presses.
		//
		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mRenderOptions = RenderOptionsBasic;

		if (Win32::GetAsyncKeyState('3') & 0x8000)
			mRenderOptions = RenderOptionsNormalMap;

		if (Win32::GetAsyncKeyState('4') & 0x8000)
			mRenderOptions = RenderOptionsDisplacementMap;

		//
		// Animate the lights (and hence shadows).
		//

		mLightRotationAngle += 0.1f * dt;

		DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(mLightRotationAngle);
		for (int i = 0; i < 3; ++i)
		{
			DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mOriginalLightDir[i]);
			lightDir = DirectX::XMVector3TransformNormal(lightDir, R);
			DirectX::XMStoreFloat3(&mDirLights[i].Direction, lightDir);
		}

		BuildShadowTransform();

		mCam.UpdateViewMatrix();
	}

	void DrawScene() override
	{
		mSmap->BindDsvAndSetNullRenderTarget(
			md3dImmediateContext.get());
		DrawSceneToShadowMap();

		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(),
			D3D11::D3D11_CLEAR_FLAG{
				D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL },
			1.0f,
			0);
		mSsao->SetNormalDepthRenderTarget(
			mDepthStencilView.get());
		DrawSceneToSsaoNormalDepthMap();

		mSsao->ComputeSsao(mCam);
		mSsao->BlurAmbientMap(4);

		md3dImmediateContext->OMSetRenderTargets(
			1,
			mRenderTargetView.GetAddressOf(),
			mDepthStencilView.get());
		md3dImmediateContext->RSSetViewports(
			1, &mScreenViewport);
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(),
			reinterpret_cast<const float*>(
				&DirectX::Colors::Silver));

		if (mRenderOptions == RenderOptionsDisplacementMap)
		{
			// The normal/depth pass uses the undisplaced geometry, so its depth
			// values cannot be reused with an equality test after tessellation.
			md3dImmediateContext->ClearDepthStencilView(
				mDepthStencilView.get(),
				D3D11::D3D11_CLEAR_FLAG{
					D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL },
				1.0f,
				0);
			md3dImmediateContext->OMSetDepthStencilState(
				nullptr, 0);
		}
		else
		{
			md3dImmediateContext->OMSetDepthStencilState(
				mRenderStates->EqualDSS.get(), 0);
		}

		auto view = mCam.View();
		auto proj = mCam.Proj();
		auto viewProj = view * proj;
		auto shadowTransform =
			DirectX::XMLoadFloat4x4(&mShadowTransform);
		const auto toTexSpace = DirectX::XMMATRIX{
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f,
		};

		auto basicPerFrameConstants = Basic::PerFrameConstants{
			.gDirLights = {
				mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = 3,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = false,
			.gPadding = DirectX::XMFLOAT2{ 0.0f, 0.0f },
			.gFogColor =
				DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f },
		};
		md3dImmediateContext->UpdateSubresource(
			mBasicPerFrameCB.get(),
			0,
			nullptr,
			&basicPerFrameConstants,
			0,
			0);

		auto normalPerFrameConstants = Normal::PerFrameConstants{
			.gDirLights = {
				mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = 3,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = false,
			.gPadding = DirectX::XMFLOAT2{ 0.0f, 0.0f },
			.gFogColor =
				DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f },
		};
		md3dImmediateContext->UpdateSubresource(
			mNormalMapPerFrameCB.get(),
			0,
			nullptr,
			&normalPerFrameConstants,
			0,
			0);

		auto displacementPerFrameConstants =
			Displacement::PerFrameConstants{
				.gDirLights = {
					mDirLights[0],
					mDirLights[1],
					mDirLights[2] },
				.gEyePosW = mCam.GetPosition(),
				.gFogStart = 15.0f,
				.gFogRange = 175.0f,
				.gLightCount = 3,
				.gUseTexture = true,
				.gAlphaClip = false,
				.gFogEnabled = false,
				.gReflectionEnabled = false,
				.gPadding =
					DirectX::XMFLOAT2{ 0.0f, 0.0f },
				.gFogColor =
					DirectX::XMFLOAT4{
						0.7f, 0.7f, 0.7f, 1.0f },
				.gHeightScale = 0.07f,
				.gMaxTessDistance = 1.0f,
				.gMinTessDistance = 25.0f,
				.gMinTessFactor = 1.0f,
				.gMaxTessFactor = 5.0f,
				.gTessellationPadding =
					DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			};
		md3dImmediateContext->UpdateSubresource(
			mDisplacementMapPerFrameCB.get(), 0, nullptr, &displacementPerFrameConstants, 0, 0);

		switch (mRenderOptions)
		{
		case RenderOptionsBasic:
			md3dImmediateContext->IASetPrimitiveTopology(
				D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			md3dImmediateContext->IASetInputLayout(
				mBasic32InputLayout.get());
			md3dImmediateContext->VSSetShader(
				mBasic32VertexShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(
				nullptr, nullptr, 0);
			md3dImmediateContext->DSSetShader(
				nullptr, nullptr, 0);
			md3dImmediateContext->PSSetShader(
				mBasic32PixelShader.get(), nullptr, 0);
			break;

		case RenderOptionsNormalMap:
			md3dImmediateContext->IASetPrimitiveTopology(
				D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			md3dImmediateContext->IASetInputLayout(
				mNormalMapInputLayout.get());
			md3dImmediateContext->VSSetShader(
				mNormalMapVertexShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(
				nullptr, nullptr, 0);
			md3dImmediateContext->DSSetShader(
				nullptr, nullptr, 0);
			md3dImmediateContext->PSSetShader(
				mNormalMapPixelShader.get(), nullptr, 0);
			break;

		case RenderOptionsDisplacementMap:
			md3dImmediateContext->IASetPrimitiveTopology(
				D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
			md3dImmediateContext->IASetInputLayout(
				mDisplacementMapInputLayout.get());
			md3dImmediateContext->VSSetShader(
				mDisplacementMapVertexShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(
				mDisplacementMapHullShader.get(), nullptr, 0);
			md3dImmediateContext->DSSetShader(
				mDisplacementMapDomainShader.get(), nullptr, 0);
			md3dImmediateContext->PSSetShader(
				mDisplacementMapPixelShader.get(), nullptr, 0);
			break;
		}
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);

		auto shapeStride =
			static_cast<std::uint32_t>(
				sizeof(Vertices::PosNormalTexTan));
		auto offset = 0u;
		md3dImmediateContext->IASetVertexBuffers(
			0,
			1,
			mShapesVB.GetAddressOf(),
			&shapeStride,
			&offset);
		md3dImmediateContext->IASetIndexBuffer(
			mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
		{
			md3dImmediateContext->RSSetState(
				mRenderStates->WireframeRS.get());
		}

		auto DrawTexturedObject =
			[&](
				DirectX::CXMMATRIX world,
				DirectX::CXMMATRIX texTransform,
				const Material& material,
				D3D11::ID3D11ShaderResourceView* diffuseMap,
				D3D11::ID3D11ShaderResourceView* normalMap,
				std::uint32_t indexCount,
				std::uint32_t indexOffset,
				std::int32_t vertexOffset)
			{
				auto worldInvTranspose =
					MathHelper::InverseTranspose(world);
				auto worldViewProj = world * view * proj;

				switch (mRenderOptions)
				{
				case RenderOptionsBasic:
					{
						auto constants =
							Basic::PerObjectConstants{};
						DirectX::XMStoreFloat4x4(
							&constants.gWorld, world);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldInvTranspose,
							worldInvTranspose);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldViewProj,
							worldViewProj);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldViewProjTex,
							worldViewProj * toTexSpace);
						DirectX::XMStoreFloat4x4(
							&constants.gTexTransform,
							texTransform);
						DirectX::XMStoreFloat4x4(
							&constants.gShadowTransform,
							world * shadowTransform);
						constants.gMaterial = material;
						md3dImmediateContext->UpdateSubresource(
							mBasicPerObjectCB.get(), 0, nullptr, &constants, 0, 0);

						auto pixelResources = std::array{
							diffuseMap,
							mSky->CubeMapSRV(),
							mSmap->DepthMapSRV(),
							mSsao->AmbientSRV(),
						};
						auto pixelBuffers = std::array{
							mBasicPerFrameCB.get(),
							mBasicPerObjectCB.get(),
						};
						auto samplers = std::array{
							mLinearSampler.get(),
							mShadowSampler.get(),
						};
						md3dImmediateContext->VSSetConstantBuffers(
							1, 1, mBasicPerObjectCB.GetAddressOf());
						md3dImmediateContext->PSSetConstantBuffers(
							0, static_cast<std::uint32_t>(pixelBuffers.size()), pixelBuffers.data());
						md3dImmediateContext->PSSetShaderResources(
							0, static_cast<std::uint32_t>(pixelResources.size()), pixelResources.data());
						md3dImmediateContext->PSSetSamplers(
							0, static_cast<std::uint32_t>(samplers.size()), samplers.data());
						break;
					}

				case RenderOptionsNormalMap:
					{
						auto constants =
							Normal::PerObjectConstants{};
						DirectX::XMStoreFloat4x4(
							&constants.gWorld, world);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldInvTranspose,
							worldInvTranspose);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldViewProj,
							worldViewProj);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldViewProjTex,
							worldViewProj * toTexSpace);
						DirectX::XMStoreFloat4x4(
							&constants.gTexTransform,
							texTransform);
						DirectX::XMStoreFloat4x4(
							&constants.gShadowTransform,
							world * shadowTransform);
						constants.gMaterial = material;
						md3dImmediateContext->UpdateSubresource(
							mNormalMapPerObjectCB.get(),
							0,
							nullptr,
							&constants,
							0,
							0);

						auto pixelResources = std::array{
							diffuseMap,
							normalMap,
							mSky->CubeMapSRV(),
							mSmap->DepthMapSRV(),
							mSsao->AmbientSRV(),
						};
						auto pixelBuffers = std::array{
							mNormalMapPerFrameCB.get(),
							mNormalMapPerObjectCB.get(),
						};
						auto samplers = std::array{
							mLinearSampler.get(),
							mShadowLessEqualSampler.get(),
						};
						md3dImmediateContext->VSSetConstantBuffers(
							1,
							1,
							mNormalMapPerObjectCB.GetAddressOf());
						md3dImmediateContext->PSSetConstantBuffers(
							0,
							static_cast<std::uint32_t>(
								pixelBuffers.size()),
							pixelBuffers.data());
						md3dImmediateContext->PSSetShaderResources(
							0,
							static_cast<std::uint32_t>(
								pixelResources.size()),
							pixelResources.data());
						md3dImmediateContext->PSSetSamplers(
							0,
							static_cast<std::uint32_t>(
								samplers.size()),
							samplers.data());
						break;
					}

				case RenderOptionsDisplacementMap:
					{
						auto constants =
							Displacement::PerObjectConstants{};
						DirectX::XMStoreFloat4x4(
							&constants.gWorld, world);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldInvTranspose,
							worldInvTranspose);
						DirectX::XMStoreFloat4x4(
							&constants.gViewProj, viewProj);
						DirectX::XMStoreFloat4x4(
							&constants.gWorldViewProj,
							worldViewProj);
						DirectX::XMStoreFloat4x4(
							&constants.gTexTransform,
							texTransform);
						DirectX::XMStoreFloat4x4(
							&constants.gShadowTransform,
							shadowTransform);
						constants.gMaterial = material;
						md3dImmediateContext->UpdateSubresource(
							mDisplacementMapPerObjectCB.get(),
							0,
							nullptr,
							&constants,
							0,
							0);

						auto constantBuffers = std::array{
							mDisplacementMapPerFrameCB.get(),
							mDisplacementMapPerObjectCB.get(),
						};
						auto pixelResources = std::array{
							diffuseMap,
							normalMap,
							mSky->CubeMapSRV(),
							mSmap->DepthMapSRV(),
						};
						auto samplers = std::array{
							mLinearSampler.get(),
							mShadowSampler.get(),
						};
						md3dImmediateContext->VSSetConstantBuffers(
							0,
							static_cast<std::uint32_t>(
								constantBuffers.size()),
							constantBuffers.data());
						md3dImmediateContext->DSSetConstantBuffers(
							0,
							static_cast<std::uint32_t>(
								constantBuffers.size()),
							constantBuffers.data());
						md3dImmediateContext->PSSetConstantBuffers(
							0,
							static_cast<std::uint32_t>(
								constantBuffers.size()),
							constantBuffers.data());
						md3dImmediateContext->DSSetShaderResources(
							1, 1, &normalMap);
						md3dImmediateContext->DSSetSamplers(
							0, 1, mLinearSampler.GetAddressOf());
						md3dImmediateContext->PSSetShaderResources(
							0,
							static_cast<std::uint32_t>(
								pixelResources.size()),
							pixelResources.data());
						md3dImmediateContext->PSSetSamplers(
							0,
							static_cast<std::uint32_t>(
								samplers.size()),
							samplers.data());
						break;
					}
				}

				md3dImmediateContext->DrawIndexed(
					indexCount, indexOffset, vertexOffset);
			};

		DrawTexturedObject(
			DirectX::XMLoadFloat4x4(&mGridWorld),
			DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f),
			mGridMat,
			mStoneTexSRV.get(),
			mStoneNormalTexSRV.get(),
			mGridIndexCount,
			mGridIndexOffset,
			mGridVertexOffset);

		DrawTexturedObject(
			DirectX::XMLoadFloat4x4(&mBoxWorld),
			DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f),
			mBoxMat,
			mBrickTexSRV.get(),
			mBrickNormalTexSRV.get(),
			mBoxIndexCount,
			mBoxIndexOffset,
			mBoxVertexOffset);

		for (auto i = 0; i < 10; ++i)
		{
			DrawTexturedObject(
				DirectX::XMLoadFloat4x4(&mCylWorld[i]),
				DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f),
				mCylinderMat,
				mBrickTexSRV.get(),
				mBrickNormalTexSRV.get(),
				mCylinderIndexCount,
				mCylinderIndexOffset,
				mCylinderVertexOffset);
		}

		auto nullResource =
			static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->DSSetShaderResources(
			1, 1, &nullResource);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);

		md3dImmediateContext->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->IASetInputLayout(
			mBasic32InputLayout.get());
		md3dImmediateContext->VSSetShader(
			mBasic32VertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(
			mBasic32PixelShader.get(), nullptr, 0);

		basicPerFrameConstants.gUseTexture = false;
		basicPerFrameConstants.gReflectionEnabled = true;
		md3dImmediateContext->UpdateSubresource(
			mBasicPerFrameCB.get(),
			0,
			nullptr,
			&basicPerFrameConstants,
			0,
			0);

		auto reflectiveResources =
			std::array<D3D11::ID3D11ShaderResourceView*, 4>{
				nullptr,
				mSky->CubeMapSRV(),
				mSmap->DepthMapSRV(),
				mSsao->AmbientSRV(),
			};
		auto reflectiveSamplers = std::array{
			mLinearSampler.get(),
			mShadowSampler.get(),
		};
		md3dImmediateContext->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(
				reflectiveResources.size()),
			reflectiveResources.data());
		md3dImmediateContext->PSSetSamplers(
			0,
			static_cast<std::uint32_t>(
				reflectiveSamplers.size()),
			reflectiveSamplers.data());

		auto DrawReflectiveObject =
			[&](
				DirectX::CXMMATRIX world,
				const Material& material,
				std::uint32_t indexCount,
				std::uint32_t indexOffset,
				std::int32_t vertexOffset)
			{
				auto worldViewProj = world * view * proj;
				auto constants = Basic::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(
					&constants.gWorld, world);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldInvTranspose,
					MathHelper::InverseTranspose(world));
				DirectX::XMStoreFloat4x4(
					&constants.gWorldViewProj,
					worldViewProj);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldViewProjTex,
					worldViewProj * toTexSpace);
				DirectX::XMStoreFloat4x4(
					&constants.gTexTransform,
					DirectX::XMMatrixIdentity());
				DirectX::XMStoreFloat4x4(
					&constants.gShadowTransform,
					world * shadowTransform);
				constants.gMaterial = material;
				md3dImmediateContext->UpdateSubresource(
					mBasicPerObjectCB.get(),
					0,
					nullptr,
					&constants,
					0,
					0);

				auto pixelBuffers = std::array{
					mBasicPerFrameCB.get(),
					mBasicPerObjectCB.get(),
				};
				md3dImmediateContext->VSSetConstantBuffers(
					1,
					1,
					mBasicPerObjectCB.GetAddressOf());
				md3dImmediateContext->PSSetConstantBuffers(
					0,
					static_cast<std::uint32_t>(
						pixelBuffers.size()),
					pixelBuffers.data());
				md3dImmediateContext->DrawIndexed(
					indexCount, indexOffset, vertexOffset);
			};

		for (auto i = 0; i < 10; ++i)
		{
			DrawReflectiveObject(
				DirectX::XMLoadFloat4x4(&mSphereWorld[i]),
				mSphereMat,
				mSphereIndexCount,
				mSphereIndexOffset,
				mSphereVertexOffset);
		}

		md3dImmediateContext->RSSetState(nullptr);

		auto skullStride =
			static_cast<std::uint32_t>(
				sizeof(Vertices::Basic32));
		offset = 0;
		md3dImmediateContext->IASetVertexBuffers(
			0,
			1,
			mSkullVB.GetAddressOf(),
			&skullStride,
			&offset);
		md3dImmediateContext->IASetIndexBuffer(
			mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);
		DrawReflectiveObject(
			DirectX::XMLoadFloat4x4(&mSkullWorld),
			mSkullMat,
			mSkullIndexCount,
			0,
			0);

		md3dImmediateContext->OMSetDepthStencilState(
			nullptr, 0);

		DrawScreenQuad(mSsao->AmbientSRV());
		mSky->Draw(md3dImmediateContext.get(), mCam);

		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->OMSetDepthStencilState(nullptr, 0);

		auto nullResources =
			std::array<D3D11::ID3D11ShaderResourceView*, 16>{};
		md3dImmediateContext->PSSetShaderResources(
			0, static_cast<std::uint32_t>(nullResources.size()), nullResources.data());

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
	void DrawSceneToSsaoNormalDepthMap()
	{
		auto view = mCam.View();
		auto proj = mCam.Proj();

		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->IASetInputLayout(mBasic32InputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(
			mNormalDepthVertexShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(
			mNormalDepthPixelShader.get(), nullptr, 0);

		auto shapeStride =
			static_cast<std::uint32_t>(sizeof(Vertices::PosNormalTexTan));
		auto offset = 0u;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mShapesVB.GetAddressOf(), &shapeStride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto drawNormalDepthObject =
			[&](
				DirectX::CXMMATRIX world,
				DirectX::CXMMATRIX texTransform,
				std::uint32_t indexCount,
				std::uint32_t indexOffset,
				std::int32_t vertexOffset)
			{
				auto constants = NormalDepth::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(
					&constants.gWorldView,
					world * view);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldInvTransposeView,
					MathHelper::InverseTranspose(world) * view);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldViewProj,
					world * view * proj);
				DirectX::XMStoreFloat4x4(
					&constants.gTexTransform,
					texTransform);
				md3dImmediateContext->UpdateSubresource(
					mNormalDepthPerObjectCB.get(),
					0,
					nullptr,
					&constants,
					0,
					0);
				md3dImmediateContext->VSSetConstantBuffers(
					0, 1, mNormalDepthPerObjectCB.GetAddressOf());
				md3dImmediateContext->DrawIndexed(
					indexCount, indexOffset, vertexOffset);
			};

		drawNormalDepthObject(
			DirectX::XMLoadFloat4x4(&mGridWorld),
			DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f),
			mGridIndexCount,
			mGridIndexOffset,
			mGridVertexOffset);

		drawNormalDepthObject(
			DirectX::XMLoadFloat4x4(&mBoxWorld),
			DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f),
			mBoxIndexCount,
			mBoxIndexOffset,
			mBoxVertexOffset);

		for (auto i = 0; i < 10; ++i)
		{
			drawNormalDepthObject(
				DirectX::XMLoadFloat4x4(&mCylWorld[i]),
				DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f),
				mCylinderIndexCount,
				mCylinderIndexOffset,
				mCylinderVertexOffset);
		}

		for (auto i = 0; i < 10; ++i)
		{
			drawNormalDepthObject(
				DirectX::XMLoadFloat4x4(&mSphereWorld[i]),
				DirectX::XMMatrixIdentity(),
				mSphereIndexCount,
				mSphereIndexOffset,
				mSphereVertexOffset);
		}

		auto skullStride =
			static_cast<std::uint32_t>(sizeof(Vertices::Basic32));
		offset = 0;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mSkullVB.GetAddressOf(), &skullStride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);
		drawNormalDepthObject(
			DirectX::XMLoadFloat4x4(&mSkullWorld),
			DirectX::XMMatrixIdentity(),
			mSkullIndexCount,
			0,
			0);
	}

	void DrawSceneToShadowMap()
	{
		auto lightView = DirectX::XMLoadFloat4x4(&mLightView);
		auto lightProj = DirectX::XMLoadFloat4x4(&mLightProj);
		auto lightViewProj = lightView * lightProj;

		auto perFrameConstants = Shadow::PerFrameConstants{
			.gEyePosW = mCam.GetPosition(),
			.gHeightScale = 0.07f,
			.gMaxTessDistance = 1.0f,
			.gMinTessDistance = 25.0f,
			.gMinTessFactor = 1.0f,
			.gMaxTessFactor = 5.0f,
		};
		md3dImmediateContext->UpdateSubresource(
			mShadowMapPerFrameCB.get(), 0, nullptr, &perFrameConstants, 0, 0);

		md3dImmediateContext->RSSetState(mRenderStates->ShadowMapRS.get());
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->IASetInputLayout(mBasic32InputLayout.get());

		auto shapeStride =
			static_cast<std::uint32_t>(sizeof(Vertices::PosNormalTexTan));
		auto offset = 0u;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mShapesVB.GetAddressOf(), &shapeStride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto drawShadowObject =
			[&](
				DirectX::CXMMATRIX world,
				DirectX::CXMMATRIX texTransform,
				D3D11::ID3D11ShaderResourceView* normalMap,
				bool tessellated,
				std::uint32_t indexCount,
				std::uint32_t indexOffset,
				std::int32_t vertexOffset)
			{
				auto constants = Shadow::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(&constants.gWorld, world);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldInvTranspose,
					MathHelper::InverseTranspose(world));
				DirectX::XMStoreFloat4x4(&constants.gViewProj, lightViewProj);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldViewProj,
					world * lightViewProj);
				DirectX::XMStoreFloat4x4(
					&constants.gTexTransform,
					texTransform);
				md3dImmediateContext->UpdateSubresource(
					mShadowMapPerObjectCB.get(), 0, nullptr, &constants, 0, 0);

				if (tessellated)
				{
					md3dImmediateContext->IASetPrimitiveTopology(
						D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
					md3dImmediateContext->VSSetShader(
						mShadowMapTessVertexShader.get(), nullptr, 0);
					md3dImmediateContext->HSSetShader(
						mShadowMapTessHullShader.get(), nullptr, 0);
					md3dImmediateContext->DSSetShader(
						mShadowMapTessDomainShader.get(), nullptr, 0);

					auto constantBuffers = std::array{
						mShadowMapPerFrameCB.get(),
						mShadowMapPerObjectCB.get(),
					};
					md3dImmediateContext->VSSetConstantBuffers(
						0,
						static_cast<std::uint32_t>(constantBuffers.size()),
						constantBuffers.data());
					md3dImmediateContext->DSSetConstantBuffers(
						0,
						static_cast<std::uint32_t>(constantBuffers.size()),
						constantBuffers.data());
					md3dImmediateContext->DSSetSamplers(
						0, 1, mLinearSampler.GetAddressOf());
					md3dImmediateContext->DSSetShaderResources(
						1, 1, &normalMap);
				}
				else
				{
					md3dImmediateContext->IASetPrimitiveTopology(
						D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					md3dImmediateContext->VSSetShader(
						mShadowMapVertexShader.get(), nullptr, 0);
					md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
					md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
					md3dImmediateContext->VSSetConstantBuffers(
						1, 1, mShadowMapPerObjectCB.GetAddressOf());
				}

				md3dImmediateContext->DrawIndexed(
					indexCount, indexOffset, vertexOffset);
			};

		auto tessellated =
			mRenderOptions == RenderOptionsDisplacementMap;

		drawShadowObject(
			DirectX::XMLoadFloat4x4(&mGridWorld),
			DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f),
			mStoneNormalTexSRV.get(),
			tessellated,
			mGridIndexCount,
			mGridIndexOffset,
			mGridVertexOffset);

		drawShadowObject(
			DirectX::XMLoadFloat4x4(&mBoxWorld),
			DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f),
			mBrickNormalTexSRV.get(),
			tessellated,
			mBoxIndexCount,
			mBoxIndexOffset,
			mBoxVertexOffset);

		for (auto i = 0; i < 10; ++i)
		{
			drawShadowObject(
				DirectX::XMLoadFloat4x4(&mCylWorld[i]),
				DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f),
				mBrickNormalTexSRV.get(),
				tessellated,
				mCylinderIndexCount,
				mCylinderIndexOffset,
				mCylinderVertexOffset);
		}

		auto nullResource =
			static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->DSSetShaderResources(
			1, 1, &nullResource);

		for (auto i = 0; i < 10; ++i)
		{
			drawShadowObject(
				DirectX::XMLoadFloat4x4(&mSphereWorld[i]),
				DirectX::XMMatrixIdentity(),
				nullptr,
				false,
				mSphereIndexCount,
				mSphereIndexOffset,
				mSphereVertexOffset);
		}

		auto skullStride =
			static_cast<std::uint32_t>(sizeof(Vertices::Basic32));
		offset = 0;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mSkullVB.GetAddressOf(), &skullStride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);
		drawShadowObject(
			DirectX::XMLoadFloat4x4(&mSkullWorld),
			DirectX::XMMatrixIdentity(),
			nullptr,
			false,
			mSkullIndexCount,
			0,
			0);
	}

	void DrawScreenQuad(D3D11::ID3D11ShaderResourceView* srv)
	{
		auto stride = static_cast<std::uint32_t>(sizeof(Vertices::Basic32));
		auto offset = 0u;

		md3dImmediateContext->IASetInputLayout(mBasic32InputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mScreenQuadVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mScreenQuadIB.get(), DXGI_FORMAT_R32_UINT, 0);

		md3dImmediateContext->VSSetShader(mDebugTextureVertexShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(mDebugTexturePixelShader.get(), nullptr, 0);

		// Scale and shift quad to lower-right corner.
		DirectX::XMMATRIX world(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, -0.5f, 0.0f, 1.0f);

		auto constants = DebugTexture::PerObjectConstants{};
		DirectX::XMStoreFloat4x4(&constants.gWorldViewProj, world);
		md3dImmediateContext->UpdateSubresource(
			mDebugTexturePerObjectCB.get(), 0, nullptr, &constants, 0, 0);
		md3dImmediateContext->VSSetConstantBuffers(
			0, 1, mDebugTexturePerObjectCB.GetAddressOf());
		md3dImmediateContext->PSSetShaderResources(0, 1, &srv);
		md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

		md3dImmediateContext->DrawIndexed(6, 0, 0);

		auto nullResource = static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->PSSetShaderResources(0, 1, &nullResource);
	}

	void BuildShadowTransform()
	{
		// Only the first "main" light casts a shadow.
		DirectX::XMVECTOR lightDir = XMLoadFloat3(&mDirLights[0].Direction);
		DirectX::XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
		DirectX::XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(lightPos, targetPos, up);

		// Transform bounding sphere to light space.
		DirectX::XMFLOAT3 sphereCenterLS;
		DirectX::XMStoreFloat3(&sphereCenterLS, DirectX::XMVector3TransformCoord(targetPos, V));

		// Ortho frustum in light space encloses scene.
		float l = sphereCenterLS.x - mSceneBounds.Radius;
		float b = sphereCenterLS.y - mSceneBounds.Radius;
		float n = sphereCenterLS.z - mSceneBounds.Radius;
		float r = sphereCenterLS.x + mSceneBounds.Radius;
		float t = sphereCenterLS.y + mSceneBounds.Radius;
		float f = sphereCenterLS.z + mSceneBounds.Radius;
		DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
		DirectX::XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		DirectX::XMMATRIX S = V * P * T;

		DirectX::XMStoreFloat4x4(&mLightView, V);
		DirectX::XMStoreFloat4x4(&mLightProj, P);
		DirectX::XMStoreFloat4x4(&mShadowTransform, S);
	}

	void BuildShapeGeometryBuffers()
	{
		auto box = GeometryGenerator::MeshData{};
		auto grid = GeometryGenerator::MeshData{};
		auto sphere = GeometryGenerator::MeshData{};
		auto cylinder = GeometryGenerator::MeshData{};

		auto geoGen = GeometryGenerator{};
		geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
		geoGen.CreateGrid(20.0f, 30.0f, 50, 40, grid);
		geoGen.CreateSphere(0.5f, 20, 20, sphere);
		geoGen.CreateCylinder(0.5f, 0.5f, 3.0f, 15, 15, cylinder);

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

		auto totalVertexCount = 
			static_cast<std::uint32_t>(box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size());

		auto totalIndexCount =
			mBoxIndexCount +
			mGridIndexCount +
			mSphereIndexCount +
			mCylinderIndexCount;

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		std::vector<Vertices::PosNormalTexTan> vertices(totalVertexCount);

		UINT k = 0;
		for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
			vertices[k].Tex = box.Vertices[i].TexC;
			vertices[k].TangentU = box.Vertices[i].TangentU;
		}

		for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Normal = grid.Vertices[i].Normal;
			vertices[k].Tex = grid.Vertices[i].TexC;
			vertices[k].TangentU = grid.Vertices[i].TangentU;
		}

		for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Normal = sphere.Vertices[i].Normal;
			vertices[k].Tex = sphere.Vertices[i].TexC;
			vertices[k].TangentU = sphere.Vertices[i].TangentU;
		}

		for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Normal = cylinder.Vertices[i].Normal;
			vertices[k].Tex = cylinder.Vertices[i].TexC;
			vertices[k].TangentU = cylinder.Vertices[i].TangentU;
		}

		D3D11::D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertices::PosNormalTexTan) * totalVertexCount;
		vbd.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11::D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		std::vector<UINT> indices;
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
		indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
		indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
		indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
	}

	void BuildSkullGeometryBuffers()
	{
		auto fin = std::ifstream{"Models/skull.txt"};

		if (not fin)
			throw std::runtime_error{ "Models/skull.txt not found." };

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<Vertices::Basic32>(vcount);
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
			.ByteWidth = sizeof(Vertices::Basic32) * vcount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem=&vertices[0]};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(std::uint32_t) * mSkullIndexCount,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem=&indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}

	void BuildScreenQuadGeometryBuffers()
	{
		auto quad = GeometryGenerator::MeshData{};

		auto geoGen = GeometryGenerator{};
		geoGen.CreateFullscreenQuad(quad);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		auto vertices = std::vector<Vertices::Basic32>(quad.Vertices.size());

		for (auto i = 0u; i < quad.Vertices.size(); ++i)
		{
			vertices[i].Pos = quad.Vertices[i].Position;
			vertices[i].Normal = quad.Vertices[i].Normal;
			vertices[i].Tex = quad.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Vertices::Basic32) * quad.Vertices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &vertices[0] };
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * quad.Indices.size()),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = &quad.Indices[0] };
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
	}

	void BuildShaders()
	{
		auto readShaderBytecode = [](const wchar_t* filename, const char* errorMessage)
		{
			auto bytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(filename, &bytecode), errorMessage);
			return bytecode;
		};

		auto basicVertexShaderBytecode = readShaderBytecode(
			L"Shaders/BasicVS.cso",
			"Failed to read basic vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				basicVertexShaderBytecode->GetBufferPointer(),
				basicVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mBasic32VertexShader),
			"Failed to create basic vertex shader.");

		auto basicPixelShaderBytecode = readShaderBytecode(
			L"Shaders/BasicPS.cso",
			"Failed to read basic pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				basicPixelShaderBytecode->GetBufferPointer(),
				basicPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mBasic32PixelShader),
			"Failed to create basic pixel shader.");

		auto normalMapVertexShaderBytecode = readShaderBytecode(
			L"Shaders/NormalMapVS.cso",
			"Failed to read normal-map vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				normalMapVertexShaderBytecode->GetBufferPointer(),
				normalMapVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalMapVertexShader),
			"Failed to create normal-map vertex shader.");

		auto normalMapPixelShaderBytecode = readShaderBytecode(
			L"Shaders/NormalMapPS.cso",
			"Failed to read normal-map pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				normalMapPixelShaderBytecode->GetBufferPointer(),
				normalMapPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalMapPixelShader),
			"Failed to create normal-map pixel shader.");

		auto displacementMapVertexShaderBytecode = readShaderBytecode(
			L"Shaders/DisplacementMapVS.cso",
			"Failed to read displacement-map vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				displacementMapVertexShaderBytecode->GetBufferPointer(),
				displacementMapVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapVertexShader),
			"Failed to create displacement-map vertex shader.");

		auto displacementMapHullShaderBytecode = readShaderBytecode(
			L"Shaders/DisplacementMapHS.cso",
			"Failed to read displacement-map hull shader file.");
		HR(
			md3dDevice->CreateHullShader(
				displacementMapHullShaderBytecode->GetBufferPointer(),
				displacementMapHullShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapHullShader),
			"Failed to create displacement-map hull shader.");

		auto displacementMapDomainShaderBytecode = readShaderBytecode(
			L"Shaders/DisplacementMapDS.cso",
			"Failed to read displacement-map domain shader file.");
		HR(
			md3dDevice->CreateDomainShader(
				displacementMapDomainShaderBytecode->GetBufferPointer(),
				displacementMapDomainShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapDomainShader),
			"Failed to create displacement-map domain shader.");

		auto displacementMapPixelShaderBytecode = readShaderBytecode(
			L"Shaders/DisplacementMapPS.cso",
			"Failed to read displacement-map pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				displacementMapPixelShaderBytecode->GetBufferPointer(),
				displacementMapPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapPixelShader),
			"Failed to create displacement-map pixel shader.");

		auto shadowMapVertexShaderBytecode = readShaderBytecode(
			L"Shaders/BuildShadowMapVS.cso",
			"Failed to read shadow-map vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				shadowMapVertexShaderBytecode->GetBufferPointer(),
				shadowMapVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapVertexShader),
			"Failed to create shadow-map vertex shader.");

		auto shadowMapPixelShaderBytecode = readShaderBytecode(
			L"Shaders/BuildShadowMapPS.cso",
			"Failed to read shadow-map pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				shadowMapPixelShaderBytecode->GetBufferPointer(),
				shadowMapPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapAlphaClipPixelShader),
			"Failed to create alpha-clipped shadow-map pixel shader.");

		auto shadowMapTessVertexShaderBytecode = readShaderBytecode(
			L"Shaders/BuildShadowMapTessVS.cso",
			"Failed to read tessellated shadow-map vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				shadowMapTessVertexShaderBytecode->GetBufferPointer(),
				shadowMapTessVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessVertexShader),
			"Failed to create tessellated shadow-map vertex shader.");

		auto shadowMapTessHullShaderBytecode = readShaderBytecode(
			L"Shaders/BuildShadowMapTessHS.cso",
			"Failed to read tessellated shadow-map hull shader file.");
		HR(
			md3dDevice->CreateHullShader(
				shadowMapTessHullShaderBytecode->GetBufferPointer(),
				shadowMapTessHullShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessHullShader),
			"Failed to create tessellated shadow-map hull shader.");

		auto shadowMapTessDomainShaderBytecode = readShaderBytecode(
			L"Shaders/BuildShadowMapTessDS.cso",
			"Failed to read tessellated shadow-map domain shader file.");
		HR(
			md3dDevice->CreateDomainShader(
				shadowMapTessDomainShaderBytecode->GetBufferPointer(),
				shadowMapTessDomainShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessDomainShader),
			"Failed to create tessellated shadow-map domain shader.");

		auto shadowMapTessPixelShaderBytecode = readShaderBytecode(
			L"Shaders/BuildShadowMapTessPS.cso",
			"Failed to read tessellated shadow-map pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				shadowMapTessPixelShaderBytecode->GetBufferPointer(),
				shadowMapTessPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessAlphaClipPixelShader),
			"Failed to create tessellated alpha-clipped shadow-map pixel shader.");

		auto normalDepthVertexShaderBytecode = readShaderBytecode(
			L"Shaders/SsaoNormalDepthVS.cso",
			"Failed to read normal-depth vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				normalDepthVertexShaderBytecode->GetBufferPointer(),
				normalDepthVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalDepthVertexShader),
			"Failed to create normal-depth vertex shader.");

		auto normalDepthPixelShaderBytecode = readShaderBytecode(
			L"Shaders/SsaoNormalDepthPS.cso",
			"Failed to read normal-depth pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				normalDepthPixelShaderBytecode->GetBufferPointer(),
				normalDepthPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalDepthPixelShader),
			"Failed to create normal-depth pixel shader.");

		auto normalDepthAlphaClipPixelShaderBytecode = readShaderBytecode(
			L"Shaders/SsaoNormalDepthAlphaClipPS.cso",
			"Failed to read alpha-clipped normal-depth pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				normalDepthAlphaClipPixelShaderBytecode->GetBufferPointer(),
				normalDepthAlphaClipPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalDepthAlphaClipPixelShader),
			"Failed to create alpha-clipped normal-depth pixel shader.");

		auto debugTextureVertexShaderBytecode = readShaderBytecode(
			L"Shaders/DebugTextureVS.cso",
			"Failed to read debug-texture vertex shader file.");
		HR(
			md3dDevice->CreateVertexShader(
				debugTextureVertexShaderBytecode->GetBufferPointer(),
				debugTextureVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mDebugTextureVertexShader),
			"Failed to create debug-texture vertex shader.");

		auto debugTexturePixelShaderBytecode = readShaderBytecode(
			L"Shaders/DebugTextureRedPS.cso",
			"Failed to read debug-texture pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				debugTexturePixelShaderBytecode->GetBufferPointer(),
				debugTexturePixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mDebugTexturePixelShader),
			"Failed to create debug-texture pixel shader.");

		BuildInputLayouts(
			basicVertexShaderBytecode.get(),
			normalMapVertexShaderBytecode.get(),
			displacementMapVertexShaderBytecode.get());

		auto CreateConstantBuffer = 
			[this](std::uint32_t byteWidth, auto& buffer)
			{
				auto bufferDesc = D3D11::D3D11_BUFFER_DESC{
					.ByteWidth = byteWidth,
					.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
					.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
					.CPUAccessFlags = 0,
					.MiscFlags = 0,
					.StructureByteStride = 0,
				};
				HR(md3dDevice->CreateBuffer(&bufferDesc, nullptr, &buffer), "Failed to create constant buffer.");
			};

		CreateConstantBuffer(sizeof(Basic::PerFrameConstants), mBasicPerFrameCB);
		CreateConstantBuffer(sizeof(Basic::PerObjectConstants), mBasicPerObjectCB);
		CreateConstantBuffer(sizeof(Normal::PerFrameConstants), mNormalMapPerFrameCB);
		CreateConstantBuffer(sizeof(Normal::PerObjectConstants), mNormalMapPerObjectCB);
		CreateConstantBuffer(sizeof(Displacement::PerFrameConstants), mDisplacementMapPerFrameCB);
		CreateConstantBuffer(sizeof(Displacement::PerObjectConstants), mDisplacementMapPerObjectCB);
		CreateConstantBuffer(sizeof(Shadow::PerFrameConstants), mShadowMapPerFrameCB);
		CreateConstantBuffer(sizeof(Shadow::PerObjectConstants), mShadowMapPerObjectCB);
		CreateConstantBuffer(sizeof(NormalDepth::PerObjectConstants), mNormalDepthPerObjectCB);
		CreateConstantBuffer(sizeof(DebugTexture::PerObjectConstants), mDebugTexturePerObjectCB);

		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS,
			.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(md3dDevice->CreateSamplerState(&samplerDesc, &mLinearSampler), "Failed to create linear sampler state.");

		auto shadowSamplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS,
			.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(md3dDevice->CreateSamplerState(&shadowSamplerDesc, &mShadowSampler), "Failed to create shadow sampler state.");

		shadowSamplerDesc.ComparisonFunc =
			D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
		HR(md3dDevice->CreateSamplerState(&shadowSamplerDesc, &mShadowLessEqualSampler), "Failed to create less-equal shadow sampler state.");
	}

	void BuildInputLayouts(
		D3D::ID3DBlob* basicVertexShaderBytecode,
		D3D::ID3DBlob* normalMapVertexShaderBytecode,
		D3D::ID3DBlob* displacementMapVertexShaderBytecode)
	{
		auto basic32Desc = std::array{
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 24,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
		};
		HR(
			md3dDevice->CreateInputLayout(
				basic32Desc.data(),
				static_cast<std::uint32_t>(basic32Desc.size()),
				basicVertexShaderBytecode->GetBufferPointer(),
				basicVertexShaderBytecode->GetBufferSize(),
				&mBasic32InputLayout),
			"Failed to create basic32 input layout.");

		auto posNormalTexTanDesc = std::array{
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 12,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 24,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11::D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TANGENT",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 32,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
		};
		HR(
			md3dDevice->CreateInputLayout(
				posNormalTexTanDesc.data(),
				static_cast<std::uint32_t>(posNormalTexTanDesc.size()),
				normalMapVertexShaderBytecode->GetBufferPointer(),
				normalMapVertexShaderBytecode->GetBufferSize(),
				&mNormalMapInputLayout),
			"Failed to create normal-map input layout.");

		HR(
			md3dDevice->CreateInputLayout(
				posNormalTexTanDesc.data(),
				static_cast<std::uint32_t>(posNormalTexTanDesc.size()),
				displacementMapVertexShaderBytecode->GetBufferPointer(),
				displacementMapVertexShaderBytecode->GetBufferSize(),
				&mDisplacementMapInputLayout),
			"Failed to create displacement-map input layout.");
	}

private:
	std::optional<Sky> mSky;

	ComPtr<D3D11::ID3D11Buffer> mShapesVB;
	ComPtr<D3D11::ID3D11Buffer> mShapesIB;

	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;

	ComPtr<D3D11::ID3D11Buffer> mSkySphereVB;
	ComPtr<D3D11::ID3D11Buffer> mSkySphereIB;

	ComPtr<D3D11::ID3D11Buffer> mScreenQuadVB;
	ComPtr<D3D11::ID3D11Buffer> mScreenQuadIB;

	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickTexSRV;

	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneNormalTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickNormalTexSRV;

	ComPtr<D3D11::ID3D11InputLayout> mBasic32InputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mNormalMapInputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mDisplacementMapInputLayout;

	ComPtr<D3D11::ID3D11Buffer> mBasicPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mBasicPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mDisplacementMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mDisplacementMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mShadowMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mShadowMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalDepthPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mDebugTexturePerObjectCB;

	ComPtr<D3D11::ID3D11VertexShader> mBasic32VertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mBasic32PixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mNormalMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalMapPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mDisplacementMapVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mDisplacementMapHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mDisplacementMapDomainShader;
	ComPtr<D3D11::ID3D11PixelShader> mDisplacementMapPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mShadowMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mShadowMapAlphaClipPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mShadowMapTessVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mShadowMapTessHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mShadowMapTessDomainShader;
	ComPtr<D3D11::ID3D11PixelShader> mShadowMapTessAlphaClipPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mNormalDepthVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalDepthPixelShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalDepthAlphaClipPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mDebugTextureVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mDebugTexturePixelShader;

	ComPtr<D3D11::ID3D11SamplerState> mLinearSampler;
	ComPtr<D3D11::ID3D11SamplerState> mShadowSampler;
	ComPtr<D3D11::ID3D11SamplerState> mShadowLessEqualSampler;

	std::optional<RenderStates> mRenderStates;

	BoundingSphere mSceneBounds{};

	static constexpr auto SMapSize = 2048;
	std::optional<ShadowMap> mSmap;
	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;

	std::optional<Ssao> mSsao;

	float mLightRotationAngle = 0.0f;
	DirectX::XMFLOAT3 mOriginalLightDir[3];
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

	RenderOptions mRenderOptions = RenderOptionsNormalMap;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};
