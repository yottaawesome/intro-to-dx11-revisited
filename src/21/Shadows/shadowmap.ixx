export module shadowsdemo:shadowmap;
import std;
import shared;

class ShadowMap
{
public:
	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;

	ShadowMap(D3D11::ID3D11Device* device, std::uint32_t width, std::uint32_t height)
        : mWidth(width), mHeight(height)
    {
		mViewport = D3D11::D3D11_VIEWPORT{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(width),
			.Height = static_cast<float>(height),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};

        // Use typeless format because the DSV is going to interpret
        // the bits as DXGI_FORMAT_D24_UNORM_S8_UINT, whereas the SRV is going to interpret
        // the bits as DXGI_FORMAT_R24_UNORM_X8_TYPELESS.
        auto texDesc = D3D11::D3D11_TEXTURE2D_DESC{
            .Width = mWidth,
            .Height = mHeight,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS,
            .SampleDesc = {1, 0},
            .Usage = D3D11::D3D11_USAGE::D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11::D3D11_BIND_FLAG{D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE},
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };

        // View saves a reference to the texture so we can release our reference.
        auto depthMap = ComPtr<D3D11::ID3D11Texture2D>{};
        HR(device->CreateTexture2D(&texDesc, 0, &depthMap));

        auto dsvDesc = D3D11::D3D11_DEPTH_STENCIL_VIEW_DESC{
            .Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT,
            .ViewDimension = D3D11::D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D,
            .Flags = 0,
            .Texture2D = {.MipSlice = 0}
        };
        
        HR(device->CreateDepthStencilView(depthMap.get(), &dsvDesc, &mDepthMapDSV));

        auto srvDesc = D3D11::D3D11_SHADER_RESOURCE_VIEW_DESC{
            .Format = DXGI::DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
            .ViewDimension = D3D11::D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = texDesc.MipLevels,
            }
        };
        HR(device->CreateShaderResourceView(depthMap.get(), &srvDesc, &mDepthMapSRV));
    }

	auto DepthMapSRV() -> D3D11::ID3D11ShaderResourceView*
	{
		return mDepthMapSRV.get();
	}

	void BindDsvAndSetNullRenderTarget(D3D11::ID3D11DeviceContext* dc)
	{
		dc->RSSetViewports(1, &mViewport);

		// Set null render target because we are only going to draw to depth buffer.
		// Setting a null render target will disable color writes.
		D3D11::ID3D11RenderTargetView* renderTargets[1] = { 0 };
		dc->OMSetRenderTargets(1, renderTargets, mDepthMapDSV.get());

		dc->ClearDepthStencilView(mDepthMapDSV.get(), D3D11::D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

private:
	std::uint32_t mWidth;
	std::uint32_t mHeight;

	ComPtr<D3D11::ID3D11ShaderResourceView> mDepthMapSRV;
	ComPtr<D3D11::ID3D11DepthStencilView> mDepthMapDSV;

	D3D11::D3D11_VIEWPORT mViewport{};
};