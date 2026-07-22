import std;
import shared;
import box;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

int wWinMain(Win32::HINSTANCE hInstance, Win32::HINSTANCE, Win32::LPWSTR, int)
{
	// Enable run-time memory check for debug builds.
	if constexpr (IsDebugBuild)
		Win32::_CrtSetDbgFlag(Win32::CrtAllocMemDf | Win32::CrtLeakCheckDf);

	BoxApp theApp(hInstance);
	if (!theApp.Init())
		return 0;
	return theApp.Run();
}
