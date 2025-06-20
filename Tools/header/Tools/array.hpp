#ifndef INCLUDE_CTP_TOOLS_ARRAY_HPP
#define INCLUDE_CTP_TOOLS_ARRAY_HPP

#include "config.hpp"
#include "iterator.hpp"
#include "reverse_iterator.hpp"
#include "uninitialized_storage.hpp"
#include "trivial_allocator_adapter.hpp"

#include <memory>
#include <type_traits>

#if CTP_USE_EXCEPTIONS
#include <stdexcept>
#include <format>
#endif

namespace ctp {

// Allocator-aware array. Storage is on the stack, but can use the provided allocator
// for elements.
template <typename T, std::size_t N, typename Allocator = std::allocator<T>>
class array {
protected:
	using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using alloc_traits = std::allocator_traits<rebind_alloc>;

	using storage_type = uninit::array<T, N>;

	storage_type storage_;
	// Potentially only have this conditionally if the stored elements can actually use the allocator?
	// Could more easily do this with a helper template to get teh array type, and this is the base class.
	CTP_NO_UNIQUE_ADDRESS rebind_alloc alloc_;

	// Used to dispatch copy/move constructors which would otherwise be implicitly deleted.
	struct dispatch_to_template_tag {};
	template <typename A2>
	constexpr array(dispatch_to_template_tag, const array<T, N, A2>& o)
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		: array(o, alloc_traits::select_on_container_copy_construction(o.alloc_)) {}
	template <typename A2>
	constexpr array(dispatch_to_template_tag, array<T, N, A2>&& o)
		noexcept(std::is_nothrow_move_constructible_v<T>)
		: array(std::move(o), std::move(o.alloc_)) {}
	template <typename A2>
	constexpr array(array<T, N, A2>&& o, Allocator&& alloc)
		noexcept(std::is_nothrow_move_constructible_v<T>);

	template <class A2>
	constexpr array& copy_assignment(const array<T, N, A2>& o)
		noexcept(std::is_nothrow_copy_assignable_v<T>);
	template <class A2>
	constexpr array& move_assignment(array<T, N, A2>&& o)
		noexcept(std::is_nothrow_move_assignable_v<T>);

public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

	using iterator = typename storage_type::iterator;
	using const_iterator = typename storage_type::const_iterator;
	using reverse_iterator = ctp::reverse_iterator<iterator>;
	using const_reverse_iterator = ctp::reverse_iterator<const_iterator>;

	using allocator_type = Allocator;

