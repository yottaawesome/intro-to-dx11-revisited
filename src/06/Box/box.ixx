export module box;
import std;
import shared;

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj;
};

export class BoxApp : public D3DApp
{
public:
	BoxApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		mMainWndCaption = L"Box Demo";

		auto I = DirectX::XMMATRIX{DirectX::XMMatrixIdentity()};
		DirectX::XMStoreFloat4x4(&mWorld, I);
		DirectX::XMStoreFloat4x4(&mView, I);
		DirectX::XMStoreFloat4x4(&mProj, I);

		Init();
	}

	~BoxApp()
	{
		mBoxVB.reset();
		mBoxIB.reset();
		mColorVS.reset();
		mColorPS.reset();
		mPerObjectCB.reset();
		mInputLayout.reset();
	}

	void OnResize()
	{
		D3DApp::OnResize();

		// The window resized, so update the aspect ratio and recompute the projection matrix.
		auto P = DirectX::XMMATRIX{ DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f)};
		DirectX::XMStoreFloat4x4(&mProj, P);
	}
	void UpdateScene(float dt)
	{
		// Convert Spherical to Cartesian coordinates.
		auto x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		auto z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		auto y = mRadius * std::cosf(mPhi);

		// Build the view matrix.
		auto pos = DirectX::XMVECTOR{ DirectX::XMVectorSet(x, y, z, 1.0f) };
		auto target = DirectX::XMVECTOR{ DirectX::XMVectorZero() };
		auto up = DirectX::XMVECTOR{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
		auto V = DirectX::XMMATRIX{ DirectX::XMMatrixLookAtLH(pos, target, up) };
		DirectX::XMStoreFloat4x4(&mView, V);
	}

	void DrawScene()
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::LightSteelBlue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto stride = std::uint32_t{ sizeof(Vertex) };
		auto offset = 0u;
		md3dImmediateContext->IASetVertexBuffers(0, 1, mBoxVB.GetAddressOf(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mBoxIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set constants
		auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mWorld)};
		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto worldViewProj = DirectX::XMMATRIX{world * view * proj};

		auto perObject = PerObjectConstants{};
		DirectX::XMStoreFloat4x4(&perObject.WorldViewProj, worldViewProj);
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, 0, &perObject, 0, 0);

		md3dImmediateContext->VSSetShader(mColorVS.get(), 0, 0);
		md3dImmediateContext->VSSetConstantBuffers(0, 1, mPerObjectCB.GetAddressOf());
		md3dImmediateContext->PSSetShader(mColorPS.get(), 0, 0);

		// 36 indices for the box.
		md3dImmediateContext->DrawIndexed(36, 0, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;
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
			// Make each pixel correspond to 0.005 unit in the scene.
			auto dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
			auto dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = std::clamp(mRadius, 3.0f, 15.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

private:
	void Init()
	{
		D3DApp::Init();

		BuildGeometryBuffers();
		BuildShaders();
	}

	void BuildGeometryBuffers()
	{
		// Create vertex buffer
		auto vertices = std::array{
			Vertex{ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) },
			Vertex{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) },
			Vertex{ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) },
			Vertex{ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) },
			Vertex{ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) },
			Vertex{ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) },
			Vertex{ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) },
			Vertex{ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) }
		};

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(Vertex) * 8,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = vertices.data()
		};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

		// Create the index buffer
		auto indices = std::array<std::uint32_t, 36>{
			// front face
			0, 1, 2,
			0, 2, 3,

			// back face
			4, 6, 5,
			4, 7, 6,

			// left face
			4, 5, 1,
			4, 1, 0,

			// right face
			3, 2, 6,
			3, 6, 7,

			// top face
			1, 5, 6,
			1, 6, 2,

			// bottom face
			4, 0, 3,
			4, 3, 7
		};

		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(UINT) * 36,
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{
			.pSysMem = indices.data()
		};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		auto hr = D3D::D3DReadFileToBlob(L"FX/color_VS.cso", &vertexShaderBytecode);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to read vertex shader file." };

		hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create vertex shader." };

		if (!BuildVertexLayout(vertexShaderBytecode.get()))
			throw std::runtime_error{ "Failed to create input layout." };
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		hr = D3DReadFileToBlob(L"FX/color_PS.cso", &pixelShaderBytecode);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to read pixel shader file." };

		hr = md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(),
			pixelShaderBytecode->GetBufferSize(), 0, &mColorPS);
		pixelShaderBytecode.reset();
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create pixel shader." };

		auto cbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(PerObjectConstants),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};
		
		hr = md3dDevice->CreateBuffer(&cbd, 0, &mPerObjectCB);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create constant buffer." };
	}

	auto BuildVertexLayout(D3D::ID3DBlob* vertexShaderBytecode) -> bool
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
				.SemanticName = "COLOR",    
				.SemanticIndex = 0, 
				.Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 
				.InputSlot = 0, 
				.AlignedByteOffset = 12, 
				.InputSlotClass = D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 
				.InstanceDataStepRate = 0
			}
		};

		auto hr = md3dDevice->CreateInputLayout(
			vertexDesc.data(),
			static_cast<std::uint32_t>(vertexDesc.size()),
			vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(),
			&mInputLayout
		);
		return Win32::Succeeded(hr);
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mBoxVB;
	ComPtr<D3D11::ID3D11Buffer> mBoxIB;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;

	DirectX::XMFLOAT4X4 mWorld;
	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	float mTheta = 1.5f * MathHelper::Pi;
	float mPhi = 0.25f * MathHelper::Pi;
	float mRadius = 5.0f;

	Win32::POINT mLastMousePos{};
};