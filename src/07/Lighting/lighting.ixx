export module lighting;
import std;
import shared;
import :waves;

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};

struct PerObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj;
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTranspose;
};

export class WavesDemo : public D3DApp
{
public:
	WavesDemo(Win32::HINSTANCE hInstance);
	~WavesDemo();

	void Init() override;
	void OnResize() override;
	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override;
	void OnMouseUp(Win32::WPARAM btnState, int x, int y) override;
	void OnMouseMove(Win32::WPARAM btnState, int x, int y) override;

private:
	auto GetHeight(float x, float z)const -> float;
	void BuildLandGeometryBuffers();
	void BuildWavesGeometryBuffers();
	void BuildShaders();
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
		HR(hr, "Failed to create input layout.");
	}

private:
	ComPtr<D3D11::ID3D11Buffer> mLandVB;
	ComPtr<D3D11::ID3D11Buffer> mLandIB;
	ComPtr<D3D11::ID3D11Buffer> mWavesVB;
	ComPtr<D3D11::ID3D11Buffer> mWavesIB;
	ComPtr<D3D11::ID3D11VertexShader> mColorVS;
	ComPtr<D3D11::ID3D11PixelShader> mColorPS;

	ComPtr<D3D11::ID3D11InputLayout> mInputLayout;

	ComPtr<D3D11::ID3D11RasterizerState> mWireframeRS;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;

	std::uint32_t mGridIndexCount = 0;

	Waves mWaves;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	float mTheta = 1.5f * MathHelper::Pi;
	float mPhi = 0.1f * MathHelper::Pi;
	float mRadius = 80.f;

	Win32::POINT mLastMousePos{};
};