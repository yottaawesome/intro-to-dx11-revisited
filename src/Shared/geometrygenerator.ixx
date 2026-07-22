//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//***************************************************************************************

export module shared:geometrygenerator;
import std;
import :win32;
import :mathhelper;

export class GeometryGenerator
{
public:
	struct Vertex
	{
		Vertex() = default;
		Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n, const DirectX::XMFLOAT3& t, const DirectX::XMFLOAT2& uv)
			: Position(p), Normal(n), TangentU(t), TexC(uv) {}
		Vertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v)
			: Position(px, py, pz), Normal(nx, ny, nz),
			TangentU(tx, ty, tz), TexC(u, v) {}

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 TangentU;
		DirectX::XMFLOAT2 TexC;
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<std::uint32_t> Indices;
	};

	///<summary>
	/// Creates a box centered at the origin with the given dimensions.
	///</summary>
	void CreateBox(float width, float height, float depth, MeshData& meshData)
	{
		//
		// Create the vertices.
		//

		Vertex v[24];

		float w2 = 0.5f * width;
		float h2 = 0.5f * height;
		float d2 = 0.5f * depth;

		// Fill in the front face vertex data.
		v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
		v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

		// Fill in the back face vertex data.
		v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
		v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

		// Fill in the top face vertex data.
		v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
		v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

		// Fill in the bottom face vertex data.
		v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
		v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

		// Fill in the left face vertex data.
		v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
		v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
		v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
		v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

		// Fill in the right face vertex data.
		v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
		v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
		v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
		v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

		meshData.Vertices.assign(&v[0], &v[24]);

		//
		// Create the indices.
		//

		std::uint32_t i[36];

		// Fill in the front face index data
		i[0] = 0; i[1] = 1; i[2] = 2;
		i[3] = 0; i[4] = 2; i[5] = 3;

		// Fill in the back face index data
		i[6] = 4; i[7] = 5; i[8] = 6;
		i[9] = 4; i[10] = 6; i[11] = 7;

		// Fill in the top face index data
		i[12] = 8; i[13] = 9; i[14] = 10;
		i[15] = 8; i[16] = 10; i[17] = 11;

		// Fill in the bottom face index data
		i[18] = 12; i[19] = 13; i[20] = 14;
		i[21] = 12; i[22] = 14; i[23] = 15;

		// Fill in the left face index data
		i[24] = 16; i[25] = 17; i[26] = 18;
		i[27] = 16; i[28] = 18; i[29] = 19;

		// Fill in the right face index data
		i[30] = 20; i[31] = 21; i[32] = 22;
		i[33] = 20; i[34] = 22; i[35] = 23;

		meshData.Indices.assign(&i[0], &i[36]);
	}

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	void CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData)
	{
		meshData.Vertices.clear();
		meshData.Indices.clear();

		//
		// Compute the vertices stating at the top pole and moving down the stacks.
		//

		// Poles: note that there will be texture coordinate distortion as there is
		// not a unique point on the texture map to assign to the pole when mapping
		// a rectangular texture onto a sphere.
		Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

		meshData.Vertices.push_back(topVertex);

		float phiStep = DirectX::Pi / stackCount;
		float thetaStep = 2.0f * DirectX::TwoPi / sliceCount;

		// Compute vertices for each stack ring (do not count the poles as rings).
		for (std::uint32_t i = 1; i <= stackCount - 1; ++i)
		{
			float phi = i * phiStep;

			// Vertices of ring.
			for (std::uint32_t j = 0; j <= sliceCount; ++j)
			{
				float theta = j * thetaStep;

				Vertex v;

				// spherical to cartesian
				v.Position.x = radius * std::sinf(phi) * std::cosf(theta);
				v.Position.y = radius * std::cosf(phi);
				v.Position.z = radius * std::sinf(phi) * std::sinf(theta);

				// Partial derivative of P with respect to theta
				v.TangentU.x = -radius * std::sinf(phi) * std::sinf(theta);
				v.TangentU.y = 0.0f;
				v.TangentU.z = +radius * std::sinf(phi) * std::cosf(theta);

				DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&v.TangentU);
				DirectX::XMStoreFloat3(&v.TangentU, DirectX::XMVector3Normalize(T));

				DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&v.Position);
				DirectX::XMStoreFloat3(&v.Normal, DirectX::XMVector3Normalize(p));

				v.TexC.x = theta / DirectX::TwoPi;
				v.TexC.y = phi / DirectX::Pi;

				meshData.Vertices.push_back(v);
			}
		}

		meshData.Vertices.push_back(bottomVertex);

		//
		// Compute indices for top stack.  The top stack was written first to the vertex buffer
		// and connects the top pole to the first ring.
		//

		for (std::uint32_t i = 1; i <= sliceCount; ++i)
		{
			meshData.Indices.push_back(0);
			meshData.Indices.push_back(i + 1);
			meshData.Indices.push_back(i);
		}

		//
		// Compute indices for inner stacks (not connected to poles).
		//

		// Offset the indices to the index of the first vertex in the first ring.
		// This is just skipping the top pole vertex.
		std::uint32_t baseIndex = 1;
		std::uint32_t ringVertexCount = sliceCount + 1;
		for (std::uint32_t i = 0; i < stackCount - 2; ++i)
		{
			for (std::uint32_t j = 0; j < sliceCount; ++j)
			{
				meshData.Indices.push_back(baseIndex + i * ringVertexCount + j);
				meshData.Indices.push_back(baseIndex + i * ringVertexCount + j + 1);
				meshData.Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

				meshData.Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
				meshData.Indices.push_back(baseIndex + i * ringVertexCount + j + 1);
				meshData.Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
			}
		}

		//
		// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
		// and connects the bottom pole to the bottom ring.
		//

		// South pole vertex was added last.
		std::uint32_t southPoleIndex = (std::uint32_t)meshData.Vertices.size() - 1;

		// Offset the indices to the index of the first vertex in the last ring.
		baseIndex = southPoleIndex - ringVertexCount;

		for (std::uint32_t i = 0; i < sliceCount; ++i)
		{
			meshData.Indices.push_back(southPoleIndex);
			meshData.Indices.push_back(baseIndex + i);
			meshData.Indices.push_back(baseIndex + i + 1);
		}
	}

	///<summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	void CreateGeosphere(float radius, std::uint32_t numSubdivisions, MeshData& meshData)
	{
		// Put a cap on the number of subdivisions.
		numSubdivisions = std::min(numSubdivisions, 5u);

		// Approximate a sphere by tessellating an icosahedron.

		const float X = 0.525731f;
		const float Z = 0.850651f;

		DirectX::XMFLOAT3 pos[12] =
		{
			DirectX::XMFLOAT3(-X, 0.0f, Z),  DirectX::XMFLOAT3(X, 0.0f, Z),
			DirectX::XMFLOAT3(-X, 0.0f, -Z), DirectX::XMFLOAT3(X, 0.0f, -Z),
			DirectX::XMFLOAT3(0.0f, Z, X),   DirectX::XMFLOAT3(0.0f, Z, -X),
			DirectX::XMFLOAT3(0.0f, -Z, X),  DirectX::XMFLOAT3(0.0f, -Z, -X),
			DirectX::XMFLOAT3(Z, X, 0.0f),   DirectX::XMFLOAT3(-Z, X, 0.0f),
			DirectX::XMFLOAT3(Z, -X, 0.0f),  DirectX::XMFLOAT3(-Z, -X, 0.0f)
		};

		Win32::DWORD k[60] =
		{
			1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
			1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
			3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
			10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
		};

		meshData.Vertices.resize(12);
		meshData.Indices.resize(60);

		for (std::uint32_t i = 0; i < 12; ++i)
			meshData.Vertices[i].Position = pos[i];

		for (std::uint32_t i = 0; i < 60; ++i)
			meshData.Indices[i] = k[i];

		for (std::uint32_t i = 0; i < numSubdivisions; ++i)
			Subdivide(meshData);

		// Project vertices onto sphere and scale.
		for (std::uint32_t i = 0; i < meshData.Vertices.size(); ++i)
		{
			// Project onto unit sphere.
			DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&meshData.Vertices[i].Position));

			// Project onto sphere.
			DirectX::XMVECTOR p = radius * n;

			DirectX::XMStoreFloat3(&meshData.Vertices[i].Position, p);
			DirectX::XMStoreFloat3(&meshData.Vertices[i].Normal, n);

			// Derive texture coordinates from spherical coordinates.
			float theta = MathHelper::AngleFromXY(
				meshData.Vertices[i].Position.x,
				meshData.Vertices[i].Position.z);

			float phi = std::acosf(meshData.Vertices[i].Position.y / radius);

			meshData.Vertices[i].TexC.x = theta / DirectX::TwoPi;
			meshData.Vertices[i].TexC.y = phi / DirectX::Pi;

			// Partial derivative of P with respect to theta
			meshData.Vertices[i].TangentU.x = -radius * std::sinf(phi) * std::sinf(theta);
			meshData.Vertices[i].TangentU.y = 0.0f;
			meshData.Vertices[i].TangentU.z = +radius * std::sinf(phi) * std::cosf(theta);

			DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&meshData.Vertices[i].TangentU);
			DirectX::XMStoreFloat3(&meshData.Vertices[i].TangentU, DirectX::XMVector3Normalize(T));
		}
	}

	///<summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	///</summary>
	void CreateCylinder(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData)
	{
		meshData.Vertices.clear();
		meshData.Indices.clear();

		//
		// Build Stacks.
		// 

		float stackHeight = height / stackCount;

		// Amount to increment radius as we move up each stack level from bottom to top.
		float radiusStep = (topRadius - bottomRadius) / stackCount;

		std::uint32_t ringCount = stackCount + 1;

		// Compute vertices for each stack ring starting at the bottom and moving up.
		for (std::uint32_t i = 0; i < ringCount; ++i)
		{
			float y = -0.5f * height + i * stackHeight;
			float r = bottomRadius + i * radiusStep;

			// vertices of ring
			float dTheta = 2.0f * DirectX::Pi / sliceCount;
			for (std::uint32_t j = 0; j <= sliceCount; ++j)
			{
				Vertex vertex;

				float c = std::cosf(j * dTheta);
				float s = std::sinf(j * dTheta);

				vertex.Position = DirectX::XMFLOAT3(r * c, y, r * s);

				vertex.TexC.x = (float)j / sliceCount;
				vertex.TexC.y = 1.0f - (float)i / stackCount;

				// Cylinder can be parameterized as follows, where we introduce v
				// parameter that goes in the same direction as the v tex-coord
				// so that the bitangent goes in the same direction as the v tex-coord.
				//   Let r0 be the bottom radius and let r1 be the top radius.
				//   y(v) = h - hv for v in [0,1].
				//   r(v) = r1 + (r0-r1)v
				//
				//   x(t, v) = r(v)*cos(t)
				//   y(t, v) = h - hv
				//   z(t, v) = r(v)*sin(t)
				// 
				//  dx/dt = -r(v)*sin(t)
				//  dy/dt = 0
				//  dz/dt = +r(v)*cos(t)
				//
				//  dx/dv = (r0-r1)*cos(t)
				//  dy/dv = -h
				//  dz/dv = (r0-r1)*sin(t)

				// This is unit length.
				vertex.TangentU = DirectX::XMFLOAT3(-s, 0.0f, c);

				float dr = bottomRadius - topRadius;
				DirectX::XMFLOAT3 bitangent(dr * c, -height, dr * s);

				DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&vertex.TangentU);
				DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&bitangent);
				DirectX::XMVECTOR N = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(T, B));
				DirectX::XMStoreFloat3(&vertex.Normal, N);

				meshData.Vertices.push_back(vertex);
			}
		}

		// Add one because we duplicate the first and last vertex per ring
		// since the texture coordinates are different.
		std::uint32_t ringVertexCount = sliceCount + 1;

		// Compute indices for each stack.
		for (std::uint32_t i = 0; i < stackCount; ++i)
		{
			for (std::uint32_t j = 0; j < sliceCount; ++j)
			{
				meshData.Indices.push_back(i * ringVertexCount + j);
				meshData.Indices.push_back((i + 1) * ringVertexCount + j);
				meshData.Indices.push_back((i + 1) * ringVertexCount + j + 1);

				meshData.Indices.push_back(i * ringVertexCount + j);
				meshData.Indices.push_back((i + 1) * ringVertexCount + j + 1);
				meshData.Indices.push_back(i * ringVertexCount + j + 1);
			}
		}

		BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
		BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	}

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	void CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n, MeshData& meshData)
	{
		std::uint32_t vertexCount = m * n;
		std::uint32_t faceCount = (m - 1) * (n - 1) * 2;

		//
		// Create the vertices.
		//

		float halfWidth = 0.5f * width;
		float halfDepth = 0.5f * depth;

		float dx = width / (n - 1);
		float dz = depth / (m - 1);

		float du = 1.0f / (n - 1);
		float dv = 1.0f / (m - 1);

		meshData.Vertices.resize(vertexCount);
		for (std::uint32_t i = 0; i < m; ++i)
		{
			float z = halfDepth - i * dz;
			for (std::uint32_t j = 0; j < n; ++j)
			{
				float x = -halfWidth + j * dx;

				meshData.Vertices[i * n + j].Position = DirectX::XMFLOAT3(x, 0.0f, z);
				meshData.Vertices[i * n + j].Normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
				meshData.Vertices[i * n + j].TangentU = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);

				// Stretch texture over grid.
				meshData.Vertices[i * n + j].TexC.x = j * du;
				meshData.Vertices[i * n + j].TexC.y = i * dv;
			}
		}

		//
		// Create the indices.
		//

		meshData.Indices.resize(faceCount * 3); // 3 indices per face

		// Iterate over each quad and compute indices.
		std::uint32_t k = 0;
		for (std::uint32_t i = 0; i < m - 1; ++i)
		{
			for (std::uint32_t j = 0; j < n - 1; ++j)
			{
				meshData.Indices[k] = i * n + j;
				meshData.Indices[k + 1] = i * n + j + 1;
				meshData.Indices[k + 2] = (i + 1) * n + j;

				meshData.Indices[k + 3] = (i + 1) * n + j;
				meshData.Indices[k + 4] = i * n + j + 1;
				meshData.Indices[k + 5] = (i + 1) * n + j + 1;

				k += 6; // next quad
			}
		}
	}

	///<summary>
	/// Creates a quad covering the screen in NDC coordinates.  This is useful for
	/// postprocessing effects.
	///</summary>
	void CreateFullscreenQuad(MeshData& meshData)
	{
		meshData.Vertices.resize(4);
		meshData.Indices.resize(6);

		// Position coordinates specified in NDC space.
		meshData.Vertices[0] = Vertex(
			-1.0f, -1.0f, 0.0f,
			0.0f, 0.0f, -1.0f,
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f);

		meshData.Vertices[1] = Vertex(
			-1.0f, +1.0f, 0.0f,
			0.0f, 0.0f, -1.0f,
			1.0f, 0.0f, 0.0f,
			0.0f, 0.0f);

		meshData.Vertices[2] = Vertex(
			+1.0f, +1.0f, 0.0f,
			0.0f, 0.0f, -1.0f,
			1.0f, 0.0f, 0.0f,
			1.0f, 0.0f);

		meshData.Vertices[3] = Vertex(
			+1.0f, -1.0f, 0.0f,
			0.0f, 0.0f, -1.0f,
			1.0f, 0.0f, 0.0f,
			1.0f, 1.0f);

		meshData.Indices[0] = 0;
		meshData.Indices[1] = 1;
		meshData.Indices[2] = 2;

		meshData.Indices[3] = 0;
		meshData.Indices[4] = 2;
		meshData.Indices[5] = 3;
	}


