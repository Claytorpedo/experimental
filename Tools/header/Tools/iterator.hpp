#ifndef INCLUDE_CTP_TOOLS_ITERATOR_HPP
#define INCLUDE_CTP_TOOLS_ITERATOR_HPP

#include "config.hpp"
#include "CrtpHelper.hpp"
#include "type_traits.hpp"

#include <memory> // addressof

// Avoid including <iterator>, as we only need these tags and it's huge.
namespace std {
struct input_iterator_tag;
struct forward_iterator_tag;
struct bidirectional_iterator_tag;
struct random_access_iterator_tag;
struct contiguous_iterator_tag;
} // std

namespace ctp {

template <typename Derived, typename ValueType, typename IteratorType, typename DifferenceType, typename ReferenceType>
class iterator_t;

// Friend iterator_accessor to enable making iterator_t support functions private.
class iterator_accessor;

// iterator_t
//
// iterator creation helper based on boost/iterator_facade that is constexpr/noexcept friendly.
// The user declares their type of iterator by setting the types in the base class iterator_t,
// and defining requires functionality by implementing the member functions in the table below.
// 
// When more than one provided function can be used for an operation, the "most specialized"
// function will be selected, allowing behavioural overrides if desired.
// For example, operator== can be implemented with:
//		equals (most specialized);   distance_to;   or   get_index (least specialized)
// depending on what the derived class provides.
//
//
//      Derived class function signature                      | description
// ----------------------------------------------------------------------------------------------
//                                                            | Peek the iterator to get a pointer to the value type.
//                                                            |
//                                                            | Some STL implementations use operator->() on iterators
//                                                            | to fetch a pointer to end(), so we use a pointer here.
// *pointer type*   peek()                                    | This also unfortunately means that if peek() does range
//                                                            | validation, it should include the one-past-the-end
//                                                            | position.
//                                                            |
//                                                            | pointer type should either be value_type*, or a plain
//                                                            | value if the value is generated/expiring.
// -----------------------------------------------------------|----------------------------------
//                                                            | Direct access to the iterator's index type, for
//                                                            | random access iterators. Implementing this function
//                                                            | obviates the need for all remaining functions.
// *index type*     get_index()                               | 
//                                                            | IndexType should be a reference to an index
//                                                            | (e.g. std::size_t&), or a proxy that defines the
//                                                            | operations outlined by IndexLike, found below.
// -----------------------------------------------------------|----------------------------------
//                                                            | Advance this iterator forward or backward by the given
// *unused*         advance(difference_type)                  | amount.
//                                                            | For random access iterators that can't use get_index.
// -----------------------------------------------------------|----------------------------------
// difference_type  distance_to(const Derived& other) const   | Distance to another iterator as if "other - this".
// -----------------------------------------------------------|----------------------------------
// *bool-like*      equals(const Derived& other) const        | Equality comparison with another iterator.
// -----------------------------------------------------------|----------------------------------
// Derived&         pre_increment()                           | Pre-increment the iterator.
//                                                            | For input/forward/bidirectional iterators.
// -----------------------------------------------------------|----------------------------------
// Derived&         pre_decrement()                           | Pre-decrement the iterator.
//                                                            | Primarily for bidirectional iterators.
// -----------------------------------------------------------|----------------------------------

// -------------------------------------------------------------------------------------------
// Special Operator Attributes for Expiring Values
//
// If creating an input iterator whose values expire after incrementing, the postfix operator++
// needs to use a proxy to keep the value alive for use. Enable with:
// using iterator_postfix_values_expire = void;
//
// If creating a random access or contiguous iterator whos references may expire if a particular
// iterator expires (e.g. the iterator generates a value and holds it), then operator[] needs
// to use a proxy to keep the value alive for use. Enable with:
// using iterator_brackets_values_expire = void;
// -------------------------------------------------------------------------------------------

// This macro provides the function head for the converting constructor from non-const
// iterators to const iterators, with the non-const iterator named "other".
// It expects a simple iterator type defined like:
//     template <typename ValueType>
//     class my_derived_t<ValueType> :
//         public iterator_t<my_derived_t<ValueType>, ValueType, ... other types ... >
//     {
//       ...
//       // Common use-case with default macro.
//       CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(my_derived_t, ValueType)
//           : my_var_{other.my_var_}
//       {}
//     };
//     using iterator       = my_derived_t<my_value>;
//     using const_iterator = my_derived_t<const my_value>;
// where ValueType is used to make the const and non-const versions of the iterator.
#define CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD( \
	DerivedTemplateType, ValueType, NonConstValueType, ConstValueType, ClassOrStruct, MaybeConstexpr, NoThrowSpec ) \
	private: \
		friend ClassOrStruct DerivedTemplateType<ConstValueType>; \
		using nonconst_t = ::std::conditional_t< \
			::std::is_same_v<ValueType, NonConstValueType>, \
				::ctp::nonesuch, \
				DerivedTemplateType<NonConstValueType>>; \
	public: \
		template <typename NonConstT = nonconst_t, \
			std::enable_if_t<!::std::is_same_v<NonConstT, ::ctp::nonesuch>, int> = 0> \
		MaybeConstexpr DerivedTemplateType(const nonconst_t& other) NoThrowSpec
