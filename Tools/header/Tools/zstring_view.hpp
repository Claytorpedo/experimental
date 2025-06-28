#ifndef INCLUDE_CTP_TOOLS_ZSTRING_VIEW_HPP
#define INCLUDE_CTP_TOOLS_ZSTRING_VIEW_HPP

#include "config.hpp"
#include "debug.hpp"

#include <string_view>
#include <string>

#if CTP_USE_EXCEPTIONS
#include <stdexcept>
#endif

// zstring_view based off wg21.link/P3655

namespace ctp {

namespace detail {
template <typename CharT>
inline constexpr CharT NullString[1]{};
} // detail

// Simple wrapper for std::string_view that ensures given string is null terminated.
template<typename CharT, class Traits = std::char_traits<CharT>>
class basic_zstring_view {
	using impl_type = std::basic_string_view<CharT, Traits>;
	impl_type impl;

	[[nodiscard]] static constexpr const CharT* EmptyPtr() noexcept { return detail::NullString<CharT>; }
public:
	using traits_type = typename impl_type::traits_type;
	using value_type = typename impl_type::value_type;
	using pointer = typename impl_type::pointer;
	using const_pointer = typename impl_type::const_pointer;
	using reference = typename impl_type::reference;
	using const_reference = typename impl_type::const_reference;
	using const_iterator = typename impl_type::const_iterator;
	using iterator = typename impl_type::iterator;
	using reverse_iterator = typename impl_type::reverse_iterator;
	using const_reverse_iterator = typename impl_type::const_reverse_iterator;
	using size_type = typename impl_type::size_type;
	using difference_type = typename impl_type::difference_type;

	static constexpr size_type npos = impl_type::npos;

	constexpr basic_zstring_view() noexcept : impl{EmptyPtr(), 0} {}
	constexpr basic_zstring_view(const basic_zstring_view&) noexcept = default;
	constexpr basic_zstring_view& operator=(const basic_zstring_view&) noexcept = default;
	constexpr basic_zstring_view(const_pointer str) noexcept : impl{str} {}
	constexpr basic_zstring_view(const_pointer str, size_type len) CTP_NOEXCEPT(false) : impl{str, len} {
		if (str[len] != value_type{}) [[unlikely]] {
#if CTP_USE_EXCEPTIONS
			throw std::invalid_argument("given string is not null terminated");
#else
			std::terminate();
#endif
		}
	}

	// User provides guarantee that string is null terminated.
	// This is intentionally unergonomic.
	struct null_terminated_tag {};
	static constexpr null_terminated_tag null_terminated;
	constexpr basic_zstring_view(null_terminated_tag, const_pointer str, size_type len) noexcept
		: impl{str, len}
	{
		ctpExpects(str[len] == value_type{});
	}
	constexpr basic_zstring_view(null_terminated_tag, impl_type str) noexcept
		: impl{str}
	{
		if (str.data() != nullptr) {
			ctpExpects(str.data()[str.size()] == value_type{});
		} else {
			ctpExpects(str.empty()); // malformed string_view?
			impl = impl_type{EmptyPtr(), 0};
		}
	}

	constexpr basic_zstring_view(const std::basic_string<CharT, Traits>& str) noexcept : impl{str} {
		if (str.empty())
			impl_type{EmptyPtr(), 0};
	}

	basic_zstring_view(std::nullptr_t) = delete;
	basic_zstring_view(std::nullptr_t, size_type) = delete;

	friend constexpr bool operator==(basic_zstring_view lhs, basic_zstring_view rhs) noexcept = default;
	friend constexpr auto operator<=>(basic_zstring_view lhs, basic_zstring_view rhs) noexcept = default;

	constexpr const_iterator begin() const noexcept { return impl.begin(); }
	constexpr const_iterator end() const noexcept { return impl.end(); }
	constexpr const_iterator cbegin() const noexcept { return impl.cbegin(); };
	constexpr const_iterator cend() const noexcept { return impl.cend(); };
	constexpr const_reverse_iterator rbegin() const noexcept { return impl.rbegin(); };
	constexpr const_reverse_iterator rend() const noexcept { return impl.rend(); };
	constexpr const_reverse_iterator crbegin() const noexcept { return impl.crbegin(); };
	constexpr const_reverse_iterator crend() const noexcept { return impl.crend(); };

