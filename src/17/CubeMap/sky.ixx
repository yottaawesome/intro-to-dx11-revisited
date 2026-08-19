export module cubemap:sky;
import std;
import shared;

class Sky
{
public:
	Sky(D3D11::ID3D11Device* device, const std::wstring& cubemapFilename, float skySphereRadius);

	auto CubeMapSRV() -> D3D11::ID3D11ShaderResourceView*
	{
		return mCubeMapSRV.get();
	}

	void Draw(D3D11::ID3D11DeviceContext* dc, const Camera& camera)
	{
		throw std::runtime_error{ "This function is not yet complete." };

		// center Sky about eye in world space
		DirectX::XMFLOAT3 eyePos = camera.GetPosition();
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);


		DirectX::XMMATRIX WVP = DirectX::XMMatrixMultiply(T, camera.ViewProj());

		// TODO: implement per frame constant buffer for sky effect
		/*Effects::SkyFX->SetWorldViewProj(WVP);
		Effects::SkyFX->SetCubeMap(mCubeMapSRV);*/

		auto stride = static_cast<std::uint32_t>(sizeof(DirectX::XMFLOAT3));
		auto offset = 0u;
		dc->IASetVertexBuffers(0, 1, mVB.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mIB.get(), DXGI_FORMAT_R16_UINT, 0);
		// TODO: implement handling  of different input layouts
		//dc->IASetInputLayout(InputLayouts::Pos); 
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dc->DrawIndexed(mIndexCount, 0, 0);
	}

private:
	Sky(const Sky& rhs);
	auto operator=(const Sky& rhs) -> Sky&;

private:
	ComPtr<D3D11::ID3D11Buffer> mVB;
	ComPtr<D3D11::ID3D11Buffer> mIB;
	ComPtr<D3D11::ID3D11ShaderResourceView> mCubeMapSRV;
	std::uint32_t mIndexCount;
};