	constexpr array()
		CTP_NOEXCEPT(std::is_nothrow_default_constructible_v<Allocator> && std::is_nothrow_default_constructible_v<T>)
		requires (std::is_default_constructible_v<T>)
	{
		uninit::construct_n(alloc_, begin(), N);
	}
	constexpr array(const Allocator& alloc)
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<Allocator> && std::is_nothrow_default_constructible_v<T>)
		requires (std::is_default_constructible_v<T>)
		: alloc_{alloc} {
		uninit::construct_n(alloc_, begin(), N);
	}
	constexpr array(const T& value, const Allocator& alloc = Allocator{})
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<Allocator> && std::is_nothrow_copy_constructible_v<T>)
		: alloc_{alloc} {
		uninit::construct_n(alloc_, begin(), N, value);
	}

	template <typename A2>
	constexpr array(const array<T, N, A2>& o)
		CTP_NOEXCEPT(noexcept(array{dispatch_to_template_tag{}, o}))
		: array{dispatch_to_template_tag{}, o} {}
	constexpr array(const array& o)
		CTP_NOEXCEPT(noexcept(array{dispatch_to_template_tag{}, o}))
		: array{dispatch_to_template_tag{}, o} {}
	template <typename A2>
	constexpr array(const array<T, N, A2>& o, const Allocator& alloc)
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<Allocator> && std::is_nothrow_constructible_v<T>);

	template <typename A2>
	constexpr array(array<T, N, A2>&& o)
		CTP_NOEXCEPT(noexcept(array{dispatch_to_template_tag{}, std::move(o)}))
		: array{dispatch_to_template_tag{}, std::move(o)} {}
	constexpr array(array&& o)
		CTP_NOEXCEPT(noexcept(array{dispatch_to_template_tag{}, std::move(o)}))
		: array{dispatch_to_template_tag{}, std::move(o)} {}
	template <typename A2>
	constexpr array(array<T, N, A2>&& o, const Allocator& alloc)
		CTP_NOEXCEPT(noexcept(array{std::move(o), Allocator{alloc}}))
		: array{std::move(o), Allocator{alloc}} {}

	template <class A2>
	constexpr array& operator=(const array<T, N, A2>& o)
		CTP_NOEXCEPT(std::is_nothrow_default_constructible_v<Allocator>&& std::is_nothrow_copy_assignable_v<T>) {
		return this->copy_assignment(o);
	}
	constexpr array& operator=(const array& o)
		CTP_NOEXCEPT(std::is_nothrow_default_constructible_v<Allocator>&& std::is_nothrow_copy_assignable_v<T>) {
		return this->copy_assignment(o);
	}

	template <class A2>
	constexpr array& operator=(array<T, N, A2>&& o)
		CTP_NOEXCEPT(std::is_nothrow_default_constructible_v<Allocator>&& std::is_nothrow_move_assignable_v<T>) {
		return this->move_assignment(std::move(o));
	}
	constexpr array& operator=(array&& o)
		CTP_NOEXCEPT(std::is_nothrow_default_constructible_v<Allocator>&& std::is_nothrow_move_assignable_v<T>) {
		return this->move_assignment(std::move(o));
	}

	template <concepts::AllConvertibleTo<T>... Ts>
		requires (sizeof...(Ts) == N)
	constexpr array(Ts&&... values)
		CTP_NOEXCEPT(std::is_nothrow_default_constructible_v<Allocator>
			&& noexcept((uninit::do_construct_at(alloc_, std::declval<iterator>(), std::forward<Ts>(values)), ...)))
	{
		std::size_t i = 0;
		(uninit::do_construct_at(alloc_, storage_.get(i++), std::forward<Ts>(values)), ...);
	}

	template <concepts::AllConvertibleTo<T>... Ts>
		requires (sizeof...(Ts) == N)
	constexpr array(const Allocator& alloc, Ts&&... values)
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<Allocator>
			&& noexcept((uninit::do_construct_at(alloc_, std::declval<iterator>(), std::forward<Ts>(values)), ...)))
		: alloc_{alloc} {
		std::size_t i = 0;
		(uninit::do_construct_at(alloc_, storage_.get(i++), std::forward<Ts>(values)), ...);
	}

	template <class A2>
	constexpr void swap(array<T, N, A2>& o) noexcept(std::is_nothrow_move_assignable_v<T>);

	constexpr ~array() noexcept { uninit::destroy_n(alloc_, begin(), N); }

	[[nodiscard]] constexpr static std::size_t size() noexcept { return N; }
	[[nodiscard]] constexpr static bool empty() noexcept { return N == 0; }
	[[nodiscard]] constexpr static bool is_empty() noexcept { return N == 0; }
	[[nodiscard]] constexpr static std::size_t max_size() noexcept { return N; }

	[[nodiscard]] constexpr pointer data() noexcept { return storage_.data_ptr(); }
	[[nodiscard]] constexpr const_pointer data() const noexcept { return storage_.data_ptr(); }

	[[nodiscard]] constexpr iterator begin() noexcept { return storage_.begin(); }
	[[nodiscard]] constexpr const_iterator begin() const noexcept { return storage_.begin(); }
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
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
	[[nodiscard]] constexpr iterator last() noexcept { return storage_.get(size() - 1); }
	// Get iterator to the last element.
	[[nodiscard]] constexpr const_iterator last() const noexcept { return storage_.get(size() - 1); }

	[[nodiscard]] constexpr reference front() noexcept { return storage_[0]; }
	[[nodiscard]] constexpr const_reference front() const noexcept { return storage_[0]; }
	[[nodiscard]] constexpr reference back() noexcept { return storage_[size() - 1]; }
	[[nodiscard]] constexpr const_reference back() const noexcept { return storage_[size() - 1]; }

	[[nodiscard]] constexpr reference operator[](std::size_t i) noexcept {
		ctpExpects(i < size());
		return storage_[i];
	}

	[[nodiscard]] constexpr const_reference operator[](std::size_t i) const noexcept {
		ctpExpects(i < size());
		return storage_[i];
	}

	[[nodiscard]] constexpr auto at(this auto& self, std::size_t i) CTP_NOEXCEPT(false) -> decltype(self[i]) {
		if (i >= size()) [[unlikely]] {
#if CTP_USE_EXCEPTIONS
			throw std::out_of_range(
				std::format("ctp::array index out of range (requested: {} size: {})", i, size()));
#else
			std::terminate();
#endif
		}
		return self.storage_[i];
	}

	constexpr void fill(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		for (std::size_t i = 0; i < size(); ++i)
			storage_[i] = value;
	}
};

template< class T, class... U >
array(T, U...) -> array<T, 1 + sizeof...(U)>;

template <typename T, std::size_t N, typename Allocator>
template <typename A2>
constexpr array<T, N, Allocator>::
array(const array<T, N, A2>& o, const Allocator& alloc)
	CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<Allocator>&& std::is_nothrow_constructible_v<T>)
	: alloc_{alloc}
{
	for (size_type i = 0; i < N; ++i)
		uninit::do_construct_at(alloc_, storage_.get(i), o[i]);
}

template <typename T, std::size_t N, typename Allocator>
template <typename A2>
constexpr array<T, N, Allocator>::
array(array<T, N, A2>&& o, Allocator&& alloc) noexcept(std::is_nothrow_move_constructible_v<T>)
	: alloc_{std::move(alloc)}
{
	for (size_type i = 0; i < N; ++i)
		uninit::do_construct_at(alloc_, storage_.get(i), std::move(o[i]));
}

