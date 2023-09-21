#ifndef INCLUDE_CTP_TOOLS_CHARCONV_HPP
#define INCLUDE_CTP_TOOLS_CHARCONV_HPP

#include "config.hpp"

#include <array>
#include <charconv>
#include <concepts>
#include <string_view>

namespace ctp {

// Get the maximum number of chars needed to hold an integral type in base 10 without data loss.
template <std::integral I>
struct max_char_digits_10 {
	static constexpr int value = std::numeric_limits<I>::digits10 + 1 + std::numeric_limits<I>::is_signed;
};

template <std::integral I>
static constexpr auto max_char_digits_10_v = max_char_digits_10<I>::value;

template <typename U, typename I>
concept IsToCharsCompatible = 
	std::integral<I> && std::integral<U> &&
	!std::is_same_v<I, bool> && !std::is_same_v<U, bool> &&
	max_char_digits_10_v<U> <= max_char_digits_10_v<I>;

template <std::integral I>
struct ToCharsConverter {
	std::array<char, max_char_digits_10_v<I>> mem;
	std::size_t size;
	constexpr ToCharsConverter() noexcept = default; // Value-initialize for use in constant expressions.
	constexpr ToCharsConverter(I i) noexcept {
		if CTP_IS_CONSTEVAL {
			mem = {};
		}
		convert(i);
	}

	template <IsToCharsCompatible<I> U>
	constexpr void convert(U i) noexcept {
		// We guarantee enough space.
		size = std::to_chars(mem.data(), mem.data() + mem.size(), i).ptr - mem.data();
	}

	template <IsToCharsCompatible<I> U>
	constexpr std::string_view operator()(U i) noexcept {
		convert(i);
		return GetView();
	}

	[[nodiscard]] constexpr std::string_view GetView() const noexcept { return std::string_view{mem.data(), size}; }
	constexpr operator std::string_view() const noexcept { return GetView(); }
};

namespace detail {
// From tests, this only helps MSVC.
template <std::size_t ReducedSize, std::size_t FullSize>
consteval auto ShrinkToFit(const std::array<char, FullSize>& arr) {
	std::array<char, ReducedSize> out{};
	std::copy_n(arr.begin(), ReducedSize, out.data());
	return out;
}
} // detail

template <std::integral auto num>
[[nodiscard]] consteval auto ToConstantString() {
	constexpr ToCharsConverter converter{num};
	return detail::ShrinkToFit<converter.size>(converter.mem);
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_CHARCONV_HPP
