export module litskull:vertex;
import std;
import shared;


struct InputLayoutDesc
{
	// Init like const int A::a[4] = {0, 1, 2, 3}; in .cpp file.
	static constexpr inline D3D11::D3D11_INPUT_ELEMENT_DESC PosNormal[2] = {
		{"POSITION", 0, DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI::DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11::D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
};
