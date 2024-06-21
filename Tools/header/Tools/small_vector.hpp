#ifndef INCLUDE_CTP_TOOLS_SMALL_VECTOR_HPP
#define INCLUDE_CTP_TOOLS_SMALL_VECTOR_HPP

#include "small_storage.hpp"
#include "trivial_allocator_adapter.hpp"

namespace ctp {

struct static_vector_options : small_storage::default_options {
	static constexpr bool has_large_mode = false;
	static constexpr bool constexpr_friendly = true;
};

// A statically sized vector with local storage for NumItems.
// Users are expected to ensure they do not attempt to
// store more than NumItems items.
template <class T, std::size_t NumItems, class Alloc = trivial_init_allocator<T>>
class static_vector : public small_storage::container<T, NumItems, Alloc, static_vector_options> {
public:
	using Base = small_storage::container<T, NumItems, Alloc, static_vector_options>;
	using Base::Base;
};

struct small_vector_options : small_storage::default_options {
	static constexpr bool has_large_mode = true;
	static constexpr bool constexpr_friendly = true;
};

// A vector with enough local storage for at least NumItemsInSmallMode.
// Will allocate using Alloc and grow if it runs out of space in local storage.
template <class T, std::size_t NumItemsInSmallMode, class Alloc = trivial_init_allocator<T>, class Options = small_vector_options>
class small_vector : public small_storage::container<T, NumItemsInSmallMode, Alloc, Options>{
public:
	using Base = small_storage::container<T, NumItemsInSmallMode, Alloc, Options>;
	using Base::Base;
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_SMALL_VECTOR_HPP
