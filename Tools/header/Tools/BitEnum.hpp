#ifndef INCLUDE_CTP_TOOLS_BIT_ENUM_HPP
#define INCLUDE_CTP_TOOLS_BIT_ENUM_HPP

#include "type_traits.hpp"

namespace ctp {

// Wrapper class for bitfield-like enums to enable bit ops.
template <typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
struct BitEnum {
	using type = Enum;
	using utype = std::underlying_type_t<Enum>;

	// Some helpers. BitEnums should be able to work with the underlying Enum type, or another BitEnum of the same type.
	template <typename Op> struct is_bit_enum_convertible : std::disjunction<std::is_convertible<Op, Enum>, std::is_same<Op, BitEnum<Enum>>> {};
	template <typename... Ops> struct is_bit_enum_ops : std::conjunction<is_bit_enum_convertible<Ops>...> {};
	template <typename... Ops> static constexpr bool is_bit_enum_ops_v = is_bit_enum_ops<Ops...>::value;

	Enum val = static_cast<Enum>(0);

	constexpr BitEnum() noexcept = default;
	constexpr BitEnum(const BitEnum&) noexcept = default;
	constexpr BitEnum(BitEnum&&) noexcept = default;
	constexpr BitEnum& operator=(const BitEnum&) noexcept = default;
	constexpr BitEnum& operator=(BitEnum&&) noexcept = default;

	template <typename... Ops, std::enable_if_t<is_bit_enum_ops_v<Ops...>, int> = 0>
	constexpr BitEnum(Ops... ops) noexcept : val{static_cast<Enum>((static_cast<utype>(ops) | ...))} {}

	[[nodiscard]] constexpr Enum value() const noexcept { return val; }
	[[nodiscard]] constexpr utype underlying() const noexcept { return static_cast<utype>(val); }
	constexpr explicit operator Enum() const noexcept { return val; }
	constexpr explicit operator utype() const noexcept { return static_cast<utype>(val); }

	constexpr bool operator==(BitEnum o) const noexcept { return val == o.val; }
	constexpr bool operator!=(BitEnum o) const noexcept { return val != o.val; }

	constexpr BitEnum operator|(BitEnum e) const noexcept { return static_cast<Enum>(static_cast<utype>(val) | static_cast<utype>(e)); }
	constexpr BitEnum& operator|=(BitEnum e) noexcept { return *this = *this | e; }
	constexpr BitEnum operator&(BitEnum e) const noexcept { return static_cast<Enum>(static_cast<utype>(val) & static_cast<utype>(e)); }
	constexpr BitEnum& operator&=(BitEnum e) noexcept { return *this = *this & e; }
	constexpr BitEnum operator^(BitEnum e) const noexcept { return static_cast<Enum>(static_cast<utype>(val) ^ static_cast<utype>(e)); }
	constexpr BitEnum& operator^=(BitEnum e) noexcept { return *this = *this ^ e; }
	[[nodiscard]] constexpr BitEnum operator~() const noexcept { return static_cast<Enum>(~static_cast<utype>(val)); }

	template <typename... Ops> struct is_non_empty_bit_ops : std::conjunction<pack_not_empty<Ops...>, is_bit_enum_ops<Ops...>> {};
	template <typename... Ops> static constexpr bool is_non_empty_bit_ops_v = is_non_empty_bit_ops<Ops...>::value;

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr BitEnum& set(Ops... ops) noexcept { return *this |= BitEnum{ops...}; }

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr BitEnum& unset(Ops... ops) noexcept { return *this &= ~BitEnum{ops...}; }

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr BitEnum& flip(Ops... ops) noexcept { return *this ^= BitEnum{ops...}; }

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr bool any_of(Ops... ops) const noexcept
	{
		return (*this & BitEnum{ops...}).val != static_cast<Enum>(0);
	}

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr bool all_of(Ops... ops) const noexcept
	{
		const BitEnum expected{ops...};
		return (*this & expected) == expected;
	}

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr bool none_of(Ops... ops) const noexcept
	{
		return !any_of(ops...);
	}

	template <typename... Ops, std::enable_if_t<is_non_empty_bit_ops_v<Ops...>, int> = 0>
	constexpr bool exactly(Ops... ops) const noexcept
	{
		return *this == BitEnum{ops...};
	}
};
template <typename... Ops> BitEnum(Ops...) -> BitEnum<typename first_element<Ops...>::type>;

} // namespace ctp

#endif // INCLUDE_CTP_TOOLS_BIT_ENUM_HPP
