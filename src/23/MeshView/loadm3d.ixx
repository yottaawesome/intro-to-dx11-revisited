export module meshviewdemo:loadm3d;
import std;
import shared;
import :sharedvertices;
import :meshgeometry;

struct M3dMaterial
{
	Material Mat;
	bool AlphaClip;
	std::string EffectTypeName;
	std::wstring DiffuseMapName;
	std::wstring NormalMapName;
};

class M3DLoader
{
public:
	bool LoadM3d(const std::string& filename,
		std::vector<Vertices::PosNormalTexTan>& vertices,
		std::vector<std::uint16_t>& indices,
		std::vector<MeshGeometry::Subset>& subsets,
		std::vector<M3dMaterial>& mats)
	{
		std::ifstream fin(filename);

		auto numMaterials = 0u;
		auto numVertices = 0u;
		auto numTriangles = 0u;
		auto numBones = 0u;
		auto numAnimationClips = 0u;

		std::string ignore;

		if (fin)
		{
			fin >> ignore; // file header text
			fin >> ignore >> numMaterials;
			fin >> ignore >> numVertices;
			fin >> ignore >> numTriangles;
			fin >> ignore >> numBones;
			fin >> ignore >> numAnimationClips;

			ReadMaterials(fin, numMaterials, mats);
			ReadSubsetTable(fin, numMaterials, subsets);
			ReadVertices(fin, numVertices, vertices);
			ReadTriangles(fin, numTriangles, indices);

			return true;
		}
		return false;
	}

private:
	void ReadMaterials(std::ifstream& fin, std::uint32_t numMaterials, std::vector<M3dMaterial>& mats)
	{
		std::string ignore;
		mats.resize(numMaterials);

		std::string diffuseMapName;
		std::string normalMapName;

		fin >> ignore; // materials header text
		for (auto i = 0u; i < numMaterials; ++i)
		{
			fin >> ignore >> mats[i].Mat.Ambient.x >> mats[i].Mat.Ambient.y >> mats[i].Mat.Ambient.z;
			fin >> ignore >> mats[i].Mat.Diffuse.x >> mats[i].Mat.Diffuse.y >> mats[i].Mat.Diffuse.z;
			fin >> ignore >> mats[i].Mat.Specular.x >> mats[i].Mat.Specular.y >> mats[i].Mat.Specular.z;
			fin >> ignore >> mats[i].Mat.Specular.w;
			fin >> ignore >> mats[i].Mat.Reflect.x >> mats[i].Mat.Reflect.y >> mats[i].Mat.Reflect.z;
			fin >> ignore >> mats[i].AlphaClip;
			fin >> ignore >> mats[i].EffectTypeName;
			fin >> ignore >> diffuseMapName;
			fin >> ignore >> normalMapName;

			mats[i].DiffuseMapName.resize(diffuseMapName.size(), ' ');
			mats[i].NormalMapName.resize(normalMapName.size(), ' ');
			std::copy(diffuseMapName.begin(), diffuseMapName.end(), mats[i].DiffuseMapName.begin());
			std::copy(normalMapName.begin(), normalMapName.end(), mats[i].NormalMapName.begin());
		}
	}

	void ReadSubsetTable(std::ifstream& fin, std::uint32_t numSubsets, std::vector<MeshGeometry::Subset>& subsets)
	{
		std::string ignore;
		subsets.resize(numSubsets);

		fin >> ignore; // subset header text
		for (auto i = 0u; i < numSubsets; ++i)
		{
			fin >> ignore >> subsets[i].Id;
			fin >> ignore >> subsets[i].VertexStart;
			fin >> ignore >> subsets[i].VertexCount;
			fin >> ignore >> subsets[i].FaceStart;
			fin >> ignore >> subsets[i].FaceCount;
		}
	}

	void ReadVertices(std::ifstream& fin, std::uint32_t numVertices, std::vector<Vertices::PosNormalTexTan>& vertices)
	{
		std::string ignore;
		vertices.resize(numVertices);

		fin >> ignore; // vertices header text
		for (auto i = 0u; i < numVertices; ++i)
		{
			fin >> ignore >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> ignore >> vertices[i].TangentU.x >> vertices[i].TangentU.y >> vertices[i].TangentU.z >> vertices[i].TangentU.w;
			fin >> ignore >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
			fin >> ignore >> vertices[i].Tex.x >> vertices[i].Tex.y;
		}
	}

	void ReadTriangles(std::ifstream& fin, std::uint32_t numTriangles, std::vector<std::uint16_t>& indices)
	{
		std::string ignore;
		indices.resize(numTriangles * 3);

		fin >> ignore; // triangles header text
		for (auto i = 0u; i < numTriangles; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}
	}
};