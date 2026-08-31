export module skinnedmeshdemo:skinnedmodel;
import std;
import shared;
import :loadm3d;
import :meshgeometry;
import :sharedvertices;
import :skinneddata;

class SkinnedModel
{
public:
	SkinnedModel(
		D3D11::ID3D11Device* device,
		TextureMgr& textureManager,
		const std::string& modelFilename,
		const std::wstring& texturePath)
	{
		auto materials = std::vector<M3dMaterial>{};
		auto loader = M3DLoader{};
		if (!loader.LoadM3d(
			modelFilename,
			Vertices,
			Indices,
			Subsets,
			materials,
			SkinnedData))
		{
			throw std::runtime_error(
				"Failed to load skinned model: " + modelFilename);
		}

		ModelMesh.SetVertices(
			device,
			Vertices.data(),
			static_cast<std::uint32_t>(Vertices.size()));
		ModelMesh.SetIndices(
			device,
			Indices.data(),
			static_cast<std::uint32_t>(Indices.size()));
		ModelMesh.SetSubsetTable(Subsets);

		SubsetCount = static_cast<std::uint32_t>(materials.size());
		Mat.reserve(SubsetCount);
		DiffuseMapSRV.reserve(SubsetCount);
		NormalMapSRV.reserve(SubsetCount);

		for (const auto& material : materials)
		{
			Mat.push_back(material.Mat);
			DiffuseMapSRV.push_back(
				textureManager.CreateTexture(
					texturePath + material.DiffuseMapName));
			NormalMapSRV.push_back(
				textureManager.CreateTexture(
					texturePath + material.NormalMapName));
		}
	}

	std::uint32_t SubsetCount = 0;

	std::vector<Material> Mat;
	std::vector<ComPtr<D3D11::ID3D11ShaderResourceView>> DiffuseMapSRV;
	std::vector<ComPtr<D3D11::ID3D11ShaderResourceView>> NormalMapSRV;

	std::vector<Vertices::PosNormalTexTanSkinned> Vertices;
	std::vector<std::uint16_t> Indices;
	std::vector<MeshGeometry::Subset> Subsets;

	MeshGeometry ModelMesh;
	class SkinnedData SkinnedData;
};

struct SkinnedModelInstance
{
	void Update(float elapsedTime)
	{
		if (Model == nullptr)
		{
			throw std::logic_error(
				"Cannot update a skinned model instance without a model.");
		}

		TimePos += elapsedTime;
		if (TimePos > Model->SkinnedData.GetClipEndTime(ClipName))
		{
			TimePos = 0.0f;
		}

		Model->SkinnedData.GetFinalTransforms(
			ClipName, TimePos, FinalTransforms);
	}

	SkinnedModel* Model = nullptr;
	float TimePos = 0.0f;
	std::string ClipName;
	DirectX::XMFLOAT4X4 World = d3dHelper::Identity4x4;
	std::vector<DirectX::XMFLOAT4X4> FinalTransforms;
};
