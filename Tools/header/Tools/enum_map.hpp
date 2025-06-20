#ifndef INCLUDE_CTP_TOOLS_ENUM_MAP_HPP
#define INCLUDE_CTP_TOOLS_ENUM_MAP_HPP

#include "array.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "enum_reflection.hpp"
#include "iterator.hpp"
#include "reverse_iterator.hpp"
#include "uninitialized_storage.hpp"

#include <type_traits>
#include <utility>

namespace ctp {

template <enums::Enum E, typename Iterator, bool IsConst>
class enum_map_iterator_t :
	public iterator_t<
		enum_map_iterator_t<E, Iterator, IsConst>,
		std::pair<E, typename Iterator::value_type&>,
		std::random_access_iterator_tag,
		std::ptrdiff_t,
		// peek returns a by-value proxy.
		std::pair<E, typename Iterator::value_type&>
	>
{
	using T = typename Iterator::value_type;

	// Iterator pointing to the first member of the value array.
	// We store this to handle the two different iterator types depending on what the
	// underlying uninit::array uses, to ensure valid access in constant expressions.
	Iterator begin_ = nullptr;
	std::size_t index_ = static_cast<std::size_t>(-1);

	// iterator_t interface functions

	friend iterator_accessor;
	constexpr std::size_t& get_index() noexcept { return index_; }
	constexpr bool equals(const enum_map_iterator_t& o) const noexcept {
		return begin_ == o.begin_ && index_ == o.index_;
	}
	constexpr std::pair<E, T&> peek() noexcept {
		return {enums::values<E>()[index_], *(begin_ + index_)};
	}

	// Allow construction of const iterators from nonconst iterators.
	friend class enum_map_iterator_t<E, T, true>;
	using nonconst_t = std::conditional_t<IsConst, enum_map_iterator_t<E, Iterator, false>, nonesuch>;
public:
	template <typename NonConstT = nonconst_t, std::enable_if_t<!std::is_same_v<NonConstT, nonesuch>, int> = 0>
	constexpr enum_map_iterator_t(const nonconst_t& other) noexcept
		: begin_{other.begin_}
		, index_{other.index_}
	{}

	constexpr enum_map_iterator_t() noexcept = default;
	constexpr enum_map_iterator_t(Iterator begin, std::size_t index) noexcept
		: begin_{begin}
		, index_{index}
	{}
};

namespace enum_detail {

template <typename T, std::size_t Size, typename Allocator>
class enum_map_base {
protected:
	// Helper for noexcept specs since MSVC has trouble.
	using DataT = array<T, Size, Allocator>;
	DataT data_;

public:
	using mapped_type = T;
	using mapped_type_reference = T&;
	using mapped_type_const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using allocator_type = Allocator;
	using pointer = std::allocator_traits<Allocator>::pointer;
	using const_pointer = std::allocator_traits<Allocator>::const_pointer;

	[[nodiscard]] static constexpr std::size_t size() noexcept { return Size; }
	[[nodiscard]] static constexpr std::size_t max_size() noexcept { return Size; }
	[[nodiscard]] static constexpr bool empty() noexcept { return Size != 0; }
	[[nodiscard]] static constexpr bool is_empty() noexcept { return empty(); }

	[[nodiscard]] constexpr const auto& values() noexcept { return data_; }

	// Explicitly not allowed. All values must be initialized.
	constexpr enum_map_base() = delete;

	// Set all values to a default value.
	explicit constexpr enum_map_base(const T& value, const Allocator& alloc = Allocator{})
		CTP_NOEXCEPT(noexcept(DataT(value, alloc)))
		: data_{value, alloc} {}

	template <concepts::AllConvertibleTo<T>... Ts>
		requires (sizeof...(Ts) == Size)
	constexpr enum_map_base(Ts&&... values)
		CTP_NOEXCEPT(noexcept(DataT(std::forward<Ts>(values)...)))
		: data_{std::forward<Ts>(values)...} {}

	template <concepts::AllConvertibleTo<T>... Ts>
		requires (sizeof...(Ts) == Size)
	constexpr enum_map_base(const Allocator& alloc, Ts&&... values)
		CTP_NOEXCEPT(noexcept(DataT(alloc, std::forward<Ts>(values)...)))
		: data_{alloc, std::forward<Ts>(values)...} {}

	constexpr enum_map_base(const enum_map_base& o) CTP_NOEXCEPT(noexcept(DataT(o.data_)))
		: data_{o.data_} {}
	template <typename A2>
	constexpr enum_map_base(const enum_map_base<T, Size, A2>& o) CTP_NOEXCEPT(noexcept(DataT(o.data_)))
		: data_{o.data_} {}

