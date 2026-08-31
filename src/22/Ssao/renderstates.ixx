export module ssaodemo:renderstates;
import std;
import shared;

struct RenderStates
{
	RenderStates(D3D11::ID3D11Device* device)
	{
		//
		// WireframeRS
		//
		auto wireframeDesc = D3D11::D3D11_RASTERIZER_DESC{
			.FillMode = D3D11::D3D11_FILL_MODE::D3D11_FILL_WIREFRAME,
			.CullMode = D3D11::D3D11_CULL_MODE::D3D11_CULL_BACK,
			.FrontCounterClockwise = false,
			.DepthClipEnable = true
		};
		HR(device->CreateRasterizerState(&wireframeDesc, &WireframeRS));

		//
		// NoCullRS
		//
		auto noCullDesc = D3D11::D3D11_RASTERIZER_DESC{
			.FillMode = D3D11::D3D11_FILL_MODE::D3D11_FILL_SOLID,
			.CullMode = D3D11::D3D11_CULL_MODE::D3D11_CULL_NONE,
			.FrontCounterClockwise = false,
			.DepthClipEnable = true
		};

		HR(device->CreateRasterizerState(&noCullDesc, &NoCullRS));

		//
		// ShadowMapRS
		//
		auto shadowMapDesc = D3D11::D3D11_RASTERIZER_DESC{
			.FillMode = D3D11::D3D11_FILL_MODE::D3D11_FILL_SOLID,
			.CullMode = D3D11::D3D11_CULL_MODE::D3D11_CULL_BACK,
			.FrontCounterClockwise = false,
			.DepthBias = 100000,
			.DepthBiasClamp = 0.0f,
			.SlopeScaledDepthBias = 1.0f,
			.DepthClipEnable = true
		};
		HR(device->CreateRasterizerState(&shadowMapDesc, &ShadowMapRS));

		//
		// AlphaToCoverageBS
		//
		auto alphaToCoverageDesc = D3D11::D3D11_BLEND_DESC{
			.AlphaToCoverageEnable = true,
			.IndependentBlendEnable = false,
			.RenderTarget = {{
				.BlendEnable = false,
				.RenderTargetWriteMask = D3D11::D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL
			}}
		};
		HR(device->CreateBlendState(&alphaToCoverageDesc, &AlphaToCoverageBS));

		//
		// TransparentBS
		//
		auto transparentDesc = D3D11::D3D11_BLEND_DESC{
			.AlphaToCoverageEnable = false,
			.IndependentBlendEnable = false,
			.RenderTarget = {{
				.BlendEnable = true,
				.SrcBlend = D3D11::D3D11_BLEND::D3D11_BLEND_SRC_ALPHA,
				.DestBlend = D3D11::D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D11::D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11::D3D11_BLEND::D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11::D3D11_BLEND::D3D11_BLEND_ZERO,
				.BlendOpAlpha = D3D11::D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11::D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL
			}}
		};
		HR(device->CreateBlendState(&transparentDesc, &TransparentBS));

		//
		// LessEqualDSS
		//
		auto lessEqualDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL,
			.StencilEnable = false
		};
		HR(device->CreateDepthStencilState(&lessEqualDesc, &LessEqualDSS));

		//
		// EqualDSS
		//
		auto equalDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL,
			.StencilEnable = false
		};
		HR(device->CreateDepthStencilState(&equalDesc, &EqualDSS));
	}

	ComPtr<D3D11::ID3D11RasterizerState> WireframeRS;
	ComPtr<D3D11::ID3D11RasterizerState> NoCullRS;
	ComPtr<D3D11::ID3D11RasterizerState> ShadowMapRS;
	ComPtr<D3D11::ID3D11BlendState> AlphaToCoverageBS;
	ComPtr<D3D11::ID3D11BlendState> TransparentBS;
	ComPtr<D3D11::ID3D11DepthStencilState> LessEqualDSS;
	ComPtr<D3D11::ID3D11DepthStencilState> EqualDSS;
};
