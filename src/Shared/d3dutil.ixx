export module shared:d3dutil;
import std;
import :win32;
import :mathhelper;

export
{
	//---------------------------------------------------------------------------------------
	// Utility classes.
	//---------------------------------------------------------------------------------------

	inline auto HR(Win32::HRESULT hr, std::source_location location = std::source_location::current())
	{                                                           
		if (Win32::Failed(hr))                                  
			throw std::runtime_error{std::format("HRESULT 0x{:08X} failed at {}:{}:{}", hr, location.function_name(), location.file_name(), location.line())};
	}

	class d3dHelper
	{
	public:
		static D3D11::ID3D11ShaderResourceView* CreateRandomTexture1DSRV(D3D11::ID3D11Device* device)
		{
			// 
			// Create the random data.
			//
			DirectX::XMFLOAT4 randomValues[1024];

			for (int i = 0; i < 1024; ++i)
			{
				randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
				randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
				randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
				randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
			}

			D3D11::D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = randomValues;
			initData.SysMemPitch = 1024 * sizeof(DirectX::XMFLOAT4);
			initData.SysMemSlicePitch = 0;

			//
			// Create the texture.
			//
			D3D11::D3D11_TEXTURE1D_DESC texDesc;
			texDesc.Width = 1024;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			texDesc.Usage = D3D11_USAGE_IMMUTABLE;
			texDesc.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			texDesc.ArraySize = 1;

			D3D11::ID3D11Texture1D* randomTex = 0;
			HR(device->CreateTexture1D(&texDesc, &initData, &randomTex));

			//
			// Create the resource view.
			//
			D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
			viewDesc.Format = texDesc.Format;
			viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
			viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
			viewDesc.Texture1D.MostDetailedMip = 0;

			D3D11::ID3D11ShaderResourceView* randomTexSRV = 0;
			HR(device->CreateShaderResourceView(randomTex, &viewDesc, &randomTexSRV));

			randomTex->Release();

			return randomTexSRV;
		}
	};

	class TextHelper
	{
	public:

		template<typename T>
		static std::wstring ToString(const T& s)
		{
			std::wostringstream oss;
			oss << s;

			return oss.str();
		}

		template<typename T>
		static T FromString(const std::wstring& s)
		{
			T x;
			std::wistringstream iss(s);
			iss >> x;

			return x;
		}
	};

	// Order: left, right, bottom, top, near, far.
	void ExtractFrustumPlanes(DirectX::XMFLOAT4 planes[6], DirectX::CXMMATRIX T)
	{
		DirectX::XMFLOAT4X4 M;
		DirectX::XMStoreFloat4x4(&M, T);

		//
		// Left
		//


		planes[0].x = M(0, 3) + M(0, 0);
		planes[0].y = M(1, 3) + M(1, 0);
		planes[0].z = M(2, 3) + M(2, 0);
		planes[0].w = M(3, 3) + M(3, 0);

		//
		// Right
		//
		planes[1].x = M(0, 3) - M(0, 0);
		planes[1].y = M(1, 3) - M(1, 0);
		planes[1].z = M(2, 3) - M(2, 0);
		planes[1].w = M(3, 3) - M(3, 0);

		//
		// Bottom
		//
		planes[2].x = M(0, 3) + M(0, 1);
		planes[2].y = M(1, 3) + M(1, 1);
		planes[2].z = M(2, 3) + M(2, 1);
		planes[2].w = M(3, 3) + M(3, 1);

		//
		// Top
		//
		planes[3].x = M(0, 3) - M(0, 1);
		planes[3].y = M(1, 3) - M(1, 1);
		planes[3].z = M(2, 3) - M(2, 1);
		planes[3].w = M(3, 3) - M(3, 1);

		//
		// Near
		//
		planes[4].x = M(0, 2);
		planes[4].y = M(1, 2);
		planes[4].z = M(2, 2);
		planes[4].w = M(3, 2);

		//
		// Far
		//
		planes[5].x = M(0, 3) - M(0, 2);
		planes[5].y = M(1, 3) - M(1, 2);
		planes[5].z = M(2, 3) - M(2, 2);
		planes[5].w = M(3, 3) - M(3, 2);

		// Normalize the plane equations.
		for (int i = 0; i < 6; ++i)
		{
			DirectX::XMVECTOR v = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&planes[i]));
			DirectX::XMStoreFloat4(&planes[i], v);
		}
	}


	// #define XMGLOBALCONST extern CONST __declspec(selectany)
	//   1. extern so there is only one copy of the variable, and not a separate
	//      private copy in each .obj.
	//   2. __declspec(selectany) so that the compiler does not complain about
	//      multiple definitions in a .cpp file (it can pick anyone and discard 
	//      the rest because they are constant--all the same).

	namespace Colors
	{
		constexpr DirectX::XMVECTORF32 White = { 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
		constexpr DirectX::XMVECTORF32 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
		constexpr DirectX::XMVECTORF32 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
	}

	///<summary>
	/// Utility class for converting between types and formats.
	///</summary>
	class Convert
	{
	public:
		///<summary>
		/// Converts XMVECTOR to XMCOLOR, where XMVECTOR represents a color.
		///</summary>
		static DirectX::PackedVector::XMCOLOR ToXmColor(DirectX::FXMVECTOR v)
		{
			DirectX::PackedVector::XMCOLOR dest;
			DirectX::PackedVector::XMStoreColor(&dest, v);
			return dest;
		}

		///<summary>
		/// Converts XMVECTOR to XMFLOAT4, where XMVECTOR represents a color.
		///</summary>
		static DirectX::XMFLOAT4 ToXmFloat4(DirectX::FXMVECTOR v)
		{
			DirectX::XMFLOAT4 dest;
			DirectX::XMStoreFloat4(&dest, v);
			return dest;
		}

		static Win32::UINT ArgbToAbgr(Win32::UINT argb)
		{
			Win32::BYTE A = (argb >> 24) & 0xff;
			Win32::BYTE R = (argb >> 16) & 0xff;
			Win32::BYTE G = (argb >> 8) & 0xff;
			Win32::BYTE B = (argb >> 0) & 0xff;

			return (A << 24) | (B << 16) | (G << 8) | (R << 0);
		}
	};
}
