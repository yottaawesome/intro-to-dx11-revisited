export module shadowsdemo:app;
import std;
import shared;
import :sky;
import :renderstates;
import :shadowmap;

enum RenderOptions
{
	RenderOptionsBasic = 0,
	RenderOptionsNormalMap = 1,
	RenderOptionsDisplacementMap = 2
};

struct BoundingSphere
{
	BoundingSphere() : Center(0.0f, 0.0f, 0.0f), Radius(0.0f) {}
	DirectX::XMFLOAT3 Center;
	float Radius;
};

namespace Basic
{
	struct Vertex
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
		DirectX::XMFLOAT4X4 gShadowTransform;
		Material gMaterial;
	};

	static_assert(sizeof(PerFrameConstants) == 256);
	static_assert(sizeof(PerObjectConstants) == 384);
}

namespace Normal
{
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Tex;
		DirectX::XMFLOAT3 Tangent;
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
		DirectX::XMFLOAT4X4 gShadowTransform;
		Material gMaterial;
	};

	static_assert(sizeof(PerFrameConstants) == 256);
	static_assert(sizeof(PerObjectConstants) == 384);
}

namespace Displacement
{
	using Vertex = Normal::Vertex;

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

namespace DebugTexture
{
	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorldViewProj;
	};

	static_assert(sizeof(PerObjectConstants) == 64);
}

