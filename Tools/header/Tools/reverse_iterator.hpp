#ifndef INCLUDE_CTP_TOOLS_REVERSE_ITERATOR_HPP
#define INCLUDE_CTP_TOOLS_REVERSE_ITERATOR_HPP

#include "concepts.hpp"
#include "config.hpp"
#include "iterator.hpp"
#include "utility.hpp"

namespace ctp {

namespace iterator_detail {

template <concepts::HasBaseTemplate<iterator_t> Iterator, BaseType>
class reverse_iterator_base;

template <concepts::HasBaseTemplate<iterator_t> Iterator>
class reverse_iterator_base<Iterator, BaseType::Bidirectional> {
protected:
	Iterator it_;
public:
	using iterator_type = Iterator;
	using difference_type = typename Iterator::difference_type;
	using value_type = typename Iterator::value_type;
	using pointer = typename Iterator::pointer;
	using reference = typename Iterator::reference;
	using iterator_concept = typename Iterator::iterator_concept;

	template <std::convertible_to<Iterator> Iter>
	constexpr explicit reverse_iterator_base(Iter&& it) CTP_NOEXCEPT(noexcept(Iterator{forward<Iter>(it)}))
		: it_{forward<Iter>(it)}
	{}
	constexpr reverse_iterator_base() noexcept = default;
	constexpr reverse_iterator_base(const reverse_iterator_base&) noexcept = default;
	constexpr reverse_iterator_base& operator=(const reverse_iterator_base&) noexcept = default;
	constexpr reverse_iterator_base(reverse_iterator_base&&) noexcept = default;
	constexpr reverse_iterator_base& operator=(reverse_iterator_base&&) noexcept = default;

	template <typename U>
		requires (!std::same_as<U, Iterator>&& std::assignable_from<Iterator&, const U&>)
	constexpr reverse_iterator_base& operator=(const reverse_iterator_base<U, BaseType::Bidirectional>& o)
		CTP_NOEXCEPT(std::is_nothrow_assignable_v<Iterator&, const U&>)
	{
		it_ = o.it_;
	}

	[[nodiscard]] constexpr const iterator_type& base() const & CTP_NOEXCEPT(std::is_nothrow_copy_assignable_v<Iterator>) {
		return it_;
	}
	[[nodiscard]] constexpr iterator_type base() && CTP_NOEXCEPT(std::is_nothrow_copy_assignable_v<Iterator>) {
		return move(it_);
	}

	[[nodiscard]] constexpr reference operator*() const
		CTP_NOEXCEPT(noexcept(base()) && noexcept(*(it_ - 1)))
	{
		return *(it_ - 1);
	}

	[[nodiscard]] constexpr pointer operator->() const
		CTP_NOEXCEPT(noexcept(it_) && noexcept((it_ - 1).operator->()))
	{
		return (it_ - 1).operator->();
	}

	constexpr auto& operator++(this auto&& self) CTP_NOEXCEPT(noexcept(--it_)) {
		--self.it_;
		return self;
	}
	[[nodiscard]] constexpr auto operator++(this auto&& self, int) CTP_NOEXCEPT(noexcept(reverse_iterator_base{it_--})) {
		auto temp = self;
		--self.it_;
		return temp;
	}
	constexpr auto& operator+=(this auto&& self, difference_type offset) CTP_NOEXCEPT(noexcept(it_ -= offset)) {
		self.it_ -= offset;
		return self;
	}

	constexpr auto& operator--(this auto&& self) CTP_NOEXCEPT(noexcept(++it_)) {
		++self.it_;
		return self;
	}
	[[nodiscard]] constexpr auto operator--(this auto&& self, int) CTP_NOEXCEPT(noexcept(reverse_iterator_base{it_++})) {
		auto temp = self;
		++self.it_;
		return temp;
	}

