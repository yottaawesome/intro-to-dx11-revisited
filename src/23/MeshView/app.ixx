export module meshviewdemo:app;
import std;
import shared;
import :sky;
import :ssao;
import :shadowmap;
import :basicmodel;
import :renderstates;
import :sharedvertices;

struct BoundingSphere
{
	DirectX::XMFLOAT3 Center = {0.0f, 0.0f, 0.0f};
	float Radius = 0.0f;
};

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

namespace Shadow
{
	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 gWorld;
		DirectX::XMFLOAT4X4 gWorldInvTranspose;
		DirectX::XMFLOAT4X4 gViewProj;
		DirectX::XMFLOAT4X4 gWorldViewProj;
		DirectX::XMFLOAT4X4 gTexTransform;
	};

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

export class MeshViewApp : public D3DApp
{
public:
	MeshViewApp(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance, L"MeshView Demo" }
	{
		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		mDirLights[0].Ambient = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
		mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.8f, 0.7f, 0.7f, 1.0f);
		mDirLights[0].Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.7f, 1.0f);
		mDirLights[0].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mDirLights[1].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f);

		mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, 0.0, -1.0f);

		mOriginalLightDir[0] = mDirLights[0].Direction;
		mOriginalLightDir[1] = mDirLights[1].Direction;
		mOriginalLightDir[2] = mDirLights[2].Direction;

		Init();
	}

	void Init() override
	{
		D3DApp::Init();

		BuildShaders();
		mRenderStates.emplace(md3dDevice.get());
		mTexMgr.emplace(md3dDevice.get());

		mSky.emplace(md3dDevice.get(), L"Textures/desertcube1024.dds", 5000.0f);
		mSmap.emplace(md3dDevice.get(), SMapSize, SMapSize);

		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		mSsao.emplace(md3dDevice, md3dImmediateContext, mClientWidth, mClientHeight, mCam.GetFovY(), mCam.GetFarZ(), mBasic32InputLayout);

		BuildScreenQuadGeometryBuffers();

		mTreeModel = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\tree.m3d", L"Textures\\");
		mBaseModel = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\base.m3d", L"Textures\\");
		mStairsModel = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\stairs.m3d", L"Textures\\");
		mPillar1Model = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\pillar1.m3d", L"Textures\\");
		mPillar2Model = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\pillar2.m3d", L"Textures\\");
		mPillar3Model = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\pillar5.m3d", L"Textures\\");
		mPillar4Model = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\pillar6.m3d", L"Textures\\");
		mRockModel = std::make_unique<BasicModel>(md3dDevice.get(), *mTexMgr, "Models\\rock.m3d", L"Textures\\");

		BasicModelInstance treeInstance;
		BasicModelInstance baseInstance;
		BasicModelInstance stairsInstance;
		BasicModelInstance pillar1Instance;
		BasicModelInstance pillar2Instance;
		BasicModelInstance pillar3Instance;
		BasicModelInstance pillar4Instance;
		BasicModelInstance rockInstance1;
		BasicModelInstance rockInstance2;
		BasicModelInstance rockInstance3;

		treeInstance.Model = mTreeModel.get();
		baseInstance.Model = mBaseModel.get();
		stairsInstance.Model = mStairsModel.get();
		pillar1Instance.Model = mPillar1Model.get();
		pillar2Instance.Model = mPillar2Model.get();
		pillar3Instance.Model = mPillar3Model.get();
		pillar4Instance.Model = mPillar4Model.get();
		rockInstance1.Model = mRockModel.get();
		rockInstance2.Model = mRockModel.get();
		rockInstance3.Model = mRockModel.get();

		DirectX::XMMATRIX modelScale = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX modelRot = DirectX::XMMatrixRotationY(0.0f);
		DirectX::XMMATRIX modelOffset = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&treeInstance.World, modelScale * modelRot * modelOffset);
		DirectX::XMStoreFloat4x4(&baseInstance.World, modelScale * modelRot * modelOffset);

		modelRot = DirectX::XMMatrixRotationY(0.5f * DirectX::Pi);
		modelOffset = DirectX::XMMatrixTranslation(0.0f, -2.5f, -12.0f);
		DirectX::XMStoreFloat4x4(&stairsInstance.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f);
		modelOffset = DirectX::XMMatrixTranslation(-5.0f, 1.5f, 5.0f);
		DirectX::XMStoreFloat4x4(&pillar1Instance.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f);
		modelOffset = DirectX::XMMatrixTranslation(5.0f, 1.5f, 5.0f);
		DirectX::XMStoreFloat4x4(&pillar2Instance.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f);
		modelOffset = DirectX::XMMatrixTranslation(5.0f, 1.5f, -5.0f);
		DirectX::XMStoreFloat4x4(&pillar3Instance.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		modelOffset = DirectX::XMMatrixTranslation(-5.0f, 1.0f, -5.0f);
		DirectX::XMStoreFloat4x4(&pillar4Instance.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f);
		modelOffset = DirectX::XMMatrixTranslation(-1.0f, 1.4f, -7.0f);
		DirectX::XMStoreFloat4x4(&rockInstance1.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f);
		modelOffset = DirectX::XMMatrixTranslation(5.0f, 1.2f, -2.0f);
		DirectX::XMStoreFloat4x4(&rockInstance2.World, modelScale * modelRot * modelOffset);

		modelScale = DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f);
		modelOffset = DirectX::XMMatrixTranslation(-4.0f, 1.3f, 3.0f);
		DirectX::XMStoreFloat4x4(&rockInstance3.World, modelScale * modelRot * modelOffset);

		mAlphaClippedModelInstances.push_back(treeInstance);

		mModelInstances.push_back(baseInstance);
		mModelInstances.push_back(stairsInstance);
		mModelInstances.push_back(pillar1Instance);
		mModelInstances.push_back(pillar2Instance);
		mModelInstances.push_back(pillar3Instance);
		mModelInstances.push_back(pillar4Instance);
		mModelInstances.push_back(rockInstance1);
		mModelInstances.push_back(rockInstance2);
		mModelInstances.push_back(rockInstance3);

		//
		// Compute scene bounding box.
		//

		DirectX::XMFLOAT3 minPt(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		DirectX::XMFLOAT3 maxPt(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
		for (auto i = 0u; i < mModelInstances.size(); ++i)
		{
			for (auto j = 0u; j < mModelInstances[i].Model->Vertices.size(); ++j)
			{
				DirectX::XMFLOAT3 P = mModelInstances[i].Model->Vertices[j].Pos;

				minPt.x = std::min(minPt.x, P.x);
				minPt.y = std::min(minPt.x, P.x);
				minPt.z = std::min(minPt.x, P.x);

				maxPt.x = std::max(maxPt.x, P.x);
				maxPt.y = std::max(maxPt.x, P.x);
				maxPt.z = std::max(maxPt.x, P.x);
			}
		}

		//
		// Derive scene bounding sphere from bounding box.
		//
		mSceneBounds.Center = DirectX::XMFLOAT3(
			0.5f * (minPt.x + maxPt.x),
			0.5f * (minPt.y + maxPt.y),
			0.5f * (minPt.z + maxPt.z));

		DirectX::XMFLOAT3 extent(
			0.5f * (maxPt.x - minPt.x),
			0.5f * (maxPt.y - minPt.y),
			0.5f * (maxPt.z - minPt.z));
		mSceneBounds.Radius = std::sqrtf(extent.x * extent.x + extent.y * extent.y + extent.z * extent.z);
		mSceneBounds.Radius = std::sqrtf(extent.x * extent.x + extent.y * extent.y + extent.z * extent.z);
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
		// Animate the lights (and hence shadows).
		//

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
		md3dImmediateContext->RSSetViewports(
			1, &mScreenViewport);
		mSsao->SetNormalDepthRenderTarget(
			mDepthStencilView.get());
		DrawSceneToSsaoNormalDepthMap();

		mSsao->ComputeSsao(mCam);
		mSsao->BlurAmbientMap(2);

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

		// The normal/depth pass already populated the scene depth buffer.
		md3dImmediateContext->OMSetDepthStencilState(
			mRenderStates->EqualDSS.get(), 0);

		auto view = mCam.View();
		auto proj = mCam.Proj();
		auto shadowTransform =
			DirectX::XMLoadFloat4x4(&mShadowTransform);
		auto texTransform = DirectX::XMMatrixIdentity();
		const auto toTexSpace = DirectX::XMMATRIX{
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f,
		};

		md3dImmediateContext->IASetInputLayout(
			mPosNormalTexTanInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(
			mNormalMapVertexShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(
			mNormalMapPixelShader.get(), nullptr, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(
				mRenderStates->WireframeRS.get());
		else
			md3dImmediateContext->RSSetState(nullptr);

		auto perFrameConstants = Normal::PerFrameConstants{
			.gDirLights = {
				mDirLights[0],
				mDirLights[1],
				mDirLights[2],
			},
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
			&perFrameConstants,
			0,
			0);
		md3dImmediateContext->PSSetConstantBuffers(
			0, 1, mNormalMapPerFrameCB.GetAddressOf());

		auto samplers = std::array{
			mLinearSampler.get(),
			mShadowSampler.get(),
		};
		md3dImmediateContext->PSSetSamplers(
			0,
			static_cast<std::uint32_t>(samplers.size()),
			samplers.data());

		auto drawInstance =
			[&](const BasicModelInstance& instance)
			{
				auto world =
					DirectX::XMLoadFloat4x4(&instance.World);
				auto worldViewProj = world * view * proj;
				auto constants = Normal::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(
					&constants.gWorld,
					world);
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
					texTransform);
				DirectX::XMStoreFloat4x4(
					&constants.gShadowTransform,
					world * shadowTransform);

				for (auto subset = 0u;
					subset < instance.Model->SubsetCount;
					++subset)
				{
					constants.gMaterial =
						instance.Model->Mat[subset];
					md3dImmediateContext->UpdateSubresource(
						mNormalMapPerObjectCB.get(),
						0,
						nullptr,
						&constants,
						0,
						0);
					md3dImmediateContext->VSSetConstantBuffers(
						1,
						1,
						mNormalMapPerObjectCB.GetAddressOf());
					md3dImmediateContext->PSSetConstantBuffers(
						1,
						1,
						mNormalMapPerObjectCB.GetAddressOf());

					auto pixelResources =
						std::array<D3D11::ID3D11ShaderResourceView*, 5>{
							instance.Model->DiffuseMapSRV[subset].get(),
							instance.Model->NormalMapSRV[subset].get(),
							mSky->CubeMapSRV(),
							mSmap->DepthMapSRV(),
							mSsao->AmbientSRV(),
						};
					md3dImmediateContext->PSSetShaderResources(
						0,
						static_cast<std::uint32_t>(
							pixelResources.size()),
						pixelResources.data());

					instance.Model->ModelMesh.Draw(
						md3dImmediateContext.get(),
						subset);
				}
			};

		for (const auto& instance : mModelInstances)
			drawInstance(instance);

		// The alpha-tested triangles are leaves, so render them double-sided.
		perFrameConstants.gAlphaClip = true;
		md3dImmediateContext->UpdateSubresource(
			mNormalMapPerFrameCB.get(),
			0,
			nullptr,
			&perFrameConstants,
			0,
			0);
		md3dImmediateContext->RSSetState(
			mRenderStates->NoCullRS.get());

		for (const auto& instance : mAlphaClippedModelInstances)
			drawInstance(instance);

		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->OMSetDepthStencilState(nullptr, 0);

		mSky->Draw(md3dImmediateContext.get(), mCam);

		md3dImmediateContext->RSSetState(nullptr);
		md3dImmediateContext->OMSetDepthStencilState(nullptr, 0);

		auto nullResources =
			std::array<D3D11::ID3D11ShaderResourceView*, 16>{};
		md3dImmediateContext->PSSetShaderResources(
			0,
			static_cast<std::uint32_t>(nullResources.size()),
			nullResources.data());

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
		auto texTransform = DirectX::XMMatrixIdentity();

		md3dImmediateContext->IASetInputLayout(
			mPosNormalTexTanInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(
			mNormalDepthVertexShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(
			mNormalDepthPixelShader.get(), nullptr, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(
				mRenderStates->WireframeRS.get());
		else
			md3dImmediateContext->RSSetState(nullptr);

		auto setObjectConstants =
			[&](const BasicModelInstance& instance)
			{
				auto world =
					DirectX::XMLoadFloat4x4(&instance.World);
				auto worldInvTranspose =
					MathHelper::InverseTranspose(world);

				auto constants =
					NormalDepth::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(
					&constants.gWorldView,
					world * view);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldInvTransposeView,
					worldInvTranspose * view);
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
			};

		for (const auto& instance : mModelInstances)
		{
			setObjectConstants(instance);

			for (auto subset = 0u;
				subset < instance.Model->SubsetCount;
				++subset)
			{
				instance.Model->ModelMesh.Draw(
					md3dImmediateContext.get(),
					subset);
			}
		}

		// The alpha-tested triangles are leaves, so render them double-sided.
		md3dImmediateContext->RSSetState(
			mRenderStates->NoCullRS.get());
		md3dImmediateContext->PSSetShader(
			mNormalDepthAlphaClipPixelShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetSamplers(
			0, 1, mLinearSampler.GetAddressOf());

		for (const auto& instance : mAlphaClippedModelInstances)
		{
			setObjectConstants(instance);

			for (auto subset = 0u;
				subset < instance.Model->SubsetCount;
				++subset)
			{
				auto diffuseMap =
					instance.Model->DiffuseMapSRV[subset].get();
				md3dImmediateContext->PSSetShaderResources(
					0, 1, &diffuseMap);
				instance.Model->ModelMesh.Draw(
					md3dImmediateContext.get(),
					subset);
			}
		}

		auto nullResource =
			static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->PSSetShaderResources(
			0, 1, &nullResource);
		md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->RSSetState(nullptr);
	}
	void DrawSceneToShadowMap()
	{
		auto lightView = DirectX::XMLoadFloat4x4(&mLightView);
		auto lightProj = DirectX::XMLoadFloat4x4(&mLightProj);
		auto lightViewProj = lightView * lightProj;
		auto texTransform = DirectX::XMMatrixIdentity();

		md3dImmediateContext->IASetInputLayout(
			mPosNormalTexTanInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->VSSetShader(
			mShadowMapVertexShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(
				mRenderStates->WireframeRS.get());
		else
			md3dImmediateContext->RSSetState(
				mRenderStates->ShadowMapRS.get());

		auto setObjectConstants =
			[&](const BasicModelInstance& instance)
			{
				auto world =
					DirectX::XMLoadFloat4x4(&instance.World);
				auto constants = Shadow::PerObjectConstants{};
				DirectX::XMStoreFloat4x4(
					&constants.gWorld,
					world);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldInvTranspose,
					MathHelper::InverseTranspose(world));
				DirectX::XMStoreFloat4x4(
					&constants.gViewProj,
					lightViewProj);
				DirectX::XMStoreFloat4x4(
					&constants.gWorldViewProj,
					world * lightViewProj);
				DirectX::XMStoreFloat4x4(
					&constants.gTexTransform,
					texTransform);
				md3dImmediateContext->UpdateSubresource(
					mShadowMapPerObjectCB.get(),
					0,
					nullptr,
					&constants,
					0,
					0);
				md3dImmediateContext->VSSetConstantBuffers(
					1, 1, mShadowMapPerObjectCB.GetAddressOf());
			};

		for (const auto& instance : mModelInstances)
		{
			setObjectConstants(instance);

			for (auto subset = 0u;
				subset < instance.Model->SubsetCount;
				++subset)
			{
				instance.Model->ModelMesh.Draw(
					md3dImmediateContext.get(),
					subset);
			}
		}

		md3dImmediateContext->PSSetShader(
			mShadowMapAlphaClipPixelShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetSamplers(
			0, 1, mLinearSampler.GetAddressOf());

		for (const auto& instance : mAlphaClippedModelInstances)
		{
			setObjectConstants(instance);

			for (auto subset = 0u;
				subset < instance.Model->SubsetCount;
				++subset)
			{
				auto diffuseMap =
					instance.Model->DiffuseMapSRV[subset].get();
				md3dImmediateContext->PSSetShaderResources(
					0, 1, &diffuseMap);
				instance.Model->ModelMesh.Draw(
					md3dImmediateContext.get(),
					subset);
			}
		}

		auto nullResource =
			static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->PSSetShaderResources(
			0, 1, &nullResource);
		md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->RSSetState(nullptr);
	}
	void DrawScreenQuad(D3D11::ID3D11ShaderResourceView* srv)
	{
		auto stride = static_cast<std::uint32_t>(sizeof(Vertices::Basic32));
		auto offset = 0u;

		md3dImmediateContext->IASetInputLayout(mBasic32InputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dImmediateContext->IASetVertexBuffers(
			0, 1, mScreenQuadVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(
			mScreenQuadIB.get(), DXGI_FORMAT_R32_UINT, 0);

		md3dImmediateContext->VSSetShader(
			mDebugTextureVertexShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
		md3dImmediateContext->PSSetShader(
			mDebugTexturePixelShader.get(), nullptr, 0);

		// Scale and shift quad to lower-right corner.
		auto world = DirectX::XMMATRIX{
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, -0.5f, 0.0f, 1.0f,
		};

		auto constants = DebugTexture::PerObjectConstants{};
		DirectX::XMStoreFloat4x4(
			&constants.gWorldViewProj,
			world);
		md3dImmediateContext->UpdateSubresource(
			mDebugTexturePerObjectCB.get(),
			0,
			nullptr,
			&constants,
			0,
			0);
		md3dImmediateContext->VSSetConstantBuffers(
			0, 1, mDebugTexturePerObjectCB.GetAddressOf());
		md3dImmediateContext->PSSetShaderResources(0, 1, &srv);
		md3dImmediateContext->PSSetSamplers(
			0, 1, mLinearSampler.GetAddressOf());

		md3dImmediateContext->DrawIndexed(6, 0, 0);

		auto nullResource =
			static_cast<D3D11::ID3D11ShaderResourceView*>(nullptr);
		md3dImmediateContext->PSSetShaderResources(
			0, 1, &nullResource);
	}

	void BuildShaders()
	{
		auto readShaderBytecode = [](const wchar_t* filename, const char* errorMessage)
		{
			auto bytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(filename, &bytecode), errorMessage);
			return bytecode;
		};

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
			"Failed to read alpha-clipped shadow-map pixel shader file.");
		HR(
			md3dDevice->CreatePixelShader(
				shadowMapPixelShaderBytecode->GetBufferPointer(),
				shadowMapPixelShaderBytecode->GetBufferSize(),
				nullptr,
				&mShadowMapAlphaClipPixelShader),
			"Failed to create alpha-clipped shadow-map pixel shader.");

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
			debugTextureVertexShaderBytecode.get(),
			normalMapVertexShaderBytecode.get());

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
			HR(
				md3dDevice->CreateBuffer(&bufferDesc, nullptr, &buffer),
				"Failed to create constant buffer.");
		};

		createConstantBuffer(
			sizeof(Normal::PerFrameConstants),
			mNormalMapPerFrameCB);
		createConstantBuffer(
			sizeof(Normal::PerObjectConstants),
			mNormalMapPerObjectCB);
		createConstantBuffer(
			sizeof(Shadow::PerObjectConstants),
			mShadowMapPerObjectCB);
		createConstantBuffer(
			sizeof(NormalDepth::PerObjectConstants),
			mNormalDepthPerObjectCB);
		createConstantBuffer(
			sizeof(DebugTexture::PerObjectConstants),
			mDebugTexturePerObjectCB);

		auto linearSamplerDesc = D3D11::D3D11_SAMPLER_DESC{
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
		HR(
			md3dDevice->CreateSamplerState(
				&linearSamplerDesc,
				&mLinearSampler),
			"Failed to create linear sampler state.");

		auto shadowSamplerDesc = D3D11::D3D11_SAMPLER_DESC{
			.Filter = D3D11::D3D11_FILTER::D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
			.AddressU = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressV = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressW = D3D11::D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL,
			.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
			.MinLOD = 0.0f,
			.MaxLOD = std::numeric_limits<float>::max(),
		};
		HR(
			md3dDevice->CreateSamplerState(
				&shadowSamplerDesc,
				&mShadowSampler),
			"Failed to create shadow sampler state.");
	}

	void BuildInputLayouts(
		D3D::ID3DBlob* debugTextureVertexShaderBytecode,
		D3D::ID3DBlob* posNormalTexTanVertexShaderBytecode)
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
				debugTextureVertexShaderBytecode->GetBufferPointer(),
				debugTextureVertexShaderBytecode->GetBufferSize(),
				&mBasic32InputLayout),
			"Failed to create Basic32 input layout.");

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
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
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
				posNormalTexTanVertexShaderBytecode->GetBufferPointer(),
				posNormalTexTanVertexShaderBytecode->GetBufferSize(),
				&mPosNormalTexTanInputLayout),
			"Failed to create PosNormalTexTan input layout.");
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

private:

	std::optional<TextureMgr> mTexMgr;

	std::optional<Sky> mSky;

	std::unique_ptr<BasicModel> mTreeModel;
	std::unique_ptr<BasicModel> mBaseModel;
	std::unique_ptr<BasicModel> mStairsModel;
	std::unique_ptr<BasicModel> mPillar1Model;
	std::unique_ptr<BasicModel> mPillar2Model;
	std::unique_ptr<BasicModel> mPillar3Model;
	std::unique_ptr<BasicModel> mPillar4Model;
	std::unique_ptr<BasicModel> mRockModel;

	std::vector<BasicModelInstance> mModelInstances;
	std::vector<BasicModelInstance> mAlphaClippedModelInstances;

	ComPtr<D3D11::ID3D11Buffer> mSkySphereVB;
	ComPtr<D3D11::ID3D11Buffer> mSkySphereIB;

	ComPtr<D3D11::ID3D11Buffer> mScreenQuadVB;
	ComPtr<D3D11::ID3D11Buffer> mScreenQuadIB;

	ComPtr<D3D11::ID3D11InputLayout> mBasic32InputLayout;
	ComPtr<D3D11::ID3D11InputLayout> mPosNormalTexTanInputLayout;

	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerFrameCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mShadowMapPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mNormalDepthPerObjectCB;
	ComPtr<D3D11::ID3D11Buffer> mDebugTexturePerObjectCB;

	ComPtr<D3D11::ID3D11VertexShader> mNormalMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalMapPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mShadowMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mShadowMapAlphaClipPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mNormalDepthVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalDepthPixelShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalDepthAlphaClipPixelShader;
	ComPtr<D3D11::ID3D11VertexShader> mDebugTextureVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mDebugTexturePixelShader;

	ComPtr<D3D11::ID3D11SamplerState> mLinearSampler;
	ComPtr<D3D11::ID3D11SamplerState> mShadowSampler;

	std::optional<RenderStates> mRenderStates;

	BoundingSphere mSceneBounds;

	static const int SMapSize = 2048;
	std::optional<ShadowMap> mSmap;
	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;

	std::optional<Ssao> mSsao;

	float mLightRotationAngle = 0.0f;
	DirectX::XMFLOAT3 mOriginalLightDir[3];
	DirectionalLight mDirLights[3];

	Camera mCam;

	Win32::POINT mLastMousePos{};
};