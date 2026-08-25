export module normaldisplacementmap:app;
import std;
import shared;
import :sky;
import :renderstates;

enum RenderOptions
{
	RenderOptionsBasic = 0,
	RenderOptionsNormalMap = 1,
	RenderOptionsDisplacementMap = 2
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
	static_assert(sizeof(PerFrameConstants) == 256);

	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorld;
		DirectX::XMFLOAT4X4 gWorldInvTranspose;
		DirectX::XMFLOAT4X4 gWorldViewProj;
		DirectX::XMFLOAT4X4 gTexTransform;
		Material gMaterial;
	};
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

	static_assert(sizeof(PerFrameConstants) == 256);
	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorld;
		DirectX::XMFLOAT4X4 gWorldInvTranspose;
		DirectX::XMFLOAT4X4 gWorldViewProj;
		DirectX::XMFLOAT4X4 gTexTransform;
		Material gMaterial;
	};
}

namespace NormalDisplacement
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
		Material gMaterial;
	};

	static_assert(sizeof(PerFrameConstants) == 288);
	static_assert(sizeof(PerObjectConstants) == 384);
}



// Common to both the normal and displacement map shaders.
struct NormalMappedVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
	DirectX::XMFLOAT3 Tangent;
};

export class NormalDisplacementMapApp : public D3DApp
{
public:
	NormalDisplacementMapApp(Win32::HINSTANCE hInstance)
		: D3DApp{hInstance, L"Normal- Displacement Map Demo"}
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
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(0.707f, 0.0f, 0.707f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, 0.707f);

		mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, -0.57735f);

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

		mRenderStates.emplace(md3dDevice.get());
		mSky.emplace(md3dDevice.get(), L"Textures/snowcube1024.dds", 5000.0f);
		
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
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		mCam.UpdateViewMatrix();

		DirectX::XMMATRIX view = mCam.View();
		DirectX::XMMATRIX proj = mCam.Proj();
		DirectX::XMMATRIX viewProj = mCam.ViewProj();

		// Set per frame constants.
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
			.gFogColor = DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f }
		};
		md3dImmediateContext->UpdateSubresource(mBasicVertexPerFrame.get(), 0, nullptr, &basicPerFrameConstants, 0, 0);

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
			.gFogColor = DirectX::XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.0f }
		};
		md3dImmediateContext->UpdateSubresource(mNormalMapPerFrameCB.get(), 0, nullptr, &normalPerFrameConstants, 0, 0);

		// These properties could be set per object if needed.
		auto displacementPerFrameConstants = NormalDisplacement::PerFrameConstants{
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
			.gTessellationPadding = DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f }
		};
		md3dImmediateContext->UpdateSubresource(mDisplacementMapPerFrameCB.get(), 0, nullptr, &displacementPerFrameConstants, 0, 0);

		// Figure out which technique to use for different geometry.
		switch (mRenderOptions)
		{
		case RenderOptionsBasic:
			md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			md3dImmediateContext->IASetInputLayout(mBasicVertexInputLayout.get());
			md3dImmediateContext->VSSetShader(mBasicVertexShader.get(), nullptr, 0);
			md3dImmediateContext->PSSetShader(mBasicPixelShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
			md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
			break;
		case RenderOptionsNormalMap:
			md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			md3dImmediateContext->IASetInputLayout(mNormalMappedVertexInputLayout.get());
			md3dImmediateContext->VSSetShader(mNormalMapVertexShader.get(), nullptr, 0);
			md3dImmediateContext->PSSetShader(mNormalMapPixelShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
			md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
			break;
		case RenderOptionsDisplacementMap:
			md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
			md3dImmediateContext->IASetInputLayout(mDisplacementMappedVertexInputLayout.get());
			md3dImmediateContext->VSSetShader(mDisplacementMapVertexShader.get(), nullptr, 0);
			md3dImmediateContext->PSSetShader(mDisplacementMapPixelShader.get(), nullptr, 0);
			md3dImmediateContext->HSSetShader(mDisplacementMapHullShader.get(), nullptr, 0);
			md3dImmediateContext->DSSetShader(mDisplacementMapDomainShader.get(), nullptr, 0);
			break;
		}

		//
		// Draw the grid, cylinders, and box without any cubemap reflection.
		// 

		auto stride = static_cast<std::uint32_t>(sizeof(Normal::Vertex));
		auto offset = 0u;

		md3dImmediateContext->IASetVertexBuffers(0, 1, mShapesVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mShapesIB.get(), DXGI_FORMAT_R32_UINT, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(mRenderStates->WireframeRS.get());

		// Draw the grid.
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mGridWorld) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

			switch (mRenderOptions)
			{
				case RenderOptionsBasic:
				{
					auto perObjectConstants = Basic::PerObjectConstants{ .gMaterial = mGridMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mBasicVertexPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

					auto vsConstants = std::array{ mBasicVertexPerObject.get() };
					auto psConstants = std::array{ mBasicVertexPerFrame.get(), mBasicVertexPerObject.get() };
					md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
					md3dImmediateContext->PSSetSamplers(0, 1, mAnisotropicSampler.GetAddressOf());

					auto srv = std::array{ mStoneTexSRV.get(), mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					break;
				}
		
				case RenderOptionsNormalMap:
				{
					auto perObjectConstants = Normal::PerObjectConstants{ .gMaterial = mGridMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mNormalMapPerObjectCB.get(), 0, nullptr, &perObjectConstants, 0, 0);

					auto vsConstants = std::array{ mNormalMapPerObjectCB.get() };
					auto psConstants = std::array{ mNormalMapPerFrameCB.get(), mNormalMapPerObjectCB.get() };
					md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
					md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

					auto srv = std::array{ mStoneTexSRV.get(), mStoneNormalTexSRV.get(),  mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					break;
				}

				case RenderOptionsDisplacementMap:
				{
					auto perObjectConstants = NormalDisplacement::PerObjectConstants{ .gMaterial = mGridMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gViewProj, viewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(8.0f, 10.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mDisplacementMapPerObjectCB.get(), 0, nullptr, &perObjectConstants, 0, 0);
				
					auto constants = std::array{ mDisplacementMapPerFrameCB.get(), mDisplacementMapPerObjectCB.get() };
					md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->DSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->DSSetSamplers(0, 1, mLinearSampler.GetAddressOf());
					md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

					auto srv = std::array{ mStoneTexSRV.get(), mStoneNormalTexSRV.get(),  mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					md3dImmediateContext->DSSetShaderResources(1, 1, mStoneNormalTexSRV.GetAddressOf());
					break;
				}
			}
			md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);
		}

		// Draw the box.
		{
			auto world = DirectX::XMMATRIX{ DirectX::XMLoadFloat4x4(&mBoxWorld) };
			auto worldInvTranspose = DirectX::XMMATRIX{ MathHelper::InverseTranspose(world) };
			auto worldViewProj = DirectX::XMMATRIX{ world * viewProj };

			switch (mRenderOptions)
			{
				case RenderOptionsBasic:
			{
				auto perObjectConstants = Basic::PerObjectConstants{ .gMaterial = mBoxMat, };
				DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
				DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
				DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
				DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f));
				md3dImmediateContext->UpdateSubresource(mBasicVertexPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

				auto vsConstants = std::array{ mBasicVertexPerObject.get() };
				auto psConstants = std::array{ mBasicVertexPerFrame.get(), mBasicVertexPerObject.get() };
				md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
				md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
				md3dImmediateContext->PSSetSamplers(0, 1, mAnisotropicSampler.GetAddressOf());

				auto srv = std::array{ mBrickTexSRV.get(), mSky->CubeMapSRV() };
				md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
				break;
			}
				case RenderOptionsNormalMap:
			{
				auto perObjectConstants = Normal::PerObjectConstants{ .gMaterial = mBoxMat, };
				DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
				DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
				DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
				DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f));
				md3dImmediateContext->UpdateSubresource(mNormalMapPerObjectCB.get(), 0, nullptr, &perObjectConstants, 0, 0);

				auto vsConstants = std::array{ mNormalMapPerObjectCB.get() };
				auto psConstants = std::array{ mNormalMapPerFrameCB.get(), mNormalMapPerObjectCB.get() };
				md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
				md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
				md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

				auto srv = std::array{ mBrickTexSRV.get(), mBrickNormalTexSRV.get(), mSky->CubeMapSRV() };
				md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
				break;
			}
				case RenderOptionsDisplacementMap:
				{
					auto perObjectConstants = NormalDisplacement::PerObjectConstants{ .gMaterial = mBoxMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gViewProj, viewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(2.0f, 1.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mDisplacementMapPerObjectCB.get(), 0, nullptr, &perObjectConstants, 0, 0);

					auto constants = std::array{ mDisplacementMapPerFrameCB.get(), mDisplacementMapPerObjectCB.get() };
					md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->DSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->DSSetSamplers(0, 1, mLinearSampler.GetAddressOf());
					md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

					auto srv = std::array{ mBrickTexSRV.get(), mBrickNormalTexSRV.get(), mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					md3dImmediateContext->DSSetShaderResources(1, 1, mBrickNormalTexSRV.GetAddressOf());
					break;
				}
			}

			md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
		}

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			auto world = XMLoadFloat4x4(&mCylWorld[i]);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;

			switch (mRenderOptions)
			{
				case RenderOptionsBasic:
				{
					auto perObjectConstants = Basic::PerObjectConstants{ .gMaterial = mCylinderMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mBasicVertexPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

					auto vsConstants = std::array{ mBasicVertexPerObject.get() };
					auto psConstants = std::array{ mBasicVertexPerFrame.get(), mBasicVertexPerObject.get() };
					md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
					md3dImmediateContext->PSSetSamplers(0, 1, mAnisotropicSampler.GetAddressOf());

					auto srv = std::array{ mBrickTexSRV.get(), mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					break;
				}
				case RenderOptionsNormalMap:
				{
					auto perObjectConstants = Normal::PerObjectConstants{ .gMaterial = mCylinderMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mNormalMapPerObjectCB.get(), 0, nullptr, &perObjectConstants, 0, 0);

					auto vsConstants = std::array{ mNormalMapPerObjectCB.get() };
					auto psConstants = std::array{ mNormalMapPerFrameCB.get(), mNormalMapPerObjectCB.get() };
					md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());
					md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

					auto srv = std::array{ mBrickTexSRV.get(), mBrickNormalTexSRV.get(), mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					break;
				}
				case RenderOptionsDisplacementMap:
				{
					auto perObjectConstants = NormalDisplacement::PerObjectConstants{ .gMaterial = mCylinderMat, };
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gViewProj, viewProj);
					DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixScaling(1.0f, 2.0f, 1.0f));
					md3dImmediateContext->UpdateSubresource(mDisplacementMapPerObjectCB.get(), 0, nullptr, &perObjectConstants, 0, 0);

					auto constants = std::array{ mDisplacementMapPerFrameCB.get(), mDisplacementMapPerObjectCB.get() };
					md3dImmediateContext->VSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->DSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(constants.size()), constants.data());
					md3dImmediateContext->DSSetSamplers(0, 1, mLinearSampler.GetAddressOf());
					md3dImmediateContext->PSSetSamplers(0, 1, mLinearSampler.GetAddressOf());

					auto srv = std::array{ mBrickTexSRV.get(), mBrickNormalTexSRV.get(), mSky->CubeMapSRV() };
					md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());
					md3dImmediateContext->DSSetShaderResources(1, 1, mBrickNormalTexSRV.GetAddressOf());
					break;
				}
			}

			md3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// FX sets tessellation stages, but it does not disable them.  So do that here
		// to turn off tessellation.
		auto nullSrv = static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->DSSetShaderResources(1, 1, &nullSrv);
		md3dImmediateContext->HSSetShader(0, 0, 0);
		md3dImmediateContext->DSSetShader(0, 0, 0);

		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->IASetInputLayout(mBasicVertexInputLayout.get());
		md3dImmediateContext->VSSetShader(mBasicVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mBasicPixelShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetSamplers(0, 1, mAnisotropicSampler.GetAddressOf());

		//
		// Draw the spheres with cubemap reflection.
		//
		basicPerFrameConstants.gUseTexture = false;
		basicPerFrameConstants.gReflectionEnabled = true;
		md3dImmediateContext->UpdateSubresource(mBasicVertexPerFrame.get(), 0, nullptr, &basicPerFrameConstants, 0, 0);
		auto srv = std::array<ID3D11ShaderResourceView*, 2>{ nullptr, mSky->CubeMapSRV() };
		md3dImmediateContext->PSSetShaderResources(0, static_cast<std::uint32_t>(srv.size()), srv.data());

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			auto world = XMLoadFloat4x4(&mSphereWorld[i]);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;

			auto perObjectConstants = Basic::PerObjectConstants{ .gMaterial = mSphereMat, };
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());
			md3dImmediateContext->UpdateSubresource(mBasicVertexPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

			auto vsConstants = std::array{ mBasicVertexPerObject.get() };
			auto psConstants = std::array{ mBasicVertexPerFrame.get(), mBasicVertexPerObject.get() };
			md3dImmediateContext->VSSetConstantBuffers(1, static_cast<std::uint32_t>(vsConstants.size()), vsConstants.data());
			md3dImmediateContext->PSSetConstantBuffers(0, static_cast<std::uint32_t>(psConstants.size()), psConstants.data());

			md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		stride = sizeof(Basic::Vertex);
		offset = 0;

		md3dImmediateContext->RSSetState(nullptr);

		md3dImmediateContext->IASetVertexBuffers(0, 1, mSkullVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mSkullIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Draw the skull.
		{
			auto world = XMLoadFloat4x4(&mSkullWorld);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto worldViewProj = world * viewProj;

			auto perObjectConstants = Basic::PerObjectConstants{ .gMaterial = mSkullMat, };
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldInvTranspose, worldInvTranspose);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
			DirectX::XMStoreFloat4x4(&perObjectConstants.gTexTransform, DirectX::XMMatrixIdentity());

			md3dImmediateContext->UpdateSubresource(mBasicVertexPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);
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

		auto totalVertexCount = static_cast<std::uint32_t>(
			box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size());
		auto totalIndexCount = static_cast<std::uint32_t>(
			mBoxIndexCount + mGridIndexCount + mSphereIndexCount + mCylinderIndexCount);

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
		auto indices = MergeVectors(box.Indices, grid.Indices, sphere.Indices, cylinder.Indices);

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
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Basic::Vertex) * vcount),
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
		auto basicVertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicVS.cso", &basicVertexShaderBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(
				basicVertexShaderBytecode->GetBufferPointer(), basicVertexShaderBytecode->GetBufferSize(), 0, &mBasicVertexShader);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mBasicPixelShader), "Failed to create pixel shader.");
		}

		auto normalMapShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/NormalMapVS.cso", &normalMapShaderBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(
				normalMapShaderBytecode->GetBufferPointer(), normalMapShaderBytecode->GetBufferSize(), 0, &mNormalMapVertexShader);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/NormalMapPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mNormalMapPixelShader), "Failed to create pixel shader.");
		}
		
		auto displacementMapShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapVS.cso", &displacementMapShaderBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(
				displacementMapShaderBytecode->GetBufferPointer(), displacementMapShaderBytecode->GetBufferSize(), 0, &mDisplacementMapVertexShader);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mDisplacementMapPixelShader), "Failed to create pixel shader.");

			// Domain shader
			{
				auto domainShaderBytecode = ComPtr<D3D::ID3DBlob>{};
				HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapDS.cso", &domainShaderBytecode), "Failed to read domain shader file.");
				auto hr = md3dDevice->CreateDomainShader(
					domainShaderBytecode->GetBufferPointer(), domainShaderBytecode->GetBufferSize(), 0, &mDisplacementMapDomainShader);
				HR(hr, "Failed to create domain shader.");
			}

			// Hull shader
			{
				auto hullShaderBytecode = ComPtr<D3D::ID3DBlob>{};
				HR(D3D::D3DReadFileToBlob(L"Shaders/DisplacementMapHS.cso", &hullShaderBytecode), "Failed to read hull shader file.");
				auto hr = md3dDevice->CreateHullShader(
					hullShaderBytecode->GetBufferPointer(), hullShaderBytecode->GetBufferSize(), 0, &mDisplacementMapHullShader);
				HR(hr, "Failed to create hull shader.");
			}
		}

		BuildInputLayouts(basicVertexShaderBytecode.get(), normalMapShaderBytecode.get(), displacementMapShaderBytecode.get());

		// constant buffers basicvertex
		{
			auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(Basic::PerFrameConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perFrameCbd, 0, &mBasicVertexPerFrame), "Failed to create constant buffer.");

			auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(Basic::PerObjectConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mBasicVertexPerObject), "Failed to create constant buffer.");
		}

		// constant buffers normalmap
		{
			auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(Normal::PerFrameConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perFrameCbd, 0, &mNormalMapPerFrameCB), "Failed to create constant buffer.");
			auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(Normal::PerObjectConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mNormalMapPerObjectCB), "Failed to create constant buffer.");
		}

		// constant buffers displacementmap
		{
			auto perFrameCbd = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(NormalDisplacement::PerFrameConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perFrameCbd, 0, &mDisplacementMapPerFrameCB), "Failed to create constant buffer.");
			auto perObjectCbd = D3D11::D3D11_BUFFER_DESC{
				.ByteWidth = sizeof(NormalDisplacement::PerObjectConstants),
				.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0,
			};
			HR(md3dDevice->CreateBuffer(&perObjectCbd, 0, &mDisplacementMapPerObjectCB), "Failed to create constant buffer.");
		}	
	}

	void BuildInputLayouts(
		D3D::ID3DBlob* vsBytecode,
		D3D::ID3DBlob* normalMapVsBytecode,
		D3D::ID3DBlob* displacementMapVsBytecode
	)
	{
		// Create the basic vertex input layout.
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
			vsBytecode->GetBufferPointer(),
			vsBytecode->GetBufferSize(),
			&mBasicVertexInputLayout),
			"Failed to create basic32 input layout.");

		// Create the normal input layout.
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
			normalMapVsBytecode->GetBufferPointer(),
			normalMapVsBytecode->GetBufferSize(),
			&mNormalMappedVertexInputLayout),
			"Failed to create normal mapped input layout.");

		// Create the displacement input layout.
		auto displacementMapDesc = std::array{
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
			displacementMapDesc.data(),
			static_cast<std::uint32_t>(displacementMapDesc.size()),
			displacementMapVsBytecode->GetBufferPointer(),
			displacementMapVsBytecode->GetBufferSize(),
			&mDisplacementMappedVertexInputLayout),
			"Failed to create displacement mapped input layout.");

		// Create anisotropic sampler state.
		auto samplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_ANISOTROPIC,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 8u,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS,
			.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max()
		};
		HR(md3dDevice->CreateSamplerState(&samplerDesc, &mAnisotropicSampler), "Failed to create sampler state.");

		// Create linear sampler state.
		samplerDesc.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		HR(md3dDevice->CreateSamplerState(&samplerDesc, &mLinearSampler), "Failed to create sampler state.");
	}

private:
	std::optional<Sky> mSky;

	ComPtr<D3D11::ID3D11Buffer> mShapesVB;
	ComPtr<D3D11::ID3D11Buffer> mShapesIB;
	ComPtr<D3D11::ID3D11Buffer> mSkullVB;
	ComPtr<D3D11::ID3D11Buffer> mSkullIB;
	ComPtr<D3D11::ID3D11Buffer> mSkySphereVB;
	ComPtr<D3D11::ID3D11Buffer> mSkySphereIB;
	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mStoneNormalTexSRV;
	ComPtr<D3D11::ID3D11ShaderResourceView> mBrickNormalTexSRV;
	ComPtr<D3D11::ID3D11InputLayout> mBasicVertexInputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mNormalMappedVertexInputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mDisplacementMappedVertexInputLayout;

	ComPtr<D3D11::ID3D11Buffer> mBasicVertexPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mBasicVertexPerObject;

	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerObjectCB;

	ComPtr<D3D11::ID3D11Buffer> mDisplacementMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mDisplacementMapPerObjectCB;

	ComPtr<D3D11::ID3D11VertexShader> mBasicVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mBasicPixelShader;

	ComPtr<D3D11::ID3D11VertexShader> mNormalMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalMapPixelShader;

	ComPtr<D3D11::ID3D11VertexShader> mDisplacementMapVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mDisplacementMapHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mDisplacementMapDomainShader;
	ComPtr<D3D11::ID3D11PixelShader> mDisplacementMapPixelShader;

	ComPtr<D3D11::ID3D11SamplerState> mAnisotropicSampler;
	ComPtr<D3D11::ID3D11SamplerState> mLinearSampler;

	std::optional<RenderStates> mRenderStates;

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

	RenderOptions mRenderOptions = RenderOptionsNormalMap;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};
