export module shared:d3dutil;
import std;
import :win32;
import :mathhelper;
import :comptr;

export
{
	void ErrorMsg(const std::exception& ex)
	{
		Win32::MessageBoxA(0, ex.what(), "Error", Win32::MbOK);

	}
	void ErrorMsg(std::string_view msg)
	{
		Win32::MessageBoxA(0, msg.data(), "Error", Win32::MbOK);
	}

	void ErrorMsg(std::wstring_view msg)
	{
		Win32::MessageBoxW(0, msg.data(), L"Error", Win32::MbOK);
	}

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
		static auto CreateRandomTexture1DSRV(D3D11::ID3D11Device* device) -> ComPtr<D3D11::ID3D11ShaderResourceView>
		{
			// 
			// Create the random data.
			//
			auto randomValues = std::array<DirectX::XMFLOAT4, 1024>{};

			for (int i = 0; i < 1024; ++i)
			{
				randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
				randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
				randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
				randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
			}

			auto initData = D3D11::D3D11_SUBRESOURCE_DATA{
				.pSysMem = randomValues.data(),
				.SysMemPitch = static_cast<std::uint32_t>(randomValues.size() * sizeof(DirectX::XMFLOAT4)),
				.SysMemSlicePitch = 0
			};
			

			//
			// Create the texture.
			//
			auto texDesc = D3D11::D3D11_TEXTURE1D_DESC{
				.Width = 1024,
				.MipLevels = 1,
				.ArraySize = 1,
				.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
			};
			

			auto randomTex = ComPtr<D3D11::ID3D11Texture1D>{};
			HR(device->CreateTexture1D(&texDesc, &initData, randomTex.ReleaseAndGetAddressOf()));

			//
			// Create the resource view.
			//
			auto viewDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
				.Format = texDesc.Format,
				.ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE1D,
				.Texture1D = {
					.MostDetailedMip = 0,
					.MipLevels = texDesc.MipLevels,
				}
			};

			auto randomTexSRV = ComPtr<D3D11::ID3D11ShaderResourceView>{};
			HR(device->CreateShaderResourceView(randomTex.get(), &viewDesc, randomTexSRV.ReleaseAndGetAddressOf()));

			return randomTexSRV;
		}
	};

	class TextHelper
	{
	public:
		template<typename T>
		static auto ToString(const T& s) -> std::wstring
		{
			auto oss = std::wostringstream{};
			oss << s;
			return oss.str();
		}

		template<typename T>
		static auto FromString(const std::wstring& s) -> T
		{
			auto x = T{};
			auto iss = std::wistringstream{s};
			iss >> x;

			return x;
		}
	};

	// Order: left, right, bottom, top, near, far.
	void ExtractFrustumPlanes(DirectX::XMFLOAT4 planes[6], DirectX::CXMMATRIX T)
	{
		auto M = DirectX::XMFLOAT4X4{};
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
			auto v = DirectX::XMVECTOR{DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&planes[i]))};
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
		constexpr auto White = DirectX::XMVECTORF32{ 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr auto Black = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 1.0f };
		constexpr auto Red = DirectX::XMVECTORF32{ 1.0f, 0.0f, 0.0f, 1.0f };
		constexpr auto Green = DirectX::XMVECTORF32{ 0.0f, 1.0f, 0.0f, 1.0f };
		constexpr auto Blue = DirectX::XMVECTORF32{ 0.0f, 0.0f, 1.0f, 1.0f };
		constexpr auto Yellow = DirectX::XMVECTORF32{ 1.0f, 1.0f, 0.0f, 1.0f };
		constexpr auto Cyan = DirectX::XMVECTORF32{ 0.0f, 1.0f, 1.0f, 1.0f };
		constexpr auto Magenta = DirectX::XMVECTORF32{ 1.0f, 0.0f, 1.0f, 1.0f };
		constexpr auto Silver = DirectX::XMVECTORF32{ 0.75f, 0.75f, 0.75f, 1.0f };
		constexpr auto LightSteelBlue = DirectX::XMVECTORF32{ 0.69f, 0.77f, 0.87f, 1.0f };
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
		static auto ToXmColor(DirectX::FXMVECTOR v) -> DirectX::PackedVector::XMCOLOR
		{
			auto dest = DirectX::PackedVector::XMCOLOR{};
			DirectX::PackedVector::XMStoreColor(&dest, v);
			return dest;
		}

		///<summary>
		/// Converts XMVECTOR to XMFLOAT4, where XMVECTOR represents a color.
		///</summary>
		static auto ToXmFloat4(DirectX::FXMVECTOR v) -> DirectX::XMFLOAT4
		{
			auto dest = DirectX::XMFLOAT4{};
			DirectX::XMStoreFloat4(&dest, v);
			return dest;
		}

		static auto ArgbToAbgr(Win32::UINT argb) -> Win32::UINT
		{
			Win32::BYTE A = (argb >> 24) & 0xff;
			Win32::BYTE R = (argb >> 16) & 0xff;
			Win32::BYTE G = (argb >> 8) & 0xff;
			Win32::BYTE B = (argb >> 0) & 0xff;

			return (A << 24) | (B << 16) | (G << 8) | (R << 0);
		}
	};
}
