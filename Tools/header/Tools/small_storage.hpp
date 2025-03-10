#ifndef INCLUDE_CTP_TOOLS_SMALL_STORAGE_HPP
#define INCLUDE_CTP_TOOLS_SMALL_STORAGE_HPP

#include "config.hpp"
#include "concepts.hpp"
#include "debug.hpp"
#include "iterator.hpp"
#include "move_iterator.hpp"
#include "reverse_iterator.hpp"
#include "scope.hpp"
#include "type_traits.hpp"
#include "uninitialized_storage.hpp"
#include "uninitialized_storage_iterator.hpp"

#include <bit>
#include <cstddef>
#include <compare>
#include <concepts>
#include <memory> // construct_at, addressof

#if CTP_USE_EXCEPTIONS
#include <stdexcept>
#include <format>
#endif

// small_storage::container.
// constexpr friendly contiguous storage with small size optimization.
// Can also be used to make fixed-size local storage vector types.

// TODO: if a paper like http://wg21.link/p3074 is accepted, then
// I can get rid of uninitialized_item<T>/iterator usage.

namespace ctp::small_storage {

enum class Mode { Small, Large };

// ---------------------------------------------------------------------------------------
// small_storage::options
// ---------------------------------------------------------------------------------------

template <float GrowthFactor>
struct growth_policy_impl {
	template <typename S>
	[[nodiscard]] static constexpr
		S apply(const S currentCapacity, const S neededCapacity, const S maxCapacity) noexcept {
		return std::min<S>(std::max<S>(static_cast<S>(currentCapacity * GrowthFactor + 0.5f), neededCapacity), maxCapacity);
	}
};

// Slow growth policy for containers unlikely to grow much. Increases ~30% increments.
using slow_growth_policy = growth_policy_impl<1.35f>;
using medium_growth_policy = growth_policy_impl<1.65f>;
using normal_growth_policy = growth_policy_impl<2.0f>;

// Inherit from this to customize.
struct default_options {
	using growth_policy = medium_growth_policy;
	using large_size_type = std::size_t;
	static constexpr bool has_large_mode = true;
	static constexpr bool constexpr_friendly = false;

	// Until a paper such as http://wg21.link/p3074 is adopted to enable
	// uninitialized array storage on the stack in constant expressions
	// possible, we must either default-construct all items in the array or use uninitialized_item<T>.
	// Default-construction allows a simpler iterator to be used and for data() to return a pointer at compile time.
	static constexpr bool allow_default_construction_in_constant_expressions = true;
};

template <class Options>
concept SmallStorageOptions = std::derived_from<Options, default_options>;

template <typename T, bool AllowDefaultConstructionInConstantExpressions>
concept IsDataConstexprFriendly =
	std::is_trivial_v<T> ||
	(AllowDefaultConstructionInConstantExpressions && std::is_default_constructible_v<T>);


template <typename T, std::size_t MinSmallCapacity, typename Allocator = std::allocator<T>, SmallStorageOptions options = default_options>
class container;

// ---------------------------------------------------------------------------------------
// small_storage::iterators
// ---------------------------------------------------------------------------------------

namespace detail { class const_iterator_converter; }

// Wrap a pointer in a class for a strongly-typed iterator.
template <typename T>
class ptr_iterator_t : public iterator_t<ptr_iterator_t<T>, T> {
	T* t_ = nullptr;

	friend iterator_accessor;
	constexpr T*& get_index() noexcept { return t_; }
	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(ptr_iterator_t, T)
		: t_{other.t_}
	{}
	constexpr auto peek() noexcept { return t_; }

	friend detail::const_iterator_converter;
	// A few of the STL interface functions take const_iterators even when the intention
	// is to modify the container with them. Provide a method to allow convertion to a non-const iterator.
	constexpr auto to_non_const() noexcept {
		if constexpr (std::same_as<nonconst_t, nonesuch>) {
			return *this;
		} else {
			return nonconst_t{const_cast<std::remove_const_t<T>*>(t_)};
		}
	}
public:
	constexpr ptr_iterator_t() noexcept = default;
	constexpr ptr_iterator_t(T* t) noexcept : t_{t} {}
};

// Special iterator when small_data uses uninitialized_item.
template <typename T>
class combo_uninitialized_iterator_t : public iterator_t<combo_uninitialized_iterator_t<T>, T> {
	using small_type = std::conditional_t<std::is_const_v<T>, const uninitialized_item<std::remove_const_t<T>>, uninitialized_item<T>>;

#ifdef __cpp_lib_within_lifetime
	#warning "Should be able to get rid of is_small_ now."
#endif
	[[maybe_unused]] bool is_small_; // Intentionally uninitialized at runtime.
	union {
		small_type* small;
		T* large;
	};
	static_assert(sizeof(uninitialized_item<T>) == sizeof(T),
		"Should be the same size so we don't care which is active at run time.");

	friend iterator_accessor;
	constexpr void advance(std::ptrdiff_t n) noexcept {
		if CTP_IS_CONSTEVAL{
			if (is_small_)
				small += n;
			else
				large += n;
		} else {
			large += n;
		}
	}
	constexpr auto distance_to(const combo_uninitialized_iterator_t& o) const noexcept {
		if CTP_IS_CONSTEVAL{
			// If this fails, we're comparing iterators from different storage, or one is expired.
			ctpExpects(is_small_ == o.is_small_);
			if (is_small_)
				return o.small - small;
			return o.large - large;
		} else {
			return o.large - large;
		}
	}
	constexpr T* peek() noexcept {
		if CTP_IS_CONSTEVAL{
			if (is_small_)
				return std::addressof(small->item);
			return large;
		} else {
			return large;
		}
	}

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(combo_uninitialized_iterator_t, T) {
		if CTP_IS_CONSTEVAL {
			is_small_ = other.is_small_;
			if (is_small_)
				std::construct_at(small, other.small);
			else
				std::construct_at(large, other.large);
		} else {
			small = other.small;
		}
	}

	friend detail::const_iterator_converter;
	// A few of the STL interface functions take const_iterators even when the intention
	// is to modify the container with them. Provide a method to allow convertion to a non-const iterator.
	constexpr auto to_non_const() noexcept {
		if constexpr (std::same_as<nonconst_t, nonesuch>) {
			return *this;
		} else {
			nonconst_t nonconst;
			if CTP_IS_CONSTEVAL{
				nonconst.is_small_ = is_small_;
				if (is_small_) {
					nonconst.small = const_cast<std::remove_const_t<small_type>*>(small);
				} else {
					nonconst.large = const_cast<std::remove_const_t<T>*>(large);;
				}
			} else {
				nonconst.large = const_cast<std::remove_const_t<T>*>(large);
			}
			return nonconst;
		}
	}
public:
	constexpr combo_uninitialized_iterator_t() noexcept = default;
	constexpr combo_uninitialized_iterator_t(uninitialized_item_iterator<std::remove_const_t<T>> it) noexcept
		: small{it.get_item()}
	{
		if CTP_IS_CONSTEVAL {
			is_small_ = true;
		}
	}
	constexpr combo_uninitialized_iterator_t(const_uninitialized_item_iterator<std::remove_const_t<T>> it) noexcept
		requires std::is_const_v<T>
		: small{it.get_item()}
	{
		if CTP_IS_CONSTEVAL {
			is_small_ = true;
		}
	}

	constexpr combo_uninitialized_iterator_t(ptr_iterator_t<T> it) noexcept
		: large{it.get()}
	{
		if CTP_IS_CONSTEVAL {
			is_small_ = false;
		}
	}
	constexpr combo_uninitialized_iterator_t(ptr_iterator_t<std::remove_const_t<T>> it) noexcept
		requires std::is_const_v<T>
		: large{it.get()}
	{
		if CTP_IS_CONSTEVAL {
			is_small_ = false;
		}
	}
};

template <
	typename T,
	bool HasLargeMode,
	bool ConstexprFriendly,
	bool CanDefaultConstructInConstantExpressions>
struct iterator_selector {
	static constexpr bool CanDefaultConstruct =
		CanDefaultConstructInConstantExpressions && std::is_default_constructible_v<T>;
	using type = std::conditional_t<
		std::is_trivial_v<T> || !ConstexprFriendly || CanDefaultConstruct,
		ptr_iterator_t<T>,
		std::conditional_t<
			HasLargeMode,
			combo_uninitialized_iterator_t<T>,
			std::conditional_t<std::is_const_v<T>,
				const_uninitialized_item_iterator<std::remove_const_t<T>>,
				uninitialized_item_iterator<T>>>>;
};
template <class T, bool HasLargeMode, bool ConstexprFriendly, bool CanDefaultConstructInConstantExpressions>
using iterator_selector_t = typename iterator_selector<T,
                                                       HasLargeMode,
                                                       ConstexprFriendly,
                                                       CanDefaultConstructInConstantExpressions>::type;

namespace detail {

class const_iterator_converter {
	template <class T2, std::size_t S2, class A2, SmallStorageOptions O2>
	friend class container;

	template <typename ConstIterator>
	static constexpr auto cast(ConstIterator it) {
		return it.to_non_const();
	}
};

// Get the max number that can be represented with i bits.
consteval std::uint64_t bits_max(std::uint64_t i) {
	ctpAssert(i < sizeof(std::uint64_t) * 8);
	return (std::uint64_t(1) << i) - 1;
}

template <std::size_t SmallCapacity, bool HasLargeMode>
struct small_size_type {
	using type =
		std::conditional_t<SmallCapacity <= bits_max(HasLargeMode ? 7 : 8),
			std::uint8_t,
			std::conditional_t<SmallCapacity <= bits_max(HasLargeMode ? 15 : 16),
				std::uint16_t,
				std::conditional_t<SmallCapacity <= bits_max(HasLargeMode ? 31 : 32), std::uint32_t, std::uint64_t>>>;
};
template <std::size_t SmallCapacity, bool HasLargeMode>
using small_size_type_t = typename small_size_type<SmallCapacity, HasLargeMode>::type;

template <std::size_t SmallCapacity, typename LargeSizeType = void>
struct shared_size_traits {
	using small_type = small_size_type_t<SmallCapacity, !std::is_void_v<LargeSizeType>>;

	using type = std::conditional_t<std::is_void_v<LargeSizeType>, small_type, LargeSizeType>;
	static constexpr std::align_val_t alignment{alignof(type)};
	static_assert(sizeof(small_type) <= sizeof(type),
		"Creating storage where the number of items that can be held in small mode is greater than can be held in large mode.");
};

// Makes it so we can use std::construct_at and std::destroy_at within a union.
// I think this is technically not supposed to be allowed, but is currently
// supported by MSVC, clang, and gcc.
template <typename T, std::size_t Size>
struct array_wrapper { T elems[Size]; };

// Non-trivial types that can be default constructed within constant expressions.
template <typename T, std::size_t SmallCapacity, bool NeedsConstexprHelp>
struct small_data {
	// Storage that will be uninitialized at run time, but can be default initialized at compile time.
	uninitialized_item<array_wrapper<T, SmallCapacity>> data;

	using value_type = T;
	using iterator = ptr_iterator_t<T>;
	using const_iterator = const ptr_iterator_t<const T>;

	constexpr small_data() noexcept {
		if CTP_IS_CONSTEVAL {
			// Default construct all elements in constant expressions.
			std::construct_at(&data.item);
		}
	}

