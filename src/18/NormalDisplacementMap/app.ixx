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

struct BasicVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

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
	NormalDisplacementMapApp(Win32::HINSTANCE hInstance);

	void Init() override;
	void OnResize() override;
	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(Win32::WPARAM btnState, int x, int y) override;
	void OnMouseUp(Win32::WPARAM btnState, int x, int y) override;
	void OnMouseMove(Win32::WPARAM btnState, int x, int y) override;

private:
	void BuildShapeGeometryBuffers();
	void BuildSkullGeometryBuffers();
	void BuildShaders();
	void BuildInputLayouts(
		D3D::ID3DBlob* vsBytecode, 
		D3D::ID3DBlob* normalMapVsBytecode, 
		D3D::ID3DBlob* displacementMapVsBytecode
	);

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

	ComPtr<D3D11::ID3D11VertexShader> mBasicVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mBasicPixelShader;

	ComPtr<D3D11::ID3D11VertexShader> mNormalMapVertexShader;
	ComPtr<D3D11::ID3D11PixelShader> mNormalMapPixelShader;

	ComPtr<D3D11::ID3D11VertexShader> mDisplacementMapVertexShader;
	ComPtr<D3D11::ID3D11HullShader> mDisplacementMapHullShader;
	ComPtr<D3D11::ID3D11DomainShader> mDisplacementMapDomainShader;
	ComPtr<D3D11::ID3D11PixelShader> mDisplacementMapPixelShader;

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

	RenderOptions mRenderOptions;

	Camera mCam;

	Win32::POINT mLastMousePos{};
};
