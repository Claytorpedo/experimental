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


namespace detail {

template <typename T>
class uninitialized_storage_base {
protected:
	void destroy(uninitialized_item<T>* items, std::size_t numToDestroy) {
		for (std::size_t i = 0; i < numToDestroy; ++i)
			std::destroy_at(std::addressof(items[i].item_));
	}
};

} // detail

// Uninitialized stack memory array of type T that can be used in constexpr contexts, with the
// restrictions that you cannot retrieve a raw T* to the underlying array at compile time.
template <typename T, std::size_t NumItems>
class uninitialized_storage : detail::uninitialized_storage_base<T> {
	uninitialized_item<T> items_[NumItems];
	std::size_t size_ = 0;
public:
	using size_type = std::size_t;
	static constexpr size_type size = NumItems;

	constexpr ~uninitialized_storage() noexcept requires (!std::is_trivially_destructible_v<T>) {
		destroy(items_, size_);
	}

	// TODO: basically can make this class via small_storage,
	// or can copy and simplify code here when small_storage is done.

};

} // ctp

#endif // INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP
