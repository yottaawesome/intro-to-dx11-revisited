export module meshviewdemo:basicmodel;
import std;
import shared;
import :sharedvertices;
import :meshgeometry;
import :loadm3d;

class BasicModel
{
public:
	BasicModel(
		D3D11::ID3D11Device* device, 
		TextureMgr& texMgr, 
		const std::string& modelFilename, 
		const std::wstring& texturePath
	)
	{
		std::vector<M3dMaterial> mats;
		M3DLoader m3dLoader;
		m3dLoader.LoadM3d(modelFilename, Vertices, Indices, Subsets, mats);

		ModelMesh.SetVertices(device, &Vertices[0], static_cast<std::uint32_t>(Vertices.size()));
		ModelMesh.SetIndices(device, &Indices[0], static_cast<std::uint32_t>(Indices.size()));
		ModelMesh.SetSubsetTable(Subsets);

		SubsetCount = static_cast<std::uint32_t>(mats.size());

		for (auto i = 0u; i < SubsetCount; ++i)
		{
			Mat.push_back(mats[i].Mat);

			ComPtr<ID3D11ShaderResourceView> diffuseMapSRV = texMgr.CreateTexture(texturePath + mats[i].DiffuseMapName);
			DiffuseMapSRV.push_back(diffuseMapSRV);

			ComPtr<ID3D11ShaderResourceView> normalMapSRV = texMgr.CreateTexture(texturePath + mats[i].NormalMapName);
			NormalMapSRV.push_back(normalMapSRV);
		}
	}

	std::uint32_t SubsetCount;

	std::vector<Material> Mat;
	std::vector<ComPtr<D3D11::ID3D11ShaderResourceView>> DiffuseMapSRV;
	std::vector<ComPtr<D3D11::ID3D11ShaderResourceView>> NormalMapSRV;

	// Keep CPU copies of the mesh data to read from.  
	std::vector<Vertices::PosNormalTexTan> Vertices;
	std::vector<std::uint16_t> Indices;
	std::vector<MeshGeometry::Subset> Subsets;

	MeshGeometry ModelMesh;
};

struct BasicModelInstance
{
	BasicModel* Model;
	DirectX::XMFLOAT4X4 World;
};