// Default is a class with constexpr and noexcept constructor, where the non-const
// value type can be obtained by removing any inner const from a value, reference, or pointer.
#define CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(DerivedTemplateType, ValueType) \
	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD( \
		DerivedTemplateType, \
		ValueType, \
		::std::remove_const_t<ValueType>, \
		::std::add_const_t<ValueType>, \
		class, \
		constexpr, \
		noexcept )

namespace iterator_detail {

template <class ReferenceType>
struct op_arrow {
	class proxy {
		ReferenceType ref_;
	public:
		template <typename R>
		constexpr proxy(R&& ref) CTP_NOEXCEPT(noexcept(ReferenceType{std::forward<R>(ref)}))
			: ref_{std::forward<R>(ref)} {}
		constexpr auto operator->() noexcept { return std::addressof(ref_); }
	};
	template <typename R>
	static constexpr proxy do_op(R&& r) noexcept(noexcept(proxy{std::forward<R>(r)})) {
		return proxy{std::forward<R>(r)};
	}
};
// Normal pointer, noop.
template <typename T>
struct op_arrow<T*> { static constexpr auto do_op(T* t) noexcept { return t; } };

template <class Iterator>
class brackets_proxy {
	Iterator it_;
public:
	constexpr brackets_proxy(const Iterator& it) CTP_NOEXCEPT(noexcept(Iterator{it})) : it_{it} {}
	constexpr operator typename Iterator::reference() const CTP_NOEXCEPT(noexcept(*it_)) {
		return *it_;
	}
	constexpr auto& operator=(const typename Iterator::value_type& v) CTP_NOEXCEPT(noexcept(*it_ = v)) {
		*it_ = v;
		return *this;
	}
	constexpr auto& operator=(typename Iterator::value_type&& v) CTP_NOEXCEPT(noexcept(*it_ = std::move(v))) {
		*it_ = std::move(v);
		return *this;
	}
};

template <typename T>
class input_iterator_expiring_value_postfix_proxy {
	mutable T value_;
public:
	template <typename R>
	constexpr input_iterator_expiring_value_postfix_proxy(R&& val)
		CTP_NOEXCEPT(noexcept(T{std::forward<R>(val)}))
		: value_{std::forward<R>(val)}
	{}
	constexpr auto& operator*() const noexcept { return value_; }
};

template <typename I, typename DiffT>
concept IndexLike = requires (I i, DiffT n) {
	// Operations used with get_index.
	++i;
	--i;
	i += n;
	{ i - i } -> std::convertible_to<DiffT>;
	i == i;
};

// See if we have e.g. one-const and one non const iterator where the non-const can be converted to const.
template <typename Iterator1, typename Iterator2>
concept iterators_interopable = std::disjunction_v<
	std::is_convertible<Iterator1, Iterator2>,
	std::is_convertible<Iterator2, Iterator1>>;

template <typename Iterator1, typename Iterator2>
struct iterators_interop_type : std::conditional<std::is_convertible_v<Iterator1, Iterator2>, Iterator2, Iterator1> {};
template <typename Iterator1, typename Iterator2>
using iterators_interop_t = typename iterators_interop_type<Iterator1, Iterator2>::type;

enum class BaseType {
	InputOrForward,
	Bidirectional,
	Random,
	NoneSuch
};

template <typename IteratorTag>
struct tag_to_base_type {
	static constexpr BaseType value = ctp::one_of_v<IteratorTag, std::input_iterator_tag, std::forward_iterator_tag> ?
		BaseType::InputOrForward :
		std::is_same_v<IteratorTag, std::bidirectional_iterator_tag> ?
		BaseType::Bidirectional :
		ctp::one_of_v<IteratorTag, std::random_access_iterator_tag, std::contiguous_iterator_tag> ?
		BaseType::Random :
		BaseType::NoneSuch;
	static_assert(value != BaseType::NoneSuch);
};
template <typename IteratorTag>
inline constexpr BaseType tag_to_base_type_v = tag_to_base_type<IteratorTag>::value;

template <typename Derived, typename ValueType, typename DifferenceType, typename ReferenceType, BaseType>
class iterator_base;

} // iterator_detail

