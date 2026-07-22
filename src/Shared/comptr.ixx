export module shared:comptr;
import std;
import :win32;

export template<typename T>
class ComPtr
{
public:
	struct Own { T* ptr = nullptr; };
	struct Copy { T* ptr = nullptr; };
	constexpr ~ComPtr()
	{
		if (m_ptr)
			m_ptr->Release();
	}

	constexpr ComPtr() = default;

	constexpr ComPtr(Own own)
		: m_ptr(own.ptr)
	{}
	constexpr ComPtr(Copy copy)
		: m_ptr(copy.ptr)
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

	constexpr void swap(ComPtr<T>& other)
	{
		std::swap(m_ptr, other.m_ptr);
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

private:
	T* m_ptr = nullptr;
};