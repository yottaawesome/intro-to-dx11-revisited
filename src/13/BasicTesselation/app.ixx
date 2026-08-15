export module basictesselation:app;
import std;
import shared;
import :renderstates;

struct cbPerFrame
{
	DirectX::XMFLOAT3 gEyePosW;
	float gPadding;
};

struct cbPerObject
{
	DirectX::XMFLOAT4X4 gWorld;
	DirectX::XMFLOAT4X4 gWorldViewProj;
};

struct Vertex
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

export class BasicTessellation : public D3DApp
{
public:
	BasicTessellation(Win32::HINSTANCE hInstance)
		: D3DApp{ hInstance }
	{
		mMainWndCaption = L"Basic Tessellation Demo";
		mEnable4xMsaa = false;
		Init();
	}

	~BasicTessellation()
	{
		md3dImmediateContext->ClearState();
	}

	void Init() override
	{
		D3DApp::Init();

		mRenderStates = RenderStates{ md3dDevice.get()};
		BuildShaders();
		BuildQuadPatchBuffer();
	}

	void OnResize() override
	{
		D3DApp::OnResize();

		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt) override
	{
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		float z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		float y = mRadius * std::cosf(mPhi);

		mEyePosW = DirectX::XMFLOAT3(x, y, z);

		// Build the view matrix.
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);
		DirectX::XMStoreFloat4x4(&mView, V);
	}

	void DrawScene() override
	{
		md3dImmediateContext->ClearRenderTargetView(
			mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::Silver));
		md3dImmediateContext->ClearDepthStencilView(
			mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		auto blendFactor = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };

		DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&mView);
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);
		DirectX::XMMATRIX viewProj = view * proj;

		md3dImmediateContext->IASetInputLayout(mPos.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
		md3dImmediateContext->VSSetShader(mVertexShader.get(), nullptr, 0);
		md3dImmediateContext->PSSetShader(mPixelShader.get(), nullptr, 0);
		md3dImmediateContext->HSSetShader(mHullShader.get(), nullptr, 0);
		md3dImmediateContext->DSSetShader(mDomainShader.get(), nullptr, 0);

		auto stride = static_cast<std::uint32_t>(sizeof(Vertex));
		auto offset = 0u;

		// Set per frame constants.
		auto perFrameConstants = cbPerFrame{
			.gEyePosW = mEyePosW,
		};
		md3dImmediateContext->UpdateSubresource(mPerFrame.get(), 0, nullptr, &perFrameConstants, 0, 0);

		md3dImmediateContext->IASetVertexBuffers(0, 1, mQuadPatchVB.GetAddressOf(), &stride, &offset);

		// Set per object constants.
		DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		DirectX::XMMATRIX worldViewProj = world * view * proj;

		auto perObjectConstants = cbPerObject{
			.gWorld = DirectX::XMFLOAT4X4{},
			.gWorldViewProj = DirectX::XMFLOAT4X4{},
		};
		DirectX::XMStoreFloat4x4(&perObjectConstants.gWorld, world);
		DirectX::XMStoreFloat4x4(&perObjectConstants.gWorldViewProj, worldViewProj);
		md3dImmediateContext->UpdateSubresource(mPerObject.get(), 0, nullptr, &perObjectConstants, 0, 0);

		auto hsConstantBuffers = std::array{ mPerFrame.get(), mPerObject.get() };
		md3dImmediateContext->HSSetConstantBuffers(0, static_cast<std::uint32_t>(hsConstantBuffers.size()), hsConstantBuffers.data());
		auto dsConstantBuffers = std::array{ mPerObject.get() };
		md3dImmediateContext->DSSetConstantBuffers(1, static_cast<std::uint32_t>(dsConstantBuffers.size()), dsConstantBuffers.data());

		md3dImmediateContext->RSSetState(mRenderStates.WireframeRS.get());
		md3dImmediateContext->Draw(4, 0);

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

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = std::clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			auto dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
			auto dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = std::clamp(mRadius, 5.0f, 300.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
	void BuildQuadPatchBuffer()
	{
		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(DirectX::XMFLOAT3) * 4,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vertices = std::array{
			DirectX::XMFLOAT3{-10.0f, 0.0f, +10.0f},
			DirectX::XMFLOAT3{+10.0f, 0.0f, +10.0f},
			DirectX::XMFLOAT3{-10.0f, 0.0f, -10.0f},
			DirectX::XMFLOAT3{+10.0f, 0.0f, -10.0f}
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = vertices.data()};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mQuadPatchVB));
	}

	void BuildShaders()
	{
		// basic32 shaders
		auto vsbytecode = ComPtr<D3D::ID3DBlob>{};
		{
			HR(D3D::D3DReadFileToBlob(L"Shaders/vs.cso", &vsbytecode), "Failed to read vertex shader file.");
			auto hr = md3dDevice->CreateVertexShader(vsbytecode->GetBufferPointer(), vsbytecode->GetBufferSize(), 0, &mVertexShader);
			HR(hr, "Failed to create vertex shader.");
			auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
			HR(D3D::D3DReadFileToBlob(L"Shaders/ps.cso", &pixelShaderBytecode), "Failed to read pixel shader file.");
			HR(md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), 0, &mPixelShader), "Failed to create pixel shader.");
		}
		BuildInputLayout(vsbytecode.get());

		// domain shader
		auto dsbytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/ds.cso", &dsbytecode), "Failed to read domain shader file.");
		HR(md3dDevice->CreateDomainShader(dsbytecode->GetBufferPointer(), dsbytecode->GetBufferSize(), 0, &mDomainShader), "Failed to create domain shader.");

		// hull shader
		auto hsbytecode = ComPtr<D3D::ID3DBlob>{};
		HR(D3D::D3DReadFileToBlob(L"Shaders/hs.cso", &hsbytecode), "Failed to read hull shader file.");
		HR(md3dDevice->CreateHullShader(hsbytecode->GetBufferPointer(), hsbytecode->GetBufferSize(), 0, &mHullShader), "Failed to create hull shader.");

		// constant buffers basic32
		auto perFrame = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(cbPerFrame),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perFrame, 0, &mPerFrame), "Failed to create constant buffer.");

		auto perObject = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(cbPerObject),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		HR(md3dDevice->CreateBuffer(&perObject, 0, &mPerObject), "Failed to create constant buffer.");
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
		};
		HR(md3dDevice->CreateInputLayout(
			basic32Desc.data(),
			static_cast<std::uint32_t>(basic32Desc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mPos),
			"Failed to create basic32 input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mQuadPatchVB;
	ComPtr<D3D11::ID3D11InputLayout> mPos;
	ComPtr<D3D11::ID3D11VertexShader> mVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mPixelShader;
	ComPtr<D3D11::ID3D11DomainShader> mDomainShader;
	ComPtr<D3D11::ID3D11HullShader> mHullShader;
	ComPtr<D3D11::ID3D11Buffer> mPerFrame;
	ComPtr<D3D11::ID3D11Buffer> mPerObject;

	DirectX::XMFLOAT4X4 mView = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mProj = d3dHelper::Identity4x4;
	DirectX::XMFLOAT3 mEyePosW = {0.0f, 0.0f, 0.0f};
	RenderStates mRenderStates;
	float mTheta = 1.3f * MathHelper::Pi;
	float mPhi = 0.2f * MathHelper::Pi;
	float mRadius = 80.0f;

	Win32::POINT mLastMousePos{};
};