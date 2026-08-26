export module terraindemo:app;
import std;
import shared;
import :sky;
import :terrain;
import :renderstates;

export class TerrainApp : public D3DApp
{
public:
	TerrainApp(Win32::HINSTANCE hInstance)
		: D3DApp{hInstance, L"Terrain Demo"}
	{
		mEnable4xMsaa = false;
		Init();
	}

	void Init()override
	{
		D3DApp::Init();

		mSky.emplace(md3dDevice.get(), L"Textures/grasscube1024.dds", 5000.0f);

		auto tii = Terrain::InitInfo{
			.HeightMapFilename = L"Textures/terrain.raw",
			.LayerMapFilename0 = L"Textures/grass.dds",
			.LayerMapFilename1 = L"Textures/darkdirt.dds",
			.LayerMapFilename2 = L"Textures/stone.dds",
			.LayerMapFilename3 = L"Textures/lightdirt.dds",
			.LayerMapFilename4 = L"Textures/snow.dds",
			.BlendMapFilename = L"Textures/blend.dds",
			.HeightScale = 50.0f,
			.HeightmapWidth = 2049,
			.HeightmapHeight = 2049,
			.CellSpacing = 0.5f
		};

		mTerrain.emplace(md3dDevice.get(), md3dImmediateContext.get(), tii);
		mRenderStates.emplace(md3dDevice.get());
		BuildShaders();
	}

	void OnResize()override
	{
		D3DApp::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 3000.0f);
	}

	void UpdateScene(float dt)override
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
		// Walk/fly mode
		//
		if (Win32::GetAsyncKeyState('2') & 0x8000)
			mWalkCamMode = true;
		if (Win32::GetAsyncKeyState('3') & 0x8000)
			mWalkCamMode = false;

		// 
		// Clamp camera to terrain surface in walk mode.
		//
		if (mWalkCamMode)
		{
			auto camPos = DirectX::XMFLOAT3{mCam.GetPosition()};
			auto y = mTerrain->GetHeight(camPos.x, camPos.z);
			mCam.SetPosition(camPos.x, y + 2.0f, camPos.z);
		}

		mCam.UpdateViewMatrix();
	}

	void DrawScene()override
	{
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11::D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (Win32::GetAsyncKeyState('1') & 0x8000)
			md3dImmediateContext->RSSetState(mRenderStates->WireframeRS.get());

		mTerrain->Draw(md3dImmediateContext.get(), mCam, mDirLights);

		md3dImmediateContext->RSSetState(0);

		mSky->Draw(md3dImmediateContext.get(), mCam);

		// restore default states, as the SkyFX changes them in the effect file.
		md3dImmediateContext->RSSetState(0);
		md3dImmediateContext->OMSetDepthStencilState(0, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)override
	{
		mLastMousePos = {x, y};
		Win32::SetCapture(mhMainWnd);
	}

	void OnMouseUp(Win32::WPARAM btnState, int x, int y)override
	{
		Win32::ReleaseCapture();
	}

	void OnMouseMove(Win32::WPARAM btnState, int x, int y)override
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
	void BuildShaders()
	{
		// Vertex shader
		auto vsBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicVS.cso", &vsBytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(
				vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), 0, &mVertexShader);
			HR(hr, "Failed to create vertex shader.");
		}

		// Pixel shader
		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/BasicPS.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			auto hr = md3dDevice->CreatePixelShader(
				pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader);
			HR(hr, "Failed to create pixel shader.");
		}
		BuildInputLayout(vsBytecode.get());
	}

	void BuildInputLayout(D3D::ID3DBlob* vsBlob)
	{
		auto inputDesc = std::array{
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
			inputDesc.data(), static_cast<std::uint32_t>(inputDesc.size()),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &mInputLayout), "Failed to create input layout.");
	}

	std::optional<Sky> mSky;
	std::optional<Terrain> mTerrain;
	std::optional<RenderStates> mRenderStates;
	ComPtr<ID3D11InputLayout> mInputLayout;
	ComPtr<ID3D11VertexShader> mVertexShader;
	ComPtr<ID3D11PixelShader> mPixelShader;

	DirectionalLight mDirLights[3]{
		DirectionalLight{
			.Ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f),
			.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f),
			.Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f)
		},
		DirectionalLight{
			.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
			.Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f)
		},
		DirectionalLight{
			.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
			.Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f),
			.Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, -0.57735f)
		}
	};

	Camera mCam = Camera::Position{ 0.0f, 2.0f, 100.0f };

	bool mWalkCamMode = false;

	Win32::POINT mLastMousePos{};
};