export class ShadowsApp : public D3DApp
{
public:
	ShadowsApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance),
		mSkullIndexCount(0),
		mRenderOptions(RenderOptionsNormalMap),
		mLightRotationAngle(0.0f)
	{
		mMainWndCaption = L"Shadows Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		// Estimate the scene bounding sphere manually since we know how the scene was constructed.
		// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
		// the world space origin.  In general, you need to loop over every world space vertex
		// position and compute the bounding sphere.
		mSceneBounds.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		mSceneBounds.Radius = std::sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

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
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		// Shadow acne gets worse as we increase the slope of the polygon (from the
		// perspective of the light).
		//mDirLights[0].Direction = DirectX::XMFLOAT3(5.0f/sqrtf(50.0f), -5.0f/sqrtf(50.0f), 0.0f);
		//mDirLights[0].Direction = DirectX::XMFLOAT3(10.0f/sqrtf(125.0f), -5.0f/sqrtf(125.0f), 0.0f);
		//mDirLights[0].Direction = DirectX::XMFLOAT3(10.0f/sqrtf(116.0f), -4.0f/sqrtf(116.0f), 0.0f);
		//mDirLights[0].Direction = DirectX::XMFLOAT3(10.0f/sqrtf(109.0f), -3.0f/sqrtf(109.0f), 0.0f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f);

		mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, 0.0, -1.0f);

		mOriginalLightDir[0] = mDirLights[0].Direction;
		mOriginalLightDir[1] = mDirLights[1].Direction;
		mOriginalLightDir[2] = mDirLights[2].Direction;

		mGridMat.Ambient = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mGridMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
		mGridMat.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f);
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

		mSky.emplace(md3dDevice.get(), L"Textures/desertcube1024.dds", 5000.0f);
		mSmap.emplace(md3dDevice.get(), SMapSize, SMapSize);
		mRenderStates.emplace(md3dDevice.get());

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
		mSmap->BindDsvAndSetNullRenderTarget(md3dImmediateContext.get());
		DrawSceneToShadowMap();

		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->OMSetRenderTargets(
			1,
			mRenderTargetView.GetAddressOf(),
			mDepthStencilView.get());
		md3dImmediateContext->RSSetViewports(1, &mScreenViewport);

		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(),
			reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(),
			D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL },
			1.0f,
			0);

		auto viewProj = mCam.ViewProj();
		auto shadowTransform = DirectX::XMLoadFloat4x4(&mShadowTransform);

		auto basicPerFrameConstants = Basic::PerFrameConstants{
			.gDirLights = { mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = false,
			.gPadding = DirectX::XMFLOAT2{ 0.0f, 0.0f },
			.gFogColor = DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f },
		};
		md3dImmediateContext->UpdateSubresource(
			mBasicPerFrameCB.get(), 0, nullptr, &basicPerFrameConstants, 0, 0);

		auto normalPerFrameConstants = Normal::PerFrameConstants{
			.gDirLights = { mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = false,
			.gPadding = DirectX::XMFLOAT2{ 0.0f, 0.0f },
			.gFogColor = DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f },
		};
		md3dImmediateContext->UpdateSubresource(
			mNormalMapPerFrameCB.get(), 0, nullptr, &normalPerFrameConstants, 0, 0);

		auto displacementPerFrameConstants = Displacement::PerFrameConstants{
			.gDirLights = { mDirLights[0], mDirLights[1], mDirLights[2] },
			.gEyePosW = mCam.GetPosition(),
			.gFogStart = 15.0f,
			.gFogRange = 175.0f,
			.gLightCount = mLightCount,
			.gUseTexture = true,
			.gAlphaClip = false,
			.gFogEnabled = false,
			.gReflectionEnabled = false,
			.gPadding = DirectX::XMFLOAT2{ 0.0f, 0.0f },
			.gFogColor = DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f },
			.gHeightScale = 0.07f,
			.gMaxTessDistance = 1.0f,
			.gMinTessDistance = 25.0f,
			.gMinTessFactor = 1.0f,
			.gMaxTessFactor = 5.0f,
			.gTessellationPadding = DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f },
		};
		md3dImmediateContext->UpdateSubresource(
			mDisplacementMapPerFrameCB.get(), 0, nullptr, &displacementPerFrameConstants, 0, 0);

		switch (mRenderOptions)
		{
		case RenderOptionsBasic:
			md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			md3dImmediateContext->IASetInputLayout(mBasicVertexInputLayout.get());
			md3dImmediateContext->VSSetShader(mBasicVertexShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
			md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
			md3dImmediateContext->PSSetShader(mBasicPixelShader.get(), nullptr, 0);
			break;
		case RenderOptionsNormalMap:
			md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			md3dImmediateContext->IASetInputLayout(mNormalMapVertexInputLayout.get());
			md3dImmediateContext->VSSetShader(mNormalMapVertexShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
			md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
			md3dImmediateContext->PSSetShader(mNormalMapPixelShader.get(), nullptr, 0);
			break;
		case RenderOptionsDisplacementMap:
			md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
			md3dImmediateContext->IASetInputLayout(mDisplacementMapVertexInputLayout.get());
			md3dImmediateContext->VSSetShader(mDisplacementMapVertexShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(mDisplacementMapHullShader.get(), nullptr, 0);
			md3dImmediateContext->DSSetShader(mDisplacementMapDomainShader.get(), nullptr, 0);
			md3dImmediateContext->PSSetShader(mDisplacementMapPixelShader.get(), nullptr, 0);
			break;
		}
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);

		auto stride = static_cast<std::uint32_t>(sizeof(Normal::Vertex));
		auto offset = 0u;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mShapesVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
		{
			md3dImmediateContext->RSSetState(mRenderStates->WireframeRS.get());
		}

		auto drawTexturedObject = [&](DirectX::CXMMATRIX world,
			DirectX::CXMMATRIX texTransform,
			const Material& material,
			D3D11::ID3D11ShaderResourceView* diffuseMap,
			D3D11::ID3D11ShaderResourceView* normalMap,
			std::uint32_t indexCount,
			std::uint32_t indexOffset,
			std::int32_t vertexOffset)
		{
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;

			switch (mRenderOptions)
			{
			case RenderOptionsBasic:
				{
					auto constants = Basic::PerObjectConstants{};
					DirectX::XMStoreFloat4x4(&constants.gWorld, world);
					DirectX::XMStoreFloat4x4(&constants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&constants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&constants.gTexTransform, texTransform);
					DirectX::XMStoreFloat4x4(&constants.gShadowTransform, world * shadowTransform);
					constants.gMaterial = material;
					md3dImmediateContext->UpdateSubresource(
						mBasicPerObjectCB.get(), 0, nullptr, &constants, 0, 0);

					auto vsConstants = std::array{ mBasicPerObjectCB.get() };
					auto psConstants = std::array{ mBasicPerFrameCB.get(), mBasicPerObjectCB.get() };
					auto samplers = std::array<D3D11::ID3D11SamplerState*, 2>{
						mLinearSampler.get(), mShadowSampler.get()
					};
					auto resources = std::array<D3D11::ID3D11ShaderResourceView*, 3>{
						diffuseMap, mSky->CubeMapSRV(), mSmap->DepthMapSRV()
					};

					md3dImmediateContext->VSSetConstantBuffers(
						1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
					md3dImmediateContext->PSSetConstantBuffers(
						0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
					md3dImmediateContext->PSSetSamplers(
						0, static_cast<std::uint32_t>(samplers.size()), samplers.data());
					md3dImmediateContext->PSSetShaderResources(
						0, static_cast<std::uint32_t>(resources.size()), resources.data());
					break;
				}
			case RenderOptionsNormalMap:
				{
					auto constants = Normal::PerObjectConstants{};
					DirectX::XMStoreFloat4x4(&constants.gWorld, world);
					DirectX::XMStoreFloat4x4(&constants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&constants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&constants.gTexTransform, texTransform);
					DirectX::XMStoreFloat4x4(&constants.gShadowTransform, world * shadowTransform);
					constants.gMaterial = material;
					md3dImmediateContext->UpdateSubresource(
						mNormalMapPerObjectCB.get(), 0, nullptr, &constants, 0, 0);

					auto vsConstants = std::array{ mNormalMapPerObjectCB.get() };
					auto psConstants = std::array{ mNormalMapPerFrameCB.get(), mNormalMapPerObjectCB.get() };
					auto samplers = std::array<D3D11::ID3D11SamplerState*, 2>{
						mLinearSampler.get(), mShadowLessEqualSampler.get()
					};
					auto resources = std::array<D3D11::ID3D11ShaderResourceView*, 4>{
						diffuseMap, normalMap, mSky->CubeMapSRV(), mSmap->DepthMapSRV()
					};

					md3dImmediateContext->VSSetConstantBuffers(
						1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
					md3dImmediateContext->PSSetConstantBuffers(
						0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
					md3dImmediateContext->PSSetSamplers(
						0, static_cast<std::uint32_t>(samplers.size()), samplers.data());
					md3dImmediateContext->PSSetShaderResources(
						0, static_cast<std::uint32_t>(resources.size()), resources.data());
					break;
				}
			case RenderOptionsDisplacementMap:
				{
					auto constants = Displacement::PerObjectConstants{};
					DirectX::XMStoreFloat4x4(&constants.gWorld, world);
					DirectX::XMStoreFloat4x4(&constants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&constants.gViewProj, viewProj);
					DirectX::XMStoreFloat4x4(&constants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&constants.gTexTransform, texTransform);
					DirectX::XMStoreFloat4x4(&constants.gShadowTransform, shadowTransform);
					constants.gMaterial = material;
					md3dImmediateContext->UpdateSubresource(
						mDisplacementMapPerObjectCB.get(), 0, nullptr, &constants, 0, 0);

					auto constantsBuffers = std::array{
						mDisplacementMapPerFrameCB.get(), mDisplacementMapPerObjectCB.get()
					};
					auto samplers = std::array<D3D11::ID3D11SamplerState*, 2>{
						mLinearSampler.get(), mShadowSampler.get()
					};
					auto resources = std::array<D3D11::ID3D11ShaderResourceView*, 4>{
						diffuseMap, normalMap, mSky->CubeMapSRV(), mSmap->DepthMapSRV()
					};

					md3dImmediateContext->VSSetConstantBuffers(
						0, static_cast<std::uint32_t>(constantsBuffers.size()), constantsBuffers.data());
					md3dImmediateContext->DSSetConstantBuffers(
						0, static_cast<std::uint32_t>(constantsBuffers.size()), constantsBuffers.data());
					md3dImmediateContext->PSSetConstantBuffers(
						0, static_cast<std::uint32_t>(constantsBuffers.size()), constantsBuffers.data());
					md3dImmediateContext->DSSetSamplers(0, 1, mLinearSampler.GetAddressOf());
					md3dImmediateContext->PSSetSamplers(
						0, static_cast<std::uint32_t>(samplers.size()), samplers.data());
					md3dImmediateContext->DSSetShaderResources(1, 1, &normalMap);
					md3dImmediateContext->PSSetShaderResources(
						0, static_cast<std::uint32_t>(resources.size()), resources.data());
					break;
				}
			}

			md3dImmediateContext->DrawIndexed(indexCount, indexOffset, vertexOffset);
		};

		drawTexturedObject(
			DirectX::XMLoadFloat4x4(&mGridWorld),
			DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f),
			mGridMat,
			mStoneTexSRV.get(),
			mStoneNormalTexSRV.get(),
			mGridIndexCount,
			mGridIndexOffset,
			mGridVertexOffset);

		drawTexturedObject(
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
			drawTexturedObject(
				DirectX::XMLoadFloat4x4(&mCylWorld[i]),
				DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f),
				mCylinderMat,
				mBrickTexSRV.get(),
				mBrickNormalTexSRV.get(),
				mCylinderIndexCount,
				mCylinderIndexOffset,
				mCylinderVertexOffset);
		}

		auto nullResource = static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->DSSetShaderResources(1, 1, &nullResource);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);

		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->IASetInputLayout(mBasicVertexInputLayout.get());
		md3dImmediateContext->VSSetShader(mBasicVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mBasicPixelShader.get(), nullptr, 0);

		basicPerFrameConstants.gUseTexture = false;
		basicPerFrameConstants.gReflectionEnabled = true;
		md3dImmediateContext->UpdateSubresource(
			mBasicPerFrameCB.get(), 0, nullptr, &basicPerFrameConstants, 0, 0);

		auto basicSamplers = std::array<D3D11::ID3D11SamplerState*, 2>{
			mLinearSampler.get(), mShadowSampler.get()
		};
		auto basicResources = std::array<D3D11::ID3D11ShaderResourceView*, 3>{
			nullptr, mSky->CubeMapSRV(), mSmap->DepthMapSRV()
		};
		md3dImmediateContext->PSSetSamplers(
			0, static_cast<std::uint32_t>(basicSamplers.size()), basicSamplers.data());
		md3dImmediateContext->PSSetShaderResources(
			0, static_cast<std::uint32_t>(basicResources.size()), basicResources.data());

		auto drawReflectiveObject = [&](DirectX::CXMMATRIX world,
			const Material& material,
			std::uint32_t indexCount,
			std::uint32_t indexOffset,
			std::int32_t vertexOffset)
		{
			auto constants = Basic::PerObjectConstants{};
			DirectX::XMStoreFloat4x4(&constants.gWorld, world);
			DirectX::XMStoreFloat4x4(
				&constants.gWorldInvTranspose, MathHelper::InverseTranspose(world));
			DirectX::XMStoreFloat4x4(&constants.gWorldViewProj, world * viewProj);
			DirectX::XMStoreFloat4x4(&constants.gTexTransform, DirectX::XMMatrixIdentity());
			DirectX::XMStoreFloat4x4(
				&constants.gShadowTransform, world * shadowTransform);
			constants.gMaterial = material;
			md3dImmediateContext->UpdateSubresource(
				mBasicPerObjectCB.get(), 0, nullptr, &constants, 0, 0);

			auto vsConstants = std::array{ mBasicPerObjectCB.get() };
			auto psConstants = std::array{ mBasicPerFrameCB.get(), mBasicPerObjectCB.get() };
			md3dImmediateContext->VSSetConstantBuffers(
				1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
			md3dImmediateContext->PSSetConstantBuffers(
				0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
			md3dImmediateContext->DrawIndexed(indexCount, indexOffset, vertexOffset);
		};

		for (auto i = 0; i < 10; ++i)
		{
			drawReflectiveObject(
				DirectX::XMLoadFloat4x4(&mSphereWorld[i]),
				mSphereMat,
				mSphereIndexCount,
				mSphereIndexOffset,
				mSphereVertexOffset);
		}

		md3dImmediateContext->RSSetState(nullptr);

		stride = static_cast<std::uint32_t>(sizeof(Basic::Vertex));
		offset = 0;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mSkullVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);
		drawReflectiveObject(
			DirectX::XMLoadFloat4x4(&mSkullWorld),
			mSkullMat,
			mSkullIndexCount,
			0,
			0);

		DrawScreenQuad();
		mSky->Draw(md3dImmediateContext.get(), mCam);

		auto nullResources = std::array<D3D11::ID3D11ShaderResourceView*, 16>{};
		md3dImmediateContext->PSSetShaderResources(
			0, static_cast<std::uint32_t>(nullResources.size()), nullResources.data());

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

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
		md3dImmediateContext->IASetInputLayout(mBasicVertexInputLayout.get());

		auto shapeStride = static_cast<std::uint32_t>(sizeof(Normal::Vertex));
		auto offset = 0u;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mShapesVB.GetAddressOf(), &shapeStride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		auto DrawShadowObject = 
			[&](
				DirectX::CXMMATRIX world,
				DirectX::CXMMATRIX texTransform,
				D3D11::ID3D11ShaderResourceView* normalMap,
				bool tessellated,
				std::uint32_t indexCount,
				std::uint32_t indexOffset,
				std::int32_t vertexOffset
			)
			{
				auto constants = Shadow::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(&constants.gWorld, world);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldInvTranspose, MathHelper::InverseTranspose(world));
				DirectX::XMStoreFloat4x4(&constants.gViewProj, lightViewProj);
				DirectX::XMStoreFloat4x4(&constants.gWorldViewProj, world * lightViewProj);
				DirectX::XMStoreFloat4x4(&constants.gTexTransform, texTransform);
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
						mShadowMapPerFrameCB.get(), mShadowMapPerObjectCB.get()
					};
					md3dImmediateContext->VSSetConstantBuffers(
						0, static_cast<std::uint32_t>(constantBuffers.size()), constantBuffers.data());
					md3dImmediateContext->DSSetConstantBuffers(
						0, static_cast<std::uint32_t>(constantBuffers.size()), constantBuffers.data());
					md3dImmediateContext->DSSetSamplers(
						0, 1, mLinearSampler.GetAddressOf());
					md3dImmediateContext->DSSetShaderResources(1, 1, &normalMap);
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

				md3dImmediateContext->DrawIndexed(indexCount, indexOffset, vertexOffset);
			};

		auto tessellated = mRenderOptions == RenderOptionsDisplacementMap;
		DrawShadowObject(
			DirectX::XMLoadFloat4x4(&mGridWorld),
			DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f),
			mStoneNormalTexSRV.get(),
			tessellated,
			mGridIndexCount,
			mGridIndexOffset,
			mGridVertexOffset);

		DrawShadowObject(
			DirectX::XMLoadFloat4x4(&mBoxWorld),
			DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f),
			mBrickNormalTexSRV.get(),
			tessellated,
			mBoxIndexCount,
			mBoxIndexOffset,
			mBoxVertexOffset);

		for (auto i = 0; i < 10; ++i)
		{
			DrawShadowObject(
				DirectX::XMLoadFloat4x4(&mCylWorld[i]),
				DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f),
				mBrickNormalTexSRV.get(),
				tessellated,
				mCylinderIndexCount,
				mCylinderIndexOffset,
				mCylinderVertexOffset);
		}

		auto nullResource = static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->DSSetShaderResources(1, 1, &nullResource);

		for (auto i = 0; i < 10; ++i)
		{
			DrawShadowObject(
				DirectX::XMLoadFloat4x4(&mSphereWorld[i]),
				DirectX::XMMatrixIdentity(),
				nullptr,
				false,
				mSphereIndexCount,
				mSphereIndexOffset,
				mSphereVertexOffset);
		}

		auto skullStride = static_cast<std::uint32_t>(sizeof(Basic::Vertex));
		offset = 0;
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mSkullVB.GetAddressOf(), &skullStride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);
		DrawShadowObject(
			DirectX::XMLoadFloat4x4(&mSkullWorld),
			DirectX::XMMatrixIdentity(),
			nullptr,
			false,
			mSkullIndexCount,
			0,
			0
		);
	}

	void DrawScreenQuad()
	{
		auto stride = static_cast<std::uint32_t>(sizeof(Basic::Vertex));
		auto offset = 0u;

		md3dImmediateContext->IASetInputLayout(mBasicVertexInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mScreenQuadVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mScreenQuadIB.get(), DXGI_FORMAT_R32_UINT, 0);
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
		auto shadowMap = mSmap->DepthMapSRV();
		md3dImmediateContext->PSSetShaderResources(0, 1, &shadowMap);
		md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

		md3dImmediateContext->DrawIndexed(6, 0, 0);
	}

	void BuildShadowTransform()
	{
		// Only the first "main" light casts a shadow.
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mDirLights[0].Direction);
		DirectX::XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
		DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&mSceneBounds.Center);
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
		auto totalIndexCount = mBoxIndexCount + mGridIndexCount + mSphereIndexCount + mCylinderIndexCount;

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//
		auto vertices = std::vector<Normal::Vertex>(totalVertexCount);

		auto k = 0u;
		for (auto i = 0ull; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
			vertices[k].Tex = box.Vertices[i].TexC;
			vertices[k].Tangent = box.Vertices[i].TangentU;
		}

		for (auto i = 0ull; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Normal = grid.Vertices[i].Normal;
			vertices[k].Tex = grid.Vertices[i].TexC;
			vertices[k].Tangent = grid.Vertices[i].TangentU;
		}

		for (auto i = 0ull; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Normal = sphere.Vertices[i].Normal;
			vertices[k].Tex = sphere.Vertices[i].TexC;
			vertices[k].Tangent = sphere.Vertices[i].TangentU;
		}

		for (auto i = 0ull; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Normal = cylinder.Vertices[i].Normal;
			vertices[k].Tex = cylinder.Vertices[i].TexC;
			vertices[k].Tangent = cylinder.Vertices[i].TangentU;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Normal::Vertex) * totalVertexCount),
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
			.ByteWidth = sizeof(std::uint32_t) * totalIndexCount,
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
		auto fin = std::ifstream{ "Models/skull.txt" };
		if (not fin)
			throw std::runtime_error{ "Models/skull.txt not found." };

		auto vcount = 0u;
		auto tcount = 0u;
		auto ignore = std::string{};

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		auto vertices = std::vector<Basic::Vertex>(vcount);
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
			.ByteWidth = sizeof(Basic::Vertex) * vcount,
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

	void BuildScreenQuadGeometryBuffers()
	{
		auto quad = GeometryGenerator::MeshData{};

		auto geoGen = GeometryGenerator{};
		geoGen.CreateFullscreenQuad(quad);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		auto vertices = std::vector<Basic::Vertex>(quad.Vertices.size());

		for (auto i = 0u; i < quad.Vertices.size(); ++i)
		{
			vertices[i].Pos = quad.Vertices[i].Position;
			vertices[i].Normal = quad.Vertices[i].Normal;
			vertices[i].Tex = quad.Vertices[i].TexC;
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic::Vertex) * quad.Vertices.size()),
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
		auto basicVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicVS.cso", &basicVertexShaderBytecode), "Failed to read basic vertex shader file.");
			HR(md3dDevice->CreateVertexShader(
				basicVertexShaderBytecode->GetBufferPointer(),
				basicVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mBasicVertexShader),
				"Failed to create basic vertex shader.");

			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicPS.cso", &pixelShaderBytecode), "Failed to read basic pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(),
				pixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mBasicPixelShader),
				"Failed to create basic pixel shader.");
		}

		auto normalMapVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/NormalMapVS.cso", &normalMapVertexShaderBytecode), "Failed to read normal-map vertex shader file.");
			HR(md3dDevice->CreateVertexShader(
				normalMapVertexShaderBytecode->GetBufferPointer(),
				normalMapVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalMapVertexShader),
				"Failed to create normal-map vertex shader.");

			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/NormalMapPS.cso", &pixelShaderBytecode), "Failed to read normal-map pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(),
				pixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mNormalMapPixelShader),
				"Failed to create normal-map pixel shader.");
		}

		auto displacementMapVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapVS.cso", &displacementMapVertexShaderBytecode), "Failed to read displacement-map vertex shader file.");
			HR(md3dDevice->CreateVertexShader(
				displacementMapVertexShaderBytecode->GetBufferPointer(),
				displacementMapVertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapVertexShader),
				"Failed to create displacement-map vertex shader.");

			auto hullShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapHS.cso", &hullShaderBytecode), "Failed to read displacement-map hull shader file.");
			HR(md3dDevice->CreateHullShader(
				hullShaderBytecode->GetBufferPointer(),
				hullShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapHullShader),
				"Failed to create displacement-map hull shader.");

			auto domainShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapDS.cso", &domainShaderBytecode), "Failed to read displacement-map domain shader file.");
			HR(md3dDevice->CreateDomainShader(
				domainShaderBytecode->GetBufferPointer(),
				domainShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapDomainShader),
				"Failed to create displacement-map domain shader.");

			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapPS.cso", &pixelShaderBytecode), "Failed to read displacement-map pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(),
				pixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mDisplacementMapPixelShader),
				"Failed to create displacement-map pixel shader.");
		}

		{
			auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BuildShadowMapVS.cso", &vertexShaderBytecode), "Failed to read shadow-map vertex shader file.");
			HR(md3dDevice->CreateVertexShader(
				vertexShaderBytecode->GetBufferPointer(),
				vertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapVertexShader),
				"Failed to create shadow-map vertex shader.");
		}

		{
			auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BuildShadowMapTessVS.cso", &vertexShaderBytecode), "Failed to read tessellated shadow-map vertex shader file.");
			HR(md3dDevice->CreateVertexShader(
				vertexShaderBytecode->GetBufferPointer(),
				vertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessVertexShader),
				"Failed to create tessellated shadow-map vertex shader.");

			auto hullShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BuildShadowMapTessHS.cso", &hullShaderBytecode), "Failed to read tessellated shadow-map hull shader file.");
			HR(md3dDevice->CreateHullShader(
				hullShaderBytecode->GetBufferPointer(),
				hullShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessHullShader),
				"Failed to create tessellated shadow-map hull shader.");

			auto domainShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BuildShadowMapTessDS.cso", &domainShaderBytecode), "Failed to read tessellated shadow-map domain shader file.");
			HR(md3dDevice->CreateDomainShader(
				domainShaderBytecode->GetBufferPointer(),
				domainShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapTessDomainShader),
				"Failed to create tessellated shadow-map domain shader.");
		}

		{
			auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/DebugTextureVS.cso", &vertexShaderBytecode), "Failed to read debug-texture vertex shader file.");
			HR(md3dDevice->CreateVertexShader(
				vertexShaderBytecode->GetBufferPointer(),
				vertexShaderBytecode->GetBufferSize(),
				nullptr,
				&mDebugTextureVertexShader),
				"Failed to create debug-texture vertex shader.");

			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/DebugTextureRedPS.cso", &pixelShaderBytecode), "Failed to read debug-texture pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(),
				pixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mDebugTexturePixelShader),
				"Failed to create debug-texture pixel shader.");
		}

		BuildInputLayouts(
			basicVertexShaderBytecode.get(),
			normalMapVertexShaderBytecode.get(),
			displacementMapVertexShaderBytecode.get());

		auto createConstantBuffer = [this](std::uint32_t byteWidth, auto& buffer)
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

		createConstantBuffer(sizeof(Basic::PerFrameConstants), mBasicPerFrameCB);
		createConstantBuffer(sizeof(Basic::PerObjectConstants), mBasicPerObjectCB);
		createConstantBuffer(sizeof(Normal::PerFrameConstants), mNormalMapPerFrameCB);
		createConstantBuffer(sizeof(Normal::PerObjectConstants), mNormalMapPerObjectCB);
		createConstantBuffer(sizeof(Displacement::PerFrameConstants), mDisplacementMapPerFrameCB);
		createConstantBuffer(sizeof(Displacement::PerObjectConstants), mDisplacementMapPerObjectCB);
		createConstantBuffer(sizeof(Shadow::PerFrameConstants), mShadowMapPerFrameCB);
		createConstantBuffer(sizeof(Shadow::PerObjectConstants), mShadowMapPerObjectCB);
		createConstantBuffer(sizeof(DebugTexture::PerObjectConstants), mDebugTexturePerObjectCB);

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

		shadowSamplerDesc.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
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
			basicVertexShaderBytecode->GetBufferPointer(),
			basicVertexShaderBytecode->GetBufferSize(),
			&mBasicVertexInputLayout),
			"Failed to create basic32 input layout.");

		auto normalMapDesc = std::array{
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
				.SemanticName = "TANGENT",
				.SemanticIndex = 0,
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 32,
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};
		HR(md3dDevice->CreateInputLayout(
			normalMapDesc.data(),
			static_cast<std::uint32_t>(normalMapDesc.size()),
			normalMapVertexShaderBytecode->GetBufferPointer(),
			normalMapVertexShaderBytecode->GetBufferSize(),
			&mNormalMapVertexInputLayout),
			"Failed to create normal-map input layout.");

		HR(md3dDevice->CreateInputLayout(
			normalMapDesc.data(),
			static_cast<std::uint32_t>(normalMapDesc.size()),
			displacementMapVertexShaderBytecode->GetBufferPointer(),
			displacementMapVertexShaderBytecode->GetBufferSize(),
			&mDisplacementMapVertexInputLayout),
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

	ComPtr<D3D11::ID3D11InputLayout> mBasicVertexInputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mNormalMapVertexInputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mDisplacementMapVertexInputLayout;

	ComPtr<D3D11::ID3D11Buffer> mBasicPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mBasicPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mDisplacementMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mDisplacementMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mShadowMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mShadowMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mDebugTexturePerObjectCB;

	ComPtr<D3D11::ID3D11VertexShader> mBasicVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mBasicPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mNormalMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalMapPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mDisplacementMapVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mDisplacementMapHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mDisplacementMapDomainShader;
	ComPtr<D3D11::ID3D11PixelShader> mDisplacementMapPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mShadowMapVertexShader;
	ComPtr<D3D11::ID3D11VertexShader> mShadowMapTessVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mShadowMapTessHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mShadowMapTessDomainShader;
	ComPtr<D3D11::ID3D11VertexShader> mDebugTextureVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mDebugTexturePixelShader;

	ComPtr<D3D11::ID3D11SamplerState> mLinearSampler;
	ComPtr<D3D11::ID3D11SamplerState> mShadowSampler;
	ComPtr<D3D11::ID3D11SamplerState> mShadowLessEqualSampler;

	std::optional<RenderStates> mRenderStates;

	BoundingSphere mSceneBounds;

	static constexpr auto SMapSize = 2048;
	std::optional<ShadowMap> mSmap;
	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;

	float mLightRotationAngle;
	DirectX::XMFLOAT3 mOriginalLightDir[3];
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

	RenderOptions mRenderOptions;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};