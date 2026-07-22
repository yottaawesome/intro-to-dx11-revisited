export module initdirect3d;
import std;
import shared;

export class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{
		Init();
	}

	void OnResize()
	{
		D3DApp::OnResize();
	}

	void UpdateScene(float dt)
	{
	}

	void DrawScene()
	{
		//assert(md3dImmediateContext);
		//assert(mSwapChain);

		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.get(), reinterpret_cast<const float*>(&Colors::Blue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.get(),D3D11::D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11::D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 1.0f, 0);

		HR(mSwapChain->Present(0, 0));
	}
private:
	void Init()
	{
		D3DApp::Init();
	}
};