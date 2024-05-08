#ifndef INCLUDE_CTP_TOOLS_ITER_MOVE_HPP
#define INCLUDE_CTP_TOOLS_ITER_MOVE_HPP

#include "utility.hpp"

#include <type_traits>

namespace ctp {

namespace iterator_detail::iterator_move {

// Customization point object for iter_move. Based on ranges v3.

// ADL helpers for any iter_move customization point.
template <typename I>
concept HasIterMoveADL = requires (I && i) {
	iter_move(static_cast<I&&>(i));
};

template <typename I>
struct iter_ref {
	using type = decltype(*std::declval<I&>());
};
template <typename I> using iter_ref_t = typename iter_ref<I>::type;

struct func {
	template <HasIterMoveADL Iter>
	constexpr auto operator()(Iter&& it) const
		noexcept(noexcept(iter_move(static_cast<Iter&&>(it))))
		-> decltype(iter_move(static_cast<Iter&&>(it)))
	{
		return iter_move(static_cast<Iter&&>(it));
	}

	template <typename Iter, typename R = iter_ref_t<Iter>>
		requires (!HasIterMoveADL<Iter>)
	constexpr auto operator()(Iter&& it) const
		noexcept(noexcept(static_cast<remove_ref_t<R>&&>(::ctp::move(*it))))
		-> decltype(static_cast<remove_ref_t<R>&&>(::ctp::move(*it)))
	{
		return static_cast<remove_ref_t<R>&&>(::ctp::move(*it));
	}
};

} // iterator_detail::iterator_move

namespace iterator_detail { inline constexpr iterator_move::func iter_move; }

} // ctp

#endif // INCLUDE_CTP_TOOLS_ITER_MOVE_HPP