	constexpr enum_map_base(enum_map_base&& o) CTP_NOEXCEPT(noexcept(DataT(std::move(o.data_))))
		: data_{std::move(o.data_)} {}
	template <typename A2>
	constexpr enum_map_base(enum_map_base<T, Size, A2>&& o) CTP_NOEXCEPT(noexcept(DataT(std::move(o.data_))))
		: data_{std::move(o.data_)} {}

	constexpr ~enum_map_base() noexcept = default;

	constexpr allocator_type get_allocator() const noexcept { return data_.get_allocator(); }

	[[nodiscard]] constexpr T* data() noexcept { return data_.data(); }
	[[nodiscard]] constexpr const T* data() const noexcept { return data_.data(); }

	constexpr enum_map_base& operator=(const enum_map_base& o) CTP_NOEXCEPT(noexcept(data_ = o.data_)) {
		data_ = o.data_;
		return *this;
	}
	template <typename A2>
	constexpr enum_map_base& operator=(const enum_map_base<T, Size, A2>& o) CTP_NOEXCEPT(noexcept(data_ = o.data_)) {
		data_ = o.data_;
		return *this;
	}

	constexpr enum_map_base& operator=(enum_map_base&& o)
		CTP_NOEXCEPT(noexcept(data_ = std::move(o.data_))) {
		data_ = std::move(o.data_);
		return *this;
	}
	template <typename A2>
	constexpr enum_map_base& operator=(enum_map_base<T, Size, A2>&& o)
		CTP_NOEXCEPT(noexcept(data_ = std::move(o.data_))) {
		data_ = std::move(o.data_);
		return *this;
	}

	template <typename A2>
	constexpr void swap(enum_map_base<T, Size, A2>& o) CTP_NOEXCEPT(noexcept(data_.swap(o.data_))) {
		data_.swap(o.data_);
	}

	constexpr void fill(const T& value) CTP_NOEXCEPT(noexcept(data_.fill(value))) {
		data_.fill(value);
	}

	template <typename T, std::size_t N, typename A1, typename A2>
	friend constexpr bool operator==(const enum_map_base<T, N, A1>& lhs, const enum_map_base<T, N, A2>& rhs) noexcept;
	template <typename T, std::size_t N, typename A1, typename A2>
	friend constexpr auto operator<=>(const enum_map_base<T, N, A1>& lhs, const enum_map_base<T, N, A2>& rhs)
		noexcept -> decltype(lhs.data_ <=> rhs.data_);

protected:
	// Gets ith value in the underlying array (i.e. not the underlying value of the enum).
	[[nodiscard]] constexpr mapped_type_reference operator[](size_type i) noexcept {
		ctpExpects(i >= 0 && i < Size);
		return data_[i];
	}

	// Gets ith value in the underlying array (i.e. not the underlying value of the enum).
	[[nodiscard]] constexpr mapped_type_const_reference operator[](size_type i) const noexcept {
		ctpExpects(i >= 0 && i < Size);
		return data_[i];
	}

	// Gets ith value in the underlying array (i.e. not the underlying value of the enum).
	[[nodiscard]] constexpr mapped_type_reference at(size_type i) noexcept {
		ctpExpects(i >= 0 && i < Size);
		return data_.at(i);
	}

	// Gets ith value in the underlying array (i.e. not the underlying value of the enum).
	[[nodiscard]] constexpr mapped_type_const_reference at(size_type i) const noexcept {
		ctpExpects(i >= 0 && i < Size);
		return data_.at(i);
	}
};

template <typename T, std::size_t N, typename A1, typename A2>
constexpr bool operator==(const enum_map_base<T, N, A1>& lhs, const enum_map_base<T, N, A2>& rhs) noexcept {
	return lhs.data_ == rhs.data_;
}
template <typename T, std::size_t N, typename A1, typename A2>
constexpr auto operator<=>(const enum_map_base<T, N, A1>& lhs, const enum_map_base<T, N, A2>& rhs)
	noexcept  -> decltype(lhs.data_ <=> rhs.data_) {
	return lhs.data_ <=> rhs.data_;
}

} // enum_detail

// enum_map, a container that has a value for every named enum value and can be indexed by it.
// Expects that all named values that are used with it are reflected, and that only valid
// named values are used.
template <enums::Enum E, typename T, typename Allocator = std::allocator<T>>
class enum_map : public enum_detail::enum_map_base<T, enums::size<E>(), Allocator> {
protected:
	using Base = enum_detail::enum_map_base<T, enums::size<E>(), Allocator>;

	constexpr auto arr_begin(this auto& self) noexcept { return self.data_.begin(); }
public:

	using key_type = E;
	using mapped_type = Base::mapped_type;
	using mapped_type_reference = Base::mapped_type_reference;
	using mapped_type_const_reference = Base::mapped_type_const_reference;
	using value_type = std::pair<const key_type, mapped_type_reference>;
	using size_type = Base::size_type;
	using difference_type = Base::difference_type;
	using key_compare = std::less<key_type>;
	using allocator_type = Base::allocator_type;
	using reference = value_type;
	using const_reference = const value_type;
	using pointer = Base::pointer;
	using const_pointer = Base::pointer;

	using iterator = enum_map_iterator_t<E, typename Base::DataT::iterator, false>;
	using const_iterator = enum_map_iterator_t<E, typename Base::DataT::const_iterator, true>;
	using reverse_iterator = ctp::reverse_iterator<iterator>;
	using const_reverse_iterator = ctp::reverse_iterator<const_iterator>;

	static constexpr std::size_t Size = enums::size<E>();

	using Base::Base;
	using Base::size;
	using Base::empty;
	using Base::is_empty;
	using Base::values;
	using Base::get_allocator;
	using Base::data;

	[[nodiscard]] static constexpr const auto& keys() noexcept { return enums::values<E>(); }

	[[nodiscard]] constexpr iterator begin() noexcept { return {arr_begin(), 0}; }
	[[nodiscard]] constexpr const_iterator begin() const noexcept { return {arr_begin(), 0}; }
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
	[[nodiscard]] constexpr iterator end() noexcept { return {arr_begin(), Size}; }
	[[nodiscard]] constexpr const_iterator end() const noexcept { return {arr_begin(), Size}; }
	[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{cend()}; }
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{cbegin()}; }
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

	// Get iterator to the last element.
	[[nodiscard]] constexpr iterator last() noexcept { return {arr_begin(), Size - 1}; }
	// Get iterator to the last element.
	[[nodiscard]] constexpr const_iterator last() const noexcept { return {arr_begin(), Size - 1}; }

	[[nodiscard]] constexpr mapped_type_reference operator[](key_type e) noexcept {
		ctpExpects(enums::try_get_index(e));
		return data()[enums::index(e)];
	}

	[[nodiscard]] constexpr mapped_type_const_reference operator[](key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return data()[enums::index(e)];
	}

	[[nodiscard]] constexpr mapped_type_reference at(key_type e) noexcept {
		ctpExpects(enums::try_get_index(e));
		return this->data_.at(enums::index(e));
	}

	[[nodiscard]] constexpr mapped_type_const_reference at(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return this->data_.at(enums::index(e));
	}

	[[nodiscard]] constexpr std::size_t count(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return 1;
	}
	[[nodiscard]] constexpr iterator find(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return data()[enums::index(e)];
	}
	[[nodiscard]] constexpr bool contains(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return true;
	}

	[[nodiscard]] constexpr iterator lower_bound(key_type e) noexcept {
		ctpExpects(enums::try_get_index(e));
		return {arr_begin(), enums::index(e)};
	}
	[[nodiscard]] constexpr const_iterator lower_bound(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return {arr_begin(), enums::index(e)};
	}
	[[nodiscard]] constexpr iterator upper_bound(key_type e) noexcept {
		ctpExpects(enums::try_get_index(e));
		return {arr_begin(), enums::index(e) + 1};
	}
	[[nodiscard]] constexpr const_iterator upper_bound(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		return {arr_begin(), enums::index(e) + 1};
	}
	[[nodiscard]] constexpr std::pair<iterator, iterator> equal_range(key_type e) noexcept {
		ctpExpects(enums::try_get_index(e));
		const auto idx = enums::index(e);
		return {iterator{arr_begin(), idx}, iterator{arr_begin(), idx + 1}};
	}
	[[nodiscard]] constexpr std::pair<const_iterator, const_iterator> equal_range(key_type e) const noexcept {
		ctpExpects(enums::try_get_index(e));
		const auto idx = enums::index(e);
		return {const_iterator{arr_begin(), idx}, const_iterator{arr_begin(), idx + 1}};
	}
};

// Same as enum_map, but also supports operator[std::size_t]
template <enums::Enum E, typename T, typename Allocator = std::allocator<T>>
class indexible_enum_map : public enum_map<E, T, Allocator> {
	using Base = enum_map<E, T, Allocator>;
public:
	using Base::Base;

	// Pull in enum_map's operator[Enum]
	using Base::operator[];
	// Pull in enum_map_base's operator[size_type]
	using Base::Base::operator[];
	using Base::Base::at;
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_ENUM_MAP_HPP