private:
	void Subdivide(MeshData& meshData)
	{
		// Save a copy of the input geometry.
		MeshData inputCopy = meshData;


		meshData.Vertices.resize(0);
		meshData.Indices.resize(0);

		//       v1
		//       *
		//      / \
		//     /   \
		//  m0*-----*m1
		//   / \   / \
		//  /   \ /   \
		// *-----*-----*
		// v0    m2     v2

		std::uint32_t numTris = static_cast<std::uint32_t>(inputCopy.Indices.size() / 3);
		for (std::uint32_t i = 0; i < numTris; ++i)
		{
			Vertex v0 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 0]];
			Vertex v1 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 1]];
			Vertex v2 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 2]];

			//
			// Generate the midpoints.
			//

			Vertex m0, m1, m2;

			// For subdivision, we just care about the position component.  We derive the other
			// vertex components in CreateGeosphere.

			m0.Position = DirectX::XMFLOAT3(
				0.5f * (v0.Position.x + v1.Position.x),
				0.5f * (v0.Position.y + v1.Position.y),
				0.5f * (v0.Position.z + v1.Position.z));

			m1.Position = DirectX::XMFLOAT3(
				0.5f * (v1.Position.x + v2.Position.x),
				0.5f * (v1.Position.y + v2.Position.y),
				0.5f * (v1.Position.z + v2.Position.z));

			m2.Position = DirectX::XMFLOAT3(
				0.5f * (v0.Position.x + v2.Position.x),
				0.5f * (v0.Position.y + v2.Position.y),
				0.5f * (v0.Position.z + v2.Position.z));

			//
			// Add new geometry.
			//

			meshData.Vertices.push_back(v0); // 0
			meshData.Vertices.push_back(v1); // 1
			meshData.Vertices.push_back(v2); // 2
			meshData.Vertices.push_back(m0); // 3
			meshData.Vertices.push_back(m1); // 4
			meshData.Vertices.push_back(m2); // 5

			meshData.Indices.push_back(i * 6 + 0);
			meshData.Indices.push_back(i * 6 + 3);
			meshData.Indices.push_back(i * 6 + 5);

			meshData.Indices.push_back(i * 6 + 3);
			meshData.Indices.push_back(i * 6 + 4);
			meshData.Indices.push_back(i * 6 + 5);

			meshData.Indices.push_back(i * 6 + 5);
			meshData.Indices.push_back(i * 6 + 4);
			meshData.Indices.push_back(i * 6 + 2);

			meshData.Indices.push_back(i * 6 + 3);
			meshData.Indices.push_back(i * 6 + 1);
			meshData.Indices.push_back(i * 6 + 4);
		}
	}
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData)
	{
		std::uint32_t baseIndex = (std::uint32_t)meshData.Vertices.size();

		float y = 0.5f * height;
		float dTheta = 2.0f * DirectX::Pi / sliceCount;

		// Duplicate cap ring vertices because the texture coordinates and normals differ.
		for (std::uint32_t i = 0; i <= sliceCount; ++i)
		{
			float x = topRadius * std::cosf(i * dTheta);
			float z = topRadius * std::sinf(i * dTheta);

			// Scale down by the height to try and make top cap texture coord area
			// proportional to base.
			float u = x / height + 0.5f;
			float v = z / height + 0.5f;

			meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
		}

		// Cap center vertex.
		meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

		// Index of center vertex.
		std::uint32_t centerIndex = (std::uint32_t)meshData.Vertices.size() - 1;

		for (std::uint32_t i = 0; i < sliceCount; ++i)
		{
			meshData.Indices.push_back(centerIndex);
			meshData.Indices.push_back(baseIndex + i + 1);
			meshData.Indices.push_back(baseIndex + i);
		}
	}

	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData)
	{
		// 
		// Build bottom cap.
		//

		std::uint32_t baseIndex = (std::uint32_t)meshData.Vertices.size();
		float y = -0.5f * height;

		// vertices of ring
		float dTheta = 2.0f * DirectX::Pi / sliceCount;
		for (std::uint32_t i = 0; i <= sliceCount; ++i)
		{
			float x = bottomRadius * std::cosf(i * dTheta);
			float z = bottomRadius * std::sinf(i * dTheta);

			// Scale down by the height to try and make top cap texture coord area
			// proportional to base.
			float u = x / height + 0.5f;
			float v = z / height + 0.5f;

			meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
		}

		// Cap center vertex.
		meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

		// Cache the index of center vertex.
		std::uint32_t centerIndex = (std::uint32_t)meshData.Vertices.size() - 1;

		for (std::uint32_t i = 0; i < sliceCount; ++i)
		{
			meshData.Indices.push_back(centerIndex);
			meshData.Indices.push_back(baseIndex + i);
			meshData.Indices.push_back(baseIndex + i + 1);
		}
	}
};