#ifndef INCLUDE_CTP_TOOLS_STRONG_TYPE_HPP
#define INCLUDE_CTP_TOOLS_STRONG_TYPE_HPP

#include "CrtpHelper.hpp"
#include "type_traits.hpp"

namespace ctp {

// StrongTypes can be used to prevent accidental interop of one StrongType with another StrongType,
// or to automatically perform the proper conversion from one type to another if one is provided.

// StrongTypes can opt-in to an implicit conversion to their underlying value type via
// ImplicitlyConvertible. This is mainly for ease of use with interop between StrongTypes and STL.
// However, these implicit conversions would normally enable two different StrongTypes to interop
// with each other by converting to their underlying type (e.g. by using int == int rather than
// StrongType1 == StrongType2). To avoid this, StrongType operators are templates that do not use
// SFINAE, but instead static_asserts internally, so that the compiler cannot default back to
// arithmetic operations with implicit conversions. It is still possible if unrecommended to
// overload or specialize them.

// There is no mechanism to detect types that are implicitly convertible to a type that
// would otherwise be valid, since such a use-case seemed questionable.

// Allow another StrongType to act as the right-hand argument to operations with a StrongType by
// declaring:
// using operable_with = RightHandType;
// in LeftHandStrongType's class definition, or several types with:
// using operable_with = operable_with_types<T1, T2, ..., Tn>;
// Will convert to the left-hand StrongType via castTo.

template <typename... StrongTypes>
struct operable_with_types {};

// Provide a method to convert from one StrongType to another by definiing a static convert
// function. This will be used in castTo, which is used any time one StrongType is converted to
// another StrongType.
template <typename To, typename From> using has_convert = decltype(To::convert(std::declval<From>()));

// StrongType
// Example usage, creating a summable strong type that can operate with other signed integral types:
// struct MyStrongType : StrongType<MyStrongType, int>, Summable, ValueOperable<> { using StrongType::StrongType, StrongType::operator=; };

template <typename Derived, typename T, typename ConstructFrom = Derived>
struct StrongType;

// Define valid(), operator bool, and invalidate.
// Requires specifying a static invalid_value member in the class.
struct InvalidValue {};

// Define operators: ++T, T++, --T, T--.
struct Incrementable {};

// Define operators: +, +=, -, -=.
struct Summable {};

// Define operators: *, *=, /, /=, %, %=.
struct Multipliable {};

// Define all arithmetic operations.
struct Arithmetic : Incrementable, Summable, Multipliable {};

struct strictness_exact_match {}; // Arithmetic types must match exactly to the value_type.
struct strictness_similar {}; // Arithmetic types must match signedness and wether floating-point or integral with the value_type.
struct strictness_relaxed {}; // Arithmetic types must match floating-point or integral with the value_type.

// Allow an arithmetic value_type StrongType to interop normally as the left-hand argument with
// arithmetic types for any specified operations.
template <typename Strictness = strictness_relaxed>
struct ValueOperable {
	static_assert(std::disjunction_v<
		std::is_same<Strictness, strictness_exact_match>,
		std::is_same<Strictness, strictness_similar>,
		std::is_same<Strictness, strictness_relaxed>>, "Use one of the provided types above.");
};

// Arithmetic and ValueOperable.
template <typename Strictness = strictness_relaxed>
struct ValueArithmetic : Arithmetic, ValueOperable<Strictness> {};

// Can implicitly convert to the value_type. Useful for index types wtih STL.
// Needs to use CRTP to work with operator[] properly, otherwise is mixed in the same as the rest.
template <typename Derived, typename ConstructFrom = Derived>
struct ImplicitlyConvertible;

// An IndexType is a StrongType with all arithmetic operations enabled, as well as value operations,
// and is implicitly convertible to the value_type.
template <typename Derived, typename T, typename Strictness = strictness_relaxed, typename ConstructFrom = Derived>
struct IndexType :
	StrongType<Derived, T, IndexType<Derived, T, Strictness, ConstructFrom>>,
	ValueArithmetic<Strictness>,
	ImplicitlyConvertible<Derived, IndexType<Derived, T, Strictness, ConstructFrom>> {

	using Base = StrongType<Derived, T, IndexType<Derived, T, Strictness, ConstructFrom>>;
	using Base::Base, Base::operator=;

private:
	friend ConstructFrom;
	IndexType() = default;
	~IndexType() = default;
};

namespace detail {
template <typename Derived, typename MaybeStrongType, typename R = typename Derived::value_type>
constexpr R getCompatibleValue(const MaybeStrongType& maybeStrongType) noexcept;

template <typename To, typename From, typename = void>
struct can_operate_with : std::false_type {};

template <typename To, typename From>
struct can_operate_with<To, From, typename std::void_t<typename To::operable_with>> {
	static constexpr bool value = std::disjunction_v<
		std::is_same<typename To::operable_with, From>,
		template_typelist_contains_type<From, typename To::operable_with>>;
};

template <typename To, typename From>
static constexpr bool can_operate_with_v = can_operate_with<To, From>::value;

template <typename T> struct is_strong_type : has_base_template<T, StrongType> {};
template <typename T> static constexpr bool is_strong_type_v = is_strong_type<T>::value;
} // detail

// For eye-catching compiler errors, since it may fail a few levels deep. undef'd at file end.
#define CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER \
	"\n^^^^^ static_assert failed ^^^^^\n"\
	"**********************************\n"\
	"*\n StrongType invalid operation error\n*\n"
#define CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER \
	"\n*\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"

// StrongTypes disable implicit comparisons between different StrongTypes.
template <typename Derived, typename T, typename ConstructFrom /*= Derived*/>
struct StrongType : CrtpHelper<Derived, StrongType<Derived, T, ConstructFrom>> {
	using value_type = T;