class iterator_accessor {
	iterator_accessor() = delete;

	template <typename Derived, typename ValueType, typename IteratorConcept, typename DifferenceType, typename Reference>
	friend class iterator_t;

	template <typename Derived, typename ValueType, typename DifferenceType, typename ReferenceType, iterator_detail::BaseType>
	friend class iterator_detail::iterator_base;

	// We use is_detected pattern rather than concepts to bypass private function restrictions, since we expect
	// iterator_accessor to be a friend of the derived type.
	// We need the detector to be a local class so that we don't lose the friendship relation during the checks
	// (clang 16 requires this)
	template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
	struct detector {
		using value_t = std::false_type;
		using type = Default;
	};

	template <class Default, template<class...> class Op, class... Args>
	struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
		using value_t = std::true_type;
		using type = Op<Args...>;
	};

	template <template<class...> class Op, class... Args>
	using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

	template< template<class...> class Op, class... Args >
	static constexpr bool is_detected_v = is_detected<Op, Args...>::value;

	template <template<class...> class Op, class... Args>
	using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

	template <typename I> using DirectIndexible = decltype(std::declval<I>().get_index());
	template <typename I> using HasPeek = decltype(std::declval<I>().peek());
	template <typename I> using HasPreIncr = decltype(std::declval<I>().pre_increment());
	template <typename I> using HasPreDecr = decltype(std::declval<I>().pre_decrement());
	template <typename I> using HasAdvance = decltype(std::declval<I>().advance(std::declval<typename I::difference_type>()));
	template <typename I> using HasEquals = decltype(std::declval<I>().equals(std::declval<I>()));
	template <typename I> using HasDistanceTo = decltype(std::declval<I>().distance_to(std::declval<I>()));

	template <typename I> static constexpr bool IsDirectIndexible =
		iterator_detail::IndexLike<detected_t<DirectIndexible, I>, typename I::difference_type>;
	template <typename I> static constexpr bool IsPeekable = is_detected_v<HasPeek, I>;
	template <typename I> static constexpr bool IsPreIncrable = is_detected_v<HasPreIncr, I>;
	template <typename I> static constexpr bool IsPreDecrable = is_detected_v<HasPreDecr, I>;
	template <typename I> static constexpr bool IsAdvanceable = is_detected_v<HasAdvance, I>;

	template <typename I> using ConstPeekable = decltype(std::declval<const I&>().peek());
	template <typename I> static constexpr bool IsConstPeekable = is_detected_v<ConstPeekable, I>;

	template <typename I> using ConstDirectIndexible = decltype(std::declval<const I&>().get_index());
	template <typename I> static constexpr bool IsConstDirectIndexible = is_detected_v<ConstDirectIndexible, I>;

	// peek ----------------------------------------------------
	template <typename I>
		requires IsPeekable<I>
	static constexpr decltype(auto) peek(const I& i) CTP_NOEXCEPT(noexcept(std::declval<I&>().peek())) {
		if constexpr (IsConstPeekable<I>) {
			return i.peek();
		} else {
			// Iterators are pointer-like and accessing the referenced data should be a const operation.
			return const_cast<I&>(i).peek();
		}
	}

	// dereference ---------------------------------------------
	template <typename I>
		requires IsPeekable<I>
	static constexpr decltype(auto) dereference(const I& i) CTP_NOEXCEPT(noexcept(std::declval<I&>().peek())) {
		auto val = peek(i);
		if constexpr (std::is_pointer_v<std::invoke_result_t<decltype(peek<I>), const I&>>) {
			return *val;
		} else {
			return val;
		}
	}

	// Pre increment -------------------------------------------
	template <typename I>
		requires IsPreIncrable<I>
	static constexpr void pre_increment(I& i) CTP_NOEXCEPT(noexcept(i.pre_increment())) {
		i.pre_increment();
	}
	template <typename I>
		requires (!IsPreIncrable<I>&& IsDirectIndexible<I>)
	static constexpr void pre_increment(I& i) CTP_NOEXCEPT(noexcept(++i.get_index())) {
		++i.get_index();
	}
	template <typename I>
		requires (!IsPreIncrable<I> && !IsDirectIndexible<I>&& IsAdvanceable<I>)
	static constexpr void pre_increment(I& i) CTP_NOEXCEPT(noexcept(i.advance(typename I::difference_type{1}))) {
		i.advance(typename I::difference_type{1});
	}

	// Pre decrement -------------------------------------------
	template <typename I>
		requires IsPreDecrable<I>
	static constexpr void pre_decrement(I& i) CTP_NOEXCEPT(noexcept(i.pre_decrement())) {
		i.pre_decrement();
	}
	template <typename I>
		requires (!IsPreDecrable<I>&& IsDirectIndexible<I>)
	static constexpr void pre_decrement(I& i) CTP_NOEXCEPT(noexcept(--i.get_index())) {
		--i.get_index();
	}
	template <typename I>
		requires (!IsPreDecrable<I> && !IsDirectIndexible<I>&& IsAdvanceable<I>)
	static constexpr void pre_decrement(I& i) CTP_NOEXCEPT(noexcept(i.advance(typename I::difference_type{-1}))) {
		i.advance(typename I::difference_type{-1});
	}

	// Advance -------------------------------------------------
	template <typename I, typename DiffType>
		requires IsAdvanceable<I>
	static constexpr void advance(I& i, DiffType n) CTP_NOEXCEPT(noexcept(i.advance(n))) {
		i.advance(n);
	}

	template <typename I>
	using IndexType = std::remove_reference_t<detected_t<DirectIndexible, I>>;
	template <typename I, typename DiffType>
	struct advance_nothrow_class_test {
		static constexpr bool value = CTP_NOEXCEPT(std::declval<IndexType<I>&>() += std::declval<DiffType>());
	};
	template <typename I, typename DiffType>
	static constexpr bool is_advance_nothrow = std::conditional_t<std::is_integral_v<IndexType<I>> && std::is_integral_v<DiffType>,
		std::true_type,
		advance_nothrow_class_test<I, DiffType>>::value;

	template <typename I, typename DiffType>
		requires (!IsAdvanceable<I>&& IsDirectIndexible<I>)
	static constexpr void advance(I& i, DiffType n) CTP_NOEXCEPT((is_advance_nothrow<I, DiffType>)) {
		using IndexType = std::remove_reference_t<detected_t<DirectIndexible, I>>;
		if constexpr (std::is_integral_v<IndexType> && sizeof(DiffType) > sizeof(IndexType)) {
			// DiffType is bigger, and must be signed. Convert to DiffType first to avoid potential loss of range.
			DiffType temp = i.get_index();
			temp += n;
			i.get_index() = static_cast<IndexType>(temp);
		} else {
			// Either the index type is large enough, or is a special class.
			i.get_index() += n;
		}
	}

	// Helper if Derived only defines non-const get_index.
	template <typename Derived>
	static constexpr decltype(auto) const_get_index(const Derived& d) CTP_NOEXCEPT(noexcept(d.get_index()))
		requires IsConstDirectIndexible<Derived>
	{
		return d.get_index();
	}
	template <typename Derived>
	static constexpr decltype(auto) const_get_index(const Derived& d)
		CTP_NOEXCEPT(noexcept(const_cast<Derived&>(d).get_index()))
		requires (!IsConstDirectIndexible<Derived>)
	{
		using Ret = decltype(std::declval<Derived&>().get_index());
		return static_cast<add_inner_const_t<Ret>>(const_cast<Derived&>(d).get_index());
	}

	// Distance ------------------------------------------------
	template <typename I>
		requires is_detected_v<HasDistanceTo, I>
	static constexpr decltype(auto) distance(const I& lhs, const I& rhs) CTP_NOEXCEPT(noexcept(lhs.distance_to(rhs))) {
		return static_cast<typename I::difference_type>(lhs.distance_to(rhs));
	}
	template <typename I>
		requires (!is_detected_v<HasDistanceTo, I>&& IsDirectIndexible<I>)
	static constexpr decltype(auto) distance(const I& lhs, const I& rhs)
		CTP_NOEXCEPT(noexcept(const_get_index(rhs) - const_get_index(lhs)))
	{
		return static_cast<typename I::difference_type>(const_get_index(rhs) - const_get_index(lhs));
	}

	// Equals --------------------------------------------------
	template <typename I>
		requires is_detected_v<HasEquals, I>
	static constexpr bool equals(const I& lhs, const I& rhs) CTP_NOEXCEPT(noexcept(lhs.equals(rhs))) {
		return lhs.equals(rhs);
	}
	template <typename I>
		requires (!is_detected_v<HasEquals, I>&& is_detected_v<HasDistanceTo, I>)
	static constexpr bool equals(const I& lhs, const I& rhs) noexcept(noexcept(lhs.distance_to(rhs) == 0)) {
		return lhs.distance_to(rhs) == 0;
	}
	template <typename I>
		requires (!is_detected_v<HasEquals, I> && !is_detected_v<HasDistanceTo, I>&& IsDirectIndexible<I>)
	static constexpr bool equals(const I& lhs, const I& rhs)
		CTP_NOEXCEPT(noexcept(const_get_index(lhs) == const_get_index(rhs) == 0))
	{
		return const_get_index(lhs) == const_get_index(rhs);
	}

	// Hint tag helpers, defined here so they are accesible if they are private in Derived.
	template <typename I> using HasPostfixHint = typename I::iterator_postfix_values_expire;
	template <typename I> using HasBracketsHint = typename I::iterator_brackets_values_expire;

	// ---------------------------------------------------------
	// Ugly friend declarations for iterator_t friend functions.
	// Friends here so that iterator_accessor functions can stay private to iterator_t.

	template <typename D, typename V, typename I, typename Diff, typename R,
		typename D2, typename V2, typename I2, typename Diff2, typename R2,
		typename CommonDerived>
		requires iterator_detail::iterators_interopable<D, D2>
	friend constexpr auto operator-(const iterator_t<D, V, I, Diff, R>& lhs, const iterator_t<D2, V2, I2, Diff2, R2>& rhs)
		CTP_NOEXCEPT(noexcept(iterator_accessor::distance<CommonDerived>(static_cast<const D2&>(rhs), static_cast<const D&>(lhs))));

	// Child classes can also define comparisons and equality directly themselves.
	template <typename D, typename V, typename I, typename Diff, typename R,
		typename D2, typename V2, typename I2, typename Diff2, typename R2,
		typename CommonDerived>
		requires iterator_detail::iterators_interopable<D, D2>
	friend constexpr auto operator<=>(const iterator_t<D, V, I, Diff, R>& lhs, const iterator_t<D2, V2, I2, Diff2, R2>& rhs)
		CTP_NOEXCEPT(noexcept(iterator_accessor::distance<CommonDerived>(static_cast<const D&>(lhs), static_cast<const D2&>(rhs))));

	template <typename D, typename V, typename I, typename Diff, typename R,
		typename D2, typename V2, typename I2, typename Diff2, typename R2,
		typename CommonDerived>
		requires iterator_detail::iterators_interopable<D, D2>
	friend constexpr bool operator==(const iterator_t<D, V, I, Diff, R>& lhs, const iterator_t<D2, V2, I2, Diff2, R2>& rhs)
		CTP_NOEXCEPT(noexcept(iterator_accessor::equals<CommonDerived>(static_cast<const D&>(lhs), static_cast<const D2&>(rhs))));
}; // iterator_accessor

