#ifndef INCLUDE_CTP_ENUM_REFLECTION_HPP
#define INCLUDE_CTP_ENUM_REFLECTION_HPP

#include "zstring_view.hpp"
#include "type_traits.hpp"

#include <array>
#include <optional>

// Entry point to magic_enum.
// Should only inclue this file, not magic_enum directly.

// By default I suspect I will not use the negative range much.
#define MAGIC_ENUM_RANGE_MIN -32
#define MAGIC_ENUM_RANGE_MAX 128

// Do not need iostreams support.
#define MAGIC_ENUM_NO_STREAMS 1
// Do not need magic_enum's enum flag support for bitwise operators.
#define MAGIC_ENUM_NO_CHECK_FLAGS 1

namespace poison {
// Attempt to contain magic_enum usage to just this header.
#include <magic_enum.hpp>
} // poison

// Sets min/max vals for enum reflection detection. Use in global scope.
#define CTP_CUSTOM_ENUM_MIN_MAX(EnumName, MinVal, MaxVal)\
template <> \
struct poison::magic_enum::customize::enum_range<EnumName> { \
	static constexpr int min = MinVal; \
	static constexpr int max = MaxVal; \
};

namespace ctp::enums {
template <class E>
concept Enum = std::is_enum_v<E>;

template <Enum E>
struct enum_traits {
	// specialize and override this for your enum if you want
	// to force use a lookup table.
	// Still only applies if the enum is not contiguous.
	static constexpr bool force_enable_lookup_table = false;
};

template <Enum E>
using HasForceEnableLookupSetting = decltype(enum_traits<E>::force_enable_lookup_table);

// Name of the enum class.
template <Enum E>
constexpr zstring_view type_name() noexcept {
	return zstring_view{zstring_view::null_terminated_tag{}, poison::magic_enum::enum_type_name<E>()};
}

// Name of an enum value. Empty string if not found.
template <Enum E>
constexpr zstring_view name(E e) noexcept {
	return zstring_view{zstring_view::null_terminated_tag{}, poison::magic_enum::enum_name<E>(e)};
}

// Number of distinct named values.
template <Enum E>
constexpr std::size_t size() noexcept {
	return poison::magic_enum::enum_count<E>();
}

// Get a std::array of all distinct named values in the enum; ascending order.
template <Enum E>
constexpr auto values() noexcept -> decltype(poison::magic_enum::enum_values<E>()) {
	return poison::magic_enum::enum_values<E>();
}

// Get the index of an enum member into the values or names array.
template <Enum auto E>
constexpr std::size_t index() noexcept {
	return poison::magic_enum::enum_index<E>();
}

namespace detail {
// How large the values() array can get before we consider making
// a lookup table. For small tabels, linear search is fine.
// This value has not been tuned and should not be trusted.
inline constexpr std::size_t EnumIndexLookupTableMin = 7;

// We want to use a lookup table when the values are close to contiguous,
// but have some gaps. The density is how much of the lookup table would
// be populated with useful values rather than waste.
inline constexpr float MinDensityForLookupTable = 0.6f;

template <Enum E>
constexpr bool is_lookup_table_enabled() {
	constexpr bool IsTooBig = size<E>() > 255;
	if constexpr (is_detected_convertible_v<bool, HasForceEnableLookupSetting, E>) {
		if constexpr (enum_traits<E>::force_enable_lookup_table) {
			static_assert(!IsTooBig,
				"This enum has too many entries to use a lookup table. Disable force_enable_lookup_table.");
			return true;
		}
	}

	if constexpr (IsTooBig)
		return false;

	// too small
	if constexpr (size<E>() < detail::EnumIndexLookupTableMin)
		return false;

	// density check
	constexpr auto Range = std::to_underlying(max_val<E>()) - std::to_underlying(min_val<E>());
	// Max density for non contiguous arrays approaches 1.
	// This value represents how much of the lookup table would be empty.
	constexpr float Density = size<E>() / static_cast<float>(Range);
	if constexpr (Density < MinDensityForLookupTable) {
		return false;
	}
	return true;
}

template <Enum E>
inline constexpr bool is_lookup_table_enabled_v = is_lookup_table_enabled<E>();

// Make an array lookup table to map enum values to their index in the value array.
// This is intended to give non-contiguous, medium-sized enums fast lookup time.
// Note it is not benchmarked so this is merely an assumption and should not be trusted.
template <Enum E>
inline constexpr std::array enum_to_index_lookup_table = [] {
	constexpr auto Minimum = std::to_underlying(min_val<E>());
	constexpr std::size_t Entries = std::to_underlying(max_val<E>()) - Minimum + 1;
	static_assert(Entries <= 255);

	// By zero initializing, "bad" entries within range will default to the min val.
	std::array<std::uint8_t, Entries> arr{};

	const auto& vals = values<E>();
	for (std::size_t i = 0; i < vals.size(); ++i) {
		const auto valIdx = std::to_underlying(vals[i]) - Minimum;
		arr[static_cast<std::size_t>(valIdx)] = static_cast<std::uint8_t>(i);
	}
	return arr;
}();
} // detail

// Get the index of an enum member into the values or names array.
// Value must exist in values().
template <Enum E>
constexpr std::size_t index(E e) noexcept {
	constexpr auto Minimum = std::to_underlying(min_val<E>());

	const auto val = std::to_underlying(e);
	if constexpr (poison::magic_enum::detail::is_sparse_v<E>) {
		const auto& vals = values<E>();

		if constexpr (detail::is_lookup_table_enabled_v<E>)
		{
			const auto lookupIdx = static_cast<std::size_t>(val - Minimum);
			return detail::enum_to_index_lookup_table<E>[lookupIdx];
		} else {
			// Due to the limitations in magic_enum, I don't expect to have huge
			// enums where binary search would be a meaningful benefit, and since
			// the values array is integers linear search should be fast.
			// Can still cut the initial search range in half to help.

			const std::size_t midIdx = vals.size() / 2;
			const auto mid = vals[midIdx];
			if (mid <= e) {
				for (std::size_t i = midIdx; i < vals.size(); ++i) {
					if (vals[i] == e)
						return i;
				}
			}

			for (std::size_t i = midIdx; i > 0; --i) {
				if (vals[i - 1] == e)
					return i - 1;
			}

			// not found
			std::terminate();
		}
	} else {
		return static_cast<std::size_t>(val - Minimum);
	}
}

// Get the index of an enum member into the values or names array.
template <Enum E>
constexpr std::optional<std::size_t> try_get_index(E e) noexcept {
	return poison::magic_enum::enum_index<E>(e);
}

template <Enum E>
constexpr E min_val() noexcept {
	static_assert(size<E>() > 0);
	return values<E>().front();
}

template <Enum E>
constexpr E max_val() noexcept {
	static_assert(size<E>() > 0);
	return values<E>().back();
}

namespace detail {
template <Enum E>
static constexpr auto zstring_names = [] {
	auto stringViewNames = poison::magic_enum::enum_names<E>();
	std::array<zstring_view, size<E>()> converted;
	for (std::size_t i = 0; i < size<E>(); ++i)
		converted[i] = zstring_view{zstring_view::null_terminated_tag{}, stringViewNames[i]};
	return converted;
}();
} // detail

// Get a std::array<zstring_view> of all distinct named value names in
// the enum; ascending value order.
template <Enum E>
constexpr auto names() noexcept -> const decltype(detail::zstring_names<E>)& {
	return detail::zstring_names<E>;
}

// Try to cast an integer into a named entry of the enum.
template <Enum E>
constexpr std::optional<E> try_cast(std::underlying_type_t<E> value) noexcept {
	return poison::magic_enum::enum_cast<E>(value);
}

// Try to cast a string into a named entry of the enum.
template <Enum E>
constexpr std::optional<E> try_cast(std::string_view name) noexcept {
	return poison::magic_enum::enum_cast<E>(name);
}

// Try to cast a case insensitive string into a named entry of the enum.
template <Enum E>
constexpr std::optional<E> try_cast_icase(std::string_view name) noexcept {
	return poison::magic_enum::enum_cast<E>(name, poison::magic_enum::case_insensitive);
}

} // ctp::enums

#endif // INCLUDE_CTP_ENUM_REFLECTION_HPP
