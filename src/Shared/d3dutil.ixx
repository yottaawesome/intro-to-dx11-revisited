export module shared:d3dutil;
import std;
import :win32;
import :mathhelper;
import :comptr;

export
{
	[[nodiscard]]
	auto WStringToAnsi(std::wstring_view wstr) -> std::string
	{
		if (wstr.empty())
			return {};

		// https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
		// Returns the size in bytes, this differs from MultiByteToWideChar, which returns the size in characters
		auto sizeInBytes =
			Win32::WideCharToMultiByte(
				Win32::CpUtf8,									// CodePage
				Win32::WcNoBestFitChars,						// dwFlags 
				wstr.data(),									// lpWideCharStr
				static_cast<int>(wstr.size()),					// cchWideChar 
				nullptr,										// lpMultiByteStr
				0,												// cbMultiByte
				nullptr,										// lpDefaultChar
				nullptr											// lpUsedDefaultChar
			);
		if (sizeInBytes == 0)
			throw std::runtime_error{ "WideCharToMultiByte() [1] failed" };

		auto strTo = std::string(sizeInBytes / sizeof(char), '\0');
		auto status =
			WideCharToMultiByte(
				Win32::CpUtf8,									// CodePage
				Win32::WcNoBestFitChars,						// dwFlags 
				wstr.data(),									// lpWideCharStr
				static_cast<int>(wstr.size()),					// cchWideChar 
				strTo.data(),									// lpMultiByteStr
				static_cast<int>(strTo.size() * sizeof(char)),	// cbMultiByte
				nullptr,										// lpDefaultChar
				nullptr											// lpUsedDefaultChar
			);
		if (status == 0)
			throw std::runtime_error{ "WideCharToMultiByte() [2] failed" };

		return strTo;
	}

	[[nodiscard]]
	auto AnsiToWString(std::string_view str) -> std::wstring
	{
		if (str.empty())
			return {};

		// https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
		// Returns the size in characters, this differs from WideCharToMultiByte, which returns the size in bytes
		auto sizeInCharacters =
			Win32::MultiByteToWideChar(
				Win32::CpUtf8,									// CodePage
				0,											// dwFlags
				str.data(),									// lpMultiByteStr
				static_cast<int>(str.size() * sizeof(char)),// cbMultiByte
				nullptr,									// lpWideCharStr
				0											// cchWideChar
			);
		if (sizeInCharacters == 0)
			throw std::runtime_error{ "MultiByteToWideChar() [1] failed" };

		auto wstrTo = std::wstring(sizeInCharacters, '\0');
		auto status =
			Win32::MultiByteToWideChar(
				Win32::CpUtf8,									// CodePage
				0,											// dwFlags
				str.data(),									// lpMultiByteStr
				static_cast<int>(str.size() * sizeof(char)),	// cbMultiByte
				wstrTo.data(),									// lpWideCharStr
				static_cast<int>(wstrTo.size())				// cchWideChar
			);
		if (status == 0)
			throw std::runtime_error{ "MultiByteToWideChar() [2] failed" };

		return wstrTo;
	}

	class DxException : public std::runtime_error
	{
	public:
		DxException() = default;
		DxException(
			Win32::HRESULT hr,
			const std::source_location& location = std::source_location::current()
		) : errorCode(hr),
			location(location),
			std::runtime_error{ ToString(hr, location) }
		{}
		DxException(
			Win32::HRESULT hr,
			std::string_view msg,
			const std::source_location& location = std::source_location::current()
		) : errorCode(hr),
			location(location),
			std::runtime_error{ ToString(hr, location, msg) }
		{}

		auto ErrorCode() const noexcept -> Win32::HRESULT { return errorCode; }
		auto Location() const noexcept -> std::source_location { return location; }

	private:
		static auto ToString(Win32::HRESULT errorCode, const std::source_location& location, std::string_view customMsg = {}) -> std::string
		{
			// Get the string description of the error code.
			auto msg = std::wstring{ Win32::_com_error{ errorCode }.ErrorMessage() };
			auto err1 = std::format(
				"{} failed in {} at line {}",
				location.function_name(),
				location.file_name(),
				location.line()
			);
			if (not customMsg.empty())
				return std::format("{}; error: {}; message: {}", err1, WStringToAnsi(msg), customMsg);
			return std::format("{}; error: {}", err1, WStringToAnsi(msg));
		}

		Win32::HRESULT errorCode = 0x0;
		std::source_location location = std::source_location::current();
	};

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

	void Append(std::ranges::range auto&& to, std::ranges::range auto&&... from)
	{
		(std::ranges::copy(from, std::back_inserter(to)), ...);
	}

	//---------------------------------------------------------------------------------------
	// Utility classes.
	//---------------------------------------------------------------------------------------

	inline auto HR(Win32::HRESULT hr, std::source_location location = std::source_location::current())
	{                                                           
		if (Win32::Failed(hr))                                  
			throw DxException{hr, location};
	}
	inline auto HR(Win32::HRESULT hr, std::string_view msg, std::source_location location = std::source_location::current())
	{
		if (Win32::Failed(hr))
			throw DxException{hr, msg, location};
	}

	struct d3dHelper
	{
		constexpr static auto Identity4x4 = DirectX::XMFLOAT4X4{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		static auto CreateConvertedTexture2DArraySRV(
			D3D11::ID3D11Device* device,
			const std::vector<std::wstring>& filenames,
			DXGI::DXGI_FORMAT format
		) -> ComPtr<D3D11::ID3D11ShaderResourceView>
		{
			if (filenames.empty())
				throw std::invalid_argument{ "At least one texture filename is required" };

			auto textures = std::vector<DirectX::ScratchImage>{};
			textures.reserve(filenames.size());

			auto elementMetadata = DirectX::TexMetadata{};

			for (const auto& filename : filenames)
			{
				auto source = DirectX::ScratchImage{};
				auto sourceMetadata = DirectX::TexMetadata{};
				HR(
					DirectX::LoadFromDDSFile(
						filename.c_str(),
						DirectX::DDS_FLAGS_NONE,
						&sourceMetadata,
						source
					),
					std::format("Failed to load texture '{}'", WStringToAnsi(filename))
				);

				if (sourceMetadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D or
					sourceMetadata.arraySize != 1 or
					sourceMetadata.IsCubemap())
				{
					throw std::runtime_error{
						std::format("'{}' is not a single 2D texture", WStringToAnsi(filename))
					};
				}

				auto converted = DirectX::ScratchImage{};
				if (DirectX::IsCompressed(sourceMetadata.format))
				{
					HR(
						DirectX::Decompress(
							source.GetImages(),
							source.GetImageCount(),
							sourceMetadata,
							format,
							converted
						),
						std::format("Failed to decompress texture '{}'", WStringToAnsi(filename))
					);
				}
				else if (sourceMetadata.format != format)
				{
					HR(
						DirectX::Convert(
							source.GetImages(),
							source.GetImageCount(),
							sourceMetadata,
							format,
							DirectX::TEX_FILTER_DEFAULT,
							DirectX::TexThresholdDefault,
							converted
						),
						std::format("Failed to convert texture '{}'", WStringToAnsi(filename))
					);
				}
				else
				{
					converted = std::move(source);
				}

				const auto& metadata = converted.GetMetadata();
				if (textures.empty())
				{
					elementMetadata = metadata;
				}
				else if (metadata.width != elementMetadata.width or
					metadata.height != elementMetadata.height or
					metadata.mipLevels != elementMetadata.mipLevels)
				{
					throw std::runtime_error{
						std::format("Texture '{}' does not match the texture array element dimensions", WStringToAnsi(filename))
					};
				}

				textures.push_back(std::move(converted));
			}

			auto textureArray = DirectX::ScratchImage{};
			HR(
				textureArray.Initialize2D(
					format,
					elementMetadata.width,
					elementMetadata.height,
					textures.size(),
					elementMetadata.mipLevels
				),
				"Failed to allocate the texture array"
			);

			for (auto arraySlice = 0ull; arraySlice < textures.size(); ++arraySlice)
			{
				for (auto mipLevel = 0ull; mipLevel < elementMetadata.mipLevels; ++mipLevel)
				{
					const auto* source = textures[arraySlice].GetImage(mipLevel, 0, 0);
					const auto* destination = textureArray.GetImage(mipLevel, arraySlice, 0);
					if (source == nullptr or destination == nullptr)
						throw std::runtime_error{ "Failed to access a texture array subresource" };

					HR(
						DirectX::CopyRectangle(
							*source,
							DirectX::Rect{ 0, 0, source->width, source->height },
							*destination,
							DirectX::TEX_FILTER_DEFAULT,
							0,
							0
						),
						"Failed to copy a texture array subresource"
					);
				}
			}

			auto textureArraySRV = ComPtr<D3D11::ID3D11ShaderResourceView>{};
			HR(
				DirectX::CreateShaderResourceView(
					device,
					textureArray.GetImages(),
					textureArray.GetImageCount(),
					textureArray.GetMetadata(),
					textureArraySRV.ReleaseAndGetAddressOf()
				),
				"Failed to create the texture array shader resource view"
			);

			return textureArraySRV;
		}

		static auto CreateTexture2DArraySRV(
			D3D11::ID3D11Device* device,
			D3D11::ID3D11DeviceContext* context,
			const std::vector<std::wstring>& filenames
		) -> ComPtr<D3D11::ID3D11ShaderResourceView>
		{
			if (filenames.empty())
				throw std::invalid_argument{ "At least one texture filename is required" };

			auto sourceTextures = std::vector<ComPtr<D3D11::ID3D11Texture2D>>{};
			sourceTextures.reserve(filenames.size());

			for (const auto& filename : filenames)
			{
				auto sourceResource = ComPtr<D3D11::ID3D11Resource>{};
				HR(
					DirectX::CreateDDSTextureFromFile(
						device,
						filename.c_str(),
						sourceResource.ReleaseAndGetAddressOf(),
						nullptr
					),
					std::format("Failed to load texture '{}'", WStringToAnsi(filename))
				);

				auto sourceTexture = ComPtr<D3D11::ID3D11Texture2D>{};
				HR(
					sourceResource->QueryInterface(
						sourceTexture.Uuid(),
						sourceTexture.ReleaseAndGetAddressOfVoid()
					),
					std::format("'{}' is not a 2D texture", WStringToAnsi(filename))
				);
				sourceTextures.push_back(std::move(sourceTexture));
			}

			auto textureElementDesc = D3D11::D3D11_TEXTURE2D_DESC{};
			sourceTextures.front()->GetDesc(&textureElementDesc);

			if (textureElementDesc.ArraySize != 1 or textureElementDesc.SampleDesc.Count != 1)
				throw std::runtime_error{ "Texture array elements must be non-multisampled 2D textures" };

			for (auto i = 1ull; i < sourceTextures.size(); ++i)
			{
				auto desc = D3D11::D3D11_TEXTURE2D_DESC{};
				sourceTextures[i]->GetDesc(&desc);

				if (desc.Width != textureElementDesc.Width or
					desc.Height != textureElementDesc.Height or
					desc.MipLevels != textureElementDesc.MipLevels or
					desc.ArraySize != 1 or
					desc.Format != textureElementDesc.Format or
					desc.SampleDesc.Count != textureElementDesc.SampleDesc.Count or
					desc.SampleDesc.Quality != textureElementDesc.SampleDesc.Quality)
				{
					throw std::runtime_error{
						std::format("Texture '{}' does not match the texture array element layout", WStringToAnsi(filenames[i]))
					};
				}
			}

			auto textureArrayDesc = D3D11::D3D11_TEXTURE2D_DESC{
				.Width = textureElementDesc.Width,
				.Height = textureElementDesc.Height,
				.MipLevels = textureElementDesc.MipLevels,
				.ArraySize = static_cast<std::uint32_t>(sourceTextures.size()),
				.Format = textureElementDesc.Format,
				.SampleDesc = {
					.Count = 1,
					.Quality = 0,
				},
				.Usage = D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11::D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
			};

			auto textureArray = ComPtr<D3D11::ID3D11Texture2D>{};
			HR(device->CreateTexture2D(&textureArrayDesc, nullptr, textureArray.ReleaseAndGetAddressOf()));

			for (auto arraySlice = 0ull; arraySlice < textureArrayDesc.ArraySize; ++arraySlice)
			{
				for (auto mipLevel = 0ull; mipLevel < textureArrayDesc.MipLevels; ++mipLevel)
				{
					const auto destinationSubresource =
						mipLevel + arraySlice * textureArrayDesc.MipLevels;

					context->CopySubresourceRegion(
						textureArray.get(),
						static_cast<std::uint32_t>(destinationSubresource),
						0,
						0,
						0,
						sourceTextures[arraySlice].get(),
						static_cast<std::uint32_t>(mipLevel),
						nullptr
					);
				}
			}

			auto viewDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
				.Format = textureArrayDesc.Format,
				.ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
				.Texture2DArray = {
					.MostDetailedMip = 0,
					.MipLevels = textureArrayDesc.MipLevels,
					.FirstArraySlice = 0,
					.ArraySize = textureArrayDesc.ArraySize,
				}
			};

			auto textureArraySRV = ComPtr<D3D11::ID3D11ShaderResourceView>{};
			HR(device->CreateShaderResourceView(
				textureArray.get(),
				&viewDesc,
				textureArraySRV.ReleaseAndGetAddressOf()
			));

			return textureArraySRV;
		}

		static auto CreateRandomTexture1DSRV(D3D11::ID3D11Device* device) -> ComPtr<D3D11::ID3D11ShaderResourceView>
		{
			// 
			// Create the random data.
			//
			auto randomValues = std::array<DirectX::XMFLOAT4, 1024>{};

			for (auto i = 0; i < 1024; ++i)
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