namespace iterator_detail {

// Input or forward iterator.
template <typename Derived, typename ValueType, typename DifferenceType, typename ReferenceType>
class iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::InputOrForward>
	: public CrtpHelper<Derived, iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::InputOrForward>> {
public:
	using derived_type = Derived;
	using difference_type = DifferenceType;
	using value_type = ValueType;
	using reference = ReferenceType;

private:
	using peek_type = std::conditional_t<std::is_reference_v<reference>, std::remove_reference_t<reference>*, reference>;
public:
	using pointer = decltype(op_arrow<peek_type>::do_op(std::declval<peek_type>()));

	static_assert(!std::is_integral_v<difference_type> || std::is_signed_v<difference_type>, "Difference type should be signed.");

	[[nodiscard]] constexpr reference operator*() const
		CTP_NOEXCEPT(noexcept(iterator_accessor::dereference(this->derived())))
	{
		return iterator_accessor::dereference(this->derived());
	}
	[[nodiscard]] constexpr pointer operator->() const
		CTP_NOEXCEPT(noexcept(iterator_accessor::peek(this->derived())))
	{
		return op_arrow<peek_type>::do_op(iterator_accessor::peek(this->derived()));
	}

	// Returns pointer to underlying data, as if by operator->
	[[nodiscard]] constexpr pointer get() const
		CTP_NOEXCEPT(noexcept(iterator_accessor::peek(this->derived())))
	{
		return op_arrow<peek_type>::do_op(iterator_accessor::peek(this->derived()));
	}

	constexpr auto& operator++() CTP_NOEXCEPT(noexcept(iterator_accessor::pre_increment(this->derived()))) {
		iterator_accessor::pre_increment(this->derived());
		return this->derived();
	}

	// Normal postfix.
	constexpr auto operator++(int)
		CTP_NOEXCEPT(noexcept(derived_type{this->derived()}, ++this->derived()))
		requires (!is_detected_v<iterator_accessor::HasPostfixHint, derived_type>)
	{
		derived_type temp{this->derived()};
		++this->derived();
		return temp;
	}
	// Proxy postfix for expiring values from input iterators.
	constexpr auto operator++(int)
		CTP_NOEXCEPT(noexcept(input_iterator_expiring_value_postfix_proxy<value_type>{*this->derived()}) && noexcept(++this->derived()))
		requires is_detected_v<iterator_accessor::HasPostfixHint, derived_type>
	{
		input_iterator_expiring_value_postfix_proxy<value_type> temp{*this->derived()};
		++this->derived();
		return temp;
	}
};

// Bidi iterator.
template <typename Derived, typename ValueType, typename DifferenceType, typename ReferenceType>
class iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::Bidirectional>
	// Is also an input/forward iterator.
	: public iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::InputOrForward> {
	using base = iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::InputOrForward>;
public:
	using derived_type = typename base::derived_type;

	constexpr auto& operator--() CTP_NOEXCEPT(noexcept(iterator_accessor::pre_decrement(this->derived()))) {
		iterator_accessor::pre_decrement(this->derived());
		return this->derived();
	}
	constexpr auto operator--(int)
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<derived_type> && noexcept(--this->derived()))
	{
		derived_type temp{this->derived()};
		--this->derived();
		return temp;
	}
};

