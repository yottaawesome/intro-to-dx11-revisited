import std;
import shared;
import box;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

auto wWinMain(Win32::HINSTANCE hInstance, Win32::HINSTANCE, Win32::LPWSTR, int) -> int
try
{
	// Enable run-time memory check for debug builds.
	if constexpr (IsDebugBuild)
		Win32::_CrtSetDbgFlag(Win32::CrtAllocMemDf | Win32::CrtLeakCheckDf);
	return BoxApp{ hInstance }.Run();
}
catch (const std::exception& e)
{
	ErrorMsg(e);
	return 1;
}
