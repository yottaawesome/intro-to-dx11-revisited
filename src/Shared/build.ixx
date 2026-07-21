export module shared:build;

export
{
	constexpr auto IsDebugBuild =
#if defined(DEBUG) | defined(_DEBUG)
		true;
#else
		false;
#endif

	constexpr auto IsReleaseBuild = not IsDebugBuild;
}