#ifndef INCLUDE_CTP_TOOLS_CONCEPTS_HPP
#define INCLUDE_CTP_TOOLS_CONCEPTS_HPP

#include "type_traits.hpp"
#include "utility.hpp"

#include <concepts>

namespace ctp::concepts {

template <class E>
concept Enum = std::is_enum_v<E>;

template <class T>
concept Pointer = std::is_pointer_v<T>;

template <class First, class... Rest>
concept AllExactlySame = (sizeof...(Rest) ? std::conjunction_v<std::is_same<First, Rest>...> : true);

template <class To, class... Rest>
concept AllConvertibleTo = (sizeof...(Rest) ? std::conjunction_v<std::is_convertible<To, Rest>...> : true);

template <template<class...> class TemplateType, class MaybeInstantiationType>
concept InstantiationOf = is_instantiation_of_v<TemplateType, std::remove_cv_t<MaybeInstantiationType>>;

template <class T, class Ptr>
concept HasData = requires (T t) {
	{ t.data() } -> std::convertible_to<Ptr>;
};

template <typename Op, typename Val>
concept UnaryOp = requires (Op && op, Val && val) {
	op(forward<Val>(val));
};

template <typename Op, typename Lhs, typename Rhs>
concept BinaryOp = requires (Op && op, Lhs && lhs, Rhs && rhs) {
	op(forward<Lhs>(lhs), forward<Rhs>(rhs));
};

template <typename Op, typename... Ts>
concept n_aryOp = requires (Op && op, Ts&&... ts) {
	op(forward<Ts>(ts)...);
};

template <class Derived, template <class...> class Base>
concept HasBaseTemplate = has_base_template_v<Derived, Base>;

} // ctp::concepts

#endif // INCLUDE_CTP_TOOLS_CONCEPTS_HPP
