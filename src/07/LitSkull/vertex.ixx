export module litskull:vertex;
import std;
import shared;

namespace Vertex
{
	struct PosNormal
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
	};
}

struct InputLayoutDesc
{
	// Init like const int A::a[4] = {0, 1, 2, 3}; in .cpp file.
	static constexpr inline D3D11::D3D11_INPUT_ELEMENT_DESC PosNormal[2] = {
		{"POSITION", 0, DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
};

struct InputLayouts
{
	static void InitAll(D3D11::ID3D11Device* device)
	{
		//
		// PosNormal
		//D3D11::D3DX11_PASS_DESC passDesc;
		//Effects::BasicFX->Light1Tech->GetPassByIndex(0)->GetDesc(&passDesc);
		//HR(device->CreateInputLayout(InputLayoutDesc::PosNormal, 2, passDesc.pIAInputSignature,
		//	passDesc.IAInputSignatureSize, &PosNormal));
	}
	static void DestroyAll()
	{
		PosNormal.reset();
	}
	static inline ComPtr<D3D11::ID3D11InputLayout> PosNormal;
};