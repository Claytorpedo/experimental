#ifndef INCLUDE_CTP_TOOLS_TYPE_TRAITS_HPP
#define INCLUDE_CTP_TOOLS_TYPE_TRAITS_HPP

#include <type_traits>

namespace ctp {
struct nonesuch {
	nonesuch() = delete;
	~nonesuch() = delete;
	nonesuch(const nonesuch&) = delete;
	void operator=(const nonesuch&) = delete;
};

namespace detail {
template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
struct detector {
	using value_t = std::false_type;
	using type = Default;
};

template <class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
	using value_t = std::true_type;
	using type = Op<Args...>;
};
} // detail

// is_detected: https://en.cppreference.com/w/cpp/experimental/is_detected
template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template< template<class...> class Op, class... Args >
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <template<class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template <class Default, template<class...> class Op, class... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

template <class Expected, template<class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template <class Expected, template<class...> class Op, class... Args>
inline constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

template <class To, template<class...> class Op, class... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

template <class To, template<class...> class Op, class... Args>
inline constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;


// is_instantiation_of: detect if MaybeInstantiationType is any instantiation of TemplateType. e.g. T<int> is an instantiation of T<U>
template <template <class...> class TemplateType, class MaybeInstantiationType>
struct is_instantiation_of : std::false_type {};

template <template<class...> class TemplateType, class... Args>
struct is_instantiation_of<TemplateType, TemplateType<Args...>> : std::true_type {};

template <template<class...> class TemplateType, class MaybeInstantiationType>
inline constexpr bool is_instantiation_of_v = is_instantiation_of<TemplateType, std::remove_cv_t<MaybeInstantiationType>>::value;


// is_explicitly_convertible: detect if static_cast<To>(From) is valid.
namespace detail { template <typename F, typename T> using explicit_conversion = decltype(static_cast<T>(std::declval<F>())); }
template <typename From, typename To> struct is_explicitly_convertible : is_detected<detail::explicit_conversion, From, To> {};
template <typename From, typename To> inline constexpr bool is_explicitly_convertible_v = is_explicitly_convertible<From, To>::value;


// has_base_template: detect if the derived class has any instantiation of a templated base class.
namespace detail {
template <template<class...> class Base> std::false_type detect_base_template(...);
template <template<class...> class Base, typename... Args> std::true_type detect_base_template(Base<Args...>&);
} // detail

template <typename Derived, template<class...> class Base>
struct has_base_template : decltype(detail::detect_base_template<Base>(std::declval<Derived&>())) {};
template <typename Derived, template<class...> class Base>
inline constexpr bool has_base_template_v = has_base_template<Derived, Base>::value;


template <typename... Args> struct always_false : std::false_type {};


// pack size checks
template <std::size_t N, typename... Args> struct pack_greater_than { static constexpr bool value = sizeof...(Args) > N; };
template <std::size_t N, typename... Args> inline constexpr bool pack_greater_than_v = pack_greater_than<N, Args...>::value;
template <typename... Args> struct pack_not_empty : pack_greater_than<0, Args...> {};
template <typename... Args> inline constexpr bool pack_not_empty_v = pack_not_empty<Args...>::value;

template <typename... Ts> struct first_element { using type = void; };
template <typename Head> struct first_element<Head> { using type = Head; };
template <typename Head, typename... Rest> struct first_element<Head, Rest...> { using type = Head; };
template <typename... Ts> using first_element_t = typename first_element<Ts...>::type;

// contains_type: see if a type exists in a parameter pack.
template <typename T, typename... Ts> struct contains_type : std::disjunction<std::is_same<T, Ts>...> {};
template <typename T, typename... Ts> inline constexpr bool contains_type_v = contains_type<T, Ts...>::value;


// template_typelist_contains_type: detect if a template parameter list contains a type.
// e.g. template_typelist_contains_type<int, MyType<char, int, bool>>::value == std::true_type
template <typename T, typename TemplateType> struct template_typelist_contains_type : std::false_type {}; // primary

template <typename T, template<class...> class TemplateType, typename... Ts>
struct template_typelist_contains_type<T, TemplateType<Ts...>> : contains_type<T, Ts...> {};
template <typename T, typename TemplateType>
inline constexpr bool template_typelist_contains_type_v = template_typelist_contains_type<T, TemplateType>::value;

template <typename T> using not_void = std::negation<std::is_void<T>>;
template <typename T> inline constexpr bool not_void_v = not_void<T>::value;


// Adds const to inner type for pointers, references, and values.
// T -> const T
// T& -> const T&
// T* -> const T*
// T* const -> const T* const
template <typename T> struct add_inner_const { using type = const T; };
template <typename T> struct add_inner_const<T&> { using type = const T&; };
template <typename T> struct add_inner_const<T*> { using type = const T*; };
template <typename T> struct add_inner_const<T* const> { using type = const T* const; };
template <typename T> using add_inner_const_t = typename add_inner_const<T>::type;

// Removes const from inner type for pointers, references, and values.
// const T -> T
// const T& -> T&
// const T* -> T*
// const T* const -> T* const
template <typename T> struct remove_inner_const { using type = std::remove_const_t<T>; };
template <typename T> struct remove_inner_const<T&> { using type = std::remove_const_t<T>&; };
template <typename T> struct remove_inner_const<T*> { using type = std::remove_const_t<T>*; };
template <typename T> struct remove_inner_const<T* const> { using type = std::remove_const_t<T>* const; };
template <typename T> using remove_inner_const_t = typename remove_inner_const<T>::type;

// Checks for const on the inner type for pointers, references, and values.
template <typename T> struct has_inner_const { static constexpr bool value = std::is_const_v<T>; };
template <typename T> struct has_inner_const<T&> { static constexpr bool value = std::is_const_v<T>; };
template <typename T> struct has_inner_const<T*> { static constexpr bool value = std::is_const_v<T>; };
template <typename T> struct has_inner_const<T* const> { static constexpr bool value = std::is_const_v<T>; };
template <typename T> inline constexpr bool has_inner_const_v = has_inner_const<T>::value;

// one_of_binary_op; Heper to apply a binary operation to a list of template types and get their disjunction.
template <template <class...> class BinaryOp, typename Test, typename... Ts>
struct one_of_binary_op : std::disjunction<BinaryOp<Test, Ts>...> {};

// one_of; See if any type in Ts compares same_as to Test.
template <typename Test, typename... Ts>
struct one_of : one_of_binary_op<std::is_same, Test, Ts...> {};
template <typename Test, typename... Ts>
inline constexpr bool one_of_v = one_of<Test, Ts...>::value;


} // ctp

#endif // INCLUDE_CTP_TOOLS_TYPE_TRAITS_HPP
