import std;
import shared;
import initdirect3d;

#pragma comment(lib, "d3d11.lib")

auto wWinMain(Win32::HINSTANCE hInstance, Win32::HINSTANCE,Win32::LPWSTR, int) -> int
try
{
	// Enable run-time memory check for debug builds.
	if constexpr (IsDebugBuild)
		Win32::_CrtSetDbgFlag(Win32::CrtAllocMemDf | Win32::CrtLeakCheckDf);
	return InitDirect3DApp{ hInstance }.Run();
}
catch (const std::exception& e)
{
	ErrorMsg(e);
	return 1;
}