	constexpr ~small_data() noexcept {
		if CTP_IS_CONSTEVAL {
			std::destroy_at(&data.item);
		}
	}

	constexpr iterator begin() noexcept { return data.item.elems; }
	constexpr const_iterator begin() const noexcept { return data.item.elems; }
};

// Non-trivial types, explicitly constexpr-friendly version if default construction is not available.
template <typename T, std::size_t SmallCapacity>
struct small_data<T, SmallCapacity, true> {
	uninitialized_item<T> data[SmallCapacity];

	using value_type = T;
	using iterator = uninitialized_item_iterator<T>;
	using const_iterator = const_uninitialized_item_iterator<T>;

	constexpr iterator begin() noexcept { return data; }
	constexpr const_iterator begin() const noexcept { return data; }
};

template <typename T, bool ConstexprFriendly, bool AllowDefaultConstructionInConstantExpressions>
struct small_data_needs_constexpr_helper {
	static constexpr bool value =
		ConstexprFriendly &&
		!std::is_trivial_v<T> &&
		(!AllowDefaultConstructionInConstantExpressions || !std::is_default_constructible_v<T>);


};
template <class T, bool ConstexprFriendly, bool CanDefaultConstructInConstantExpressions>
static constexpr bool small_data_needs_constexpr_helper_v = small_data_needs_constexpr_helper<
	T,
	ConstexprFriendly,
	CanDefaultConstructInConstantExpressions>::value;


template <typename T, typename SizeType>
struct large_data {
	T* data;
	SizeType capacity;

	using value_type = T;
	using iterator = ptr_iterator_t<T>;
	using const_iterator = ptr_iterator_t<const T>;

