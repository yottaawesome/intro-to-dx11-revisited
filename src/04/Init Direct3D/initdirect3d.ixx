export module initdirect3d;
import std;
import shared;

export class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(Win32::HINSTANCE hInstance)
		: D3DApp(hInstance)
	{}

	bool Init()
	{
		if (!D3DApp::Init())
			return false;

		return true;
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

		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Blue));
		md3dImmediateContext->ClearDepthStencilView(mDepthStencilView,D3D11::D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11::D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 1.0f, 0);

		HR(mSwapChain->Present(0, 0));
	}
};