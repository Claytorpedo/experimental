#ifndef INCLUDE_CTP_TOOLS_TRIVIAL_ALLOCATOR_ADAPTER_HPP
#define INCLUDE_CTP_TOOLS_TRIVIAL_ALLOCATOR_ADAPTER_HPP

#include "config.hpp"
#include <memory>

namespace ctp {

// Allocator adapter that overrides default construct behaviour to do
// default initialization (as opposed to value initialization) for trivial types.
template <typename T, typename Alloc = std::allocator<T>>
class trivial_init_allocator : public Alloc {
public:
	using Alloc::Alloc;

	// With no args, do default initialization in non-constexpr contexts.
	template <typename U>
	constexpr void construct(U* ptr) noexcept(std::is_nothrow_default_constructible_v<U>) {
		if CTP_IS_CONSTEVAL {
			std::construct_at(ptr);
		} else {
			::new(static_cast<void*>(ptr)) U;
		}
	}

	// If we have args, do whatever the parent allocator does.
	template <typename U, typename...Args>
	constexpr void construct(U* ptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<U, Args...>) {
		std::allocator_traits<Alloc>::construct(*static_cast<Alloc*>(this), ptr, std::forward<Args>(args)...);
	}
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_TRIVIAL_ALLOCATOR_ADAPTER_HPP