// Random access iterator.
template <typename Derived, typename ValueType, typename DifferenceType, typename ReferenceType>
class iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::Random>
	// Is also a bidirectional iterator.
	: public iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::Bidirectional> {
	using base = iterator_base<Derived, ValueType, DifferenceType, ReferenceType, BaseType::Bidirectional>;
public:
	using derived_type = typename base::derived_type;
	using difference_type = typename base::difference_type;
	using value_type = typename base::value_type;
	using reference = typename base::reference;

	[[nodiscard]] constexpr decltype(auto) operator[](difference_type offset) const
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<derived_type> && noexcept(*std::declval<derived_type>()))
		requires (!is_detected_v<iterator_accessor::HasBracketsHint, derived_type>)
	{
		Derived temp = this->derived() + offset;
		return *temp;
	}
	[[nodiscard]] constexpr decltype(auto) operator[](difference_type offset) const
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<derived_type> && noexcept(brackets_proxy{std::declval<derived_type>()}))
		requires is_detected_v<iterator_accessor::HasBracketsHint, derived_type>
	{
		derived_type temp = this->derived() + offset;
		return brackets_proxy{temp};
	}

	constexpr auto& operator+=(difference_type offset)
		CTP_NOEXCEPT(noexcept(iterator_accessor::advance(this->derived(), offset)))
	{
		iterator_accessor::advance(this->derived(), offset);
		return this->derived();
	}
	constexpr auto& operator-=(difference_type offset)
		CTP_NOEXCEPT(noexcept(iterator_accessor::advance(this->derived(), offset)))
	{
		iterator_accessor::advance(this->derived(), -offset);
		return this->derived();
	}

	[[nodiscard]] constexpr auto operator+(difference_type offset) const
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<derived_type> && noexcept(std::declval<derived_type>() += offset))
	{
		derived_type temp{this->derived()};
		temp += offset;
		return temp;
	}
	[[nodiscard]] constexpr auto operator-(difference_type offset) const
		CTP_NOEXCEPT(std::is_nothrow_copy_constructible_v<derived_type> && noexcept(std::declval<derived_type>() -= offset))
	{
		derived_type temp{this->derived()};
		temp -= offset;
		return temp;
	}
};

} // iterator_detail

template <
	typename Derived,
	typename ValueType,
	typename IteratorConcept = std::contiguous_iterator_tag,
	typename DifferenceType = std::ptrdiff_t,
	typename ReferenceType = ValueType&>
class iterator_t
	: public iterator_detail::iterator_base<
	Derived,
	ValueType,
	DifferenceType,
	ReferenceType,
	iterator_detail::tag_to_base_type_v<IteratorConcept>> {
	using base = iterator_detail::iterator_base<Derived, ValueType, DifferenceType, ReferenceType, iterator_detail::tag_to_base_type_v<IteratorConcept>>;
public:
	using derived_type = typename base::derived_type;
	using difference_type = typename base::difference_type;
	using value_type = typename base::value_type;
	using reference = typename base::reference;
	using pointer = typename base::pointer;
	using iterator_concept = IteratorConcept;
};

template <typename D, typename V, typename I, typename Diff, typename R>
[[nodiscard]] constexpr auto operator+(typename D::difference_type offset, const iterator_t<D, V, I, Diff, R>& iter)
CTP_NOEXCEPT(noexcept(static_cast<const D&>(iter) + offset))
{
	return static_cast<const D&>(iter) + offset;
}

template <typename D, typename V, typename I, typename Diff, typename R,
	typename D2, typename V2, typename I2, typename Diff2, typename R2,
	typename CommonDerived = iterator_detail::iterators_interop_t<D, D2>>
	requires iterator_detail::iterators_interopable<D, D2>