	constexpr const T* get(std::size_t index = 0) const noexcept { return data + index; }
	constexpr T* get(std::size_t index = 0) noexcept { return data + index; }
	constexpr iterator begin() noexcept { return data; }
	constexpr const_iterator begin() const noexcept { return data; }
};


// Unified construction function for small_data and large_data.
// Note: still performs allocator construction for trivial types (see "default insertable")
// Allocators can customize their construct call to specify default construction behaviour.
template <typename DataT, typename Alloc, typename... Args>
constexpr auto do_construct_at(std::size_t index, Alloc& alloc, DataT& data, Args&&... args) noexcept {
	using T = typename std::allocator_traits<Alloc>::value_type;

	if CTP_IS_CONSTEVAL {
		if constexpr (std::same_as<T*, DataT>) {
			// T* from large_data.
			std::uninitialized_construct_using_allocator(data + index, alloc, std::forward<Args>(args)...);
			return data + index;
		} else if constexpr (std::is_pointer_v<std::decay_t<DataT>>) {
			// uninitialized_item<T>[].
			auto ptr = std::addressof(data[index].item);
			std::uninitialized_construct_using_allocator(ptr, alloc, std::forward<Args>(args)...);
			return ptr;
		} else {
			// uninitialized_item<array_wrapper<T, Size>> and elements are already constructed.
			data.item.elems[index] = std::make_obj_using_allocator<T>(alloc, std::forward<Args>(args)...);
			return data.item.elems + index;
		}
	} else {
		// Always do uninitialized construct at runtime.
		if constexpr (is_instantiation_of_v<uninitialized_item, DataT>) {
			auto ptr = data.item.elems + index;
			std::uninitialized_construct_using_allocator(ptr, alloc, std::forward<Args>(args)...);
			return ptr;
		} else {
			auto ptr = reinterpret_cast<T*>(data) + index;
			std::uninitialized_construct_using_allocator(ptr, alloc, std::forward<Args>(args)...);
			return ptr;
		}
	}
}

// Unified destruction function for small_data and large_data.
template <typename DataT, typename Alloc>
constexpr void do_destroy_at(std::size_t index, Alloc& alloc, DataT& data) noexcept {
	using T = typename std::allocator_traits<Alloc>::value_type;
	if CTP_IS_CONSTEVAL {
		if constexpr (std::same_as<T*, DataT>) {
			// T* from large_data.
			std::allocator_traits<Alloc>::destroy(alloc, data + index);
		} else if constexpr (std::is_pointer_v<std::decay_t<DataT>>) {
			// uninitialized_item<T>[].
			std::allocator_traits<Alloc>::destroy(alloc, std::addressof(data[index].item));
		} else {
			// uninitialized_item<array_wrapper<T, Size>> and elements are already constructed.
			std::allocator_traits<Alloc>::destroy(alloc, data.item.elems + index);

			// Keep a default-constructed element there to keep the array from entering an odd state.
			std::construct_at(data.item.elems + index);
		}
	} else {
		// Always do plain destroy at runtime.
		if constexpr (is_instantiation_of_v<uninitialized_item, DataT>) {
			std::allocator_traits<Alloc>::destroy(alloc, data.item.elems + index);
		} else {
			std::allocator_traits<Alloc>::destroy(alloc, reinterpret_cast<T*>(data) + index);
		}
	}
}

template <typename Allocator>
constexpr auto do_allocate(Allocator& alloc, std::size_t n) CTP_NOEXCEPT_ALLOCS {
#ifdef __cpp_lib_allocate_at_least
	return std::allocator_traits<Allocator>::allocate_at_least(alloc, n);
#else
	struct allocation_result {
		T* ptr;
		std::size_t count;
	};
	return allocation_result{std::allocator_traits<Allocator>::allocate(alloc, n), n};
#endif
}

// Expects the caller to set size with large mode bit set after this call (possibly after constructing new items).
// Returns new large_data
template <typename T, typename SmallData, typename SizeType, typename Allocator>
constexpr large_data<T, SizeType> switch_to_large_storage(
	SmallData& smallData,
	SizeType currSize,
	SizeType requestedCapacity,
	Allocator& allocator) CTP_NOEXCEPT_ALLOCS
{
	auto [ptr, capacity] = do_allocate(allocator, requestedCapacity);
	auto it = smallData.begin();
	for (SizeType i = 0; i < currSize; ++i, ++it) {
		std::uninitialized_construct_using_allocator(ptr + i, allocator, std::move(*it));
		do_destroy_at(i, allocator, smallData.data);
	}

	return {ptr, capacity};
}

// Expects the caller to set size with large mode bit set after this call (possibly after constructing new items).
// Returns pointer to one after last valid item in the new storage.
template <typename T, typename SizeType, typename Allocator>
constexpr void reallocate_large_storage(
	large_data<T, SizeType>& large,
	SizeType currSize,
	SizeType requestedCapacity,
	Allocator& allocator) CTP_NOEXCEPT_ALLOCS
{
	auto [ptr, capacity] = do_allocate(allocator, requestedCapacity);

	for (SizeType i = 0; i < currSize; ++i) {
		std::uninitialized_construct_using_allocator(ptr + i, allocator, std::move(large.data[i]));
		do_destroy_at(i, allocator, large.data);
	}

	std::allocator_traits<Allocator>::deallocate(allocator, large.data, large.capacity);

	large.data = ptr;
	large.capacity = capacity;
}

enum class OverwriteMode { Copy, Move };
template <OverwriteMode Mode, typename SizeType, typename Storage>
constexpr void overwrite_storage(
	SizeType oldSize,
	std::type_identity_t<SizeType> newSize,
	auto& alloc, // allocator for constructing new items in this
	[[maybe_unused]] auto& cleanupAlloc, // allocator for cleaning up extra, old items in this
	Storage& smallOrLarge, // storage_.small or storage_.large for this
	[[maybe_unused]] auto& otherCleanupAlloc, // allocator for cleaning up items in other
	auto& smallOrLargeOther) noexcept(std::is_nothrow_move_constructible_v<typename Storage::value_type>)
{
	SizeType i = 0;

	auto oIt = smallOrLargeOther.begin();

	// Reuse any elements with overlapping lifetimes.
	{
		const auto sharedSize = oldSize < newSize ? oldSize : newSize;
		auto it = smallOrLarge.begin();
		for (; i < sharedSize; ++i) {
			if constexpr (Mode == OverwriteMode::Move) {
				it[i] = std::move(oIt[i]);
				do_destroy_at(i, otherCleanupAlloc, smallOrLargeOther.data);
			} else {
				it[i] = oIt[i];
			}
		}
	}

	// If oldSize is larger, there may be some elements left to destroy.
	for (; i < oldSize; ++i)
		do_destroy_at(i, cleanupAlloc, smallOrLarge.data);

	// If size is larger, we need to construct the rest.
	for (; i < newSize; ++i) {
		if constexpr (Mode == OverwriteMode::Move) {
			do_construct_at(i, alloc, smallOrLarge.data, std::move(oIt[i]));
			do_destroy_at(i, otherCleanupAlloc, smallOrLargeOther.data);
		} else {
			do_construct_at(i, alloc, smallOrLarge.data, oIt[i]);
		}
	}
}

template <typename SizeType>
constexpr void copy_overwrite_storage(
	SizeType oldSize,
	std::type_identity_t<SizeType> newSize,
	auto& alloc,
	auto& smallOrLarge,
	auto& smallOrLargeOther)
	CTP_NOEXCEPT(noexcept(
		overwrite_storage<OverwriteMode::Copy>(oldSize, newSize, alloc, alloc, smallOrLarge, alloc, smallOrLargeOther)))
{
	overwrite_storage<OverwriteMode::Copy>(oldSize, newSize, alloc, alloc, smallOrLarge, alloc, smallOrLargeOther);
}

template <typename SizeType>
constexpr void move_overwrite_storage(
	SizeType oldSize,
	std::type_identity_t<SizeType> newSize,
	auto& alloc, // allocator for constructing new items in this
	auto& cleanupAlloc, // allocator for cleaning up extra, old items in this
	auto& smallOrLarge, // storage_.small or storage_.large for this
	auto& otherCleanupAlloc, // allocator for cleaning up items in other
	auto& smallOrLargeOther)
	CTP_NOEXCEPT(noexcept(
		overwrite_storage<OverwriteMode::Move>(oldSize, newSize, alloc, cleanupAlloc, smallOrLarge, otherCleanupAlloc, smallOrLargeOther)))
{
	overwrite_storage<OverwriteMode::Move>(oldSize, newSize, alloc, cleanupAlloc, smallOrLarge, otherCleanupAlloc, smallOrLargeOther);
}

template <typename SizeType, typename Storage>
constexpr void swap_storage_elements(
	SizeType thisSize,
	std::type_identity_t<SizeType> oSize,
	Storage& smallOrLargeThis, // storage_.small or storage_.large for this
	auto& smallOrLargeOther, // storage_.small or storage_.large for o
	auto& nextAllocForThis,
	auto& nextAllocForOther,
	auto& thisCleanupAlloc,
	auto& otherCleanupAlloc)
	CTP_NOEXCEPT(
		std::is_nothrow_move_constructible_v<typename Storage::value_type> &&
		std::is_nothrow_move_assignable_v<typename Storage::value_type> &&
		std::is_nothrow_swappable_v<typename Storage::value_type>
	)
{
	SizeType i = 0;

	auto it = smallOrLargeThis.begin();
	auto oIt = smallOrLargeOther.begin();

	// Swap any elements with overlapping lifetimes.
	{
		using std::swap;
		const auto sharedSize = oSize < thisSize ? oSize : thisSize;
		for (; i < sharedSize; ++i)
			swap(it[i], oIt[i]);
	}

	// If oSize is larger, move elements into this.
	for (; i < oSize; ++i) {
		do_construct_at(i, nextAllocForThis, smallOrLargeThis.data, std::move(oIt[i]));
		do_destroy_at(i, otherCleanupAlloc, smallOrLargeOther.data);
	}

	// If thisSize is larger, move elements into other.
	for (; i < thisSize; ++i) {
		do_construct_at(i, nextAllocForOther, smallOrLargeOther.data, std::move(it[i]));
		do_destroy_at(i, thisCleanupAlloc, smallOrLargeThis.data);
	}
}

template <typename GrowthPolicy, typename Storage, typename Allocator, typename... Args>
constexpr auto do_insert(Storage& storage, Allocator& alloc, auto pos, std::size_t numItems, Args&&... args) {
	using size_type = typename Storage::size_type;

	if (numItems <= 0) [[unlikely]]
		return pos;

	const auto insertionPoint = static_cast<size_type>(std::distance(storage.begin(), pos));

	constexpr bool IsAssignable =
		sizeof...(args) == 1 &&
		std::is_assignable_v<typename Storage::value_type, Args...>;

	const auto oldSize = storage.size();
	const auto newSize = oldSize + numItems;
	ctpExpects(newSize <= storage.max_size());

	if constexpr (storage.HasLargeMode) {
		const auto capacity = storage.capacity();
		if (newSize > capacity) {
			const auto requestedCapacity = GrowthPolicy::apply(capacity, newSize, storage.max_size());
			auto [ptr, newCapacity] = do_allocate(alloc, requestedCapacity);

			const auto onFail = ctp::ScopeFail{[&] {
				std::allocator_traits<Allocator>::deallocate(alloc, ptr, newCapacity);
			}};

			const auto move_items_and_emplace = [&](auto& smallOrLarge) {
				auto it = smallOrLarge.begin();
				size_type i = 0;
				for (; i < insertionPoint; ++i) {
					do_construct_at(i, alloc, ptr, std::move(it[i]));
					do_destroy_at(i, alloc, smallOrLarge.data);
				}

				for (size_type oneBeforeInsertEnd = insertionPoint + numItems - 1; i < oneBeforeInsertEnd; ++i)
					do_construct_at(i, alloc, ptr, args...);
				do_construct_at(i++, alloc, ptr, std::forward<Args>(args)...);

				for (; i < newSize; ++i) {
					do_construct_at(i, alloc, ptr, std::move(it[i - numItems]));
					do_destroy_at(i - numItems, alloc, smallOrLarge.data);
				}
			};

			if (storage.is_small_mode()) {
				move_items_and_emplace(storage.small);

				std::destroy_at(&storage.small);
				std::construct_at(&storage.large);
				storage.large.data = ptr;
				storage.large.capacity = static_cast<size_type>(newCapacity);
			} else {
				move_items_and_emplace(storage.large);

				std::allocator_traits<Allocator>::deallocate(alloc, storage.large.data, storage.large.capacity);
				storage.large.data = ptr;
				storage.large.capacity = static_cast<size_type>(newCapacity);
			}

			storage.set_size(newSize, Mode::Large);
			return storage.begin() + insertionPoint;
		}
	}

	const auto shift_items_and_emplace = [&](auto& smallOrLarge) {
		// Move items after the insertion point over by numItems.
		auto it = smallOrLarge.begin();
		const size_type insertionEnd = static_cast<size_type>(insertionPoint + numItems);
		auto i = static_cast<size_type>(newSize);

		// Move construct any items in previously-unused storage.
		for (; i > oldSize && i > insertionEnd; --i)
			do_construct_at(i - 1, alloc, smallOrLarge.data, std::move(it[i - 1 - numItems]));

		// Move-assign items in any overlapping storage.
		for (; i > insertionEnd; --i)
			it[i - 1] = std::move(it[i - 1 - numItems]);

		// Inserting items starts.

		for (; i > oldSize && i > insertionPoint + 1; --i)
			do_construct_at(i - 1, alloc, smallOrLarge.data, args...);

		if (i <= oldSize) {
			for (; i > insertionPoint + 1; --i) {
				if constexpr (IsAssignable) {
					it[i - 1] = (args,...); // There's one arg, so we can expand with the comma operator.
				} else {
					it[i - 1] = typename Storage::value_type{args...};
				}
			}
			if constexpr (IsAssignable) {
				it[insertionPoint] = (std::forward<Args>(args),...);  // One arg, can expand with comma.
			} else {
				it[insertionPoint] = typename Storage::value_type{std::forward<Args>(args)...};
			}
		} else {
			do_construct_at(insertionPoint, alloc, smallOrLarge.data, std::forward<Args>(args)...);
		}
	};

	if constexpr (storage.HasLargeMode) {
		if (!storage.is_small_mode()) {
			shift_items_and_emplace(storage.large);
			storage.set_size(newSize, Mode::Large);
			return pos;
		}
	}
	shift_items_and_emplace(storage.small);
	storage.set_size(newSize, Mode::Small);
	return pos;
}

template <typename GrowthPolicy, typename Storage, typename Allocator, std::forward_iterator FwdIt>
constexpr auto do_insert_fwd_impl(Storage& storage, Allocator& alloc, auto pos, FwdIt first, FwdIt last) {
	using size_type = typename Storage::size_type;
	const auto insertionPoint = static_cast<size_type>(std::distance(storage.begin(), pos));

	const auto oldSize = storage.size();
	const auto numItems = static_cast<size_type>(std::distance(first, last));
	const auto newSize = static_cast<size_type>(oldSize + numItems);
	ctpExpects(newSize <= storage.max_size());

	if constexpr (storage.HasLargeMode) {
		const auto capacity = storage.capacity();
		if (newSize > capacity) {
			const auto requestedCapacity = GrowthPolicy::apply(capacity, newSize, storage.max_size());
			auto [ptr, newCapacity] = do_allocate(alloc, requestedCapacity);

			const auto onFail = ctp::ScopeFail{[&] {
				std::allocator_traits<Allocator>::deallocate(alloc, ptr, newCapacity);
			}};

			const auto move_items_and_emplace = [&](auto& smallOrLarge) {
				auto it = smallOrLarge.begin();
				size_type i = 0;
				for (; i < insertionPoint; ++i) {
					do_construct_at(i, alloc, ptr, std::move(it[i]));
					do_destroy_at(i, alloc, smallOrLarge.data);
				}

				for (; i < insertionPoint + numItems; ++i, ++first)
					do_construct_at(i, alloc, ptr, *first);

				for (; i < newSize; ++i) {
					do_construct_at(i, alloc, ptr, std::move(it[i - numItems]));
					do_destroy_at(i - numItems, alloc, smallOrLarge.data);
				}
			};

			if (storage.is_small_mode()) {
				move_items_and_emplace(storage.small);

				std::destroy_at(&storage.small);
				std::construct_at(&storage.large);
				storage.large.data = ptr;
				storage.large.capacity = static_cast<size_type>(newCapacity);
			} else {
				move_items_and_emplace(storage.large);

				std::allocator_traits<Allocator>::deallocate(alloc, storage.large.data, storage.large.capacity);
				storage.large.data = ptr;
				storage.large.capacity = static_cast<size_type>(newCapacity);
			}

			storage.set_size(newSize, Mode::Large);
			return storage.begin() + insertionPoint;
		}
	}

	const auto shift_items_and_emplace = [&](auto& smallOrLarge) {
		// Move items after the insertion point over by numItems.
		auto it = smallOrLarge.begin();
		const size_type insertionEnd = insertionPoint + numItems;
		size_type i = newSize;

		// Move construct any items in previously-unused storage.
		for (; i > oldSize && i > insertionEnd; --i)
			do_construct_at(i - 1, alloc, smallOrLarge.data, std::move(it[i - 1 - numItems]));

		// Move-assign items in any overlapping storage.
		for (; i > insertionEnd; --i)
			it[i - 1] = std::move(it[i - 1 - numItems]);

		// Inserting items starts. Switch to forward iteration to not require bidi iterators.

		for (i = insertionPoint; i < oldSize && i < insertionEnd; ++i, ++first)
			it[i] = *first;

		if (i >= oldSize) {
			for (; i < insertionEnd; ++i, ++first)
				do_construct_at(i, alloc, smallOrLarge.data, *first);
		}
	};

	if constexpr (storage.HasLargeMode) {
		if (!storage.is_small_mode()) {
			shift_items_and_emplace(storage.large);
			storage.set_size(newSize, Mode::Large);
			return pos;
		}
	}
	shift_items_and_emplace(storage.small);
	storage.set_size(newSize, Mode::Small);
	return pos;
}

// If using an input iterator, insert items one at a time.
template <typename GrowthPolicy, typename Storage, typename Allocator, std::input_iterator It>
constexpr auto do_insert_input_impl(Storage& storage, Allocator& alloc, auto pos, It first, It last) {
	const auto firstInserted = do_insert<GrowthPolicy>(storage, alloc, pos, 1, *first);
	const auto insertionPoint = static_cast<typename Storage::size_type>(std::distance(storage.begin(), firstInserted));

	++first;
	for (pos = firstInserted; first != last; ++first)
		pos = do_insert<GrowthPolicy>(storage, alloc, pos + 1, 1, *first);

	return storage.begin() + insertionPoint;
}

template <typename GrowthPolicy, typename Storage, typename Allocator, typename It>
constexpr auto do_insert_with_iterators(Storage& storage, Allocator& alloc, auto pos, It first, It last) {
	using size_type = typename Storage::size_type;

	if (first == last) [[unlikely]] {
		return pos;
	}

	if constexpr (std::forward_iterator<It>) {
		return do_insert_fwd_impl<GrowthPolicy>(storage, alloc, pos, first, last);
	} else {
		static_assert(std::input_iterator<It>);
		return do_insert_input_impl<GrowthPolicy>(storage, alloc, pos, first, last);
	}
}

template <typename Storage, typename Allocator, typename It>
constexpr auto do_erase(Storage& storage, Allocator& alloc, It first, It last)
	noexcept(std::is_nothrow_move_assignable_v<typename Storage::value_type>)
{
	using size_type = typename Storage::size_type;

	if (first == last) [[unlikely]] {
		return last;
	}

	using size_type = typename Storage::size_type;

	const auto deletionStart = static_cast<size_type>(std::distance<It>(storage.begin(), first));
	const auto numItems = static_cast<size_type>(std::distance<It>(first, last));
	const auto oldSize = storage.size();
	const size_type newSize = oldSize - numItems;

	const auto erase_elements = [&](auto& smallOrLarge) {
		auto it = smallOrLarge.begin();
		size_type i = deletionStart;

		for (; i < newSize; ++i)
			it[i] = std::move(it[i + numItems]);

		// Delete any extra.
		for (; i < oldSize; ++i)
			do_destroy_at(i, alloc, smallOrLarge.data);

		return (std::min)(deletionStart, newSize);
	};

	if constexpr (storage.HasLargeMode) {
		if (!storage.is_small_mode()) {
			const auto result = erase_elements(storage.large);
			storage.set_size(newSize, Mode::Large);
			return storage.begin() + result;
		}
	}

	const auto result = erase_elements(storage.small);
	storage.set_size(newSize, Mode::Small);
	return storage.begin() + result;
}

template <typename GrowthPolicy, typename Storage, typename Allocator>
constexpr void do_assign(Storage& storage, Allocator& alloc, std::size_t count, const auto& value) {
	using size_type = typename Storage::size_type;
	const auto oldSize = storage.size();

	Mode mode = Mode::Small;

	if constexpr (storage.HasLargeMode) {
		const auto [newMode, didResize] = resize_for_overwrite(storage, static_cast<size_type>(count), alloc);
		if (didResize) {
			for (size_type i = 0; i < count; ++i)
				do_construct_at(i, alloc, storage.large.data, value);
			return;
		}

		mode = newMode;
	}

	storage.set_size(count, mode);

	size_type i = 0;

	{
		auto it = storage.begin();
		// Write over any existing items.
		for (; i < oldSize && i < count; ++i)
			it[i] = value;
	}

	if constexpr (storage.HasLargeMode) {
		if (mode == Mode::Large) {
			for (; i < count; ++i)
				do_construct_at(i, alloc, storage.large.data, value);
			for (; i < oldSize; ++i)
				do_destroy_at(i, alloc, storage.large.data);
			return;
		}
	}

	for (; i < count; ++i)
		do_construct_at(i, alloc, storage.small.data, value);
	for (; i < oldSize; ++i)
		do_destroy_at(i, alloc, storage.small.data);
}

template <typename Storage, typename Allocator, std::forward_iterator It>
constexpr void do_assign_fwd_impl(Storage& storage, Allocator& alloc, It first, It last) {
	using size_type = typename Storage::size_type;
	const auto oldSize = storage.size();
	const auto count = static_cast<size_type>(std::distance<It>(first, last));

	[[maybe_unused]] const auto [mode, didResize] = resize_for_overwrite(storage, count, alloc);

	if constexpr (storage.HasLargeMode) {
		if (didResize) {
			for (std::size_t i = 0; i < count; ++i, ++first)
				do_construct_at(i, alloc, storage.large.data, *first);
			return;
		}
	}

	size_type i = 0;

	{
		auto it = storage.begin();
		// Write over any existing items.
		for (; i < oldSize && i < count; ++i, ++first)
			it[i] = *first;
	}

	if constexpr (storage.HasLargeMode) {
		if (mode == Mode::Large) {
			for (; i < count; ++i, ++first)
				do_construct_at(i, alloc, storage.large.data, *first);
			for (; i < oldSize; ++i)
				do_destroy_at(i, alloc, storage.large.data);
			return;
		}
	}

	for (; i < count; ++i, ++first)
		do_construct_at(i, alloc, storage.small.data, *first);
	for (; i < oldSize; ++i)
		do_destroy_at(i, alloc, storage.small.data);
}

template <typename GrowthPolicy, typename Storage, typename Allocator, std::input_iterator It>
constexpr void do_assign_input_impl(Storage& storage, Allocator& alloc, It first, It last) {
	destroy_elements(storage, alloc);
	for (; first != last; ++first)
		construct_one<GrowthPolicy>(storage, alloc, *first);
}

template <typename GrowthPolicy, typename Storage, typename Allocator, typename It>
constexpr void do_assign_with_iterators(Storage& storage, Allocator& alloc, It first, It last) {
	if constexpr (std::forward_iterator<It>) {
		do_assign_fwd_impl(storage, alloc, first, last);
	} else {
		static_assert(std::input_iterator<It>);
		do_assign_input_impl<GrowthPolicy>(storage, alloc, first, last);
	}
}

template <typename T, bool HasLargeMode, typename LargeSizeType>
constexpr std::size_t GetSmallCapacity(std::size_t minSmallCapacity) noexcept {
	if constexpr (!HasLargeMode)
		return minSmallCapacity;

	const auto itemSize = sizeof(T);
	const auto smallSize = minSmallCapacity * itemSize;
	// Note: conditional here is just to make over-eager compilers happy to avoid sizeof(void).
	const auto largeSize = sizeof(std::conditional_t<std::is_void_v<LargeSizeType>, std::size_t, LargeSizeType>);

	std::size_t max = minSmallCapacity;
	if (largeSize > smallSize)
		max += static_cast<std::size_t>((largeSize - smallSize) / itemSize);

	return max;
};

// If LargeSizeType is void, then there is static storage only.
template <typename T, std::size_t InSmallCapacity, typename LargeSizeType = void>
class small_container_storage_base {
public:
	static_assert(std::unsigned_integral<LargeSizeType> || std::is_void_v<LargeSizeType>);
	static constexpr bool HasLargeMode = !std::is_void_v<LargeSizeType>;

