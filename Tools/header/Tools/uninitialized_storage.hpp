#ifndef INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP
#define INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP

#include <memory>

namespace ctp {

struct default_uninitialized_item {
	// user-defined noop default constructor to avoid zero-initialization if uninitialized_storage
	// is value-initialized with {} or {0}.
	// See std::optional implementations for a similar use case.
	constexpr default_uninitialized_item() noexcept {}
};

// uninitialized_item can be used to create an uninitialized array of type T that is
// constexpr friendly, though at this current time it is not possible to get a T* to
// such an array in constant expressions, and iterators must be used instead.
template <typename T>
union uninitialized_item {
	static_assert(sizeof(T) >= sizeof(default_uninitialized_item)
		&& alignof(T) >= alignof(default_uninitialized_item));

	default_uninitialized_item default_item_;
	T item;

	constexpr uninitialized_item() noexcept {
		std::construct_at(&default_item_); // noop
	}
	constexpr ~uninitialized_item() noexcept {}
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP
