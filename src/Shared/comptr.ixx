export module shared:comptr;
import std;
import :win32;
import :error;

// A smart pointer for COM objects that automatically manages reference counting. Intended as a 
// replacement for Microsoft::WRL::ComPtr as it exposes clearer ownership semantics at 
// construction via Own and Copy tags.
export template<typename T>
class ComPtr
{
public:
	using pointer = T*;

	struct Own { T* ptr = nullptr; };
	struct Copy { T* ptr = nullptr; };
	
	constexpr ~ComPtr()
	{
		if (m_ptr)
			m_ptr->Release();
	}
	
	// Constructors and assignment operators
	constexpr ComPtr() = default;

	constexpr ComPtr(Own own) : m_ptr(own.ptr)
	{}
	constexpr ComPtr(Copy copy) : m_ptr(copy.ptr)
	{
		if (m_ptr)
			m_ptr->AddRef();
	}

	constexpr ComPtr(const ComPtr<T>& other)
		: m_ptr(other.m_ptr)
	{
		if (m_ptr)
			m_ptr->AddRef();
	}
	auto operator=(const ComPtr<T>& other) -> ComPtr<T>&
	{
		if (this != &other)
		{
			reset();
			m_ptr = other.m_ptr;
			if (m_ptr)
				m_ptr->AddRef();
		}
		return *this;
	}

	constexpr ComPtr(ComPtr<T>&& other) noexcept
		: m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}
	constexpr auto operator=(ComPtr<T>&& other) noexcept -> ComPtr<T>&
	{
		if (*this != other)
		{
			reset();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}

	// public member functions
	constexpr auto Uuid() const -> Win32::GUID
	{
		return __uuidof(T);
	}

	constexpr auto operator->() const -> T*
	{
		return m_ptr;
	}

	constexpr auto operator&() -> T**
	{
		if (m_ptr)
			m_ptr->Release();
		return &m_ptr;
	}

	constexpr auto ReleaseAndGetAddressOf() -> T**
	{
		reset();
		return &m_ptr;
	}

	constexpr auto ReleaseAndGetAddressOfVoid() -> void**
	{
		reset();
		return reinterpret_cast<void**>(&m_ptr);
	}

	constexpr auto GetAddressOf() -> T**
	{
		return &m_ptr;
	}

	constexpr void reset()
	{
		if (m_ptr)
		{
			m_ptr->Release();
			m_ptr = nullptr;
		}
	}

	constexpr void reset(T* ptr)
	{
		reset();
		m_ptr = ptr;
	}

	constexpr void swap(ComPtr<T>& other)
	{
		std::exchange(m_ptr, other.m_ptr);
	}

	constexpr auto get() const -> T*
	{
		return m_ptr;
	}

	constexpr operator bool() const
	{
		return m_ptr != nullptr;
	}

	constexpr auto operator*() const -> T&
	{
		return *m_ptr;
	}

	constexpr auto operator==(const ComPtr<T>& other) const -> bool
	{
		return m_ptr == other.m_ptr;
	}

	constexpr auto VoidAddress() -> void**
	{
		return reinterpret_cast<void**>(&m_ptr);
	}

	template<typename T>
	constexpr auto TryAs() noexcept -> std::expected<ComPtr<T>, Win32::HRESULT>
	{
		if (not m_ptr)
			return ComPtr<T>{};
		auto result = ComPtr<T>{};
		auto hr = m_ptr->QueryInterface(__uuidof(T), reinterpret_cast<void**>(&result.ptr));
		return Win32::Succeeded(hr) ? result : std::unexpected{ hr };
	}

	template<typename T>
	constexpr auto As() -> ComPtr<T>
	{
		auto result = TryAs<T>();
		if (not result)
			throw ComException{ result.error(), "QueryInterface() failed" };
		return *result;
	}

private:
	T* m_ptr = nullptr;
};