	[[nodiscard]] friend constexpr bool operator==(const reverse_iterator_base& lhs, const reverse_iterator_base& rhs)
		CTP_NOEXCEPT(noexcept(lhs.it_ == rhs.it_))
	{
		return lhs.it_ == rhs.it_;
	}
};

template <concepts::HasBaseTemplate<iterator_t> Iterator>
class reverse_iterator_base<Iterator, BaseType::Random>
	: public reverse_iterator_base<Iterator, BaseType::Bidirectional> {
	using base_type = reverse_iterator_base<Iterator, BaseType::Bidirectional>;
public:
	using difference_type = typename base_type::difference_type;
	using reference = typename base_type::reference;

	template <std::convertible_to<Iterator> Iter>
	constexpr explicit reverse_iterator_base(Iter&& it) CTP_NOEXCEPT(noexcept(base_type{forward<Iter>(it)}))
		: base_type{forward<Iter>(it)}
	{}
	constexpr reverse_iterator_base() noexcept = default;
	constexpr reverse_iterator_base(const reverse_iterator_base&) noexcept = default;
	constexpr reverse_iterator_base& operator=(const reverse_iterator_base&) noexcept = default;
	constexpr reverse_iterator_base(reverse_iterator_base&&) noexcept = default;
	constexpr reverse_iterator_base& operator=(reverse_iterator_base&&) noexcept = default;

	template <typename U>
		requires (!std::same_as<U, Iterator>&& std::assignable_from<Iterator&, const U&>)
	constexpr reverse_iterator_base& operator=(const reverse_iterator_base<U, BaseType::Random>& o)
		CTP_NOEXCEPT(std::is_nothrow_assignable_v<Iterator&, const U&>)
	{
		this->it_ = o.base();
	}

	[[nodiscard]] constexpr reference operator[](difference_type offset) const CTP_NOEXCEPT(noexcept(this->it_[-1])) {
		return this->it_[-offset - 1];
	}

	constexpr reverse_iterator_base& operator+=(difference_type offset) CTP_NOEXCEPT(noexcept(this->it_ -= offset)) {
		this->it_ -= offset;
		return *this;
	}

	constexpr reverse_iterator_base& operator-=(difference_type offset)
		CTP_NOEXCEPT(noexcept(this->it_ += offset))
	{
		this->it_ += offset;
		return *this;
	}

	[[nodiscard]] friend constexpr reverse_iterator_base operator+(const reverse_iterator_base& iter, difference_type offset)
		CTP_NOEXCEPT(noexcept(reverse_iterator_base{iter.it_ - offset}))
	{
		return reverse_iterator_base{iter.it_ - offset};
	}
	[[nodiscard]] friend constexpr reverse_iterator_base operator-(const reverse_iterator_base& iter, difference_type offset)
		CTP_NOEXCEPT(noexcept(reverse_iterator_base{iter.it_ + offset}))
	{
		return reverse_iterator_base{iter.it_ + offset};
	}

	[[nodiscard]] friend constexpr reverse_iterator_base operator+(difference_type offset, const reverse_iterator_base& iter)
		CTP_NOEXCEPT(noexcept(reverse_iterator_base{iter.it_ - offset}))
	{
		return reverse_iterator_base{iter.it_ - offset};
	}

	[[nodiscard]] friend constexpr auto operator<=>(const reverse_iterator_base& lhs, const reverse_iterator_base& rhs)
		CTP_NOEXCEPT(noexcept(rhs.it_ <=> lhs.it_))
	{
		return rhs.it_ <=> lhs.it_;
	}

	template <typename Iter1, typename Iter2>
	friend constexpr auto operator-(
		const reverse_iterator_base<Iter1, BaseType::Random>& lhs,
		const reverse_iterator_base<Iter2, BaseType::Random>& rhs)
		CTP_NOEXCEPT(noexcept(rhs.it_ - lhs.it_))
		-> decltype(rhs.it_ - lhs.it_);
};

template <typename I>
reverse_iterator_base(I) -> reverse_iterator_base<I, tag_to_base_type_v<typename I::iterator_concept>>;

template <typename Iter1, typename Iter2>
[[nodiscard]] constexpr auto operator-(
	const reverse_iterator_base<Iter1, BaseType::Random>& lhs,
	const reverse_iterator_base<Iter2, BaseType::Random>& rhs)
	CTP_NOEXCEPT(noexcept(rhs.it_ - lhs.it_))
	-> decltype(rhs.it_ - lhs.it_)
{
	return rhs.it_ - lhs.it_;
}

} // iterator_detail

// Reverse iterator, to only be used with iterator_t for now as it does not check for
// iterator_traits or indirectly_readable_traits.
template <concepts::HasBaseTemplate<iterator_t> Iterator>
using reverse_iterator =
	iterator_detail::reverse_iterator_base<
	                                       Iterator,
	                                       iterator_detail::tag_to_base_type_v<typename Iterator::iterator_concept>>;

template <class I>
reverse_iterator<I> make_reverse_iterator(I it) noexcept(std::is_nothrow_constructible_v<I>) {
	return reverse_iterator<I>(move(it));
}


} // ctp

#endif // INCLUDE_CTP_TOOLS_REVERSE_ITERATOR_HPP
