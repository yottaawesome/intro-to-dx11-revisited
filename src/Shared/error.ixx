export module shared:error;
import std;
import :win32;
import :strings;

export
{
	class ComException : public std::runtime_error
	{
	public:
		ComException() = default;
		ComException(
			Win32::HRESULT hr,
			const std::source_location& location = std::source_location::current()
		) : errorCode(hr),
			location(location),
			std::runtime_error{ ToString(hr, location) }
		{}
		ComException(
			Win32::HRESULT hr,
			std::string_view msg,
			const std::source_location& location = std::source_location::current()
		) : errorCode(hr),
			location(location),
			std::runtime_error{ ToString(hr, location, msg) }
		{}

		auto ErrorCode() const noexcept -> Win32::HRESULT { return errorCode; }
		auto Location() const noexcept -> std::source_location { return location; }

	private:
		static auto ToString(Win32::HRESULT errorCode, const std::source_location& location, std::string_view customMsg = {}) -> std::string
		{
			// Get the string description of the error code.
			auto msg = std::wstring{ Win32::_com_error{ errorCode }.ErrorMessage() };
			auto err1 = std::format(
				"{} failed in {} at line {}",
				location.function_name(),
				location.file_name(),
				location.line()
			);
			if (not customMsg.empty())
				return std::format("{}; error: {}; message: {}", err1, WStringToAnsi(msg), customMsg);
			return std::format("{}; error: {}", err1, WStringToAnsi(msg));
		}

		Win32::HRESULT errorCode = 0x0;
		std::source_location location = std::source_location::current();
	};

	class DxException : public ComException
	{
	public:
		DxException() = default;
		DxException(
			Win32::HRESULT hr,
			const std::source_location& location = std::source_location::current()
		) : ComException(hr, location)
		{}
		DxException(
			Win32::HRESULT hr,
			std::string_view msg,
			const std::source_location& location = std::source_location::current()
		) : ComException(hr, msg, location)
		{}
	};

	void ErrorMsg(const std::exception& ex)
	{
		Win32::MessageBoxA(0, ex.what(), "Error", Win32::MbOK);
	}

	void ErrorMsg(std::string_view msg)
	{
		Win32::MessageBoxA(0, msg.data(), "Error", Win32::MbOK);
	}

	void ErrorMsg(std::wstring_view msg)
	{
		Win32::MessageBoxW(0, msg.data(), L"Error", Win32::MbOK);
	}
}