template <typename T, std::size_t N, typename Allocator>
template <typename A2>
constexpr array<T, N, Allocator>& array<T, N, Allocator>::
copy_assignment(const array<T, N, A2>& o) noexcept(std::is_nothrow_copy_assignable_v<T>)
{
	if constexpr (std::same_as<array, array<T, N, A2>>) {
		if (this == &o) [[unlikely]]
			return *this;
	}

	if constexpr (alloc_traits::propagate_on_container_copy_assignment::value) {
		if constexpr (!alloc_traits::is_always_equal::value) {
			if (alloc_ != o.alloc_) {
				// Incompatible allocators, destroy any old elements.
				uninit::destroy_n(alloc_, data(), N);
				alloc_ = o.alloc_;
				for (std::size_t i = 0; i < N; ++i)
					uninit::do_construct_at(alloc_, storage_.get(i), o[i]);
				return *this;
			}
		}
		alloc_ = o.alloc_;
	}

	for (std::size_t i = 0; i < N; ++i)
		storage_[i] = o[i];

	return *this;
}

template <typename T, std::size_t N, typename Allocator>
template <typename A2>
constexpr array<T, N, Allocator>& array<T, N, Allocator>::
move_assignment(array<T, N, A2>&& o) noexcept(std::is_nothrow_move_assignable_v<T>)
{
	if constexpr (std::same_as<array, array<T, N, A2>>) {
		if (this == &o) [[unlikely]]
			return *this;
	}

	if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
		for (std::size_t i = 0; i < N; ++i)
			storage_[i] = std::move(o[i]);
		alloc_ = std::move(o.alloc_);
		return *this;
	} else {
		if constexpr (!alloc_traits::is_always_equal::value) {
			if (alloc_ != o.alloc_) [[unlikely]] {
				// Incompatible allocators, destroy any old elements.
				uninit::destroy_n(alloc_, data(), N);
				for (std::size_t i = 0; i < N; ++i)
					uninit::do_construct_at(alloc_, storage_.get(i), std::move(o[i]));
				return *this;
			}
		}
		for (std::size_t i = 0; i < N; ++i)
			storage_[i] = std::move(o[i]);
		return *this;
	}
}

template <typename T, std::size_t N, typename Allocator>
template <typename A2>
constexpr void array<T, N, Allocator>::
swap(array<T, N, A2>& o) noexcept(std::is_nothrow_move_assignable_v<T>) {
	using std::swap;

	if constexpr (alloc_traits::propagate_on_container_swap::value) {
		for (std::size_t i = 0; i < N; ++i)
			swap(storage_[i], o[i]);
		swap(alloc_, o.alloc_);
	} else {
		if constexpr (!alloc_traits::is_always_equal::value) {
			if (alloc_ != o.alloc_) {
				// Undefined behaviour. Don't think this is valuable to attempt handling.
				std::unreachable();
			}
		}
		for (std::size_t i = 0; i < N; ++i)
			swap(storage_[i], o[i]);
	}
}

template <typename T, std::size_t N, typename A1, typename A2>
constexpr bool operator==(const array<T, N, A1>& lhs, const array<T, N, A2>& rhs) noexcept
{
	for (std::size_t i = 0; i < N; ++i) {
		if (lhs[i] != rhs[i])
			return false;
	}
	return true;
}

template <typename T, std::size_t N, typename A1, typename A2>
constexpr compare_three_way_type_t<T>
operator<=>(const array<T, N, A1>& lhs, const array<T, N, A2>& rhs) noexcept {
	for (std::size_t i = 0; i < N; ++i) {
		if (const auto comp = lhs[i] <=> rhs[i]; comp != 0)
			return comp;
	}

	return std::strong_ordering::equal;
}

// Does not initialize trivially-constructible types.
template <typename T, std::size_t N>
using trivial_array = array<T, N, trivial_init_allocator<T>>;

} // ctp


namespace std {

// STL tuple-like helper functions

template <class T, std::size_t N, typename A>
struct tuple_size<::ctp::array<T, N, A>> : std::integral_constant<std::size_t, N> {};

template <std::size_t I, class T, std::size_t N, typename A>
struct tuple_element<I, ::ctp::array<T, N, A>> {
	using type = T;
};

template <std::size_t I, typename T, std::size_t N, typename A>
	requires (I < N)
constexpr T& get(ctp::array<T, N, A>& a) noexcept { return a[I]; }

template <std::size_t I, typename T, std::size_t N, typename A>
	requires (I < N)
constexpr const T& get(const ctp::array<T, N, A>& a) noexcept { return a[I]; }

template <std::size_t I, typename T, std::size_t N, typename A>
	requires (I < N)
constexpr T&& get(ctp::array<T, N, A>&& a) noexcept { return std::move(a[I]); }

template <std::size_t I, typename T, std::size_t N, typename A>
	requires (I < N)
constexpr const T&& get(const ctp::array<T, N, A>&& a) noexcept { return std::move(a[I]); }

} // std

#endif // INCLUDE_CTP_TOOLS_ARRAY_HPP
