export module skinnedmeshdemo:loadm3d;
import std;
import shared;
import :animationhelper;
import :sharedvertices;
import :meshgeometry;
import :skinneddata;

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

	bool LoadM3d(
		const std::string& filename,
		std::vector<Vertices::PosNormalTexTanSkinned>& vertices,
		std::vector<std::uint16_t>& indices,
		std::vector<MeshGeometry::Subset>& subsets,
		std::vector<M3dMaterial>& mats,
		SkinnedData& skinInfo)
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
			fin >> ignore;
			fin >> ignore >> numMaterials;
			fin >> ignore >> numVertices;
			fin >> ignore >> numTriangles;
			fin >> ignore >> numBones;
			fin >> ignore >> numAnimationClips;

			auto boneOffsets = std::vector<DirectX::XMFLOAT4X4>{};
			auto boneHierarchy = std::vector<int>{};
			auto animations = std::map<std::string, AnimationClip>{};

			ReadMaterials(fin, numMaterials, mats);
			ReadSubsetTable(fin, numMaterials, subsets);
			ReadSkinnedVertices(fin, numVertices, vertices);
			ReadTriangles(fin, numTriangles, indices);
			ReadBoneOffsets(fin, numBones, boneOffsets);
			ReadBoneHierarchy(fin, numBones, boneHierarchy);
			ReadAnimationClips(
				fin, numBones, numAnimationClips, animations);

			skinInfo.Set(
				std::move(boneHierarchy),
				std::move(boneOffsets),
				std::move(animations));

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

	void ReadSkinnedVertices(
		std::ifstream& fin,
		std::uint32_t numVertices,
		std::vector<Vertices::PosNormalTexTanSkinned>& vertices)
	{
		std::string ignore;
		vertices.resize(numVertices);

		fin >> ignore;
		for (auto& vertex : vertices)
		{
			auto weights = std::array<float, 4>{};
			auto boneIndices = std::array<unsigned int, 4>{};

			fin >> ignore >> vertex.Pos.x >> vertex.Pos.y >> vertex.Pos.z;
			fin >> ignore
				>> vertex.TangentU.x
				>> vertex.TangentU.y
				>> vertex.TangentU.z
				>> vertex.TangentU.w;
			fin >> ignore
				>> vertex.Normal.x
				>> vertex.Normal.y
				>> vertex.Normal.z;
			fin >> ignore >> vertex.Tex.x >> vertex.Tex.y;
			fin >> ignore
				>> weights[0]
				>> weights[1]
				>> weights[2]
				>> weights[3];
			fin >> ignore
				>> boneIndices[0]
				>> boneIndices[1]
				>> boneIndices[2]
				>> boneIndices[3];

			vertex.Weights = {weights[0], weights[1], weights[2]};
			for (auto i = 0u; i < boneIndices.size(); ++i)
			{
				vertex.BoneIndices[i] =
					static_cast<std::uint8_t>(boneIndices[i]);
			}
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

	void ReadBoneOffsets(
		std::ifstream& fin,
		std::uint32_t numBones,
		std::vector<DirectX::XMFLOAT4X4>& boneOffsets)
	{
		std::string ignore;
		boneOffsets.resize(numBones);

		fin >> ignore;
		for (auto& boneOffset : boneOffsets)
		{
			fin >> ignore;
			for (auto row = 0u; row < 4; ++row)
			{
				for (auto column = 0u; column < 4; ++column)
				{
					fin >> boneOffset.m[row][column];
				}
			}
		}
	}

	void ReadBoneHierarchy(
		std::ifstream& fin,
		std::uint32_t numBones,
		std::vector<int>& boneHierarchy)
	{
		std::string ignore;
		boneHierarchy.resize(numBones);

		fin >> ignore;
		for (auto& parentIndex : boneHierarchy)
		{
			fin >> ignore >> parentIndex;
		}
	}

	void ReadAnimationClips(
		std::ifstream& fin,
		std::uint32_t numBones,
		std::uint32_t numAnimationClips,
		std::map<std::string, AnimationClip>& animations)
	{
		std::string ignore;
		fin >> ignore;

		for (auto clipIndex = 0u;
			clipIndex < numAnimationClips;
			++clipIndex)
		{
			std::string clipName;
			fin >> ignore >> clipName;
			fin >> ignore;

			auto clip = AnimationClip{};
			clip.BoneAnimations.resize(numBones);
			for (auto& boneAnimation : clip.BoneAnimations)
			{
				ReadBoneKeyframes(fin, boneAnimation);
			}
			fin >> ignore;

			animations[clipName] = std::move(clip);
		}
	}

	void ReadBoneKeyframes(
		std::ifstream& fin,
		BoneAnimation& boneAnimation)
	{
		std::string ignore;
		auto numKeyframes = 0u;
		fin >> ignore >> ignore >> numKeyframes;
		fin >> ignore;

		boneAnimation.Keyframes.resize(numKeyframes);
		for (auto& keyframe : boneAnimation.Keyframes)
		{
			fin >> ignore >> keyframe.TimePos;
			fin >> ignore
				>> keyframe.Translation.x
				>> keyframe.Translation.y
				>> keyframe.Translation.z;
			fin >> ignore
				>> keyframe.Scale.x
				>> keyframe.Scale.y
				>> keyframe.Scale.z;
			fin >> ignore
				>> keyframe.RotationQuat.x
				>> keyframe.RotationQuat.y
				>> keyframe.RotationQuat.z
				>> keyframe.RotationQuat.w;
		}

		fin >> ignore;
	}
};