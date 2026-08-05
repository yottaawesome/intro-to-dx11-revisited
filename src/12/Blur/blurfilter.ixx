export module blur:blurfilter;
import std;
import shared;

class BlurFilter
{
public:
	BlurFilter() = default;
	BlurFilter(const BlurFilter&) = delete;
	auto operator=(const BlurFilter&) -> BlurFilter& = delete;
	BlurFilter(BlurFilter&&) = default;
	auto operator=(BlurFilter&&) -> BlurFilter & = default;

	/// The width and height should match the dimensions of the input texture to blur.
	/// It is OK to call Init() again to reinitialize the blur filter with a different 
	/// dimension or format.
	void Init(D3D11::ID3D11Device* device, std::uint32_t width, std::uint32_t height, DXGI::DXGI_FORMAT format)
	{
		// Start fresh.
		mBlurredOutputTexSRV.reset();
		mBlurredOutputTexUAV.reset();

		mWidth = width;
		mHeight = height;

		// Note, compressed formats cannot be used for UAV.  We get error like:
		// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
		// cannot be bound as an UnorderedAccessView, or cast to a format that
		// could be bound as an UnorderedAccessView.  Therefore this format 
		// does not support D3D11_BIND_UNORDERED_ACCESS.

		auto blurredTexDesc = D3D11::D3D11_TEXTURE2D_DESC{
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = format,
			.SampleDesc = {.Count = 1, .Quality = 0},
			.Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11::D3D11_BIND_FLAG{D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS},
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		
		auto blurredTex = ComPtr<D3D11::ID3D11Texture2D>{};
		HR(device->CreateTexture2D(&blurredTexDesc, 0, &blurredTex));

		auto srvDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
			.Format = format,
			.ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = {.MostDetailedMip = 0, .MipLevels = 1}
		};
		HR(device->CreateShaderResourceView(blurredTex.get(), &srvDesc, &mBlurredOutputTexSRV));

		auto uavDesc = D3D11::D3D11_UNORDERED_ACCESS_VIEW_DESC{
			.Format = format,
			.ViewDimension = D3D11::D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE2D,
			.Texture2D = {.MipSlice = 0}
		};
		HR(device->CreateUnorderedAccessView(blurredTex.get(), &uavDesc, &mBlurredOutputTexUAV));

		// Views save a reference to the texture so we can release our reference.
	}


	/// Blurs the input texture blurCount times.  Note that this modifies the input texture, not a copy of it.
	void BlurInPlace(
		D3D11::ID3D11DeviceContext* dc,
		D3D11::ID3D11ShaderResourceView* inputSRV,
		D3D11::ID3D11UnorderedAccessView* inputUAV,
		D3D11::ID3D11ComputeShader* horzBlurCS,
		D3D11::ID3D11ComputeShader* vertBlurCS,
		int blurCount)
	{
		auto nullSRV = std::array<D3D11::ID3D11ShaderResourceView*, 1>{ nullptr };
		auto nullUAV = std::array<D3D11::ID3D11UnorderedAccessView*, 1>{ nullptr };

		for (int i = 0; i < blurCount; ++i)
		{
			auto horizontalInput = std::array{ inputSRV };
			auto horizontalOutput = std::array{ mBlurredOutputTexUAV.get() };
			dc->CSSetShader(horzBlurCS, nullptr, 0);
			dc->CSSetShaderResources(0, static_cast<std::uint32_t>(horizontalInput.size()), horizontalInput.data());
			dc->CSSetUnorderedAccessViews(0, static_cast<std::uint32_t>(horizontalOutput.size()), horizontalOutput.data(), nullptr);

			// How many groups do we need to dispatch to cover a row of pixels, where each
			// group covers 256 pixels (the 256 is defined in the ComputeShader).
			auto numGroupsX = static_cast<std::uint32_t>(std::ceilf(mWidth / 256.0f));
			dc->Dispatch(numGroupsX, mHeight, 1);

			dc->CSSetShaderResources(0, 1, nullSRV.data());
			dc->CSSetUnorderedAccessViews(0, 1, nullUAV.data(), nullptr);

			auto verticalInput = std::array{ mBlurredOutputTexSRV.get() };
			auto verticalOutput = std::array{ inputUAV };
			dc->CSSetShader(vertBlurCS, nullptr, 0);
			dc->CSSetShaderResources(0, static_cast<std::uint32_t>(verticalInput.size()), verticalInput.data());
			dc->CSSetUnorderedAccessViews(0, static_cast<std::uint32_t>(verticalOutput.size()), verticalOutput.data(), nullptr);

			// How many groups do we need to dispatch to cover a column of pixels, where each
			// group covers 256 pixels  (the 256 is defined in the ComputeShader).
			auto numGroupsY = static_cast<std::uint32_t>(std::ceilf(mHeight / 256.0f));
			dc->Dispatch(mWidth, numGroupsY, 1);

			dc->CSSetShaderResources(0, 1, nullSRV.data());
			dc->CSSetUnorderedAccessViews(0, 1, nullUAV.data(), nullptr);
		}

		dc->CSSetShader(nullptr, nullptr, 0);
	}

private:

	std::uint32_t mWidth;
	std::uint32_t mHeight;

	ComPtr<D3D11::ID3D11ShaderResourceView> mBlurredOutputTexSRV;
	ComPtr<D3D11::ID3D11UnorderedAccessView> mBlurredOutputTexUAV;
};