	// Get iterator to the last element.
	constexpr iterator last() noexcept { return impl.begin() + (impl.size() - 1); }
	// Get iterator to the last element.
	constexpr const_iterator last() const noexcept { return impl.begin() + (impl.size() - 1); }

	constexpr size_type size() const noexcept { return impl.size(); };
	constexpr size_type length() const noexcept { return impl.length(); };
	constexpr size_type max_size() const noexcept { return impl.max_size(); };
	[[nodiscard]] constexpr bool empty() const noexcept { return impl.empty(); };
	constexpr bool is_empty() const noexcept { return impl.empty(); };

	constexpr const_reference operator[](size_type pos) const { return impl[pos]; }
	constexpr const_reference at(size_type pos) const { return impl.at(pos); }
	constexpr const_reference front() const { return impl.front(); }
	constexpr const_reference back() const { return impl.back(); }
	constexpr const_pointer data() const noexcept { return impl.data(); };
	constexpr const_pointer cstr() const noexcept { return impl.data(); };

	constexpr operator impl_type() const noexcept { return impl; }

	constexpr void remove_prefix(size_type n) { impl.remove_prefix(n); }
	constexpr void remove_suffix(size_type n) =
#ifdef __cpp_deleted_function
		delete("Cannot remove_suffix in-place on zstring_view while retaining null terminator. Use substr instead.");
#else
		delete;
#endif

	constexpr void swap(basic_zstring_view& o) noexcept { impl.swap(o); }

	constexpr size_type copy(value_type* s, size_type n, size_type pos = 0) const { return impl.copy(s, n, pos); }

	constexpr basic_zstring_view substr(size_type pos) const {
		return impl.substr(pos);
	}

	constexpr impl_type substr(size_type pos, size_type n) const {
		return impl.substr(pos, n);
	}

	constexpr int compare(impl_type o) const noexcept { return impl.compare(o); }
	constexpr int compare(size_type pos1, size_type n1, basic_zstring_view s) const {
		return impl.compare(pos1, n1, s);
	}
	constexpr int compare(
		size_type pos1, size_type n1, basic_zstring_view s,
		size_type pos2, size_type n2) const
	{
		impl.compare(pos1, n1, s, pos2, n2);
	}
	constexpr int compare(const value_type* s) const { return impl.compare(s); }
	constexpr int compare(size_type pos1, size_type n1, const value_type* s) const { return impl.compare(pos1, n1, s); }
	constexpr int compare(size_type pos1, size_type n1, const value_type* s, size_type n2) const {
		return impl.compare(pos1, n1, s, n2);
	}

	constexpr bool starts_with(impl_type o) const noexcept { return impl.starts_with(o); }
	constexpr bool starts_with(value_type c) const noexcept { return impl.starts_with(c); }
	constexpr bool starts_with(const value_type* s) const { return impl.starts_with(s); }
	constexpr bool ends_with(impl_type o) const noexcept { return impl.ends_with(o); }
	constexpr bool ends_with(value_type c) const noexcept { return impl.ends_with(c); };
	constexpr bool ends_with(const value_type* s) const { return impl.ends_with(s); }

	constexpr bool contains(impl_type o) const noexcept { return impl.contains(o); }
	constexpr bool contains(value_type c) const noexcept { return impl.contains(c); }
	constexpr bool contains(const value_type* s) const { return impl.contains(s); }

	constexpr size_type find(impl_type o, size_type pos = 0) const noexcept {
		return impl.find(o, pos);
	}
	constexpr size_type find(value_type c, size_type pos = 0) const noexcept {
		return impl.find(c, pos);
	}
	constexpr size_type find(const value_type* s, size_type pos, size_type n) const {
		return impl.find(s, pos, n);
	}
	constexpr size_type find(const value_type* s, size_type pos = 0) const {
		return impl.find(s, pos);
	}

	constexpr size_type rfind(impl_type o, size_type pos = npos) const noexcept {
		return impl.rfind(o, pos);
	}
	constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept {
		return impl.rfind(c, pos);
	}
	constexpr size_type rfind(const value_type* s, size_type pos, size_type n) const {
		return impl.rfind(s, pos, n);
	}
	constexpr size_type rfind(const value_type* s, size_type pos = npos) const {
		return impl.rfind(s, pos);
	}

