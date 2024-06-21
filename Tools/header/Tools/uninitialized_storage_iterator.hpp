#ifndef INCLUDE_CTP_TOOLS_UNINITIALIZED_ITEM_ITERATOR_HPP
#define INCLUDE_CTP_TOOLS_UNINITIALIZED_ITEM_ITERATOR_HPP

#include "config.hpp"
#include "debug.hpp"
#include "iterator.hpp"
#include "type_traits.hpp"
#include "uninitialized_storage.hpp"

namespace ctp {

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

} // ctp

#endif // !INCLUDE_CTP_TOOLS_UNINITIALIZED_ITEM_ITERATOR_HPP
