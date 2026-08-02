export module treebillboard:app;
import std;
import shared;
import :waves;
import :renderstates;

// Basic 32-byte vertex structure.
struct Basic32
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

struct TreePointSprite
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 Size;
};

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class TreeBillboardApp : public D3DApp
{
public:
	TreeBillboardApp(HINSTANCE hInstance);
	~TreeBillboardApp();

	void Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(Win32::WPARAM btnState, int x, int y);
	void OnMouseUp(Win32::WPARAM btnState, int x, int y);
	void OnMouseMove(Win32::WPARAM btnState, int x, int y);

private:
	auto GetHillHeight(float x, float z)const->float;
	auto GetHillNormal(float x, float z)const->DirectX::XMFLOAT3;
	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();
	void BuildCrateGeometryBuffers();
	void BuildTreeSpritesBuffer();
	void DrawTreeSprites(DirectX::CXMMATRIX viewProj);

private:
	D3D11::ID3D11Buffer* mLandVB;
	D3D11::ID3D11Buffer* mLandIB;

	D3D11::ID3D11Buffer* mWavesVB;
	D3D11::ID3D11Buffer* mWavesIB;

	D3D11::ID3D11Buffer* mBoxVB;
	D3D11::ID3D11Buffer* mBoxIB;

	D3D11::ID3D11Buffer* mTreeSpritesVB;

	D3D11::ID3D11ShaderResourceView* mGrassMapSRV;
	D3D11::ID3D11ShaderResourceView* mWavesMapSRV;
	D3D11::ID3D11ShaderResourceView* mBoxMapSRV;
	D3D11::ID3D11ShaderResourceView* mTreeTextureMapArraySRV;

	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;
	Material mTreeMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform;
	DirectX::XMFLOAT4X4 mWaterTexTransform;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;
	DirectX::XMFLOAT4X4 mBoxWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	UINT mLandIndexCount;

	static const UINT TreeCount = 16;

	bool mAlphaToCoverageOn;

	DirectX::XMFLOAT2 mWaterTexOffset;

	RenderOptions mRenderOptions;

	DirectX::XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	Win32::POINT mLastMousePos{};
};