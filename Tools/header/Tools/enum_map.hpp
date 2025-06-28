#ifndef INCLUDE_CTP_TOOLS_ENUM_MAP_HPP
#define INCLUDE_CTP_TOOLS_ENUM_MAP_HPP

#include "array.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "enum_reflection.hpp"
#include "iterator.hpp"
#include "reverse_iterator.hpp"
#include "uninitialized_storage.hpp"

#include <fmt/format.h>

#include <type_traits>
#include <utility>

namespace ctp {

template <auto enum_value, typename T>
	requires enums::Enum<decltype(enum_value)>
struct enum_arg_pair {
	using enum_arg_hint = void;
	static constexpr decltype(enum_value) first{enum_value};
	using first_type = decltype(enum_value);
	using second_type = T;
	T second;
};
template <typename T> struct enum_default_arg { T value; };

// Helper for enum_map constructor. Use like:
// enum class MyEnum { One, Two };
// enum_map<MyEnum, int>{
//     enum_arg<MyEnum::One>(1),
//     enum_arg<MyEnum::Two>(2)
// };
// This guarantees at compile time that all values will be initialized.
template <auto enum_value, typename T>
constexpr auto enum_arg(T&& t) { return enum_arg_pair<enum_value, T>{std::forward<T>(t) }; }
// Used with enum_map constructors when not specifying a value for all indices, to explicitly
// default to some value.
template <typename T >
constexpr auto enum_default(T&& t) { return enum_default_arg<T>{std::forward<T>(t)}; }

namespace enum_detail {

template <enums::Enum E, typename Iterator, bool IsConst>
class enum_map_iterator_t :
	public iterator_t<
	enum_map_iterator_t<E, Iterator, IsConst>,
	std::pair<E, typename Iterator::value_type&>,
	std::random_access_iterator_tag,
	std::ptrdiff_t,
	// peek returns a by-value proxy.
	std::pair<E, typename Iterator::value_type&>
	> {
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

template <class... Pairs>
constexpr bool unique_pairs_check(const Pairs&... pairs) noexcept {
	using E = typename first_element_t<Pairs...>::first_type;
	static_assert(sizeof...(Pairs) <= enums::size<E>());

	constexpr E minval = enums::min_val<E>();
	constexpr E maxval = enums::max_val<E>();

	bool found[enums::size<E>()]{};
	const auto test_unique = [&found](E e) {
		if (e < minval || e > maxval) {
			ctpFailMsg(fmt::format("creating an enum_map with a value out of range [{}::()]",
				enums::type_name<E>(),
				std::to_underlying(e)));
			return false;
		}

		if (std::exchange(found[enums::index(e)], true)) {
			ctpFailMsg(fmt::format("creating an enum_map with duplicate enum value [{}::{}]",
				enums::type_name<E>(),
				enums::name(e)));
			return false;
		}
		return true;
	};

	return true && (... && test_unique(pairs.first));
}

template <class... Args>
	requires (sizeof...(Args) > 0)
consteval bool unique_args_check() {
	using E = typename first_element_t<Args...>::first_type;

	// Create a real type from the template type. We don't need the second arg.
	return unique_pairs_check(enum_arg_pair<Args::first, char>{'\0'}...);
}

template <class MaybeArg, class EnumMap>
concept EnumArg = requires {
	typename MaybeArg::enum_arg_hint;
	typename MaybeArg::first_type;
	typename MaybeArg::second_type;
	{ MaybeArg::first };
		requires enums::Enum<typename MaybeArg::first_type>;
		requires std::convertible_to<
			typename MaybeArg::second_type,
				typename EnumMap::template arg_type<MaybeArg::first>::second_type>;
};

template <class EnumMap, std::size_t N, class... Args>
concept IsPartialArgList = requires {
	// Num of args less or equal to num of enum values.
		requires N >= sizeof...(Args);
// Ensure these are using enum_map::arg
	requires (EnumArg<Args, EnumMap> && ...);
// Ensure the enum values are unique
	requires unique_args_check<Args...>();
};

template <class EnumMap, std::size_t N, class... Args>
concept IsArgList = requires {
	// Num of args match num of enum values.
		requires N == sizeof...(Args);
// Ensure these are using enum_map::arg
	requires (EnumArg<Args, EnumMap> && ...);
// Ensure the enum values are unique
	requires unique_args_check<Args...>();
};

template <typename T, std::size_t Size, typename Allocator>
class enum_map_base {
protected:
	// Helper for noexcept specs since MSVC has trouble.
	using DataT = array<T, Size, Allocator>;
	DataT data_;

	constexpr enum_map_base(uninit::uninitialized_tag, const Allocator& alloc = Allocator{})
		CTP_NOEXCEPT(noexcept(DataT(uninit::uninitialized, alloc)))
		: data_{uninit::uninitialized, alloc} {}

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

	// Set all values to a default value.
	explicit constexpr enum_map_base(const T& value, const Allocator& alloc = Allocator{})
		CTP_NOEXCEPT(noexcept(DataT(value, alloc)))
		: data_{value, alloc} {}

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


template <typename... Args>
struct is_enum_args : std::false_type {};

template <typename T>
concept HasEnumHint = requires {typename std::remove_cvref_t<T>::enum_arg_hint; };

template <HasEnumHint Head, typename... Args>
struct is_enum_args<Head, Args...> : std::true_type {};

} // enum_detail

// enum_map, a container that has a value for every named enum value and can be indexed by it.
// Expects that all named values that are used with it are reflected, and that only valid
// named values are used.
template <enums::Enum E, typename T, typename Allocator = std::allocator<T>>
class enum_map : public enum_detail::enum_map_base<T, enums::size<E>(), Allocator> {
protected:
	using Base = enum_detail::enum_map_base<T, enums::size<E>(), Allocator>;
	using this_type = enum_map<E, T, Allocator>;

	constexpr auto arr_begin(this auto& self) noexcept { return self.data_.begin(); }

	template <typename... Args>
	constexpr void pair_construct_with_default(const enum_default_arg<T>& defaultArg, Args&&... args) {
		constexpr bool IsEnumArg = enum_detail::is_enum_args<Args...>::value;
		constexpr auto Given = sizeof...(Args);
		bool found[enums::size<E>()]{};

		const auto do_construct_at = [this, &found](auto&& arg) {
			std::size_t index;
			if constexpr (IsEnumArg) {
				index = enums::index<arg.first>();
			} else {
				index = enums::index(arg.first);
			}

			found[index] = true;
			this->data_.construct_at(index, std::forward<decltype(arg)>(arg).second);
		};

		// Construct non-defaults.
		(do_construct_at(args), ...);

		// Construct default values
		std::size_t remaining = Size - Given;
		for (std::size_t i = 0; remaining > 0; ++i) {
			if (!found[i]) {
				--remaining;
				this->data_.construct_at(i, defaultArg.value);
			}
		}
	}
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

	using iterator = enum_detail::enum_map_iterator_t<E, typename Base::DataT::iterator, false>;
	using const_iterator = enum_detail::enum_map_iterator_t<E, typename Base::DataT::const_iterator, true>;
	using reverse_iterator = ctp::reverse_iterator<iterator>;
	using const_reverse_iterator = ctp::reverse_iterator<const_iterator>;

	static constexpr std::size_t Size = enums::size<E>();

	using Base::size;
	using Base::empty;
	using Base::is_empty;
	using Base::values;
	using Base::get_allocator;
	using Base::data;

	// Explicitly not allowed. All values must be initialized.
	constexpr enum_map() = delete;

	template <E e>
	using arg_type = enum_arg_pair<e, T>;

	// ---------- construct with a default value ----------

	// Construct an enum map with a default value.
	constexpr enum_map(const T& defaultArg) CTP_NOEXCEPT(noexcept(Base(defaultArg)))
		: Base(defaultArg) {}
	// Construct an enum map with a default value.
	constexpr enum_map(const Allocator& alloc, const T& defaultArg) CTP_NOEXCEPT(noexcept(Base(defaultArg, alloc)))
		: Base(defaultArg, alloc) {}
	// Construct an enum map with a default value.
	constexpr enum_map(const enum_default_arg<T>& defaultArg) CTP_NOEXCEPT(noexcept(Base(defaultArg.value)))
		: Base(defaultArg.value) {}
	// Construct an enum map with a default value.
	constexpr enum_map(const Allocator& alloc, const enum_default_arg<T>& defaultArg)
		CTP_NOEXCEPT(noexcept(Base(defaultArg.value, alloc)))
		: Base(defaultArg.value, alloc) {}

	// ------------- construct with enum_arg -------------

	// Construct an enum map with given enum_arg<E::value>(T)
	// Can verify at compile time that all needed values are present.
	template <enum_detail::EnumArg<this_type>... Args>
		requires enum_detail::IsArgList<this_type, Size, Args...>
	constexpr enum_map(Args&&... args) : Base(uninit::uninitialized) {
		((this->data_.construct_at(enums::index<Args::first>(), std::forward<Args>(args).second)), ...);
	}

	// Construct an enum map with given enum_arg<E::value>(T)
	// Can verify at compile time that all needed values are present.
	template <enum_detail::EnumArg<this_type>... Args>
		requires enum_detail::IsArgList<this_type, Size, Args...>
	constexpr enum_map(const Allocator& alloc, Args&&... args) : Base(uninit::uninitialized, alloc) {
		((this->data_.construct_at(enums::index<Args::first>(), std::forward<Args>(args).second)), ...);
	}

	// Construct an enum map with given enum_arg<E::value>(T)
	// Can verify at compile time that there are no duplicates present.
	// Takes an explicit default.
	template <enum_detail::EnumArg<this_type>... Args>
		requires enum_detail::IsPartialArgList<this_type, Size, Args...>
	constexpr enum_map(const enum_default_arg<T>& defaultArg, Args&&... args) : Base(uninit::uninitialized) {
		pair_construct_with_default(defaultArg, std::forward<Args>(args)...);
	}

	// Construct an enum map with given enum_arg<E::value>(T)
	// Can verify at compile time that there are no duplicates present.
	// Takes an explicit default argument.
	template <enum_detail::EnumArg<this_type>... Args>
		requires enum_detail::IsPartialArgList<this_type, Size, Args...>
	constexpr enum_map(const Allocator& alloc, const enum_default_arg<T>& defaultArg, Args&&... args)
		: Base(uninit::uninitialized, alloc) {
		pair_construct_with_default(defaultArg, std::forward<Args>(args)...);
	}

	// ------------- construct with std::pair -------------

	// Constructing with pairs can take variable arguments at run time, but loses the ability to guarantee
	// correctness at compile time.
	template <std::same_as<std::pair<E, T>>... Pairs>
		requires (sizeof...(Pairs) == Size)
	constexpr enum_map(Pairs&&... pairs) : Base(uninit::uninitialized) {
		// Sanity check that we weren't given one enum value twice.
		ctpExpects(enum_detail::unique_pairs_check(pairs...));
		((this->data_[enums::index(pairs.first)] = std::forward<Pairs>(pairs).second), ...);
	}
	// Constructing with pairs can take variable arguments at run time, but loses the ability to guarantee
	// correctness at compile time.
	template <std::same_as<std::pair<E, T>>... Pairs>
		requires (sizeof...(Pairs) == Size)
	constexpr enum_map(const Allocator& alloc, Pairs&&... pairs) : Base(uninit::uninitialized, alloc) {
		// Sanity check that we weren't given one enum value twice.
		ctpExpects(enum_detail::unique_pairs_check(pairs...));
		((this->data_.construct_at(enums::index(pairs.first), std::forward<Pairs>(pairs).second)), ...);
	}

	// Constructing with pairs can take variable arguments at run time, but loses the ability to guarantee
	// correctness at compile time. Takes an explicit default argument.
	template <std::same_as<std::pair<E, T>>... Pairs>
		requires (sizeof...(Pairs) <= Size)
	constexpr enum_map(const enum_default_arg<T>& defaultArg, Pairs&&... pairs) : Base(uninit::uninitialized) {
		// Sanity check that we weren't given one enum value twice.
		ctpExpects(enum_detail::unique_pairs_check(pairs...));
		pair_construct_with_default(defaultArg, std::forward<Pairs>(pairs)...);
	}
	// Constructing with pairs can take variable arguments at run time, but loses the ability to guarantee
	// correctness at compile time. Takes an explicit default argument.
	template <std::same_as<std::pair<E, T>>... Pairs>
		requires (sizeof...(Pairs) <= Size)
	constexpr enum_map(const Allocator& alloc, const enum_default_arg<T>& defaultArg, Pairs&&... pairs)
		: Base(uninit::uninitialized, alloc) {
		// Sanity check that we weren't given one enum value twice.
		ctpExpects(enum_detail::unique_pairs_check(pairs...));
		pair_construct_with_default(defaultArg, std::forward<Pairs>(pairs)...);
	}

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
