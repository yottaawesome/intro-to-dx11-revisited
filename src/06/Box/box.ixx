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

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
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
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&mProj, P);
	}
	void UpdateScene(float dt)
	{
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		float z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		float y = mRadius * std::cosf(mPhi);

		// Build the view matrix.
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);
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
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = std::clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & Win32::MK::RButton) != 0)
		{
			// Make each pixel correspond to 0.005 unit in the scene.
			float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

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
		Vertex vertices[] =
		{
			{ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) },
			{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) },
			{ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) },
			{ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) },
			{ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) },
			{ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) },
			{ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) },
			{ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) }
		};

		D3D11::D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * 8;
		vbd.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;
		D3D11::D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = vertices;
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));


		// Create the index buffer

		UINT indices[] = {
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

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * 36;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = indices;
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}

	void BuildShaders()
	{
		ComPtr<D3D::ID3DBlob> vertexShaderBytecode;
		Win32::HRESULT hr = D3D::D3DReadFileToBlob(L"FX/color_VS.cso", &vertexShaderBytecode);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to read vertex shader file." };

		hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create vertex shader." };

		if (!BuildVertexLayout(vertexShaderBytecode.get()))
			throw std::runtime_error{ "Failed to create input layout." };
		vertexShaderBytecode.reset();

		ComPtr<D3D::ID3DBlob> pixelShaderBytecode;
		hr = D3DReadFileToBlob(L"FX/color_PS.cso", &pixelShaderBytecode);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to read pixel shader file." };

		hr = md3dDevice->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(),
			pixelShaderBytecode->GetBufferSize(), 0, &mColorPS);
		pixelShaderBytecode.reset();
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create pixel shader." };

		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT;
		cbd.ByteWidth = sizeof(PerObjectConstants);
		cbd.BindFlags =D3D11::D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		cbd.StructureByteStride = 0;
		hr = md3dDevice->CreateBuffer(&cbd, 0, &mPerObjectCB);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create constant buffer." };
	}

	bool BuildVertexLayout(D3D::ID3DBlob* vertexShaderBytecode)
	{
		// Create the vertex input layout.
		D3D11::D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR",    0, DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		return Win32::Succeeded(md3dDevice->CreateInputLayout(vertexDesc, 2, vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), &mInputLayout));
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

	float mTheta= 1.5f * MathHelper::Pi;
	float mPhi= 0.25f * MathHelper::Pi;
	float mRadius= 5.0f;

	Win32::POINT mLastMousePos;
};