	value_type value;

	// Constructors.

	explicit constexpr StrongType(const value_type& v) noexcept : value{v} {}

	constexpr StrongType(const Derived& d) noexcept : value{d.value} {}
	constexpr StrongType(Derived&& d) noexcept : value{static_cast<Derived&&>(d).value} {}

	template <typename OtherStrongType,
		std::enable_if_t<std::conjunction_v<
			detail::is_strong_type<OtherStrongType>,
			detail::can_operate_with<Derived, std::decay_t<OtherStrongType>>>, int> = 0>
	constexpr StrongType(const OtherStrongType& o) noexcept : value{o.template castTo<Derived>().value} {}

	template <typename Rhs>
	constexpr Derived& operator=(const Rhs& rhs) noexcept
	{
		value = detail::getCompatibleValue<Derived>(rhs);
		return this->derived();
	}

	// Conversions.

	template <typename D = Derived, std::enable_if_t<!has_base_template_v<D, ImplicitlyConvertible>, int> = 0>
	explicit constexpr operator value_type() const noexcept { return value; }

	template <typename U, std::enable_if_t<!detail::is_strong_type_v<U>, int> = 0>
	explicit constexpr operator U() const noexcept { return static_cast<U>(value); }

	// Cast from a StrongType to another type.
	// Will use T::convert(this) if it exists and the other type is a StrongType.
	template <typename U>
	[[nodiscard]] constexpr U castTo() const noexcept
	{
		if constexpr (std::conjunction_v<detail::is_strong_type<U>, is_detected<has_convert, U, Derived>>)
			return U::convert(this->derived());
		else
			return static_cast<U>(value);
	}

	// Comparisons.

	template <typename Rhs>
	friend constexpr bool operator==(const Derived& lhs, const Rhs& rhs) noexcept
	{
		return lhs.value == detail::getCompatibleValue<Derived>(rhs);
	}
	template <typename Lhs, std::enable_if_t<!detail::is_strong_type_v<Lhs>, int> = 0>
	friend constexpr bool operator==(const Lhs& lhs, const Derived& rhs) noexcept
	{
		return rhs == lhs;
	}

	template <typename Rhs>
	friend constexpr bool operator!=(const Derived& lhs, const Rhs& rhs) noexcept
	{
		return lhs.value != detail::getCompatibleValue<Derived>(rhs);
	}
	template <typename Lhs, std::enable_if_t<!detail::is_strong_type_v<Lhs>, int> = 0>
	friend constexpr bool operator!=(const Lhs& lhs, const Derived& rhs) noexcept
	{
		return rhs != lhs;
	}

