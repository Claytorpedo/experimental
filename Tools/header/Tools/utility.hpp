#ifndef INCLUDE_CTP_TOOLS_UTILITY_HPP
#define INCLUDE_CTP_TOOLS_UTILITY_HPP

namespace ctp {

template <typename T> struct remove_ref { using type = T; };
template <typename T> struct remove_ref<T&> { using type = T; };
template <typename T> struct remove_ref<T&&> { using type = T; };
template <typename T> using remove_ref_t = typename remove_ref<T>::type;

template <typename T>
struct forward_fn {
	[[nodiscard]] constexpr T&& operator()(remove_ref_t<T>& t) const noexcept { return static_cast<T&&>(t); }
	[[nodiscard]] constexpr T&& operator()(remove_ref_t<T>&& t) const noexcept { return static_cast<T&&>(t); }
};
struct move_fn {
	template <typename T>
	[[nodiscard]] constexpr remove_ref_t<T>&& operator()(T&& t) const noexcept { return static_cast<remove_ref_t<T>&&>(t); }
};

template <typename T> inline constexpr forward_fn<T> forward{};
inline constexpr move_fn move{};

// Default projection type: https://en.cppreference.com/w/cpp/utility/functional/identity
struct identity {
	using is_transparent = int;

	template<typename T>
	constexpr T&& operator()(T&& t) const noexcept { return forward<T>(t); }
};

struct ProjectionAccessor {
	template<typename P, typename T, typename U>
	constexpr auto operator()(P T::* f, U&& t) const noexcept -> decltype(t.*f) {
		return t.*f;
	}

	template<typename F, typename T>
	constexpr auto operator()(F&& f, T&& t) const
		noexcept(noexcept(forward<F>(f)(forward<T>(t))))
		-> decltype(forward<F>(f)(forward<T>(t)))
	{
		return forward<F>(f)(forward<T>(t));
	}
};

inline constexpr ProjectionAccessor project_onto{};

// Wrap a template into a struct to be extracted with a type via unwrap_template.
// e.g.
// using wrapped_vector = template_wrapper<std::vector>
// unwrap<wrapper_vector, int> vec; // vector<int>
template <template<class...> class T>
struct template_wrapper {
	template <class... Ts>
	using type = T<Ts...>;
};
template <typename Wrapped, typename T>
using unwrap_template = typename Wrapped::template type<T>;

} // ctp

#endif // INCLUDE_CTP_TOOLS_UTILITY_HPP
