export module shared:strings;
import std;
import :win32;

export
{
	[[nodiscard]]
	auto WStringToAnsi(std::wstring_view wstr) -> std::string
	{
		if (wstr.empty())
			return {};

		// https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
		// Returns the size in bytes, this differs from MultiByteToWideChar, which returns the size in characters
		auto sizeInBytes =
			Win32::WideCharToMultiByte(
				Win32::CpUtf8,									// CodePage
				Win32::WcNoBestFitChars,						// dwFlags 
				wstr.data(),									// lpWideCharStr
				static_cast<int>(wstr.size()),					// cchWideChar 
				nullptr,										// lpMultiByteStr
				0,												// cbMultiByte
				nullptr,										// lpDefaultChar
				nullptr											// lpUsedDefaultChar
			);
		if (sizeInBytes == 0)
			throw std::runtime_error{ "WideCharToMultiByte() [1] failed" };

		auto strTo = std::string(sizeInBytes / sizeof(char), '\0');
		auto status =
			WideCharToMultiByte(
				Win32::CpUtf8,									// CodePage
				Win32::WcNoBestFitChars,						// dwFlags 
				wstr.data(),									// lpWideCharStr
				static_cast<int>(wstr.size()),					// cchWideChar 
				strTo.data(),									// lpMultiByteStr
				static_cast<int>(strTo.size() * sizeof(char)),	// cbMultiByte
				nullptr,										// lpDefaultChar
				nullptr											// lpUsedDefaultChar
			);
		if (status == 0)
			throw std::runtime_error{ "WideCharToMultiByte() [2] failed" };

		return strTo;
	}

	[[nodiscard]]
	auto AnsiToWString(std::string_view str) -> std::wstring
	{
		if (str.empty())
			return {};

		// https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
		// Returns the size in characters, this differs from WideCharToMultiByte, which returns the size in bytes
		auto sizeInCharacters =
			Win32::MultiByteToWideChar(
				Win32::CpUtf8,									// CodePage
				0,											// dwFlags
				str.data(),									// lpMultiByteStr
				static_cast<int>(str.size() * sizeof(char)),// cbMultiByte
				nullptr,									// lpWideCharStr
				0											// cchWideChar
			);
		if (sizeInCharacters == 0)
			throw std::runtime_error{ "MultiByteToWideChar() [1] failed" };

		auto wstrTo = std::wstring(sizeInCharacters, '\0');
		auto status =
			Win32::MultiByteToWideChar(
				Win32::CpUtf8,									// CodePage
				0,											// dwFlags
				str.data(),									// lpMultiByteStr
				static_cast<int>(str.size() * sizeof(char)),	// cbMultiByte
				wstrTo.data(),									// lpWideCharStr
				static_cast<int>(wstrTo.size())				// cchWideChar
			);
		if (status == 0)
			throw std::runtime_error{ "MultiByteToWideChar() [2] failed" };

		return wstrTo;
	}
}