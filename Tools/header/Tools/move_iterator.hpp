#ifndef INCLUDE_CTP_TOOLS_MOVE_ITERATOR_HPP
#define INCLUDE_CTP_TOOLS_MOVE_ITERATOR_HPP

#include "concepts.hpp"
#include "config.hpp"
#include "iterator.hpp"
#include "iter_move.hpp"
#include "utility.hpp"

namespace ctp {

namespace iterator_detail {

template <concepts::HasBaseTemplate<iterator_t> Iterator, BaseType>
class move_iterator_base;

template <concepts::HasBaseTemplate<iterator_t> Iterator>
class move_iterator_base<Iterator, BaseType::Bidirectional> {
protected:
	Iterator it_;
public:
	using iterator_type = Iterator;
	using difference_type = typename Iterator::difference_type;
	using value_type = typename Iterator::value_type;
	using pointer = typename Iterator::pointer;
	using reference = decltype(iterator_detail::iter_move(std::declval<Iterator&>()));
	using iterator_concept = typename Iterator::iterator_concept;

	template <std::convertible_to<Iterator> Iter>
	constexpr explicit move_iterator_base(Iter&& it) CTP_NOEXCEPT(noexcept(Iterator{forward<Iter>(it)}))
		: it_{forward<Iter>(it)}
	{}
	constexpr move_iterator_base() noexcept = default;
	constexpr move_iterator_base(const move_iterator_base&) noexcept = default;
	constexpr move_iterator_base& operator=(const move_iterator_base&) noexcept = default;
	constexpr move_iterator_base(move_iterator_base&&) noexcept = default;
	constexpr move_iterator_base& operator=(move_iterator_base&&) noexcept = default;

	template <typename U>
		requires (!std::same_as<U, Iterator>&& std::assignable_from<Iterator&, const U&>)
	constexpr move_iterator_base& operator=(const move_iterator_base<U, BaseType::Bidirectional>& o)
		CTP_NOEXCEPT(std::is_nothrow_assignable_v<Iterator&, const U&>)
	{
		it_ = o.it_;
	}

	[[nodiscard]] constexpr const iterator_type& base() const & noexcept {
		return it_;
	}
	[[nodiscard]] constexpr iterator_type base() && CTP_NOEXCEPT(std::is_nothrow_move_constructible_v<Iterator>) {
		return move(it_);
	}

	[[nodiscard]] constexpr reference operator*() const CTP_NOEXCEPT(noexcept(iterator_detail::iter_move(it_)))
	{
		return iterator_detail::iter_move(it_);
	}

	[[nodiscard]] constexpr pointer operator->() const CTP_NOEXCEPT(noexcept(it_.operator->()))
	{
		return it_.operator->();
	}

	constexpr auto& operator++(this auto&& self) CTP_NOEXCEPT(noexcept(++it_)) {
		++self.it_;
		return self;
	}
	[[nodiscard]] constexpr auto operator++(this auto&& self, int) CTP_NOEXCEPT(noexcept(move_iterator_base{it_++})) {
		auto temp = self;
		++self.it_;
		return temp;
	}
	constexpr auto& operator+=(this auto&& self, difference_type offset) CTP_NOEXCEPT(noexcept(it_ += offset)) {
		self.it_ += offset;
		return self;
	}

	constexpr auto& operator--(this auto&& self) CTP_NOEXCEPT(noexcept(--it_)) {
		--self.it_;
		return self;
	}
	[[nodiscard]] constexpr auto operator--(this auto&& self, int) CTP_NOEXCEPT(noexcept(move_iterator_base{it_--})) {
		auto temp = self;
		--self.it_;
		return temp;
	}

