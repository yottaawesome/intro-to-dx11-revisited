export module ambientocclusiondemo:octree;
import std;
import shared;

struct OctreeNode
{
#pragma region Properties
	DirectX::BoundingBox Bounds;

	// This will be empty except for leaf nodes.
	std::vector<std::uint32_t> Indices;

	OctreeNode* Children[8];

	bool IsLeaf;
#pragma endregion

	OctreeNode()
	{
		for (int i = 0; i < 8; ++i)
			Children[i] = 0;

		Bounds.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		Bounds.Extents = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

		IsLeaf = false;
	}

	~OctreeNode()
	{
		for (int i = 0; i < 8; ++i)
		{
			delete Children[i];
			Children[i] = nullptr;
		}
	}

	///<summary>
	/// Subdivides the bounding box of this node into eight subboxes (vMin[i], vMax[i]) for i = 0:7.
	///</summary>
	void Subdivide(DirectX::BoundingBox box[8])
	{
		DirectX::XMFLOAT3 halfExtent(
			0.5f * Bounds.Extents.x,
			0.5f * Bounds.Extents.y,
			0.5f * Bounds.Extents.z);

		// "Top" four quadrants.
		box[0].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[0].Extents = halfExtent;

		box[1].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[1].Extents = halfExtent;

		box[2].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[2].Extents = halfExtent;

		box[3].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[3].Extents = halfExtent;

		// "Bottom" four quadrants.
		box[4].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[4].Extents = halfExtent;

		box[5].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[5].Extents = halfExtent;

		box[6].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[6].Extents = halfExtent;

		box[7].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[7].Extents = halfExtent;
	}
};

class Octree
{
public:
	~Octree()
	{
		delete mRoot;
	}

	void Build(const std::vector<DirectX::XMFLOAT3>& vertices, const std::vector<std::uint32_t>& indices)
	{
		// Cache a copy of the vertices.
		mVertices = vertices;

		// Build AABB to contain the scene mesh.
		DirectX::BoundingBox sceneBounds = BuildAABB();

		// Allocate the root node and set its AABB to contain the scene mesh.
		mRoot = new OctreeNode();
		mRoot->Bounds = sceneBounds;

		BuildOctree(mRoot, indices);
	}

	bool RayOctreeIntersect(DirectX::FXMVECTOR rayPos, DirectX::FXMVECTOR rayDir)
	{
		return RayOctreeIntersect(mRoot, rayPos, rayDir);
	}

private:
	DirectX::BoundingBox BuildAABB()
	{
		DirectX::XMVECTOR vmin = DirectX::XMVectorReplicate(+MathHelper::Infinity);
		DirectX::XMVECTOR vmax = DirectX::XMVectorReplicate(-MathHelper::Infinity);
		for (size_t i = 0; i < mVertices.size(); ++i)
		{
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&mVertices[i]);

			vmin = DirectX::XMVectorMin(vmin, P);
			vmax = DirectX::XMVectorMax(vmax, P);
		}

		DirectX::BoundingBox bounds;
		DirectX::XMVECTOR C = 0.5f * (vmin + vmax);
		DirectX::XMVECTOR E = 0.5f * (vmax - vmin);

		DirectX::XMStoreFloat3(&bounds.Center, C);
		DirectX::XMStoreFloat3(&bounds.Extents, E);

		return bounds;
	}
	
	void BuildOctree(OctreeNode* parent, const std::vector<std::uint32_t>& indices)
	{
		size_t triCount = indices.size() / 3;

		if (triCount < 60)
		{
			parent->IsLeaf = true;
			parent->Indices = indices;
		}
		else
		{
			parent->IsLeaf = false;

			DirectX::BoundingBox subbox[8];
			parent->Subdivide(subbox);

			for (int i = 0; i < 8; ++i)
			{
				// Allocate a new subnode.
				parent->Children[i] = new OctreeNode();
				parent->Children[i]->Bounds = subbox[i];

				// Find triangles that intersect this node's bounding box.
				std::vector<std::uint32_t> intersectedTriangleIndices;
				for (size_t j = 0; j < triCount; ++j)
				{
					std::uint32_t i0 = indices[j * 3 + 0];
					std::uint32_t i1 = indices[j * 3 + 1];
					std::uint32_t i2 = indices[j * 3 + 2];

					DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&mVertices[i0]);
					DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&mVertices[i1]);
					DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&mVertices[i2]);

					if (subbox[i].Intersects(v0, v1, v2))
					{
						intersectedTriangleIndices.push_back(i0);
						intersectedTriangleIndices.push_back(i1);
						intersectedTriangleIndices.push_back(i2);
					}
				}

				// Recurse.
				BuildOctree(parent->Children[i], intersectedTriangleIndices);
			}
		}
	}

	bool RayOctreeIntersect(OctreeNode* parent, DirectX::FXMVECTOR rayPos, DirectX::FXMVECTOR rayDir)
	{
		// Recurs until we find a leaf node (all the triangles are in the leaves).
		if (!parent->IsLeaf)
		{
			for (int i = 0; i < 8; ++i)
			{
				// Recurse down this node if the ray hit the child's box.
				float t;
				if (parent->Children[i]->Bounds.Intersects(rayPos, rayDir, t))
				{
					// If we hit a triangle down this branch, we can bail out that we hit a triangle.
					if (RayOctreeIntersect(parent->Children[i], rayPos, rayDir))
						return true;
				}
			}

			// If we get here. then we did not hit any triangles.
			return false;
		}
		else
		{
			size_t triCount = parent->Indices.size() / 3;

			for (size_t i = 0; i < triCount; ++i)
			{
				std::uint32_t i0 = parent->Indices[i * 3 + 0];
				std::uint32_t i1 = parent->Indices[i * 3 + 1];
				std::uint32_t i2 = parent->Indices[i * 3 + 2];

				DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&mVertices[i0]);
				DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&mVertices[i1]);
				DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&mVertices[i2]);

				float t;
				if (DirectX::TriangleTests::Intersects(rayPos, rayDir, v0, v1, v2, t))
					return true;
			}

			return false;
		}
	}

private:
	OctreeNode* mRoot = nullptr;

	std::vector<DirectX::XMFLOAT3> mVertices;
};

