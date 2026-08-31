export module skinnedmeshdemo:sharedvertices;
import std;
import shared;

namespace Vertices
{
	struct Basic32
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Tex;
	};

	struct PosNormalTexTan
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Tex;
		DirectX::XMFLOAT4 TangentU;
	};

	struct PosNormalTexTanSkinned
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Tex;
		DirectX::XMFLOAT4 TangentU;
		DirectX::XMFLOAT3 Weights;
		std::uint8_t BoneIndices[4];
	};
	static_assert(sizeof(PosNormalTexTanSkinned) == 64);
}