[[nodiscard]] constexpr auto operator-(const iterator_t<D, V, I, Diff, R>& lhs, const iterator_t<D2, V2, I2, Diff2, R2>& rhs)
CTP_NOEXCEPT(noexcept(iterator_accessor::distance<CommonDerived>(static_cast<const D2&>(rhs), static_cast<const D&>(lhs))))
{
	return iterator_accessor::distance<CommonDerived>(static_cast<const D2&>(rhs), static_cast<const D&>(lhs));
}

// Child classes can also define comparisons and equality directly themselves.
template <typename D, typename V, typename I, typename Diff, typename R,
	typename D2, typename V2, typename I2, typename Diff2, typename R2,
	typename CommonDerived = iterator_detail::iterators_interop_t<D, D2>>
	requires iterator_detail::iterators_interopable<D, D2>
[[nodiscard]] constexpr auto operator<=>(const iterator_t<D, V, I, Diff, R>& lhs, const iterator_t<D2, V2, I2, Diff2, R2>& rhs)
CTP_NOEXCEPT(noexcept(iterator_accessor::distance<CommonDerived>(static_cast<const D&>(lhs), static_cast<const D2&>(rhs))))
{
	return typename CommonDerived::difference_type{} <=>
		iterator_accessor::distance<CommonDerived>(static_cast<const D&>(lhs), static_cast<const D2&>(rhs));
}

template <typename D, typename V, typename I, typename Diff, typename R,
	typename D2, typename V2, typename I2, typename Diff2, typename R2,
	typename CommonDerived = iterator_detail::iterators_interop_t<D, D2>>
	requires iterator_detail::iterators_interopable<D, D2>
[[nodiscard]] constexpr bool operator==(const iterator_t<D, V, I, Diff, R>& lhs, const iterator_t<D2, V2, I2, Diff2, R2>& rhs)
CTP_NOEXCEPT(noexcept(iterator_accessor::equals<CommonDerived>(static_cast<const D&>(lhs), static_cast<const D2&>(rhs))))
{
	return iterator_accessor::equals<CommonDerived>(static_cast<const D&>(lhs), static_cast<const D2&>(rhs));
}

} // ctp

#endif // INCLUDE_CTP_TOOLS_ITERATOR_HPP
