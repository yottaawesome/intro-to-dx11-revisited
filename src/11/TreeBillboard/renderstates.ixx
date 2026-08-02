export module treebillboard:renderstates;
import std;
import shared;

struct RenderStates
{
	void InitAll(D3D11::ID3D11Device* device)
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
		// CullClockwiseRS
		//

		// Note: Define such that we still cull backfaces by making front faces CCW.
		// If we did not cull backfaces, then we have to worry about the BackFace
		// property in the D3D11_DEPTH_STENCIL_DESC.
		auto cullClockwiseDesc = D3D11::D3D11_RASTERIZER_DESC{
			.FillMode = D3D11::D3D11_FILL_MODE::D3D11_FILL_SOLID,
			.CullMode = D3D11::D3D11_CULL_MODE::D3D11_CULL_BACK,
			.FrontCounterClockwise = true,
			.DepthClipEnable = true
		};
		HR(device->CreateRasterizerState(&cullClockwiseDesc, &CullClockwiseRS));

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
		// NoRenderTargetWritesBS
		//
		auto noRenderTargetWritesDesc = D3D11::D3D11_BLEND_DESC{
			.AlphaToCoverageEnable = false,
			.IndependentBlendEnable = false,
			.RenderTarget = {{
				.BlendEnable = false,
				.SrcBlend = D3D11::D3D11_BLEND::D3D11_BLEND_ONE,
				.DestBlend = D3D11::D3D11_BLEND::D3D11_BLEND_ZERO,
				.BlendOp = D3D11::D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11::D3D11_BLEND::D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11::D3D11_BLEND::D3D11_BLEND_ZERO,
				.BlendOpAlpha = D3D11::D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = 0
			}}
		};
		HR(device->CreateBlendState(&noRenderTargetWritesDesc, &NoRenderTargetWritesBS));

		//
		// MarkMirrorDSS
		//
		auto mirrorDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS,
			.StencilEnable = true,
			.StencilReadMask = 0xff,
			.StencilWriteMask = 0xff,
			.FrontFace = { .StencilFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilPassOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE,
				.StencilFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS
			},
			// We are not rendering backfacing polygons, so these settings do not matter.
			.BackFace = { .StencilFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilPassOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE,
				.StencilFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS
			}
		};
		HR(device->CreateDepthStencilState(&mirrorDesc, &MarkMirrorDSS));

		//
		// DrawReflectionDSS
		//
		auto drawReflectionDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS,
			.StencilEnable = true,
			.StencilReadMask = 0xff,
			.StencilWriteMask = 0xff,
			.FrontFace = { .StencilFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilPassOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL
			},
			// We are not rendering backfacing polygons, so these settings do not matter.
			.BackFace = { .StencilFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilPassOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL
			}
		};
		HR(device->CreateDepthStencilState(&drawReflectionDesc, &DrawReflectionDSS));

		//
		// NoDoubleBlendDSS
		//
		auto noDoubleBlendDesc = D3D11::D3D11_DEPTH_STENCIL_DESC{
			.DepthEnable = true,
			.DepthWriteMask = D3D11::D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS,
			.StencilEnable = true,
			.StencilReadMask = 0xff,
			.StencilWriteMask = 0xff,
			.FrontFace = { 
				.StencilFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilPassOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR,
				.StencilFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL
			},
			// We are not rendering backfacing polygons, so these settings do not matter.
			.BackFace = {
				.StencilFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP,
				.StencilPassOp = D3D11::D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR,
				.StencilFunc = D3D11::D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL
			}
		};
		HR(device->CreateDepthStencilState(&noDoubleBlendDesc, &NoDoubleBlendDSS));
	}

	// Rasterizer states
	ComPtr<D3D11::ID3D11RasterizerState> WireframeRS;
	ComPtr<D3D11::ID3D11RasterizerState> NoCullRS;
	ComPtr<D3D11::ID3D11RasterizerState> CullClockwiseRS;

	// Blend states
	ComPtr<D3D11::ID3D11BlendState> AlphaToCoverageBS;
	ComPtr<D3D11::ID3D11BlendState> TransparentBS;
	ComPtr<D3D11::ID3D11BlendState> NoRenderTargetWritesBS;

	// Depth/stencil states
	ComPtr<D3D11::ID3D11DepthStencilState> MarkMirrorDSS;
	ComPtr<D3D11::ID3D11DepthStencilState> DrawReflectionDSS;
	ComPtr<D3D11::ID3D11DepthStencilState> NoDoubleBlendDSS;
};