	[[nodiscard]] friend constexpr bool operator==(const move_iterator_base& lhs, const move_iterator_base& rhs)
		CTP_NOEXCEPT(noexcept(lhs.it_ == rhs.it_))
	{
		return lhs.it_ == rhs.it_;
	}
};

template <concepts::HasBaseTemplate<iterator_t> Iterator>
class move_iterator_base<Iterator, BaseType::Random>
	: public move_iterator_base<Iterator, BaseType::Bidirectional> {
	using base_type = move_iterator_base<Iterator, BaseType::Bidirectional>;
public:
	using difference_type = typename base_type::difference_type;
	using reference = typename base_type::reference;

	template <std::convertible_to<Iterator> Iter>
	constexpr explicit move_iterator_base(Iter&& it) CTP_NOEXCEPT(noexcept(base_type{forward<Iter>(it)}))
		: base_type{forward<Iter>(it)}
	{}
	constexpr move_iterator_base() noexcept = default;
	constexpr move_iterator_base(const move_iterator_base&) noexcept = default;
	constexpr move_iterator_base& operator=(const move_iterator_base&) noexcept = default;
	constexpr move_iterator_base(move_iterator_base&&) noexcept = default;
	constexpr move_iterator_base& operator=(move_iterator_base&&) noexcept = default;

	template <typename U>
		requires (!std::same_as<U, Iterator>&& std::assignable_from<Iterator&, const U&>)
	constexpr move_iterator_base& operator=(const move_iterator_base<U, BaseType::Random>& o)
		CTP_NOEXCEPT(std::is_nothrow_assignable_v<Iterator&, const U&>)
	{
		this->it_ = o.base();
	}

	[[nodiscard]] constexpr reference operator[](difference_type offset) const
		CTP_NOEXCEPT(noexcept(iterator_detail::iter_move(this->it_ + offset))) {
		return iterator_detail::iter_move(this->it_ + offset);
	}

	constexpr move_iterator_base& operator+=(difference_type offset) CTP_NOEXCEPT(noexcept(this->it_ -= offset)) {
		this->it_ -= offset;
		return *this;
	}

	constexpr move_iterator_base& operator-=(difference_type offset)
		CTP_NOEXCEPT(noexcept(this->it_ += offset))
	{
		this->it_ += offset;
		return *this;
	}

	[[nodiscard]] friend constexpr move_iterator_base operator+(const move_iterator_base& iter, difference_type offset)
		CTP_NOEXCEPT(noexcept(move_iterator_base{iter.it_ - offset}))
	{
		return move_iterator_base{iter.it_ - offset};
	}
	[[nodiscard]] friend constexpr move_iterator_base operator-(const move_iterator_base& iter, difference_type offset)
		CTP_NOEXCEPT(noexcept(move_iterator_base{iter.it_ + offset}))
	{
		return move_iterator_base{iter.it_ + offset};
	}

	[[nodiscard]] friend constexpr move_iterator_base operator+(difference_type offset, const move_iterator_base& iter)
		CTP_NOEXCEPT(noexcept(move_iterator_base{iter.it_ - offset}))
	{
		return move_iterator_base{iter.it_ - offset};
	}

	[[nodiscard]] friend constexpr auto operator<=>(const move_iterator_base& lhs, const move_iterator_base& rhs)
		CTP_NOEXCEPT(noexcept(rhs.it_ <=> lhs.it_))
	{
		return rhs.it_ <=> lhs.it_;
	}

	template <typename Iter1, typename Iter2>
	friend constexpr auto operator-(
		const move_iterator_base<Iter1, BaseType::Random>& lhs,
		const move_iterator_base<Iter2, BaseType::Random>& rhs)
		CTP_NOEXCEPT(noexcept(lhs.it_ - rhs.it_))
		-> decltype(lhs.it_ - rhs.it_);;
};

template <typename I>
move_iterator_base(I) -> move_iterator_base<I, tag_to_base_type_v<typename I::iterator_concept>>;

template <typename Iter1, typename Iter2>
[[nodiscard]] constexpr auto operator-(
	const move_iterator_base<Iter1, BaseType::Random>& lhs,
	const move_iterator_base<Iter2, BaseType::Random>& rhs)
	CTP_NOEXCEPT(noexcept(lhs.it_ - rhs.it_))
	-> decltype(lhs.it_ - rhs.it_)
{
	return lhs.it_ - rhs.it_;
}

} // iterator_detail

// Move iterator, to only be used with iterator_t for now as it does not check for
// iterator_traits or indirectly_readable_traits.
template <concepts::HasBaseTemplate<iterator_t> Iterator>
using move_iterator =
iterator_detail::move_iterator_base<
	Iterator,
	iterator_detail::tag_to_base_type_v<typename Iterator::iterator_concept>>;

template <class I>
move_iterator<I> make_move_iterator(I it) noexcept(std::is_nothrow_constructible_v<I>) {
	return move_iterator<I>(move(it));
}

} // ctp

#endif // INCLUDE_CTP_TOOLS_MOVE_ITERATOR_HPP
