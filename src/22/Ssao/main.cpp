#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

import std;
import shared;
import ssaodemo;

auto wWinMain(Win32::HINSTANCE hInstance, Win32::HINSTANCE, Win32::LPWSTR, int) -> int
try
{
	Win32::SetDebugBuildFlag(Win32::CrtAllocMemDf | Win32::CrtLeakCheckDf);
	return SsaoApp{ hInstance }.Run();
}
catch (const std::exception& e)
{
	ErrorMsg(e);
	return 1;
}
