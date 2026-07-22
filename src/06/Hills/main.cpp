import std;
import shared;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

auto wWinMain(Win32::HINSTANCE hInstance, Win32::HINSTANCE, Win32::LPWSTR, int) -> int
try
{
    return 0;
}
catch (const std::exception& e)
{
	ErrorMsg(e);
	return 1;
}