	static constexpr std::size_t SmallCapacity = InSmallCapacity;

	using size_traits = shared_size_traits<SmallCapacity, LargeSizeType>;
	using shared_size_type = typename size_traits::type;
	using small_size_type = typename size_traits::small_type;
	using value_type = T;

	static constexpr std::byte LargeModeBitMask{0b1000'0000};
	static constexpr std::byte SizeBitMask{~LargeModeBitMask};
	static constexpr std::size_t SizeBitByteIndex = std::endian::native == std::endian::big ?
		sizeof(shared_size_type) - 1 :
		0; // Assume little endian by default.

private:
	alignas(size_traits::alignment) std::byte size_bytes_[sizeof(shared_size_type)]{};

	// bit_cast cannot return a plain array directly.
	// May be able to revisit if e.g. reinterpret_cast gets some valid constexpr behaviour.
	constexpr void write_size_workaround(std::size_t size) {
		struct BitCastWorkaround {
			std::byte bytes[sizeof(shared_size_type)];
		};

		const auto workaround = std::bit_cast<BitCastWorkaround>(static_cast<shared_size_type>(size));
		std::copy_n(workaround.bytes, sizeof(shared_size_type), size_bytes_);
	}
public:
	[[nodiscard]] constexpr bool is_small_mode() const noexcept
		requires HasLargeMode
	{
		return (size_bytes_[SizeBitByteIndex] & LargeModeBitMask) == std::byte{0};
	}
	[[nodiscard]] constexpr Mode get_mode() const noexcept {
		if constexpr (HasLargeMode) {
			return is_small_mode() ? Mode::Small : Mode::Large;
		} else {
			return Mode::Small;
		}
	}

	constexpr void set_size(std::size_t size, [[maybe_unused]] Mode mode = Mode::Small) noexcept {
		ctpAssert((mode == Mode::Small && size <= SmallCapacity) || (mode == Mode::Large && size <= max_size()));

		write_size_workaround(size);

		if constexpr (HasLargeMode) {
			if (mode == Mode::Large)
				size_bytes_[SizeBitByteIndex] |= LargeModeBitMask;
		}
	}

	[[nodiscard]] constexpr shared_size_type size() const noexcept {
		if constexpr (HasLargeMode) {
			decltype(size_bytes_) bytes;
			std::copy(size_bytes_, size_bytes_ + sizeof(shared_size_type), bytes);
			bytes[SizeBitByteIndex] = bytes[SizeBitByteIndex] & SizeBitMask;
			return std::bit_cast<shared_size_type>(bytes);
		} else {
			return std::bit_cast<shared_size_type>(size_bytes_);
		}
	}

	[[nodiscard]] constexpr std::size_t max_size() const noexcept {
		if constexpr (HasLargeMode) {
			// Large size type still needs the reserved small/large bit.
			constexpr auto capacity = bits_max(sizeof(LargeSizeType) * 8 - 1);
			return capacity;
		} else {
			return SmallCapacity;
		}
	}
};

// Dynamic size; can allocate to grow.
template <
	typename T,
	std::size_t SmallCapacity,
	typename LargeSizeType,
	typename Iterator,
	typename ConstIterator,
	bool SmallDataNeedsConstexprHelp>
class small_container_storage
	: public small_container_storage_base<T, SmallCapacity, LargeSizeType>
{
public:
	using Base = small_container_storage_base<T, SmallCapacity, LargeSizeType>;
	using size_type = typename Base::shared_size_type;
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	static_assert(sizeof(Base::small_size_type) < sizeof(LargeSizeType),
		"Creating small_container_storage where the large mode can't contain more items than the small mode.");

public:
#ifdef __cpp_lib_within_lifetime
	#warning "Can possibly optimize small_storage for better byte packing by moving size into small and large types (p2641)."
		//struct small_size {
		//	// small_size_t size; // Includes mode bit. May allow better packing. May allow slightly larger than requested capacity.
		//	// T data[NumItems];
		//};
		//struct large_size {
		//	std::size_t size; // large size that includes mode bit
		//	std::size_t capacity;
		//	// T* data;
		//};
		//union Storage {
		//	small_size small;
		//	large_size large;
		//};
		// Test which member is active in constant expressions and use test bit at runtime.
#endif
	using small_type = small_data<T, Base::SmallCapacity, SmallDataNeedsConstexprHelp>;
	union {
		small_type small;
		large_data<T, size_type> large;
	};

	constexpr small_container_storage() noexcept {
		std::construct_at(&small);
	}
	constexpr ~small_container_storage() noexcept {
		// We don't know what type we'll be internally, so any required cleanup must happen externally.
	}

	[[nodiscard]] constexpr size_type capacity() const noexcept {
		if (this->is_small_mode())
			return Base::SmallCapacity;
		else
			return large.capacity;
	}

	[[nodiscard]] constexpr iterator begin() noexcept {
		if (this->is_small_mode())
			return small.begin();
		else
			return large.begin();
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		if (this->is_small_mode())
			return small.begin();
		else
			return large.begin();
	}

	[[nodiscard]] constexpr iterator end() noexcept {
		if (this->is_small_mode())
			return small.begin() + this->size();
		else
			return large.begin() + this->size();
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		if (this->is_small_mode())
			return small.begin() + this->size();
		else
			return large.begin() + this->size();
	}
};

// Static size, no large mode.
template <
	typename T,
	std::size_t SmallCapacity,
	typename Iterator,
	typename ConstIterator,
	bool SmallDataNeedsConstexprHelp>
class small_container_storage<T, SmallCapacity, void, Iterator, ConstIterator, SmallDataNeedsConstexprHelp>
	: public small_container_storage_base<T, SmallCapacity>
{
	using Base = small_container_storage_base<T, SmallCapacity>;
public:
	using size_type = typename Base::shared_size_type;
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	small_data<T, Base::SmallCapacity, SmallDataNeedsConstexprHelp> small;

	[[nodiscard]] constexpr std::size_t capacity() const noexcept { return Base::SmallCapacity; }

	[[nodiscard]] constexpr iterator begin() noexcept { return small.begin(); }
	[[nodiscard]] constexpr const_iterator begin() const noexcept { return small.begin(); }

	[[nodiscard]] constexpr iterator end() noexcept { return small.begin() + this->size(); }
	[[nodiscard]] constexpr const_iterator end() const noexcept { return small.begin() + this->size(); }
};

template <typename SizeType>
struct get_storage_for_n_result {
	Mode mode;
	SizeType newItemIndex;
};

// Get a pointer to the start of storage for numItems. Sets the size and switches to large
// mode if neccessary.
template <typename GrowthPolicy, typename Storage, typename Allocator>
[[nodiscard]] constexpr auto get_storage_for_n(Storage& storage, typename Storage::size_type numItems, Allocator& alloc)
	-> get_storage_for_n_result<typename Storage::size_type>
{
	const auto currentSize = storage.size();
	if constexpr (!storage.HasLargeMode) {
		ctpExpects(currentSize + numItems <= storage.capacity());
		storage.set_size(currentSize + numItems);
		return {Mode::Small, currentSize};
	} else {
		const auto requiredSize = currentSize + numItems;
		const auto capacity = storage.capacity();

		if (requiredSize <= capacity) {
			if (storage.is_small_mode()) {
				storage.set_size(requiredSize, Mode::Small);
				return {Mode::Small, currentSize};
			}
			storage.set_size(requiredSize, Mode::Large);
			return {Mode::Large, currentSize};
		}

		const auto newCapacity = GrowthPolicy::apply(capacity, requiredSize, storage.max_size());

		ctpAssert(newCapacity <= storage.max_size());

		const bool isSmallMode = storage.is_small_mode();
		storage.set_size(requiredSize, Mode::Large);

		if (isSmallMode) {
			auto result = switch_to_large_storage<typename Storage::value_type>(storage.small, currentSize, newCapacity, alloc);
			std::destroy_at(&storage.small);
			std::construct_at(&storage.large, result);
			return {Mode::Large, currentSize};
		}
		reallocate_large_storage(storage.large, currentSize, newCapacity, alloc);
		return {Mode::Large, currentSize};
	}
}

template <typename Storage, typename Allocator>
constexpr void do_reserve(Storage& storage, typename Storage::size_type newCapacity, Allocator& alloc) {
	if constexpr (!storage.HasLargeMode) {
		ctpExpects(newCapacity <= storage.capacity());
	} else {
		if (newCapacity <= storage.capacity())
			return;

		ctpAssert(newCapacity <= storage.max_size());

		const bool isSmallMode = storage.is_small_mode();
		const auto size = storage.size();

		if (isSmallMode) {
			auto result = switch_to_large_storage<typename Storage::value_type>(storage.small, size, newCapacity, alloc);
			std::destroy_at(&storage.small);
			std::construct_at(&storage.large, result);

			storage.set_size(size, Mode::Large);
			return;
		}
		reallocate_large_storage(storage.large, size, newCapacity, alloc);
	}
}

template <typename Storage, typename Allocator, typename... Args>
constexpr void do_resize(Storage& storage, typename Storage::size_type newSize, Allocator& alloc, const Args&... maybeValue) {
	const auto currentSize = storage.size();
	if (newSize > currentSize) {
		do_reserve(storage, newSize, alloc);
		if constexpr (storage.HasLargeMode) {
			if (!storage.is_small_mode()) {
				for (auto i = currentSize; i < newSize; ++i)
					do_construct_at(i, alloc, storage.large.data, maybeValue...);
				storage.set_size(newSize, Mode::Large);
				return;
			}
		}

		for (auto i = currentSize; i < newSize; ++i)
			do_construct_at(i, alloc, storage.small.data, maybeValue...);
		storage.set_size(newSize, Mode::Small);
		return;
	}

	if (newSize == currentSize)
		return;

	if constexpr (storage.HasLargeMode) {
		if (!storage.is_small_mode()) {
			for (auto i = currentSize; i > newSize; --i)
				do_destroy_at(i - 1, alloc, storage.large.data);
			storage.set_size(newSize, Mode::Large);
			return;
		}
	}

	for (auto i = currentSize; i > newSize; --i)
		do_destroy_at(i - 1, alloc, storage.small.data);
	storage.set_size(newSize, Mode::Small);
}

struct resize_for_overwrite_result {
	Mode newMode;
	bool didResize;
};

// Resize the storage buffer if needed, destroying any old items if a new allocation is needed.
// Sets the size.
template <typename Storage, typename Allocator>
[[nodiscard]] constexpr resize_for_overwrite_result
resize_for_overwrite(Storage& storage, typename Storage::size_type numItems, Allocator& alloc) {
	using size_type = typename Storage::size_type;

	if constexpr (!storage.HasLargeMode) {
		ctpExpects(numItems <= storage.capacity());
		storage.set_size(numItems);
		return {Mode::Small, false};
	} else {
		if (numItems <= storage.capacity()) {
			if (storage.is_small_mode()) {
				storage.set_size(numItems, Mode::Small);
				return {Mode::Small, false};
			}
			storage.set_size(numItems, Mode::Large);
			return {Mode::Large, false};
		}

		ctpAssert(numItems <= storage.max_size());

		const auto oldSize = storage.size();
		if (storage.is_small_mode()) {
			for (size_type i = 0; i < oldSize; ++i)
				do_destroy_at(i, alloc, storage.small.data);
			std::destroy_at(&storage.small);
			std::construct_at(&storage.large);
		} else {
			for (size_type i = 0; i < oldSize; ++i)
				do_destroy_at(i, alloc, storage.large.data);
			std::allocator_traits<Allocator>::deallocate(alloc, storage.large.data, storage.large.capacity);
		}

		const auto result = do_allocate(alloc, numItems);
		storage.large.data = result.ptr;
		storage.large.capacity = result.count;
		storage.set_size(numItems, Mode::Large);
		return {Mode::Large, true};
	}
}

// Destroys elements and sets size to zero. Does not deallocate large storage or change capacity.
template <class Storage, class Alloc>
static constexpr void destroy_elements(Storage& storage, Alloc& alloc) noexcept {
	using size_type = typename Storage::size_type;

	const auto size = storage.size();
	if constexpr (Storage::HasLargeMode) {
		if (storage.is_small_mode()) {
			for (size_type i = 0; i < size; ++i)
				do_destroy_at(i, alloc, storage.small.data);
			storage.set_size(0, Mode::Small);
		} else {
			for (size_type i = 0; i < size; ++i)
				do_destroy_at(i, alloc, storage.large.data);
			storage.set_size(0, Mode::Large);
		}
	} else {
		for (size_type i = 0; i < size; ++i)
			do_destroy_at(i, alloc, storage.small.data);
		storage.set_size(0, Mode::Small);
	}
}

// Destroys all elements and deallocates memory.
// Does not set size, capacity, or reset lifetime.
template <class Storage, class Alloc>
static constexpr void destroy_and_deallocate(Storage& storage, Alloc& alloc) noexcept {
	using size_type = typename Storage::size_type;

	const auto size = storage.size();
	if constexpr (Storage::HasLargeMode) {
		const bool isSmall = storage.is_small_mode();

		if (isSmall) {
			for (size_type i = 0; i < size; ++i)
				do_destroy_at(i, alloc, storage.small.data);
		} else {
			for (size_type i = 0; i < size; ++i)
				do_destroy_at(i, alloc, storage.large.data);
			std::allocator_traits<Alloc>::deallocate(alloc, storage.large.data, storage.large.capacity);
		}
	} else {
		for (size_type i = 0; i < size; ++i)
			do_destroy_at(i, alloc, storage.small.data);
	}
}

template <class Storage, class Alloc, class... Args>
static constexpr void impl_construct_n(Storage& storage, Alloc& alloc, Mode mode, auto start, auto end, Args&&... args) {
	// Can try forwarding the last args.
	--end;

	if constexpr (Storage::HasLargeMode) {
		if (mode == Mode::Large) {
			for (; start < end; ++start)
				do_construct_at(start, alloc, storage.large.data, args...);
			do_construct_at(start, alloc, storage.large.data, std::forward<Args>(args)...);
			return;
		}
	}

	for (; start < end; ++start)
		do_construct_at(start, alloc, storage.small.data, args...);
	do_construct_at(start, alloc, storage.small.data, std::forward<Args>(args)...);
}

// Constructs n elements, setting the size and changing to large mode if necessary.
template <class GrowthPolicy, class Storage, class Alloc, class... Args>
static constexpr void construct_n(Storage& storage, Alloc& alloc, typename Storage::size_type count, Args&&... args) {
	if (count <= 0) [[unlikely]]
		return;

	const auto [mode, idx] = get_storage_for_n<GrowthPolicy>(storage, count, alloc);
	impl_construct_n(storage, alloc, mode, idx, idx + count, std::forward<Args>(args)...);
}

// Constructs one element, setting the size and changing to large mode if necessary.
template <class GrowthPolicy, class Storage, class Alloc, class... Args>
static constexpr auto& construct_one(Storage& storage, Alloc& alloc, Args&&... args) {
	const auto [mode, idx] = get_storage_for_n<GrowthPolicy>(storage, 1, alloc);
	impl_construct_n(storage, alloc, mode, idx, idx + 1, std::forward<Args>(args)...);
	return *(storage.begin() + idx);
}

template <class Storage, class Alloc, class... Args>
static constexpr void impl_construct_from_range(Storage& storage, Alloc& alloc, Mode mode, auto start, auto end, auto it) {
	if constexpr (Storage::HasLargeMode) {
		if (mode == Mode::Large) {
			for (; start < end; ++start, ++it)
				do_construct_at(+start, alloc, storage.large.data, *it);
			return;
		}
	}

	for (; start < end; ++start, ++it)
		do_construct_at(+start, alloc, storage.small.data, *it);
}

template <class GrowthPolicy, class Storage, class Alloc, class... Args>
static constexpr void construct_from_range(Storage& storage, Alloc& alloc, auto first, auto last) {
	if (first == last) [[unlikely]]
		return;

	if constexpr (std::same_as<typename std::iterator_traits<decltype(first)>::iterator_category, std::input_iterator_tag>) {
		for (; first != last; ++first) {
			const auto [mode, idx] = get_storage_for_n<GrowthPolicy>(storage, 1, alloc);
			// Because we might change to large mode in the middle, call this with one element each time.
			impl_construct_from_range(storage, alloc, mode, idx, idx + 1, first);
		}
	} else {
		// should at least have a forward iterator.
		const auto count = static_cast<typename Storage::size_type>(std::distance(first, last));
		const auto [mode, idx] = get_storage_for_n<GrowthPolicy>(storage, count, alloc);
		impl_construct_from_range(storage, alloc, mode, idx, idx + count, first);
	}
}

template <class Storage1, class Storage2>
constexpr void move_elements(Storage1& self, Storage2&& other, auto& sAlloc, auto& oAlloc, auto& newAlloc)
	CTP_NOEXCEPT(noexcept(move_overwrite_storage(0, 0, newAlloc, sAlloc, self.small, oAlloc, other.small)))
{
	auto oldSize = self.size();
	const auto newSize = other.size();

	[[maybe_unused]] const auto [mode, didResize] = resize_for_overwrite(self, newSize, newAlloc);
	if constexpr (self.HasLargeMode) {
		if (mode == Mode::Large) {
			if (didResize)
				oldSize = 0;

			if constexpr (other.HasLargeMode) {
				if (!other.is_small_mode()) {
					// Large into large.
					// To reach this case, we must have an allocator mismatch so we can't steal the pointer.
					move_overwrite_storage(oldSize, newSize, newAlloc, sAlloc, self.large, oAlloc, other.large);
					other.set_size(0, Mode::Large);
					return;
				}
			}

			// Small into large.
			move_overwrite_storage(oldSize, newSize, newAlloc, sAlloc, self.large, oAlloc, other.small);
			other.set_size(0, Mode::Small);
			return;
		}
	}

	if constexpr (other.HasLargeMode) {
		if (!other.is_small_mode()) {
			// Large into small.
			move_overwrite_storage(oldSize, newSize, newAlloc, sAlloc, self.small, oAlloc, other.large);
			other.set_size(0, Mode::Large);
			return;
		}
	}

	// Small into small.
	move_overwrite_storage(oldSize, newSize, newAlloc, sAlloc, self.small, oAlloc, other.small);
	other.set_size(0, Mode::Small);
}

// Attempt to take a large pointer, otherwise fall back to element-wise move.
template <class Storage1, class Storage2, class OtherAlloc, class NewAlloc>
constexpr void move_storage(Storage1& self, Storage2&& other, auto& sAlloc, OtherAlloc& oAlloc, NewAlloc& newAlloc)
	CTP_NOEXCEPT(noexcept(move_elements(self, other, sAlloc, oAlloc, newAlloc)))
{
	// Handle large pointer stealing cases.
	if constexpr (self.HasLargeMode && other.HasLargeMode) {
		if (!other.is_small_mode()) {
			// MSVC seems to get confused if moving pointers around during compile time.
			if CTP_IS_CONSTEVAL {
				destroy_and_deallocate(self, sAlloc);
				if (self.is_small_mode()) {
					std::destroy_at(&self.small);
					std::construct_at(&self.large);
				}

				// Make a copy of other's pointer size.
				auto ptr = std::allocator_traits<NewAlloc>::allocate(newAlloc, other.large.capacity);
				self.large.data = ptr;
				self.large.capacity = other.large.capacity;

				const auto size = other.size();
				self.set_size(size, Mode::Large);

				for (typename Storage1::size_type i = 0; i < size; ++i) {
					std::uninitialized_construct_using_allocator(
						ptr + i,
						newAlloc,
						std::move(other.large.data[i]));
					do_destroy_at(i, oAlloc, other.large.data);
				}

				// Clean up other.
				std::allocator_traits<OtherAlloc>::deallocate(oAlloc, other.large.data, other.large.capacity);
				std::destroy_at(&other.large);
				std::construct_at(&other.small);
				other.set_size(0, Mode::Small);
				return;
			}

			// If both support large mode and other is large, take other's pointer.
			destroy_and_deallocate(self, sAlloc);
			self.large = other.large;
			self.set_size(other.size(), Mode::Large);
			other.set_size(0, Mode::Small);
			return;
		}
	}

	move_elements(self, other, sAlloc, oAlloc, newAlloc);
};

} // detail

// ---------------------------------------------------------------------------------------
// small_storage::container
// ---------------------------------------------------------------------------------------

template <
	typename T,
	std::size_t MinSmallCapacity,
	typename Allocator /*= std::allocator<T>*/,
	SmallStorageOptions options /*= default_options*/>
class container {
public:
	static constexpr bool HasLargeMode = options::has_large_mode;

