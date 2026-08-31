export module shared:texturemanager;
import std;
import :win32;
import :comptr;
import :d3dutil;

///<summary>
/// Simple texture manager to avoid loading duplicate textures from file.  That can
/// happen, for example, if multiple meshes reference the same texture filename. 
///</summary>
export class TextureMgr
{
public:
	TextureMgr(D3D11::ID3D11Device* device)
		: md3dDevice(device) {}

	auto CreateTexture(std::wstring filename) -> ComPtr<D3D11::ID3D11ShaderResourceView>
	{
		// Does it already exist?
		if (mTextureSRV.find(filename) != mTextureSRV.end())
			return mTextureSRV[filename];

		auto srv = ComPtr<D3D11::ID3D11ShaderResourceView>{};
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, filename.c_str(), nullptr, &srv, 0));
		mTextureSRV[filename] = srv;
		return srv;
	}

	TextureMgr(const TextureMgr& rhs) = delete;
	TextureMgr& operator=(const TextureMgr& rhs) = delete;

private:
	D3D11::ID3D11Device* md3dDevice;
	std::map<std::wstring, ComPtr<D3D11::ID3D11ShaderResourceView>> mTextureSRV;
};