	constexpr size_type find_first_of(impl_type o, size_type pos = 0) const noexcept {
		return impl.find_first_of(o, pos);
	}
	constexpr size_type find_first_of(value_type c, size_type pos = 0) const noexcept {
		return impl.find_first_of(c, pos);
	}
	constexpr size_type find_first_of(const value_type* s, size_type pos, size_type n) const {
		return impl.find_first_of(s, pos, n);
	}
	constexpr size_type find_first_of(const value_type* s, size_type pos = 0) const {
		return impl.find_first_of(s, pos);
	}

	constexpr size_type find_last_of(impl_type o, size_type pos = npos) const noexcept {
		return impl.find_last_of(o, pos);
	}
	constexpr size_type find_last_of(value_type c, size_type pos = npos) const noexcept {
		return impl.find_last_of(c, pos);
	}
	constexpr size_type find_last_of(const value_type* s, size_type pos, size_type n) const {
		return impl.find_last_of(s, pos, n);
	}
	constexpr size_type find_last_of(const value_type* s, size_type pos = npos) const {
		return impl.find_last_of(s, pos);
	}

	constexpr size_type find_first_not_of(impl_type o, size_type pos = 0) const noexcept {
		return impl.find_first_not_of(o, pos);
	}
	constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept {
		return impl.find_first_not_of(c, pos);
	}
	constexpr size_type find_first_not_of(const value_type* s, size_type pos, size_type n) const {
		return impl.find_first_not_of(s, pos, n);
	}
	constexpr size_type find_first_not_of(const value_type* s, size_type pos = 0) const {
		return impl.find_first_not_of(s, pos);
	}

	constexpr size_type find_last_not_of(impl_type o, size_type pos = npos) const noexcept {
		return impl.find_last_not_of(o, pos);
	}
	constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept {
		return impl.find_last_not_of(c, pos);
	}
	constexpr size_type find_last_not_of(const value_type* s, size_type pos, size_type n) const {
		return impl.find_last_not_of(s, pos, n);
	}
	constexpr size_type find_last_not_of(const value_type* s, size_type pos = npos) const {
		return impl.find_last_not_of(s, pos);
	}
};

using zstring_view = basic_zstring_view<char>;
using wzstring_view = basic_zstring_view<wchar_t>;
using u8zstring_view = basic_zstring_view<char8_t>;
using u16zstring_view = basic_zstring_view<char16_t>;
using u32zstring_view = basic_zstring_view<char32_t>;


inline namespace literals {
inline namespace zstring_view_literals {

constexpr zstring_view operator""_zv(const char* str, std::size_t len) noexcept {
	return zstring_view(zstring_view::null_terminated, str, len);
}

constexpr wzstring_view operator""_zv(const wchar_t* str, std::size_t len) noexcept {
	return wzstring_view(wzstring_view::null_terminated, str, len);
}

constexpr u8zstring_view operator""_zv(const char8_t* str, std::size_t len) noexcept {
	return u8zstring_view(u8zstring_view::null_terminated, str, len);
}

constexpr u16zstring_view operator""_zv(const char16_t* str, std::size_t len) noexcept {
	return u16zstring_view(u16zstring_view::null_terminated, str, len);
}

constexpr u32zstring_view operator""_zv(const char32_t* str, std::size_t len) noexcept {
	return u32zstring_view(u32zstring_view::null_terminated, str, len);
}
} // zstring_view_literals
} // literals
} // ctp

template <typename CharT, typename Traits>
struct std::hash<ctp::basic_zstring_view<CharT, Traits>> {
	[[nodiscard]] std::size_t operator()(ctp::basic_zstring_view<CharT, Traits> s) const noexcept {
		return hash<basic_string_view<CharT, Traits>>{}(s);
	}
};

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
	std::basic_ostream<CharT, Traits>& os,
	const ctp::basic_zstring_view<CharT, Traits>& str)
{
	return os << static_cast<std::basic_string_view<CharT, Traits>>(str);
}

#endif // INCLUDE_CTP_TOOLS_ZSTRING_VIEW_HPP