	template <typename Rhs>
	friend constexpr bool operator<(const Derived& lhs, const Rhs& rhs) noexcept
	{
		return lhs.value < detail::getCompatibleValue<Derived>(rhs);
	}
	template <typename Lhs, std::enable_if_t<!detail::is_strong_type_v<Lhs>, int> = 0>
	friend constexpr bool operator<(const Lhs& lhs, const Derived& rhs) noexcept
	{
		return rhs.value > lhs;
	}

	template <typename Rhs>
	friend constexpr bool operator>(const Derived& lhs, const Rhs& rhs) noexcept
	{
		return lhs.value > detail::getCompatibleValue<Derived>(rhs);
	}
	template <typename Lhs, std::enable_if_t<!detail::is_strong_type_v<Lhs>, int> = 0>
	friend constexpr bool operator>(const Lhs& lhs, const Derived& rhs) noexcept
	{
		return rhs.value < lhs;
	}

	template <typename Rhs>
	friend constexpr bool operator<=(const Derived& lhs, const Rhs& rhs) noexcept
	{
		return lhs.value <= detail::getCompatibleValue<Derived>(rhs);
	}
	template <typename Lhs, std::enable_if_t<!detail::is_strong_type_v<Lhs>, int> = 0>
	friend constexpr bool operator<=(const Lhs& lhs, const Derived rhs) noexcept
	{
		return rhs.value >= lhs;
	}

	template <typename Rhs>
	friend constexpr bool operator>=(const Derived& lhs, const Rhs& rhs) noexcept
	{
		return lhs.value >= detail::getCompatibleValue<Derived>(rhs);
	}
	template <typename Lhs, std::enable_if_t<!detail::is_strong_type_v<Lhs>, int> = 0>
	friend constexpr bool operator>=(const Lhs& lhs, const Derived rhs) noexcept
	{
		return rhs.value <= lhs;
	}

	// Invalid value operations.

