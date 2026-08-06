export module hills;
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

export class HillsApp : public D3DApp
{
public:
	HillsApp(Win32::HINSTANCE hInstance)
		: D3DApp{hInstance}
	{
		mMainWndCaption = L"Hills Demo";
		Init();
	}

	~HillsApp()
	{
		mVB.reset();
		mIB.reset();
		mInputLayout.reset();
	}

	void OnResize()override
	{
		D3DApp::OnResize();
		auto P = DirectX::XMMATRIX{DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f)};
		DirectX::XMStoreFloat4x4(&mProj, P);
	}

	void UpdateScene(float dt)override
	{
		// Convert Spherical to Cartesian coordinates.
		auto x = mRadius * std::sinf(mPhi) * std::cosf(mTheta);
		auto z = mRadius * std::sinf(mPhi) * std::sinf(mTheta);
		auto y = mRadius * std::cosf(mPhi);

		// Build the view matrix.
		auto pos = DirectX::XMVECTOR{DirectX::XMVectorSet(x, y, z, 1.0f)};
		auto target = DirectX::XMVECTOR{DirectX::XMVectorZero()};
		auto up = DirectX::XMVECTOR{DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)};

		auto V = DirectX::XMMATRIX{DirectX::XMMatrixLookAtLH(pos, target, up)};
		DirectX::XMStoreFloat4x4(&mView, V);
	}
	void DrawScene()override
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&DirectX::Colors::LightSteelBlue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(), D3D11::D3D11_CLEAR_FLAG{ D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL }, 1.0f, 0);

		md3dImmediateContext->IASetInputLayout(mInputLayout.get());
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto stride = std::uint32_t{ sizeof(Vertex) };
		auto offset = 0u;
		auto vertexBuffers = std::array{ mVB.get() };
		md3dImmediateContext->IASetVertexBuffers(0, static_cast<std::uint32_t>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mIB.get(), DXGI_FORMAT_R32_UINT, 0);

		// Set constants

		auto view = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mView)};
		auto proj = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mProj)};
		auto world = DirectX::XMMATRIX{DirectX::XMLoadFloat4x4(&mGridWorld)};
		auto worldViewProj = DirectX::XMMATRIX{world * view * proj};

		auto perObject = PerObjectConstants{};
		DirectX::XMStoreFloat4x4(&perObject.WorldViewProj, worldViewProj);
		md3dImmediateContext->UpdateSubresource(mPerObjectCB.get(), 0, 0, &perObject, 0, 0);

		md3dImmediateContext->VSSetShader(mColorVS.get(), 0, 0);
		md3dImmediateContext->VSSetConstantBuffers(0, 1, mPerObjectCB.GetAddressOf());
		md3dImmediateContext->PSSetShader(mColorPS.get(), 0, 0);
		md3dImmediateContext->DrawIndexed(mGridIndexCount, 0, 0);
		HR(mSwapChain->Present(0, 0));
	}

	void OnMouseDown(Win32::WPARAM btnState, int x, int y)override
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;
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
	void Init()override
	{
		D3DApp::Init();
		BuildGeometryBuffers();
		BuildShaders();
	}

	auto GetHeight(float x, float z)const -> float
	{
		return 0.3f * (z * std::sinf(0.1f * x) + x * std::cosf(0.1f * z));
	}
	
	void BuildGeometryBuffers()
	{
		auto grid = GeometryGenerator::MeshData{};
		auto geoGen = GeometryGenerator{};

		geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

		mGridIndexCount = static_cast<std::uint32_t>(grid.Indices.size());

		//
		// Extract the vertex elements we are interested and apply the height function to
		// each vertex.  In addition, color the vertices based on their height so we have
		// sandy looking beaches, grassy low hills, and snow mountain peaks.
		//
		auto vertices = std::vector<Vertex>(grid.Vertices.size());
		for (auto i = 0ull; i < grid.Vertices.size(); ++i)
		{
			auto p = DirectX::XMFLOAT3{grid.Vertices[i].Position};
			p.y = GetHeight(p.x, p.z);
			vertices[i].Pos = p;

			// Color the vertex based on its height.
			if (p.y < -10.0f) // Sandy beach color.
				vertices[i].Color = DirectX::XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
			else if (p.y < 5.0f) // Light yellow-green.
				vertices[i].Color = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
			else if (p.y < 12.0f) // Dark yellow-green.
				vertices[i].Color = DirectX::XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
			else if (p.y < 20.0f) // Dark brown.
				vertices[i].Color = DirectX::XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
			else // White snow.
				vertices[i].Color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(Vertex) * grid.Vertices.size()),
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &vertices[0]};
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint32_t) * mGridIndexCount),
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{.pSysMem = &grid.Indices[0]};
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
	}

	void BuildShaders()
	{
		auto vertexShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		auto hr = D3D::D3DReadFileToBlob(L"Shaders/color_VS.cso", &vertexShaderBytecode);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to read vertex shader file." };

		hr = md3dDevice->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(),
			vertexShaderBytecode->GetBufferSize(), 0, &mColorVS);
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create vertex shader." };

		BuildVertexLayout(vertexShaderBytecode.get());
		vertexShaderBytecode.reset();

		auto pixelShaderBytecode = ComPtr<D3D::ID3DBlob>{};
		hr = D3D::D3DReadFileToBlob(L"Shaders/color_PS.cso", &pixelShaderBytecode);
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

	void BuildVertexLayout(D3D::ID3DBlob* vertexShaderBytecode)
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
		if (Win32::Failed(hr))
			throw std::runtime_error{ "Failed to create input layout." };
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mVB;
	ComPtr<D3D11::ID3D11Buffer> mIB;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;
	
	ComPtr<D3D11::ID3D11Buffer> mPerObjectCB;
	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mGridWorld = d3dHelper::Identity4x4;

	std::uint32_t mGridIndexCount = 0;

	DirectX::XMFLOAT4X4 mView = d3dHelper::Identity4x4;
	DirectX::XMFLOAT4X4 mProj = d3dHelper::Identity4x4;

	float mTheta = 1.5f * MathHelper::Pi;
	float mPhi = 0.1f * MathHelper::Pi;
	float mRadius = 200.0f;

	Win32::POINT mLastMousePos{};
};