import std;
import shared;
import mirrordemo;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

auto wWinMain(Win32::HINSTANCE hInstance, Win32::HINSTANCE, Win32::LPWSTR, int) -> int
try
{
	if constexpr (IsDebugBuild)
		Win32::_CrtSetDbgFlag(Win32::CrtAllocMemDf | Win32::CrtLeakCheckDf);
	return MirrorApp{ hInstance }.Run();
}
catch (const std::exception& e)
{
	ErrorMsg(e);
	return 1;
}