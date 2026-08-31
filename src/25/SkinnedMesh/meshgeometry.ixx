export module skinnedmeshdemo:meshgeometry;
import std;
import shared;

class MeshGeometry
{
public:
	struct Subset
	{
		std::uint32_t Id = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t VertexStart = 0;
		std::uint32_t VertexCount = 0;
		std::uint32_t FaceStart = 0;
		std::uint32_t FaceCount = 0;
	};

public:
	MeshGeometry() = default;
	MeshGeometry(const MeshGeometry& rhs) = delete;
	MeshGeometry& operator=(const MeshGeometry& rhs) = delete;

	template <typename VertexType>
	void SetVertices(D3D11::ID3D11Device* device, const VertexType* vertices, std::uint32_t count)
	{
		mVertexStride = sizeof(VertexType);
		auto vbd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = sizeof(VertexType) * count,
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		auto vinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = vertices };
		HR(device->CreateBuffer(&vbd, &vinitData, &mVB));
	}

	void SetIndices(D3D11::ID3D11Device* device, const std::uint16_t* indices, std::uint32_t count)
	{
		auto ibd = D3D11::D3D11_BUFFER_DESC{
			.ByteWidth = static_cast<std::uint32_t>(sizeof(std::uint16_t) * count),
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		auto iinitData = D3D11::D3D11_SUBRESOURCE_DATA{ .pSysMem = indices };
		HR(device->CreateBuffer(&ibd, &iinitData, &mIB));
	}

	void SetSubsetTable(std::vector<Subset>& subsetTable)
	{
		mSubsetTable = subsetTable;
	}

	void Draw(D3D11::ID3D11DeviceContext* dc, std::uint32_t subsetId)
	{
		auto offset = 0u;

		dc->IASetVertexBuffers(0, 1, mVB.GetAddressOf(), &mVertexStride, &offset);
		dc->IASetIndexBuffer(mIB.get(), mIndexBufferFormat, 0);

		dc->DrawIndexed(
			mSubsetTable[subsetId].FaceCount * 3,
			mSubsetTable[subsetId].FaceStart * 3,
			0);
	}

	

private:
	ComPtr<D3D11::ID3D11Buffer> mVB;
	ComPtr<D3D11::ID3D11Buffer> mIB;

	DXGI::DXGI_FORMAT mIndexBufferFormat = DXGI_FORMAT_R16_UINT; // Always 16-bit
	std::uint32_t mVertexStride = 0;

	std::vector<Subset> mSubsetTable;
};
