#ifndef INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP
#define INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP

#include "config.hpp"
#include "debug.hpp"
#include "iterator.hpp"

#include <memory>
#include <type_traits>

// TODO: once http://wg21.link/p3074 is implemented, I can get rid of the uninitialized_item/iterator helpers.
// The paper makes it so union { T array[] } will be trivially constructible.

namespace ctp {

// --------------------------------------------------------
// uninitialized_item

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

	default_uninitialized_item default_item;
	T item;

	constexpr uninitialized_item() noexcept {
		std::construct_at(&default_item); // noop
	}
	constexpr ~uninitialized_item() noexcept {}
};


// --------------------------------------------------------
// uninitialized_item_iterator

namespace detail {

template <typename T, bool IsConst>
struct uninit_item_const_type {
	using type = std::conditional_t<IsConst, const uninitialized_item<T>, uninitialized_item<T>>;
};
template <typename T, bool IsConst>
using uninit_item_const_t = typename uninit_item_const_type<T, IsConst>::type;

template <typename T, bool IsConst = false>
class uninitialized_item_iterator_t :
	public iterator_t<uninitialized_item_iterator_t<T, IsConst>,
	std::conditional_t<IsConst, const T, T>> {
	using ConstQualifiedT = std::conditional_t<IsConst, const T, T>;
	using ConstQualifiedItem = detail::uninit_item_const_t<T, IsConst>;

	static_assert(!std::is_const_v<T>, "Use const_uninitialized_item_iterator instead of using <const T>.");

	ConstQualifiedItem* ptr_ = nullptr;

	friend class iterator_accessor;
	constexpr auto& get_index() noexcept { return ptr_; }
	constexpr ConstQualifiedT* peek() noexcept {
		ctpAssert(ptr_ != nullptr); // invalid iterator

		if CTP_IS_CONSTEVAL {
			return std::addressof(ptr_->item);
		} else {
			return reinterpret_cast<ConstQualifiedT*>(ptr_);
		}
	}

	// conversion from non-const iterator to const iterator.
	friend class uninitialized_item_iterator_t<T, true>;
	using nonconst_t = std::conditional_t<!IsConst, nonesuch, uninitialized_item_iterator_t<T, false>>;
public:
	// Get pointed-to uninitialized_item.
	constexpr ConstQualifiedItem* get_item() noexcept { return ptr_; }
	// Get pointed-to uninitialized_item.
	constexpr const ConstQualifiedItem* get_item() const noexcept { return ptr_; }

	constexpr uninitialized_item_iterator_t() noexcept = default;
	constexpr uninitialized_item_iterator_t(ConstQualifiedItem* ptr) noexcept
		: ptr_{ptr}
	{}

	template <typename NonConstT = nonconst_t, std::enable_if_t<!std::is_same_v<NonConstT, nonesuch>, int> = 0>
	constexpr uninitialized_item_iterator_t(const nonconst_t& o) noexcept
		: ptr_{o.ptr_}
	{}
};

} // detail

template <typename T>
using uninitialized_item_iterator = detail::uninitialized_item_iterator_t<T, false>;

template <typename T>
using const_uninitialized_item_iterator = detail::uninitialized_item_iterator_t<T, true>;

namespace uninit {

struct uninitialized_tag {};
inline constexpr uninitialized_tag uninitialized{};

// Wrap a pointer in a class for a strongly-typed iterator.
// We need it for the provided value_type in the helper functions below.
template <typename T>
class ptr_iterator_t : public iterator_t<ptr_iterator_t<T>, T> {
	T* t_ = nullptr;

	friend iterator_accessor;
	constexpr T*& get_index() noexcept { return t_; }
	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(ptr_iterator_t, T)
		: t_{other.t_}
	{}
	constexpr auto peek() noexcept { return t_; }
public:
	constexpr ptr_iterator_t() noexcept = default;
	constexpr ptr_iterator_t(T* t) noexcept : t_{t} {}
};

template <typename T, bool IsArrayWrapper>
struct default_iterator_selector {
	using iterator = ptr_iterator_t<T>;
	using const_iterator = ptr_iterator_t<const T>;
};
template <typename T>
struct default_iterator_selector<T, false> {
	using iterator = uninitialized_item_iterator<T>;
	using const_iterator = const_uninitialized_item_iterator<T>;
};


// --------------------------------------------------------
// Helper functions for uninit::array.

template <typename Alloc, typename Iterator, typename... Args>
constexpr auto& do_construct_at(Alloc& alloc, Iterator iterator, Args&&... args)
CTP_NOEXCEPT(noexcept(std::is_nothrow_constructible_v<typename Iterator::value_type, Args&&...>))
{
	if CTP_IS_CONSTEVAL {
		if constexpr (is_instantiation_of_v<ptr_iterator_t, Iterator>) {
			// array_wrapper case, where values are already default-constructed.
			return *iterator = std::make_obj_using_allocator<typename Iterator::value_type>(alloc, std::forward<Args>(args)...);
		} else {
			return *std::uninitialized_construct_using_allocator(iterator.get(), alloc, std::forward<Args>(args)...);
		}
	} else {
		return *std::uninitialized_construct_using_allocator(iterator.get(), alloc, std::forward<Args>(args)...);
	}
}

template <typename Alloc, typename Iterator, typename... Args>
constexpr void construct_n(Alloc& alloc, Iterator begin, std::size_t n, Args&&... args)
CTP_NOEXCEPT(noexcept(std::is_nothrow_constructible_v<typename Iterator::value_type, Args...>))
{
	if (n > 1) {
		for (std::size_t i = 0; i < n; ++i)
			uninit::do_construct_at(alloc, begin + i, args...);
	} else {
		uninit::do_construct_at(alloc, begin, std::forward<Args>(args)...);
	}
}

template <typename Alloc, typename Iterator>
constexpr void do_destroy_at(Alloc& alloc, Iterator iterator) noexcept {
	std::allocator_traits<Alloc>::destroy(alloc, iterator.get());

	if CTP_IS_CONSTEVAL {
		if constexpr (is_instantiation_of_v<ptr_iterator_t, Iterator>) {
			// array_wrapper case.
			// Keep a default-constructed element there to keep the array from entering an invalid state.
			// uninit::array will destroy all items upon destruction in constexpr contexts.
			std::construct_at(iterator.get());
		}
	}
}

template <typename Alloc, typename Iterator>
constexpr void destroy_n(Alloc& alloc, Iterator begin, std::size_t n) noexcept {
	for (std::size_t i = n; i > 0; --i)
		uninit::do_destroy_at(alloc, begin + (i - 1));
}


// --------------------------------------------------------
// uninit::array

// Makes it so we can use std::construct_at and std::destroy_at within a union.
// I think this is technically not supposed to be allowed, but is currently
// supported by MSVC, clang, and gcc.
template <typename T, std::size_t Size>
struct array_wrapper { T elems[Size]; };

// Creates an array of T that is initialized in constexpr contexts, but uninitialized at runtime
// and so can be contructed in directly, as needed. Use with do_construct_at/do_destroy_at above.
template <
	typename T,
	std::size_t Size,
	bool IsArrayWrapper = std::is_default_constructible_v<T>,
	// Used for niche use case where you want to specify your own iterator/const_iterator.
	// But they still need to be compatible with the array_type used.
	typename IteratorTypes = default_iterator_selector<T, IsArrayWrapper>>
	struct array {
	using array_type = std::conditional_t<IsArrayWrapper,
		uninitialized_item<array_wrapper<T, Size>>,
		uninitialized_item<T>[Size]>;

	// Storage that will be uninitialized at run time, but is default initialized at compile time.
	// Even though we always populate all values during construction, this array is constructed first.
	array_type data;

	using value_type = T;
	using iterator = typename IteratorTypes::iterator;
	using const_iterator = typename IteratorTypes::const_iterator;

	constexpr array() noexcept {
		if CTP_IS_CONSTEVAL {
			// Default construct all elements in constant expressions.
			if constexpr (IsArrayWrapper) {
				std::construct_at(&data.item);
			}
		}
	}

	constexpr ~array() noexcept {
		if CTP_IS_CONSTEVAL {
			if constexpr (IsArrayWrapper) {
				std::destroy_at(&data.item);
			}
		}
	}

	constexpr std::size_t size() const noexcept { return Size; }

	constexpr const T* data_ptr() const {
		if constexpr (IsArrayWrapper) {
			return data.item.elems;
		} else {
			if CTP_IS_CONSTEVAL {
				// This is likely to fail. You generally cannot use data() with
				// non-default constructible arrays at compile time.
				// This one pointer is fine, but using it to get further items
				// will probably not work.
				return std::addressof(data[0].item);
			} else {
				return reinterpret_cast<const T*>(data);

			}
		}
	}
	constexpr T* data_ptr() noexcept {
		return const_cast<T*>(std::as_const(*this).data_ptr());
	}

	constexpr iterator begin() noexcept {
		if constexpr (IsArrayWrapper) {
			return data.item.elems;
		} else {
			return data;
		}
	}
	constexpr const_iterator begin() const {
		if constexpr (IsArrayWrapper) {
			return data.item.elems;
		} else {
			return data;
		}
	}

	constexpr iterator end() noexcept { return begin() + Size; }
	constexpr const_iterator end() const noexcept { return begin() + Size; }

	constexpr iterator get(std::size_t i) noexcept {
		if constexpr (IsArrayWrapper) {
			return data.item.elems + i;
		} else {
			return data[i];
		}
	}
	constexpr const_iterator get(std::size_t i) const {
		if constexpr (IsArrayWrapper) {
			return data.item.elems + i;
		} else {
			return data[i];
		}
	}

	constexpr T& operator[](std::size_t i) noexcept {
		if constexpr (IsArrayWrapper) {
			return data.item.elems[i];
		} else {
			return data[i].item;
		}
	}
	constexpr const T& operator[](std::size_t i) const {
		if constexpr (IsArrayWrapper) {
			return data.item.elems[i];
		} else {
			return data[i].item;
		}
	}
};

} // uninit
} // ctp

#endif // INCLUDE_CTP_TOOLS_UNINITIALIZEDL_STORAGE_HPP