	using iterator = iterator_selector_t<
		T,
		HasLargeMode,
		options::constexpr_friendly,
		options::allow_default_construction_in_constant_expressions>;

	using const_iterator = iterator_selector_t<
		const T,
		HasLargeMode,
		options::constexpr_friendly,
		options::allow_default_construction_in_constant_expressions>;

	using reverse_iterator = ctp::reverse_iterator<iterator>;
	using const_reverse_iterator = ctp::reverse_iterator<const_iterator>;

	template <class T2, std::size_t S2, class A2, SmallStorageOptions O2>
	friend class container;

protected:
	// Used to dispatch copy/move constructors which would otherwise be implicitly deleted.
	struct dispatch_to_template_tag {};

	using storage_type = detail::small_container_storage<
	                         T,
	                         detail::GetSmallCapacity<T, HasLargeMode, typename options::large_size_type>(MinSmallCapacity),
	                         std::conditional_t<HasLargeMode, typename options::large_size_type, void>,
	                         iterator,
	                         const_iterator,
	                         detail::small_data_needs_constexpr_helper_v<
	                            T,
	                            options::constexpr_friendly,
	                            options::allow_default_construction_in_constant_expressions>>;
	using growth_policy = typename options::growth_policy;

	using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using alloc_traits = std::allocator_traits<rebind_alloc>;

	constexpr bool is_small_mode() const noexcept {
		if constexpr (HasLargeMode) {
			return storage_.is_small_mode();
		} else {
			return true;
		}
	}

	static constexpr iterator make_nonconst_iterator(const_iterator it) noexcept {
		return detail::const_iterator_converter::cast(it);
	}

private:
	storage_type storage_;
	CTP_NO_UNIQUE_ADDRESS rebind_alloc alloc_; // For large-mode allocs and allocator-aware storage.

	using storage_size_t = typename storage_type::shared_size_type;
protected:
	constexpr void set_allocator(const rebind_alloc& alloc) noexcept(noexcept(alloc_ = alloc)) {
		alloc_ = alloc;
	}

public:
	static constexpr std::size_t SmallCapacity = storage_type::SmallCapacity;

	using value_type = T;
	using allocator_type = Allocator;
	using size_type = std::size_t; // Use std::size_t for the main interface.
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename alloc_traits::pointer;
	using const_pointer = typename alloc_traits::const_pointer;

public:
	constexpr ~container() noexcept {
		detail::destroy_and_deallocate(storage_, alloc_);

		// Since destroy doesn't end lifetimes, and small_container_storage may have a union member
		// with default-constructed, non-trivial types in constant expressions, destroy them here if necessary.
		if constexpr (HasLargeMode) {
			if CTP_IS_CONSTEVAL {
				if (storage_.is_small_mode())
					std::destroy_at(&storage_.small);
			}
		}
	}
	constexpr container() noexcept(std::is_nothrow_default_constructible_v<rebind_alloc>) = default;
	explicit constexpr container(const Allocator& alloc) noexcept : alloc_{alloc} {}

	template <std::size_t C2, class A2, class O2>
	constexpr container(const container<T, C2, A2, O2>& o) : container{dispatch_to_template_tag{}, o} {}
	constexpr container(const container& o) : container{dispatch_to_template_tag{}, o} {}
	template <std::size_t C2, class A2, class O2>
	constexpr container(const container<T, C2, A2, O2>& o, const Allocator& alloc);

	template <std::size_t C2, class A2, class O2>
	constexpr container(container<T, C2, A2, O2>&& o) CTP_NOEXCEPT(noexcept(container{dispatch_to_template_tag{}, std::move(o)}))
		: container{dispatch_to_template_tag{}, std::move(o)} {}
	constexpr container(container&& o) CTP_NOEXCEPT(noexcept(container{dispatch_to_template_tag{}, std::move(o)}))
		: container{dispatch_to_template_tag{}, std::move(o)} {}
	template <std::size_t C2, class A2, class O2>
	constexpr container(container<T, C2, A2, O2>&& o, const Allocator& alloc)
		CTP_NOEXCEPT(noexcept(detail::move_storage(storage_, o.storage_, alloc_, alloc_, alloc_)));

	template <std::size_t C2, class A2, class O2>
	constexpr container& operator=(const container<T, C2, A2, O2>& o) { return this->copy_assignment(o); }
	constexpr container& operator=(const container& o) { return this->copy_assignment(o); }

	template <std::size_t C2, class A2, class O2>
	constexpr container& operator=(container<T, C2, A2, O2>&& o) CTP_NOEXCEPT(noexcept(this->move_assignment(std::move(o)))) {
		return this->move_assignment(std::move(o));
	}
	constexpr container& operator=(container&& o) CTP_NOEXCEPT(noexcept(this->move_assignment(std::move(o)))) {
		return this->move_assignment(std::move(o));
	}

	constexpr container& operator=(std::initializer_list<T> ilist) {
		assign(ilist);
		return *this;
	}

	// Construct with count default-initialized T.
	explicit constexpr container(size_type count, const Allocator& alloc = Allocator{})
		: alloc_{alloc} {
		detail::construct_n<growth_policy>(storage_, alloc_, static_cast<storage_size_t>(count));
	}

	constexpr container(size_type count, const T& value, const Allocator& alloc = Allocator{})
		: alloc_{alloc} {
		detail::construct_n<growth_policy>(storage_, alloc_, static_cast<storage_size_t>(count), value);
	}

	// Construct by copying range [first, last)
	template <std::input_iterator I>
	constexpr container(I first, I last, const Allocator& alloc = Allocator{});

	constexpr container(std::initializer_list<T> init, const Allocator& alloc = Allocator{});

	template <class Range>
	constexpr container(std::from_range_t, Range&& range, const Allocator& alloc = Allocator{});

	template <std::size_t C2, class A2, class O2>
	constexpr void swap(container<T, C2, A2, O2>& o)
		CTP_NOEXCEPT(
			noexcept(detail::swap_storage_elements(0, 0, storage_.small, o.storage_.small, alloc_, o.alloc_, alloc_, o.alloc_))
		);

	[[nodiscard]] constexpr std::size_t capacity() const noexcept { return storage_.capacity(); }
	[[nodiscard]] constexpr std::size_t max_size() const noexcept { return storage_.max_size(); }
	// Alias for max_size.
	[[nodiscard]] constexpr std::size_t max_capacity() const noexcept { return storage_.max_size(); }
	[[nodiscard]] constexpr size_type size() const noexcept { return storage_.size(); }

	[[nodiscard]] constexpr const_pointer data() const noexcept
		requires IsDataConstexprFriendly<value_type, options::allow_default_construction_in_constant_expressions>
	{
		return storage_.begin().get();
	}
	[[nodiscard]] const_pointer data() const noexcept
		requires (!IsDataConstexprFriendly<value_type, options::allow_default_construction_in_constant_expressions>)
	{
		return storage_.begin().get();
	}

	[[nodiscard]] constexpr pointer data() noexcept
		requires IsDataConstexprFriendly<value_type, options::allow_default_construction_in_constant_expressions>
	{
		return storage_.begin().get();
	}
	[[nodiscard]] pointer data() noexcept
		requires (!IsDataConstexprFriendly<value_type, options::allow_default_construction_in_constant_expressions>)
	{
		return storage_.begin().get();
	}

	[[nodiscard]] constexpr reference operator[](size_type i) noexcept {
		return storage_.begin()[static_cast<storage_size_t>(i)];
	}
	[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept {
		return storage_.begin()[static_cast<storage_size_t>(i)];
	}

	[[nodiscard]] constexpr reference at(this auto& self, size_type i) CTP_NOEXCEPT(false) {
		if (const auto size = self.size(); i >= size) [[unlikely]] {
#if CTP_USE_EXCEPTIONS
			throw std::out_of_range(
				std::format("ctp::small_storage::container index out of range (requested: {} size: {})", i, size));
#else
			std::terminate();
#endif
		}
		return self.storage_.begin()[static_cast<storage_size_t>(i)];
	}

	[[nodiscard]] constexpr bool empty() const noexcept { return storage_.size() == 0; }
	[[nodiscard]] constexpr bool is_empty() const noexcept { return storage_.size() == 0; }


	[[nodiscard]] constexpr iterator begin() noexcept { return storage_.begin(); }
	[[nodiscard]] constexpr const_iterator begin() const noexcept { return storage_.begin(); }
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return storage_.begin(); }
	[[nodiscard]] constexpr iterator end() noexcept { return storage_.end(); }
	[[nodiscard]] constexpr const_iterator end() const noexcept { return storage_.end(); }
	[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{cend()}; }
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{cbegin()}; }
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

	// Get iterator to the last element.
	[[nodiscard]] constexpr iterator last() noexcept {
		return storage_.begin() + (storage_.size() - 1);
	}
	// Get iterator to the last element.
	[[nodiscard]] constexpr const_iterator last() const noexcept {
		return storage_.begin() + (storage_.size() - 1);
	}

	[[nodiscard]] constexpr reference front() noexcept { return *storage_.begin(); }
	[[nodiscard]] constexpr const_reference front() const noexcept { return *storage_.begin(); }
	[[nodiscard]] constexpr reference back() noexcept { return *last(); }
	[[nodiscard]] constexpr const_reference back() const noexcept { return *last(); }

	constexpr void reserve(size_type new_capacity) noexcept(CTP_NOTHROW_ALLOCS && std::is_nothrow_move_constructible_v<T>) {
		detail::do_reserve(storage_, static_cast<storage_size_t>(new_capacity), alloc_);
	}

	constexpr void resize(size_type new_size)
		noexcept(CTP_NOTHROW_ALLOCS && std::is_nothrow_default_constructible_v<T>)
	{
		detail::do_resize(storage_, static_cast<storage_size_t>(new_size), alloc_);
	}
	constexpr void resize(size_type new_size, const value_type& value)
		noexcept(CTP_NOTHROW_ALLOCS && std::is_nothrow_copy_constructible_v<T>)
	{
		detail::do_resize(storage_, static_cast<storage_size_t>(new_size), alloc_, value);
	}

	constexpr reference push_back(const T& value) noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>) {
		return detail::construct_one<growth_policy>(storage_, alloc_, value);
	}
	constexpr reference push_back(T&& value) noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_move_constructible_v<T>) {
		return detail::construct_one<growth_policy>(storage_, alloc_, std::move(value));
	}

	constexpr reference emplace_back(auto&&... args)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_constructible_v<T, decltype(args)...>)
	{
		return detail::construct_one<growth_policy>(storage_, alloc_, std::forward<decltype(args)>(args)...);
	}

	// Destroys existing items. Sets size to zero. Capacity is unchanged.
	constexpr void clear() noexcept {
		detail::destroy_elements(storage_, alloc_);
	}

	constexpr allocator_type get_allocator() const noexcept { return alloc_; }

	// Destroys all items and resets container to small mode.
	constexpr void reset() noexcept {
		detail::destroy_and_deallocate(storage_, alloc_);
		if constexpr (HasLargeMode) {
			if (!storage_.is_small_mode()) {
				std::destroy_at(&storage_.large);
				std::construct_at(&storage_.small);
			}
		}
		storage_.set_size(0, Mode::Small);
	}

	constexpr void pop_back() noexcept {
		detail::do_resize(storage_, storage_.size() - 1, alloc_);
	}

	// Shrinks container's capacity to current size.
	// Will change back to small mode if possible.
	// Is not guaranteed to exactly match container's size, even in large mode.
	constexpr void shrink_to_fit() noexcept;

	constexpr iterator insert(const_iterator pos, const T& value)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		return detail::do_insert<growth_policy>(storage_, alloc_, make_nonconst_iterator(pos), 1, value);
	}

	constexpr iterator insert(const_iterator pos, T&& value)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		return detail::do_insert<growth_policy>(storage_, alloc_, make_nonconst_iterator(pos), 1, std::move(value));
	}

	constexpr iterator insert(const_iterator pos, size_type count, const T& value)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		return detail::do_insert<growth_policy>(storage_, alloc_, make_nonconst_iterator(pos), count, value);
	}

	template <std::input_iterator It>
	constexpr iterator insert(const_iterator pos, It firstIt, It lastIt)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		return detail::do_insert_with_iterators<growth_policy>(
			storage_, alloc_, make_nonconst_iterator(pos), firstIt, lastIt);
	}

	constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		return detail::do_insert_with_iterators<growth_policy>(
			storage_, alloc_, make_nonconst_iterator(pos), ilist.begin(), ilist.end());
	}

	constexpr iterator insert_range(const_iterator pos, auto&& range)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		if constexpr (std::is_rvalue_reference_v<decltype(range)>) {
			return detail::do_insert_with_iterators<growth_policy>(
				storage_,
				alloc_,
				make_nonconst_iterator(pos),
				move_iterator{range.begin()},
				move_iterator{range.end()});

			if constexpr (requires { range.clear(); }) {
				range.clear();
			}
		} else {
			return detail::do_insert_with_iterators<growth_policy>(
				storage_, alloc_, make_nonconst_iterator(pos), range.begin(), range.end());
		}
	}

	constexpr iterator append_range(auto&& range)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_copy_constructible_v<T>)
	{
		if constexpr (std::is_rvalue_reference_v<decltype(range)>) {
			return detail::do_insert_with_iterators<growth_policy>(
				storage_,
				alloc_,
				end(),
				move_iterator{range.begin()},
				move_iterator{range.end()});

			if constexpr (requires { range.clear(); }) {
				range.clear();
			}
		} else {
			return detail::do_insert_with_iterators<growth_policy>(storage_, alloc_, end(), range.begin(), range.end());
		}
	}

	template <typename... Args>
	constexpr iterator emplace(const_iterator pos, Args&&... args)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_constructible_v<T, Args...> && std::is_nothrow_copy_constructible_v<T>)
	{
		return detail::do_insert<growth_policy>(
			storage_, alloc_, make_nonconst_iterator(pos), 1, std::forward<Args>(args)...);
	}

	constexpr iterator erase(const_iterator pos)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_move_assignable_v<T>)
	{
		auto it = make_nonconst_iterator(pos);
		return detail::do_erase(storage_, alloc_, it, it + 1);
	}

	constexpr iterator erase(const_iterator first, const_iterator last)
		noexcept(CTP_NOTHROW_ALLOCS&& std::is_nothrow_move_assignable_v<T>)
	{
		return detail::do_erase(storage_, alloc_, make_nonconst_iterator(first), make_nonconst_iterator(last));
	}

	constexpr void assign(size_type count, const T& value) {
		detail::do_assign<growth_policy>(storage_, alloc_, count, value);
	}

	template <std::input_iterator Iterator>
	constexpr void assign(Iterator first, Iterator last) {
		detail::do_assign_with_iterators<growth_policy>(storage_, alloc_, first, last);
	}

	constexpr void assign(std::initializer_list<T> list) {
		detail::do_assign_with_iterators<growth_policy>(storage_, alloc_, list.begin(), list.end());
	}

	template <class Range>
	constexpr void assign_range(Range&& range) {
		if constexpr (std::ranges::sized_range<Range>) {
			reserve(std::ranges::distance(range));
		}

		if constexpr (std::is_rvalue_reference_v<decltype(range)>) {
			detail::do_assign_with_iterators<growth_policy>(
				storage_,
				alloc_,
				move_iterator{range.begin()},
				move_iterator{range.end()});

			if constexpr (requires { range.clear(); }) {
				range.clear();
			}
		} else {
			detail::do_assign_with_iterators<growth_policy>(storage_, alloc_, range.begin(), range.end());
		}
	}
private:
	template <std::size_t C2, class A2, class O2>
	constexpr container(dispatch_to_template_tag, const container<T, C2, A2, O2>& o);

	template <std::size_t C2, class A2, class O2>
	constexpr container(dispatch_to_template_tag, container<T, C2, A2, O2>&& o)
		CTP_NOEXCEPT(noexcept(detail::move_storage(storage_, o.storage_, alloc_, alloc_, alloc_)));

	template <std::size_t C2, class A2, class O2>
	constexpr container& copy_assignment(const container<T, C2, A2, O2>& o);

	template <std::size_t C2, class A2, class O2>
	constexpr container& move_assignment(container<T, C2, A2, O2>&& o)
		CTP_NOEXCEPT(noexcept(detail::move_storage(storage_, o.storage_, alloc_, o.alloc_, alloc_)));
};

// ---------------------------------------------------------
// small_storage::container implementation details
// ---------------------------------------------------------

// Copy constructor.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, class A2, class O2>
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(dispatch_to_template_tag, const container<T, C2, A2, O2>& o)
	: alloc_{alloc_traits::select_on_container_copy_construction(o.alloc_)}
{
	detail::construct_from_range<growth_policy>(storage_, alloc_, o.begin(), o.end());
}

// Move constructor.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, class A2, class O2>
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(dispatch_to_template_tag, container<T, C2, A2, O2>&& o)
	CTP_NOEXCEPT(noexcept(detail::move_storage(storage_, o.storage_, alloc_, alloc_, alloc_)))
	: alloc_{std::move(o.alloc_)}
{
	// Only one allocator matters here.
	detail::move_storage(storage_, o.storage_, alloc_, alloc_, alloc_);
}

// Copy constructor with allocator.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, typename A2, class O2>
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(const container<T, C2, A2, O2>& o, const Allocator& alloc)
	: alloc_{alloc}
{
	detail::construct_from_range<growth_policy>(storage_, alloc_, o.begin(), o.end());
}

// Move constructor with allocator.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, typename A2, class O2>
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(container<T, C2, A2, O2>&& o, const Allocator& alloc)
	CTP_NOEXCEPT(noexcept(detail::move_storage(storage_, o.storage_, alloc_, alloc_, alloc_)))
	: alloc_{alloc}
{
	// Only one allocator matters here.
	detail::move_storage(storage_, o.storage_, alloc_, alloc_, alloc_);
}

// Copy assignment.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, class A2, class O2>
constexpr container<T, MinSmallCapacity, Allocator, options>& container<T, MinSmallCapacity, Allocator, options>::
copy_assignment(const container<T, C2, A2, O2>& o)
{
	if constexpr (std::same_as<container, container<T, C2, A2, O2>>) {
		if (this == &o) [[unlikely]]
			return *this;
	}

	auto oldSize = storage_.size();
	const auto newSize = o.storage_.size();
	auto oIt = o.storage_.begin();
	[[maybe_unused]] auto [mode, didResize] = resize_for_overwrite(storage_, newSize, alloc_);

	if constexpr (alloc_traits::propagate_on_container_copy_assignment::value) {
		if constexpr (!alloc_traits::is_always_equal::value) {
			if (alloc_ != o.alloc_ && !didResize) {
				// Different allocators, destroy any old elements.
				detail::destroy_elements(storage_, alloc_);
			}
		}
		alloc_ = o.alloc_;
	}

	if constexpr (HasLargeMode) {
		if (mode == Mode::Large) {
			if (didResize)
				oldSize = 0;

			if constexpr (o.HasLargeMode) {
				if (!o.storage_.is_small_mode()) {
					detail::copy_overwrite_storage(oldSize, newSize, alloc_, storage_.large, o.storage_.large);
					return *this;
				}
			}

			detail::copy_overwrite_storage(oldSize, newSize, alloc_, storage_.large, o.storage_.small);
			return *this;
		}
	}

	// small mode

	if constexpr (o.HasLargeMode) {
		if (!o.storage_.is_small_mode()) {
			detail::copy_overwrite_storage(oldSize, newSize, alloc_, storage_.small, o.storage_.large);
			return *this;
		}
	}

	detail::copy_overwrite_storage(oldSize, newSize, alloc_, storage_.small, o.storage_.small);

	return *this;
}

// Move assignment.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, class A2, class O2>
constexpr container<T, MinSmallCapacity, Allocator, options>& container<T, MinSmallCapacity, Allocator, options>::
move_assignment(container<T, C2, A2, O2>&& o)
	CTP_NOEXCEPT(noexcept(detail::move_storage(storage_, o.storage_, alloc_, o.alloc_, alloc_)))
{
	if constexpr (std::same_as<container, container<T, C2, A2, O2>>) {
		if (this == &o) [[unlikely]]
			return *this;
	}

	if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
		// Use o's allocator we're about to take as new alloc.
		detail::move_storage(storage_, o.storage_, alloc_, o.alloc_, o.alloc_);

		// Unusual edge case: this doesn't support large mode but o does.
		// If we moved o's large mode elements into our small mode, then clean up
		// o's allocation before taking its allocator.
		if constexpr (!HasLargeMode && o.HasLargeMode) {
			if (!o.storage_.is_small_mode()) {
				alloc_traits::deallocate(o.alloc_, o.storage_.large.data, o.storage_.large.capacity);

				if CTP_IS_CONSTEVAL {
					std::destroy_at(&o.storage_.large);
					std::construct_at(&o.storage_.small);
				}
				o.storage_.set_size(0, Mode::Small);
			}
		}

		alloc_ = std::move(o.alloc_);
	} else if constexpr (alloc_traits::is_always_equal::value) {
		detail::move_storage(storage_, o.storage_, alloc_, o.alloc_, alloc_); // Keep using our allocator.
	} else {
		if (alloc_ == o.alloc_) {
			detail::move_storage(storage_, o.storage_, alloc_, o.alloc_, alloc_); // Keep using our allocator.
		} else {
			// Can only do element-wise move at best.
			detail::move_elements(storage_, o.storage_, alloc_, o.alloc_, alloc_);
		}
	}

	return *this;
}

// Construct with iterators.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::input_iterator I> // Construct by copying range [first, last)
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(I first, I last, const Allocator& alloc)
	: alloc_{alloc} {
	detail::construct_from_range<growth_policy>(storage_, alloc_, first, last);
}

// Initializer list.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(std::initializer_list<T> init, const Allocator& alloc)
	: alloc_{alloc} {
	detail::construct_from_range<growth_policy>(storage_, alloc_, init.begin(), init.end());
}

// Range initialization.
template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <class Range>
constexpr container<T, MinSmallCapacity, Allocator, options>::
container(std::from_range_t, Range&& range, const Allocator& alloc)
	: alloc_{alloc} {
	if constexpr (std::ranges::sized_range<Range> || std::ranges::forward_range<Range>) {
		const auto count = static_cast<storage_size_t>(std::ranges::distance(range));
		[[maybe_unused]] const auto [mode, idx] = get_storage_for_n<growth_policy>(storage_, count, alloc_);
		auto it = range.begin();
		if constexpr (std::is_rvalue_reference_v<decltype(range)>) {
			if constexpr (HasLargeMode) {
				if (mode == Mode::Large) {
					for (auto i = 0; i < count; ++i, ++it)
						detail::do_construct_at(i, alloc_, storage_.large.data, std::move(*it));
					return;
				}
			}

			for (auto i = 0; i < count; ++i, ++it)
				detail::do_construct_at(i, alloc_, storage_.small.data, std::move(*it));
		} else {
			if constexpr (HasLargeMode) {
				if (mode == Mode::Large) {
					for (auto i = 0; i < count; ++i, ++it)
						detail::do_construct_at(i, alloc_, storage_.large.data, *it);
					return;
				}
			}

			for (auto i = 0; i < count; ++i, ++it)
				detail::do_construct_at(i, alloc_, storage_.small.data, *it);
		}
	} else {
		if constexpr (std::is_rvalue_reference_v<decltype(range)>) {
			for (auto&& item : range) {
				const auto [mode, i] = get_storage_for_n<growth_policy>(storage_, 1, alloc_);

				if constexpr (HasLargeMode) {
					if (mode == Mode::Large) {
						detail::do_construct_at(i, alloc_, storage_.large.data, std::move(item));
						continue;
					}
				}

				detail::do_construct_at(i, alloc_, storage_.small.data, std::move(item));
			}
		} else {
			for (auto&& item : range) {
				const auto [mode, i] = get_storage_for_n<growth_policy>(storage_, 1, alloc_);

				if constexpr (HasLargeMode) {
					if (mode == Mode::Large) {
						detail::do_construct_at(i, alloc_, storage_.large.data, item);
						continue;
					}
				}

				detail::do_construct_at(i, alloc_, storage_.small.data, item);
			}
		}
	}
}

template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
template <std::size_t C2, class A2, class O2>
constexpr void container<T, MinSmallCapacity, Allocator, options>::
swap(container<T, C2, A2, O2>& o)
	CTP_NOEXCEPT(noexcept(detail::swap_storage_elements(0, 0, storage_.small, o.storage_.small, alloc_, o.alloc_, alloc_, o.alloc_)))
{
	using std::swap;

	const auto small_into_large =
		[&](auto& largeStorage, auto& smallStorage, auto& largeAlloc, auto& smallAlloc, auto& largeNextAlloc)
	{
		// Grab the large storage, move small into formerly-large, and then reset small with large storage.
		const auto largeTemp = largeStorage.large;
		std::destroy_at(&largeStorage.large);
		std::construct_at(&largeStorage.small);
		largeStorage.set_size(0, Mode::Small);

		// Sets largeStorage size
		detail::move_elements(largeStorage, smallStorage, largeAlloc, smallAlloc, largeNextAlloc);

		std::destroy_at(&smallStorage.small);
		std::construct_at(&smallStorage.large, largeTemp);
	};

	const auto do_swap = [&](auto& nextThisAlloc, auto& nextOAlloc) {
		const auto size = storage_.size();
		const auto oSize = o.storage_.size();
		const auto capacity = storage_.capacity();
		const auto oCapacity = o.storage_.capacity();

		ctpAssert(size <= o.storage_.max_size());
		ctpAssert(oSize <= storage_.max_size());

		if constexpr (HasLargeMode) {
			if (!storage_.is_small_mode()) {
				if constexpr (o.HasLargeMode) {
					if (!o.storage_.is_small_mode()) {
						// Swap large data structs.
						swap(storage_.large.capacity, o.storage_.large.capacity);
						swap(storage_.large.data, o.storage_.large.data);
						storage_.set_size(oSize, Mode::Large);
						o.storage_.set_size(size, Mode::Large);
						return;
					}
				}

				small_into_large(storage_, o.storage_, alloc_, o.alloc_, nextThisAlloc);
				o.storage_.set_size(size, Mode::Large);
				return;
			}
		}

		if constexpr (o.HasLargeMode) {
			if (!o.storage_.is_small_mode()) {
				small_into_large(o.storage_, storage_, o.alloc_, alloc_, nextOAlloc);
				storage_.set_size(oSize, Mode::Large);
				return;
			}
		}

		// this and o are both small mode. However, one may still not fit in the other
		// if they have different small capacities.

		if constexpr (HasLargeMode) {
			if (capacity < oSize) {
				auto large = detail::switch_to_large_storage<value_type>(storage_.small, size, oSize, nextThisAlloc);
				std::destroy_at(&storage_.small);
				std::construct_at(&storage_.large, large);

				detail::swap_storage_elements(
					size, oSize,
					storage_.large, o.storage_.small,
					nextThisAlloc, nextOAlloc,
					alloc_, o.alloc_);

				storage_.set_size(oSize, Mode::Large);
				o.storage_.set_size(size, Mode::Small);
				return;
			}
		}

		if constexpr (o.HasLargeMode) {
			if (oCapacity < size) {
				auto large = detail::switch_to_large_storage<value_type>(o.storage_.small, oSize, size, nextOAlloc);
				std::destroy_at(&o.storage_.small);
				std::construct_at(&o.storage_.large, large);

				detail::swap_storage_elements(
					size, oSize,
					storage_.small, o.storage_.large,
					nextThisAlloc, nextOAlloc,
					alloc_, o.alloc_);

				storage_.set_size(oSize, Mode::Small);
				o.storage_.set_size(size, Mode::Large);
				return;
			}
		}

		detail::swap_storage_elements(
			size, oSize,
			storage_.small, o.storage_.small,
			nextThisAlloc, nextOAlloc,
			alloc_, o.alloc_);

		storage_.set_size(oSize, Mode::Small);
		o.storage_.set_size(size, Mode::Small);
	};

	if constexpr (alloc_traits::propagate_on_container_swap::value) {
		do_swap(o.alloc_, alloc_);
		swap(alloc_, o.alloc_);
	} else if constexpr (alloc_traits::is_always_equal::value) {
		do_swap(alloc_, o.alloc_);
	} else {
		if (alloc_ == o.alloc_) {
			do_swap(alloc_, o.alloc_);
		} else {
			// Undefined behaviour. Don't think this is valuable to attempt handling.
			std::unreachable();
		}
	}
}

template <typename T, std::size_t MinSmallCapacity, typename Allocator, SmallStorageOptions options>
constexpr void container<T, MinSmallCapacity, Allocator, options>::
shrink_to_fit() noexcept {
	if constexpr (HasLargeMode) {
		if (storage_.is_small_mode())
			return;

		const auto size = storage_.size();

		if (size <= SmallCapacity) {
			auto large = storage_.large;

			std::destroy_at(&storage_.large);
			std::construct_at(&storage_.small);
			storage_.set_size(size, Mode::Small);

			for (size_type i = 0; i < size; ++i) {
				detail::do_construct_at(i, alloc_, storage_.small.data, std::move(large.data[i]));
				detail::do_destroy_at(i, alloc_, large.data);
			}

			std::allocator_traits<Allocator>::deallocate(alloc_, large.data, large.capacity);

			return;
		}

		if (size < storage_.large.capacity)
			detail::reallocate_large_storage(storage_.large, size, size, alloc_);
	}
}

template <typename T,
	std::size_t C1, class A1, class O1,
	std::size_t C2, class A2, class O2>
constexpr bool operator==(const container<T, C1, A1, O1>& lhs, const container<T, C2, A2, O2>& rhs) noexcept
{
	const auto lhsSize = lhs.size();
	const auto rhsSize = rhs.size();

	if (lhsSize != rhsSize)
		return false;

	auto lhsIt = lhs.cbegin();
	auto rhsIt = rhs.cbegin();

	for (decltype(lhs.size()) i = 0; i < lhsSize; ++i) {
		if (lhsIt[i] != rhsIt[i])
			return false;
	}

	return true;
}

template <typename T,
	std::size_t C1, class A1, class O1,
	std::size_t C2, class A2, class O2>
constexpr compare_three_way_type_t<T>
operator<=>(const container<T, C1, A1, O1>& lhs, const container<T, C2, A2, O2>& rhs) noexcept {
	auto lhsIt = lhs.cbegin();
	auto rhsIt = rhs.cbegin();

	const auto lhsEnd = lhs.cend();
	const auto rhsEnd = rhs.cend();

	for (;; ++lhsIt, ++rhsIt) {
		if (lhsIt == lhsEnd)
			return rhsIt == rhsEnd ? std::strong_ordering::equal : std::strong_ordering::less;
		if (rhsIt == rhsEnd)
			return std::strong_ordering::greater;

		if (const auto comp = *lhsIt <=> *rhsIt; comp != 0)
			return comp;
	}
}

} // ctp::small_storage

#endif // INCLUDE_CTP_TOOLS_SMALL_STORAGE_HPP