	template <typename U> using has_invalid_value = decltype(&U::invalid_value);

	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<InvalidValue, D>, int> = 0>
	constexpr bool valid() const noexcept
	{
		static_assert(is_detected_v<has_invalid_value, Derived>,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"The derived class of InvalidValue must specify the static member invalid_value to compare against."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
		static_assert(std::is_same_v<typename Derived::value_type, std::decay_t<decltype(Derived::invalid_value)>>,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"invalid_value should be the same type as StrongType's value_type."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);

		return value != Derived::invalid_value;
	}
	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<InvalidValue, D>, int> = 0>
	explicit constexpr operator bool() const noexcept { return valid(); }

	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<InvalidValue, D>, int> = 0>
	constexpr void invalidate() noexcept { this->derived().value = Derived::invalid_value; }

	// Incrementable operations.

	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<Incrementable, D>, int> = 0>
	constexpr Derived& operator++() noexcept { ++value; return this->derived(); }
	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<Incrementable, D>, int> = 0>
	constexpr Derived operator++(int) noexcept
	{
		const Derived temp{value};
		value++;
		return temp;
	}
	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<Incrementable, D>, int> = 0>
	constexpr Derived& operator--() noexcept { --value; return this->derived(); }
	template <typename D = Derived, std::enable_if_t<std::is_base_of_v<Incrementable, D>, int> = 0>
	constexpr Derived operator--(int) noexcept
	{
		const Derived temp{value};
		value--;
		return temp;
	}

	// Summable operators.

	template <typename Rhs, typename D = Derived, std::enable_if_t<std::is_base_of_v<Summable, D>, int> = 0>
	constexpr Derived& operator+=(const Rhs& rhs) noexcept
	{
		value += detail::getCompatibleValue<Derived>(rhs);
		return this->derived();
	}
	template <typename Rhs, typename D = Derived, std::enable_if_t<std::is_base_of_v<Summable, D>, int> = 0>
	constexpr Derived& operator-=(const Rhs& rhs) noexcept
	{
		value -= detail::getCompatibleValue<Derived>(rhs);
		return this->derived();
	}

	// Multipliable operators.

	template <typename Rhs, typename D = Derived, std::enable_if_t<std::is_base_of_v<Multipliable, D>, int> = 0>
	constexpr Derived& operator*=(const Rhs& rhs) noexcept
	{
		value *= detail::getCompatibleValue<Derived>(rhs);
		return this->derived();
	}
	template <typename Rhs, typename D = Derived, std::enable_if_t<std::is_base_of_v<Multipliable, D>, int> = 0>
	constexpr Derived& operator/=(const Rhs& rhs) noexcept
	{
		value /= detail::getCompatibleValue<Derived>(rhs);
		return this->derived();
	}
	template <typename Rhs, typename D = Derived, std::enable_if_t<std::is_base_of_v<Multipliable, D>, int> = 0>
	constexpr Derived& operator%=(const Rhs& rhs) noexcept
	{
		value %= detail::getCompatibleValue<Derived>(rhs);
		return this->derived();
	}

private:
	friend ConstructFrom;
	StrongType() = default;
	~StrongType() = default;

	// If you get a compiler error here, it's because you are attempting to use an operation your
	// StrongType does not support.

	template <typename OtherStrongType,
		std::enable_if_t<std::conjunction_v<
			detail::is_strong_type<OtherStrongType>,
			std::negation<detail::can_operate_with<Derived, std::decay_t<OtherStrongType>>>>, int> = 0>
	constexpr StrongType(const OtherStrongType&) noexcept
	{
		static_assert(always_false<OtherStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Attempting to construct or cast one StrongType to another. Declare operable_with, or use type.castTo<Other>()."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	}

	// Operator bool comes with InvalidValue.
	template <typename D = Derived, std::enable_if_t<!std::is_base_of_v<InvalidValue, D>, int> = 0>
	explicit constexpr operator bool() const noexcept = delete;
};

template <typename Derived, typename ConstructFrom /*= Derived*/>
struct ImplicitlyConvertible : CrtpHelper<Derived, ImplicitlyConvertible<Derived, ConstructFrom>> {
	constexpr operator auto() const noexcept { return this->derived().value; }

private:
	friend ConstructFrom;
	ImplicitlyConvertible() = default;
	~ImplicitlyConvertible() = default;
};

namespace detail {
template <typename T, typename Operation>
struct is_operable : std::conjunction<is_strong_type<T>, std::is_base_of<Operation, T>> {};
template <typename T, typename Operation>
struct is_not_operable : std::conjunction<is_strong_type<T>, std::negation<std::is_base_of<Operation, T>>> {};

template <typename T> static constexpr bool is_summable_v = is_operable<T, Summable>::value;
template <typename T> static constexpr bool is_not_summable_v = is_not_operable<T, Summable>::value;

template <typename T> static constexpr bool is_multipliable_v = is_operable<T, Multipliable>::value;
template <typename T> static constexpr bool is_not_multipliable_v = is_not_operable<T, Multipliable>::value;

template <typename Lhs, typename Rhs>
struct non_strong_type_left_param : std::conjunction<std::negation<is_strong_type<Lhs>>, is_strong_type<Rhs>> {};
template <typename Lhs, typename Rhs>
static constexpr bool non_strong_type_left_param_v = non_strong_type_left_param<Lhs, Rhs>::value;
} // detail

template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_summable_v<Lhs>, int> = 0>
constexpr Lhs operator+(const Lhs& lhs, const Rhs& rhs) noexcept
{
	return Lhs(lhs.value + detail::getCompatibleValue<Lhs>(rhs));
}
template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_summable_v<Lhs>, int> = 0>
constexpr Lhs operator-(const Lhs& lhs, const Rhs& rhs) noexcept
{
	return Lhs(lhs.value - detail::getCompatibleValue<Lhs>(rhs));
}

template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_multipliable_v<Lhs>, int> = 0>
constexpr Lhs operator*(const Lhs& lhs, const Rhs& rhs) noexcept
{
	return Lhs(lhs.value * detail::getCompatibleValue<Lhs>(rhs));
}
template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_multipliable_v<Lhs>, int> = 0>
constexpr Lhs operator/(const Lhs& lhs, const Rhs& rhs) noexcept
{
	return Lhs(lhs.value / detail::getCompatibleValue<Lhs>(rhs));
}
template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_multipliable_v<Lhs>, int> = 0>
constexpr Lhs operator%(const Lhs& lhs, const Rhs& rhs) noexcept
{
	return Lhs(lhs.value % detail::getCompatibleValue<Lhs>(rhs));
}

// If you get a compiler error here, it's because you are attempting to use an operation your StrongType does not support.

template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_not_summable_v<Lhs>, int> = 0>
constexpr void operator+(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_not_summable_v<Lhs>, int> = 0>
constexpr void operator-(const Lhs&, const Rhs&) noexcept = delete;

template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_not_multipliable_v<Lhs>, int> = 0>
constexpr void operator*(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_not_multipliable_v<Lhs>, int> = 0>
constexpr void operator/(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::is_not_multipliable_v<Lhs>, int> = 0>
constexpr void operator%(const Lhs&, const Rhs&) noexcept = delete;

// Disallow arithmetic operations with the StrongType as the right-hand argument and a non-StrongType as the left-hand.
template <typename Lhs, typename Rhs, std::enable_if_t<detail::non_strong_type_left_param_v<Lhs, Rhs>, int> = 0>
constexpr void operator+(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::non_strong_type_left_param_v<Lhs, Rhs>, int> = 0>
constexpr void operator-(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::non_strong_type_left_param_v<Lhs, Rhs>, int> = 0>
constexpr void operator*(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::non_strong_type_left_param_v<Lhs, Rhs>, int> = 0>
constexpr void operator/(const Lhs&, const Rhs&) noexcept = delete;
template <typename Lhs, typename Rhs, std::enable_if_t<detail::non_strong_type_left_param_v<Lhs, Rhs>, int> = 0>
constexpr void operator%(const Lhs&, const Rhs&) noexcept = delete;


namespace detail {
// Tags for the different cases so the logic is testable without trigging static_asserts.
struct same_type_conversion : std::true_type {};
struct same_value_type_conversion : std::true_type {};
struct implicit_strong_type_conversion : std::true_type {};
struct arithmetic_type_conversion : std::true_type {};
struct invalid_strong_type_conversion : std::false_type {};
struct not_arithmetic : std::false_type {};
struct not_value_operable : std::false_type {};
struct invalid_arithmetic_type_conversion_exact : std::false_type {};
struct invalid_arithmetic_type_conversion_similar : std::false_type {};
struct invalid_arithmetic_type_conversion_relaxed : std::false_type {};
struct unknown_conversion : std::false_type {};

template <typename T, typename U> struct is_arithmetic_relaxed :
	std::negation<std::disjunction<
	std::conjunction<std::is_integral<T>, std::negation<std::is_integral<U>>>,
	std::conjunction<std::is_floating_point<T>, std::negation<std::is_floating_point<U>>>>>
{};
template <typename T, typename U>
static constexpr bool is_arithmetic_relaxed_v = is_arithmetic_relaxed<T, U>::value;

template <typename T, typename U> struct is_arithmetic_similar :
	std::conjunction<is_arithmetic_relaxed<T, U>,
		std::negation<std::disjunction<
			std::conjunction<std::is_unsigned<T>, std::negation<std::is_unsigned<U>>>,
			std::conjunction<std::is_signed<T>, std::negation<std::is_signed<U>>>>>>
{};
template <typename T, typename U>
static constexpr bool is_arithmetic_similar_v = is_arithmetic_similar<T, U>::value;

template <typename Derived, typename ValueType, typename ArithmeticType>
struct arithmetic_comparison {
	using type = std::conditional_t<std::is_base_of_v<ValueOperable<strictness_exact_match>, Derived>,
			std::conditional_t<std::is_same_v<ValueType, ArithmeticType>,
				arithmetic_type_conversion,
				invalid_arithmetic_type_conversion_exact>,
			std::conditional_t<std::is_base_of_v<ValueOperable<strictness_similar>, Derived>,
				std::conditional_t<is_arithmetic_similar_v<ValueType, ArithmeticType>,
					arithmetic_type_conversion,
					invalid_arithmetic_type_conversion_similar>,
			std::conditional_t<std::is_base_of_v<ValueOperable<strictness_relaxed>, Derived>,
				std::conditional_t<is_arithmetic_relaxed_v<ValueType, ArithmeticType>,
					arithmetic_type_conversion,
					invalid_arithmetic_type_conversion_relaxed>,
			unknown_conversion>>>;
};
template <typename Derived, typename ValueType, typename ArithmeticType>
using arithmetic_comparison_t = typename arithmetic_comparison<Derived, ValueType, ArithmeticType>::type;

// Logic chain to determine the tag type.
template <typename Derived,
	typename Other,
	typename ValueType = typename Derived::value_type,
	typename O = std::decay_t<Other>>
struct conversion_type {
	using type = std::conditional_t<std::is_same_v<Derived, O>,
		same_type_conversion,
		std::conditional_t<has_base_template_v<O, StrongType>,
			std::conditional_t<can_operate_with_v<Derived, O>,
				implicit_strong_type_conversion,
				invalid_strong_type_conversion>,
			std::conditional_t<!has_base_template_v<Derived, ValueOperable>,
				not_value_operable,
				std::conditional_t<std::is_same_v<ValueType, O>,
					same_value_type_conversion,
					std::conditional_t<std::is_arithmetic_v<O>,
						std::conditional_t<std::is_arithmetic_v<ValueType>,
							arithmetic_comparison_t<Derived, ValueType, O>,
							not_arithmetic>,
						unknown_conversion>>>>>;
};
template <typename Derived, typename Other>
using conversion_type_t = typename conversion_type<Derived, Other>::type;

template <typename Derived, typename MaybeStrongType, typename R /*= typename Derived::value_type*/>
constexpr R getCompatibleValue(const MaybeStrongType& maybeStrongType) noexcept
{
	using tag_type = conversion_type_t<Derived, MaybeStrongType>;

	if constexpr (std::is_same_v<tag_type, same_type_conversion>) {
		return maybeStrongType.value;
	} else if constexpr (std::is_same_v<tag_type, same_value_type_conversion>) {
		return maybeStrongType;
	} else if constexpr (std::is_same_v<tag_type, implicit_strong_type_conversion>) {
		return maybeStrongType.template castTo<Derived>().value;
	} else if constexpr (std::is_same_v<tag_type, arithmetic_type_conversion>) {
		return static_cast<R>( maybeStrongType );

	// Errors.
	} else if constexpr (std::is_same_v<tag_type, invalid_strong_type_conversion>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"No conversion specified from the right-hand StrongType to the left-hand StrongType. Declare\n"
			"using operable_with = RightHandStrongType\n"
			"in LeftHandStrongType to enable implicit interop (use operable_with_types for multiple StrongTypes)."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	} else if constexpr (std::is_same_v<tag_type, not_arithmetic>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Using a StrongType with a non-arithmetic value_type with an arithmetic value."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	} else if constexpr (std::is_same_v<tag_type, not_value_operable>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Using a StrongType with an arithmetic value. Inherit from ValueOperable if you want to enable this behaviour."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	} else if constexpr (std::is_same_v<tag_type, invalid_arithmetic_type_conversion_exact>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Using a StrongType with a value that isn't an exact match of the underlying value_type. "
			"Right-hand value must be cast."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	} else if constexpr (std::is_same_v<tag_type, invalid_arithmetic_type_conversion_similar>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Using a StrongType with a value of different signedness, or floating point to integral. "
			"Right-hand value must be cast."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	} else if constexpr (std::is_same_v<tag_type, invalid_arithmetic_type_conversion_relaxed>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Using a StrongType with an integral value_type with a floating-point value, or vice versa."
			"Right-hand value must be cast."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	} else if constexpr (std::is_same_v<tag_type, unknown_conversion>) {
		static_assert(always_false<Derived, MaybeStrongType>::value,
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
			"Operation invalid between these types, and no valid implicit conversion detected.\n"
			"StrongTypes can operate with:\n"
			"\t1. The same StrongType.\n"
			"\t2. Any StrongType that the left-hand StrongType has declared itself to be operable_with.\n"
			"\t3. If ValueOperable, an arithmetic type that matches the strictness requirements."
			CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER);
	}
}
} // detail
} // ctp

#undef CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_HEADER
#undef CTP_STRONG_TYPE_STATIC_ASSERT_FAIL_FOOTER
#endif // INCLUDE_CTP_TOOLS_STRONG_TYPE_HPP
