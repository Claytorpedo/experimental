#ifndef INCLUDE_CTP_TOOLS_SMALL_STRING_HPP
#define INCLUDE_CTP_TOOLS_SMALL_STRING_HPP

#include "small_storage.hpp"
#include "trivial_allocator_adapter.hpp"

#include <string_view>

namespace ctp {

namespace small_string_detail {

template <class S, typename CharT, typename Traits>
concept StringViewConvertible = std::is_convertible_v<const S&, std::basic_string_view<CharT, Traits>>;

template <class S, typename CharT, typename Traits>
concept StringViewLikeNoPtr = StringViewConvertible<S, CharT, Traits> && !std::is_convertible_v<const S&, const CharT*>;

// Simple iterator that repeats one character.
// peek returns a value rather than by pointer, so we set reference_type to CharT.
// This way it can be used like an input_iterator, but still use std::distance.
template <typename CharT>
struct char_repeater : iterator_t<char_repeater<CharT>, CharT, std::random_access_iterator_tag, std::ptrdiff_t, CharT> {
	CharT ch;
	std::size_t index = 0;
	constexpr char_repeater(CharT c, std::size_t i) noexcept : ch{c}, index{i} {}

	friend iterator_accessor;
	constexpr std::size_t& get_index() noexcept { return index; }
	constexpr CharT peek() noexcept { return ch; }
};

} // small_string_detail

// Optionally growing, constexpr friendly string with customizable small storage optimization.
// Null terminating version adds an extra character to small storage for the null terminator.
// Copying or constructing a non-null-terminated basic_small_string from a null-terminated basic_small_string
// does not preserve the null terminating character unless explicitly requested.
template <
	typename CharT,
	std::size_t NumChars,
	bool IsNullTerminated,
	class Traits,
	class Alloc,
	class Options>
class basic_small_string : private small_storage::container<CharT, NumChars + IsNullTerminated, Alloc, Options> {
	using Base = small_storage::container<CharT, NumChars + IsNullTerminated, Alloc, Options>;
	static_assert(std::same_as<typename Traits::char_type, CharT>);

	static constexpr bool not_null_terminated = !IsNullTerminated;

	constexpr void add_null_terminate() noexcept(noexcept(Base::push_back(CharT()))) {
		if constexpr (is_null_terminated) {
			Base::push_back(CharT());
		}
	}
	constexpr void write_null_terminate() noexcept(noexcept(*Base::last() = CharT())) {
		if constexpr (is_null_terminated) {
			*Base::last() = CharT();
		}
	}

	constexpr void remove_null_terminate() noexcept(noexcept(Base::erase(Base::last()))) {
		if constexpr (is_null_terminated) {
			Base::erase(Base::last());
		}
	}

	template <class C2, std::size_t S2, bool T2, class Traits2, class A2, class O2>
	friend class basic_small_string;
public:
	using value_type = CharT;
	using traits_type = Traits;
	using allocator_type = Alloc;
	using size_type = typename Base::size_type;
	using difference_type = typename Base::difference_type;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename Base::alloc_traits::pointer;
	using const_pointer = typename Base::alloc_traits::const_pointer;

	using iterator = typename Base::iterator;
	using const_iterator = typename Base::const_iterator;
	using reverse_iterator = typename Base::reverse_iterator;
	using const_reverse_iterator = typename Base::const_reverse_iterator;

	static constexpr size_type npos = static_cast<size_type>(-1);
	static constexpr bool is_null_terminated = IsNullTerminated;

	using view_type = std::basic_string_view<CharT, Traits>;

	constexpr ~basic_small_string() noexcept = default;

	// ---------------------- Constructors ----------------------

	constexpr basic_small_string() noexcept(noexcept(Base())) requires not_null_terminated = default;
	constexpr basic_small_string() noexcept(noexcept(Base(1, CharT()))) requires is_null_terminated : Base(1, CharT()) {}

	// Construct with allocator.
	explicit constexpr basic_small_string(const Alloc& alloc) noexcept(noexcept(Base(alloc)))
		requires not_null_terminated
	: Base(alloc) {}
	explicit constexpr basic_small_string(const Alloc& alloc) noexcept(noexcept(Base(alloc)))
		requires is_null_terminated
	: Base(1, CharT(), alloc) {}

	// Construct with count copies of a given character.
	constexpr basic_small_string(size_type count, CharT ch, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(count, ch, alloc)))
		: Base(count + is_null_terminated, ch, alloc) {
		write_null_terminate();
	}

	// Construct by copying a substring [pos, o.end()).
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(
		const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o,
		size_type pos,
		const Alloc& alloc = Alloc{}) noexcept(noexcept(Base(o.begin() + pos, o.end(), alloc)))
		requires not_null_terminated
	: Base(o.begin() + pos, o.end(), alloc) {}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(
		const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o,
		size_type pos,
		const Alloc& alloc = Alloc{}) noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(o.begin(), o.end())))
		requires (is_null_terminated && o.is_null_terminated)
	: Base(o.begin() + pos, o.end() + 1, alloc) {}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(
		const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o,
		size_type pos,
		const Alloc& alloc = Alloc{}) noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(o.begin(), o.end())))
		requires (is_null_terminated && o.not_null_terminated)
	: Base(alloc) {
		Base::reserve(o.size() - pos + 1);
		Base::assign(o.begin() + pos, o.end());
		add_null_terminate();
	}

	// Construct by copying a substring [pos, pos + count). If pos + count is greater than size, or
	// count is npos, then uses [pos, o.end()).
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(
		const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o,
		size_type pos,
		size_type count,
		const Alloc& alloc = Alloc{}) noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(o.begin() + pos, o.end())))
	: Base(alloc) {
		size_type endPos = pos + count;
		if (const size_type oSize = o.size(); count == npos || endPos > oSize)
			endPos = oSize;

		if constexpr (is_null_terminated) {
			Base::reserve(endPos - pos + 1);
		}

		Base::assign(o.begin() + pos, o.begin() + endPos);

		add_null_terminate();
	}

	// Construct by move assigning a substring [pos, o.end()).
	// o is cleared.
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(
		basic_small_string<CharT, S2, T2, Traits, A2, O2>&& o,
		size_type pos,
		const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(alloc)) && noexcept(Base::operator=(std::move(o))) && noexcept(erase(begin(), begin())))
		: Base(alloc)
	{
		if constexpr (!Base::HasLargeMode) {
			assign(o.begin() + pos, o.end());
			o.clear();
			return;
		} else {
			if (o.is_small_mode()) {
				assign(o.begin() + pos, o.end());
				o.clear();
				return;
			}

			if constexpr (not_null_terminated && o.is_null_terminated) {
				o.erase(o.Base::end() - 1);
			}

			auto beginIt = o.begin();
			if (pos > 0)
				o.Base::erase(beginIt, beginIt + pos);

			ctpAssert(max_size() >= o.size());

			Base::operator=(std::move(o));

			if constexpr (is_null_terminated && o.not_null_terminated) {
				add_null_terminate();
			}
			if constexpr (o.is_null_terminated) {
				o.add_null_terminate();
			}
		}
	}
	// Construct by move-assigning a substring [pos, pos + count). If pos + count is greater than size, or
	// count is npos, then uses [pos, o.end()).
	// o is cleared.
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(
		basic_small_string<CharT, S2, T2, Traits, A2, O2>&& o,
		size_type pos,
		size_type count,
		const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(alloc)) && noexcept(Base::operator=(std::move(o))) && noexcept(erase(begin(), begin())))
		: Base(alloc)
	{
		size_type endPos = pos + count;
		if (const size_type oSize = o.size(); count == npos || endPos > oSize)
			endPos = oSize;

		if constexpr (!Base::HasLargeMode) {
			assign(o.begin() + pos, o.begin() + endPos);
			o.clear();
			return;
		} else {
			if (o.is_small_mode()) {
				assign(o.begin() + pos, o.begin() + endPos);
				o.clear();
				return;
			}

			o.Base::erase(o.begin() + endPos, o.Base::end() - (is_null_terminated && o.is_null_terminated));
			o.Base::erase(o.begin(), o.begin() + pos);

			ctpAssert(max_size() >= o.size());

			Base::operator=(std::move(o));

			if constexpr (is_null_terminated && o.not_null_terminated) {
				add_null_terminate();
			}
			if constexpr (o.is_null_terminated) {
				o.add_null_terminate();
			}
		}
	}

	// Construct from a C string of given length, [s, s + count). May contain null characters.
	constexpr basic_small_string(const CharT* s, size_type count, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(s, s + count, alloc)))
		requires not_null_terminated
	: Base(s, s + count, alloc) {}
	// Construct from a C string of given length, [s, s + count). May contain null characters.
	constexpr basic_small_string(const CharT* s, size_type count, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(s, s + count, alloc)))
		requires is_null_terminated
	: Base(alloc) {
		Base::reserve(count + 1);
		Base::assign(s, s + count);
		add_null_terminate();
	}

	// Construct from a C string.
	constexpr basic_small_string(const CharT* s, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(s, s + traits_type::length(s), alloc)))
		: Base(s, s + traits_type::length(s) + is_null_terminated, alloc) {}

	// Construct by copying range [first, last).
	template <std::input_iterator Iterator>
	constexpr basic_small_string(Iterator first, Iterator last, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(first, last, alloc)))
		requires not_null_terminated
	: Base(first, last, alloc) {}
	// Construct by copying range [first, last).
	template <std::input_iterator Iterator>
	constexpr basic_small_string(Iterator first, Iterator last, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(first, last)))
		requires is_null_terminated
	: Base(alloc) {
		if constexpr (std::forward_iterator<Iterator>) {
			Base::reserve(static_cast<size_type>(std::distance(first, last)) + 1);
		}
		Base::assign(first, last);
		add_null_terminate();
	}

	constexpr basic_small_string(const basic_small_string& o)
		noexcept(noexcept(Base(o)))
	: Base(o) {}

	constexpr basic_small_string(const basic_small_string& o, const Alloc& alloc)
		noexcept(noexcept(Base(o, alloc)))
	: Base(o, alloc) {}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o)
		noexcept(noexcept(Base(o)))
		requires (is_null_terminated == o.is_null_terminated)
	: Base(o) {}
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o)
		noexcept(noexcept(basic_small_string(o.begin(), o.end(), o.get_allocator())))
		requires (is_null_terminated != o.is_null_terminated)
	: basic_small_string(o.begin(), o.end(), o.get_allocator()) {}


	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o, const Alloc& alloc)
		noexcept(noexcept(Base(o, alloc)))
		requires (is_null_terminated == o.is_null_terminated)
	: Base(o, alloc) {}
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o, const Alloc& alloc)
		noexcept(noexcept(basic_small_string(o.begin(), o.end(), alloc)))
		requires (is_null_terminated != o.is_null_terminated)
	: basic_small_string(o.begin(), o.end(), alloc) {}

	constexpr basic_small_string(basic_small_string&& o)
		noexcept(noexcept(Base(std::move(o))))
		: Base(std::move(o)) {
		if constexpr (o.is_null_terminated)
			o.add_null_terminate();
	}
	constexpr basic_small_string(basic_small_string&& o, const Alloc& alloc)
		noexcept(noexcept(Base(std::move(o), alloc)))
		: Base(std::move(o), alloc) {
		if constexpr (o.is_null_terminated)
			o.add_null_terminate();
	}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(basic_small_string<CharT, S2, T2, Traits, A2, O2>&& o)
		noexcept(noexcept(this->Base::operator=(std::move(o)))) {
		if constexpr (o.is_null_terminated && not_null_terminated) {
			o.remove_null_terminate();
		}

		this->Base::operator=(std::move(o));

		if constexpr (o.is_null_terminated) {
			o.add_null_terminate();
		} else if constexpr (is_null_terminated) {
			add_null_terminate();
		}
	}
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string(basic_small_string<CharT, S2, T2, Traits, A2, O2>&& o, const Alloc& alloc)
		noexcept(noexcept(this->Base::operator=(std::move(o), alloc))) {
		if constexpr (o.is_null_terminated && not_null_terminated) {
			o.remove_null_terminate();
		}

		this->Base::operator=(std::move(o), alloc);

		if constexpr (o.is_null_terminated) {
			o.add_null_terminate();
		} else if constexpr (is_null_terminated) {
			add_null_terminate();
		}
	}

	constexpr basic_small_string(std::initializer_list<CharT> ilist, const Alloc& alloc = Alloc())
		noexcept(noexcept(Base(ilist, alloc)))
		requires not_null_terminated
	: Base(ilist, alloc) {}
	constexpr basic_small_string(std::initializer_list<CharT> ilist, const Alloc& alloc = Alloc())
		noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(ilist)))
		requires is_null_terminated
	: Base(ilist, alloc) {
		Base::reserve(ilist.size() + 1);
		Base::assign(ilist);
		add_null_terminate();
	}

	template <small_string_detail::StringViewLikeNoPtr<CharT, Traits> ViewConvertible>
	constexpr basic_small_string(const ViewConvertible& view, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(view_type{}.begin(), view_type{}.end())))
		: Base(alloc) {
		view_type v{view};
		if constexpr (is_null_terminated) {
			Base::reserve(v.size() + 1);
		}
		Base::assign(v.begin(), v.end());
		add_null_terminate();
	}
	// Construct by assigning via converting to a string_view and taking a substring of [pos, pos + count).
	// pos + count is safety checked to not go past the end of the string.
	template <small_string_detail::StringViewConvertible<CharT, Traits> ViewConvertible>
	constexpr basic_small_string(const ViewConvertible& view, size_type pos, size_type count, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(alloc)) && noexcept(Base::assign(view_type{}.begin(), view_type{}.end())))
		: Base(alloc) {
		view_type v{view};
		v = v.substr(pos, count);
		if constexpr (is_null_terminated) {
			Base::reserve(v.size() + 1);
		}
		Base::assign(v.begin(), v.end());
		add_null_terminate();
	}

	template <class Range>
	constexpr basic_small_string(std::from_range_t, Range&& range, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(std::from_range_t{}, std::forward<Range>(range), alloc)))
		requires not_null_terminated
	: Base(std::from_range_t{}, std::forward<Range>(range), alloc) {}
	template <class Range>
	constexpr basic_small_string(std::from_range_t, Range&& range, const Alloc& alloc = Alloc{})
		noexcept(noexcept(Base(alloc)) && noexcept(Base::assign_range(std::forward<Range>(range))))
		requires is_null_terminated
	: Base(alloc) {
		if constexpr (std::ranges::sized_range<Range> || std::ranges::forward_range<Range>) {
			Base::reserve(std::ranges::distance(range) + 1);
		}
		Base::assign_range(std::forward<Range>(range));
		add_null_terminate();
	}

	// ----------------------- Assignment -----------------------

	constexpr basic_small_string& operator=(const basic_small_string& o) noexcept(noexcept(Base::operator=(o))) {
		Base::operator=(o);
		return *this;
	}
	constexpr basic_small_string& operator=(basic_small_string&& o) noexcept(noexcept(Base::operator=(std::move(o)))) {
		Base::operator=(std::move(o));
		o.add_null_terminate();
		return *this;
	}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string& operator=(const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o)
		noexcept(noexcept(Base::operator=(o)))
		requires (is_null_terminated == o.is_null_terminated)
	{
		Base::operator=(o);
		return *this;
	}
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string& operator=(const basic_small_string<CharT, S2, T2, Traits, A2, O2>& o)
		noexcept(noexcept(Base::assign(o.begin(), o.end())))
		requires (is_null_terminated != o.is_null_terminated)
	{
		if constexpr (Base::alloc_traits::propagate_on_container_copy_assignment::value) {
			if constexpr (!Base::alloc_traits::is_always_equal::value) {
				if (get_allocator() != o.get_allocator()) {
					// Different allocators, destroy any old elements.
					Base::clear();
				}
			}
			set_allocator(o.get_allocator());
		}

		Base::assign(o.begin(), o.end());

		add_null_terminate();

		return *this;
	}
	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string& operator=(basic_small_string<CharT, S2, T2, Traits, A2, O2>&& o)
		noexcept(noexcept(Base::operator=(std::move(o))))
	{
		if constexpr (o.is_null_terminated && not_null_terminated) {
			o.remove_null_terminate();
		}

		Base::operator=(std::move(o));

		if constexpr (o.not_null_terminated) {
			add_null_terminate();
		} else {
			o.add_null_terminate();
		}

		return *this;
	}

	constexpr basic_small_string& operator=(const CharT* s) noexcept(noexcept(Base::assign(s, s + traits_type::length(s)))) {
		Base::assign(s, s + traits_type::length(s) + is_null_terminated);
		return *this;
	}
	constexpr basic_small_string& operator=(std::nullptr_t) noexcept = delete;

	constexpr basic_small_string& operator=(std::initializer_list<CharT> ilist) noexcept(noexcept(Base::assign(ilist))) {
		if constexpr (is_null_terminated) {
			Base::reserve(ilist.size() + 1);
		}
		Base::assign(ilist);
		add_null_terminate();
		return *this;
	}

	template <small_string_detail::StringViewLikeNoPtr<CharT, Traits> ViewConvertible>
	constexpr basic_small_string& operator=(const ViewConvertible& view)
		noexcept(noexcept(Base::assign(view_type{}.begin(), view_type{}.end())))
	{
		view_type v{view};
		if constexpr (is_null_terminated) {
			Base::reserve(view.size() + 1);
		}
		Base::assign(v.begin(), v.end());
		add_null_terminate();
		return *this;
	}

	// ----- assign -----

	constexpr basic_small_string& assign(size_type count, CharT ch) noexcept(noexcept(Base::assign(count, ch))) {
		Base::assign(count + is_null_terminated, ch);
		write_null_terminate();
		return *this;
	}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr basic_small_string& assign(basic_small_string<CharT, S2, T2, Traits, A2, O2>&& o)
		noexcept(noexcept(*this = std::move(o))) {
		return *this = std::move(o);
	}

	constexpr basic_small_string& assign(const CharT* str) noexcept(noexcept(*this = str)) {
		return *this = str;
	}

	constexpr basic_small_string& assign(const CharT* str, size_type count) noexcept(noexcept(Base::assign(str, str + count))) {
		if constexpr (is_null_terminated) {
			Base::reserve(count + 1);
		}
		Base::assign(str, str + count);
		add_null_terminate();
		return *this;
	}

	template <std::input_iterator Iterator>
	constexpr basic_small_string& assign(Iterator first, Iterator last) noexcept(noexcept(Base::assign(first, last))) {
		if constexpr (is_null_terminated && std::forward_iterator<Iterator>) {
			Base::reserve(static_cast<size_type>(std::distance(first, last)) + 1);
		}
		Base::assign(first, last);
		add_null_terminate();
		return *this;
	}

	constexpr basic_small_string& assign(std::initializer_list<CharT> ilist) noexcept(noexcept(*this = ilist)) {
		return *this = ilist;
	}

	template <small_string_detail::StringViewLikeNoPtr<CharT, Traits> ViewConvertible>
	constexpr basic_small_string& assign(const ViewConvertible& view) noexcept(noexcept(*this = view)) {
		return *this = view;
	}

	template <typename Range>
	constexpr basic_small_string& assign_range(Range&& range)
		noexcept(noexcept(Base::assign_range(std::forward<Range>(range))))
	{
		if constexpr (is_null_terminated && (std::ranges::sized_range<Range> || std::ranges::forward_range<Range>)) {
			Base::reserve(std::ranges::distance(range) + 1);
		}
		Base::assign_range(std::forward<Range>(range));
		add_null_terminate();
		return *this;
	}

	// -------------------- Member Functions --------------------

	using Base::get_allocator;

	using Base::at;
	using Base::operator[];
	using Base::data;
	constexpr const CharT* c_str() const noexcept requires is_null_terminated { return Base::data(); }
	constexpr operator view_type() const noexcept { return view_type{Base::data(), size()}; }
	constexpr view_type view() const noexcept { return view_type{Base::data(), size()}; }

	using Base::begin;
	using Base::cbegin;
	using Base::rbegin;
	constexpr iterator rbegin() noexcept { return is_null_terminated ? Base::rbegin() + 1 : Base::rbegin(); }
	constexpr const_reverse_iterator rbegin() const noexcept { return is_null_terminated ? Base::rbegin() + 1 : Base::rbegin(); }
	constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	constexpr iterator end() noexcept { return is_null_terminated ? Base::end() - 1 : Base::end(); }
	constexpr const_iterator end() const noexcept { return is_null_terminated ? Base::end() - 1 : Base::end(); }
	constexpr const_iterator cend() const noexcept { return end(); }
	using Base::rend;
	using Base::crend;
	// Get iterator to the last element.
	constexpr iterator last() noexcept { return is_null_terminated ? Base::last() - 1 : Base::last(); }
	// Get iterator to the last element.
	constexpr const_iterator last() const noexcept { return is_null_terminated ? Base::last() - 1 : Base::last(); }
	using Base::front;
	constexpr reference back() noexcept { return *last(); }
	constexpr const_reference back() const noexcept { return *last(); }

	[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
	constexpr bool is_empty() const noexcept { return size() == 0; }
	constexpr size_type size() const noexcept { return is_null_terminated ? Base::size() - 1 : Base::size(); }
	constexpr size_type length() const noexcept { return size(); }
	constexpr void reserve(size_type new_capacity) noexcept(noexcept(Base::reserve(new_capacity))) {
		Base::reserve(new_capacity + is_null_terminated);
	}
	constexpr std::size_t capacity() const noexcept { return Base::capacity() - is_null_terminated; }
	constexpr std::size_t max_size() const noexcept { return Base::max_size() - is_null_terminated; }
	// Alias for max_size.
	constexpr std::size_t max_capacity() const noexcept { return max_size(); }
	using Base::shrink_to_fit;

	// Sets size to zero. Capacity is unchanged.
	constexpr void clear() noexcept { Base::clear(); add_null_terminate(); }
	using Base::insert;
	using Base::insert_range;

	using Base::erase;
	constexpr basic_small_string& erase(size_type index = 0, size_type count = npos) {
		const auto endpos = (std::min)(count, size() - index);
		erase(begin() + index, begin() + endpos);
		return *this;
	}

	constexpr reference push_back(CharT ch) noexcept(noexcept(Base::push_back(ch)))
		requires not_null_terminated { return Base::push_back(ch); }
	constexpr reference push_back(CharT ch) noexcept(noexcept(Base::insert(end(), ch)))
		requires is_null_terminated { return *Base::insert(end(), ch); }
	constexpr void pop_back() noexcept { erase(end()); }

	// ----- append -----

	constexpr basic_small_string& append(size_type count, CharT ch) noexcept(noexcept(Base::insert(end(), count, ch))) {
		Base::insert(end(), count, ch);
		return *this;
	}
	constexpr basic_small_string& append(CharT ch) noexcept(noexcept(Base::insert(end(), ch))) {
		Base::insert(end(), ch);
		return *this;
	}
	constexpr basic_small_string& append(view_type view) noexcept(noexcept(Base::insert(end(), view.begin(), view.end()))) {
		Base::insert(end(), view.begin(), view.end());
		return *this;
	}
	constexpr basic_small_string& append(view_type view, size_type pos, size_type count = npos)
		noexcept(noexcept(Base::insert(end(), view.begin(), view.end())))
	{
		view = view.substr(pos, count);
		Base::insert(end(), view.begin(), view.end());
		return *this;
	}
	template <std::input_iterator Iterator>
	constexpr basic_small_string& append(Iterator first, Iterator last)
		noexcept(noexcept(Base::insert(end(), first, last)))
	{
		Base::insert(end(), first, last);
		return *this;
	}
	constexpr basic_small_string& append(std::initializer_list<CharT> ilist)
		noexcept(noexcept(Base::insert(end(), ilist.begin(), ilist.end())))
	{
		Base::insert(end(), ilist.begin(), ilist.end());
		return *this;
	}

	template <typename Range>
	constexpr basic_small_string& append_range(Range&& range)
		noexcept(noexcept(Base::insert_range(end(), std::forward<Range>(range))))
	{
		Base::insert_range(end(), std::forward<Range>(range));
		return *this;
	}

	// ----- operator+= -----

	constexpr basic_small_string& operator+=(CharT ch) noexcept(noexcept(append(ch))) {
		append(ch);
		return *this;
	}
	constexpr basic_small_string& operator+=(view_type view) noexcept(noexcept(append(view))) {
		append(view);
		return *this;
	}
	constexpr basic_small_string& operator+=(std::initializer_list<CharT> ilist) noexcept(noexcept(append(ilist))) {
		append(ilist);
		return *this;
	}

	// ----- replace -----

private:
	template <typename It>
	constexpr void do_replace(const_iterator myFirst, const_iterator myLast, It first, It last) {
		auto thisFirst = this->make_nonconst_iterator(myFirst);
		auto thisLast = this->make_nonconst_iterator(myLast);

		// Write over overlapping characters.
		for (; thisFirst != thisLast && first != last; ++thisFirst, ++first)
			*thisFirst = *first;

		// If there's more characters in the new string, write them.
		if (first != last)
			insert(thisFirst, first, last);
		else if (thisFirst != thisLast) // If there's more to replace, erase them.
			erase(thisFirst, thisLast);
	}
	template <typename It>
	constexpr void do_replace(size_type pos, size_type count, It first, It last) {
		const size_type end = (count == npos || pos + count > size()) ? size() : pos + count;
		do_replace(begin() + pos, begin() + end, first, last);
	}
public:
	constexpr basic_small_string& replace(size_type pos, size_type count, view_type view) {
		do_replace(pos, count, view.begin(), view.end());
		return *this;
	}
	constexpr basic_small_string& replace(const_iterator first, const_iterator last, view_type view) {
		do_replace(first, last, view.begin(), view.end());
		return *this;
	}
	constexpr basic_small_string& replace(
		size_type pos,
		size_type count,
		view_type view,
		size_type pos2,
		size_type count2 = npos) {
		const size_type end2 = (count2 == npos || pos2 + count2 > view.size()) ? view.size() : pos2 + count2;
		do_replace(pos, count, view.begin() + pos2, view.begin() + end2);
		return *this;
	}
	constexpr basic_small_string& replace(const_iterator first, const_iterator last, const CharT* str, size_type count2) {
		do_replace(first, last, str, str + count2);
		return *this;
	}
	constexpr basic_small_string& replace(size_type pos, size_type count, size_type count2, CharT ch) {
		using repeater = small_string_detail::char_repeater<CharT>;
		do_replace(pos, count, repeater(ch, 0), repeater(ch, count2));
		return *this;
	}
	constexpr basic_small_string& replace(const_iterator first, const_iterator last, size_type count2, CharT ch) {
		using repeater = small_string_detail::char_repeater<CharT>;
		do_replace(first, last, repeater(ch, 0), repeater(ch, count2));
		return *this;
	}
	template <std::input_iterator Iter>
	constexpr basic_small_string& replace(const_iterator first, const_iterator last, Iter first2, Iter last2) {
		do_replace(first, last, first2, last2);
		return *this;
	}
	constexpr basic_small_string& replace(const_iterator first, const_iterator last, std::initializer_list<CharT> ilist) {
		do_replace(first, last, ilist.begin(), ilist.end());
		return *this;
	}

	template <typename Range>
	constexpr basic_small_string& replace_with_range(const_iterator first, const_iterator last, Range&& range) {
		do_replace(first, last, range.begin(), range.end());
		return *this;
	}

	// Resizes and initializes characters to null.
	constexpr void resize(size_type new_size) noexcept(noexcept(Base::resize(new_size, CharT()))) {
		Base::resize(new_size + is_null_terminated, CharT());
	}
	constexpr void resize(size_type new_size, CharT ch) noexcept(noexcept(Base::resize(new_size, ch))) {
		remove_null_terminate();
		Base::resize(new_size + is_null_terminated, ch);
		write_null_terminate();
	}

	template <std::size_t S2, bool T2, class A2, class O2>
	constexpr void swap(basic_small_string<CharT, S2, T2, Traits, A2, O2>& o) noexcept(noexcept(Base::swap(o))) {
		if constexpr (is_null_terminated != o.is_null_terminated) {
			remove_null_terminate();
			o.remove_null_terminate();
		}

		Base::swap(o);

		if constexpr (is_null_terminated != o.is_null_terminated) {
			add_null_terminate();
			o.add_null_terminate();
		}
	}

	// ----- find -----
	
	constexpr size_type find(CharT ch, size_type pos = 0) const noexcept { return view().find(ch, pos); }
	constexpr size_type find(view_type view, size_type pos = 0) const noexcept { return view().find(view, pos); }

	constexpr size_type rfind(CharT ch, size_type pos = npos) const noexcept { return view().rfind(ch, pos); }
	constexpr size_type rfind(view_type view, size_type pos = npos) const noexcept { return view().rfind(view, pos); }

	constexpr size_type find_first_of(CharT ch, size_type pos = 0) const noexcept { return view().find_first_of(ch, pos); }
	constexpr size_type find_first_of(view_type view, size_type pos = 0) const noexcept { return view().find_first_of(view, pos); }

	constexpr size_type find_first_not_of(CharT ch, size_type pos = 0) const noexcept { return view().find_first_not_of(ch, pos); }
	constexpr size_type find_first_not_of(view_type view, size_type pos = 0) const noexcept {
		return view().find_first_not_of(view, pos);
	}

	constexpr size_type find_last_of(CharT ch, size_type pos = npos) const noexcept { return view().find_last_of(ch, pos); }
	constexpr size_type find_last_of(view_type view, size_type pos = npos) const noexcept { return view().find_last_of(view, pos); }

	constexpr size_type find_last_not_of(CharT ch, size_type pos = npos) const noexcept { return view().find_last_not_of(ch, pos); }
	constexpr size_type find_last_not_of(view_type view, size_type pos = npos) const noexcept {
		return view().find_last_not_of(view, pos);
	}

	// ----- compare -----

	constexpr int compare(view_type view) const noexcept { return view().compare(view); }
	constexpr int compare(size_type pos1, size_type count1, view_type view) const noexcept {
		return view_type{Base::data() + pos1, count1}.compare(view);
	}
	constexpr int compare(size_type pos1, size_type count1, view_type view, size_type pos2, size_type count2 = npos)
		const noexcept
	{
		return view_type{Base::data() + pos1, count1}.compare(view.substr(pos2, count2));
	}
	constexpr int compare(const CharT* str) const noexcept { return view().compare(str); }
	constexpr int compare(size_type pos1, size_type count1, const CharT* str) const noexcept {
		return view_type{Base::data() + pos1, count1}.compare(str);
	}
	// The string made from [str + pos2, str + pos2 + count2) may contain null characters.
	constexpr int compare(size_type pos1, size_type count1, const CharT* str, size_type pos2, size_type count2)
		const noexcept
	{
		return view_type{Base::data() + pos1, count1}.compare(view_type{str + pos2, count2});
	}

	// ----- starts_with / ends_with / contains -----
	
	constexpr bool starts_with(view_type view) const noexcept { return view().starts_with(view); }
	constexpr bool starts_with(CharT ch) const noexcept { return view().starts_with(ch); }
	constexpr bool starts_with(const CharT* str) const noexcept { return view().starts_with(str); }

	constexpr bool ends_with(view_type view) const noexcept { return view().ends_with(view); }
	constexpr bool ends_with(CharT ch) const noexcept { return view().ends_with(ch); }
	constexpr bool ends_with(const CharT* str) const noexcept { return view().ends_with(str); }

	constexpr bool contains(view_type view) const noexcept { return view().contains(view); }
	constexpr bool contains(CharT ch) const noexcept { return view().contains(ch); }
	constexpr bool contains(const CharT* str) const noexcept { return view().contains(str); }

	// ----- substr  -----

	constexpr basic_small_string substr(size_type pos = 0, size_type count = npos) const& {
		return basic_small_string(*this, pos, count);
	}
	constexpr basic_small_string substr(size_type pos = 0, size_type count = npos) && {
		return basic_small_string(std::move(*this), pos, count);
	}

	// ----- equality -----

	friend constexpr bool operator==(const basic_small_string& lhs, const view_type rhs) noexcept {
		return lhs.view() == rhs;
	}
	friend constexpr bool operator==(const view_type lhs, const basic_small_string& rhs) noexcept {
		return lhs == rhs.view();
	}

	friend constexpr compare_three_way_type_t<CharT> operator<=>(const basic_small_string& lhs, const view_type rhs)
		noexcept {
		return lhs.view() <=> rhs;
	}
	friend constexpr compare_three_way_type_t<CharT> operator<=>(const view_type lhs, const basic_small_string& rhs)
		noexcept {
		return lhs <=> rhs.view();
	}

	friend constexpr bool operator==(const basic_small_string& lhs, const CharT* rhs) noexcept {
		return lhs.view() == rhs;
	}
	friend constexpr compare_three_way_type_t<CharT> operator<=>(const basic_small_string& lhs, const CharT* rhs) noexcept {
		return lhs.view() <=> rhs;
	}
};

template <typename CharT, typename Traits,
	std::size_t S1, bool T1, class A1, class O1,
	std::size_t S2, bool T2, class A2, class O2>
constexpr bool operator==(
	const basic_small_string<CharT, S1, T1, Traits, A1, O1>& lhs,
	const basic_small_string<CharT, S2, T2, Traits, A2, O2>& rhs) noexcept
{
	return lhs.view() == rhs.view();
}

template <typename CharT, typename Traits,
	std::size_t S1, bool T1, class A1, class O1,
	std::size_t S2, bool T2, class A2, class O2>
constexpr compare_three_way_type_t<CharT> operator<=>(
	const basic_small_string<CharT, S1, T1, Traits, A1, O1>& lhs,
	const basic_small_string<CharT, S2, T2, Traits, A2, O2>& rhs) noexcept
{
	return lhs.view() <=> rhs.view();
}

namespace small_string_detail {
template <typename CharT, std::size_t SmallModeChars, bool IsNullTerminated, class Alloc, class Options>
struct make_basic {
	using type = basic_small_string<CharT, SmallModeChars, IsNullTerminated, std::char_traits<CharT>, Alloc, Options>;
};
template <typename CharT, std::size_t SmallModeChars, bool IsNullTerminated, class Alloc, class Options>
using make_basic_t = typename make_basic<CharT, SmallModeChars, IsNullTerminated, Alloc, Options>::type;
} // small_string_detail


// ---------------------------------------- fixed_string ----------------------------------------


struct fixed_string_options : small_storage::default_options {
	static constexpr bool has_large_mode = false;
	static constexpr bool constexpr_friendly = true;
};

// A statically sized string with local storage for NumChars. Not null terminated.
// Users are expected to ensure they do not attempt to store more than NumChars characters.
template <std::size_t NumChars, class Alloc = trivial_init_allocator<char>, class Options = fixed_string_options>
using fixed_string = small_string_detail::make_basic_t<char, NumChars, false, Alloc, Options>;

// A statically sized string with local storage for NumChars. Null terminated.
// Users are expected to ensure they do not attempt to store more than NumChars characters.
template <std::size_t NumChars, class Alloc = trivial_init_allocator<char>, class Options = fixed_string_options>
using fixed_zstring = small_string_detail::make_basic_t<char, NumChars, true, Alloc, Options>;


// ---------------------------------------- small_string ----------------------------------------


struct small_string_options : small_storage::default_options {
	static constexpr bool has_large_mode = true;
	static constexpr bool constexpr_friendly = true;
};

// A string with enough local storage for at least NumItemsInSmallMode. Not null terminated.
// Will allocate using Alloc and grow if it runs out of space in local storage.
template <std::size_t CharsInSmallMode, class Alloc = trivial_init_allocator<char>, class Options = small_string_options>
using small_string = small_string_detail::make_basic_t<char, CharsInSmallMode, false, Alloc, Options>;

// A string with enough local storage for at least NumItemsInSmallMode. Null terminated.
// Will allocate using Alloc and grow if it runs out of space in local storage.
template <std::size_t CharsInSmallMode, class Alloc = trivial_init_allocator<char>, class Options = small_string_options>
using small_zstring = small_string_detail::make_basic_t<char, CharsInSmallMode, true, Alloc, Options>;

} // ctp

template <class CharT, std::size_t N, bool T, class Traits, class Alloc, class O>
std::basic_ostream<CharT, Traits>& operator<<(
	std::basic_ostream<CharT, Traits>& os,
	const ctp::basic_small_string<CharT, N, T, Traits, Alloc, O>& str) {
	return os << str.view();
}

template <class CharT, std::size_t N, bool T, class Traits, class Alloc, class O>
std::basic_istream<CharT, Traits>& operator>>(
	std::basic_istream<CharT, Traits>& stream,
	ctp::basic_small_string<CharT, N, T, Traits, Alloc, O>& str)
{
	using stream_type = std::basic_istream<CharT, Traits>;
	using string_type = ctp::basic_small_string<CharT, N, T, Traits, Alloc, O>;
	using size_type = typename string_type::size_type;

	typename stream_type::iostate state = stream_type::goodbit;

	const typename stream_type::sentry goodStreamSentry{stream};
	if (goodStreamSentry) {
		str.erase();

		const auto getAvailableSize = [&] {
			if (0 < stream.width() && static_cast<size_type>(stream.width()) < str.max_size())
				return static_cast<size_type>(stream.width());
			else
				return str.max_size();
		};

		using ctype = std::ctype<CharT>;
		const auto& facet = std::use_facet<ctype>(stream.getloc());

		auto intType = stream.rdbuf()->sgetc();
		for (size_type availSize = getAvailableSize(); 0 < availSize; --availSize) {
			if (Traits::eq_int_type(intType, Traits::eof())) {
				state |= stream_type::eofbit;
				break;
			}
			const auto charType = Traits::to_char_type(intType);
			if (facet.is(ctype::space, charType)) {
				break;
			}
			str.push_back(charType);
			intType = stream.rdbuf()->snextc();
		}
	}

	stream.width(0);
	if (str.empty())
		state |= stream_type::failbit;

	stream.setstate(state);
	return stream;
}

namespace std {

template <class CharT, std::size_t N, bool T, class Traits, class Alloc, class O>
std::basic_istream<CharT, Traits>& getline(
	std::basic_istream<CharT, Traits>& input,
	ctp::basic_small_string<CharT, N, T, Traits, Alloc, O>& str,
	CharT delim)
{
	using stream_type = std::basic_istream<CharT, Traits>;
	using string_type = ctp::basic_small_string<CharT, N, T, Traits, Alloc, O>;
	using size_type = typename string_type::size_type;

	typename stream_type::iostate state = stream_type::goodbit;

	const typename stream_type::sentry goodStreamSentry{input, true};
	if (goodStreamSentry) {
		str.erase();

		const auto intDelim = Traits::to_int_type(delim);

		auto intType = input.rdbuf()->sgetc();
		for (size_type availSize = str.max_size(); /*checks availSize below*/; --availSize) {
			auto charType = Traits::to_char_type(intType);
			if (Traits::eq_int_type(intType, Traits::eof())) {
				state |= stream_type::eofbit;
				break;
			}
			if (intType == intDelim) {
				input.rdbuf()->sbumpc();
				break;
			}
			if (availSize <= 0) {
				state |= stream_type::failbit;
				break;
			}

			str.push_back(charType);
			intType = input.rdbuf()->snextc();
		}
	}

	if (str.empty())
		state |= stream_type::failbit;

	input.setstate(state);
	return input;
}

template <class CharT, std::size_t N, bool T, class Traits, class Alloc, class O>
std::basic_istream<CharT, Traits>& getline(
	std::basic_istream<CharT, Traits>& input,
	ctp::basic_small_string<CharT, N, T, Traits, Alloc, O>& str)
{
	return getline(input, str, input.widen('\n'));
}

template <
	typename CharT,
	std::size_t NumChars,
	bool IsNullTerminated,
	class Traits,
	class Alloc,
	class Options>
struct hash<ctp::basic_small_string<CharT, NumChars, IsNullTerminated, Traits, Alloc, Options>> {
	using type = ctp::basic_small_string<CharT, NumChars, IsNullTerminated, Traits, Alloc, Options>;
	using sv_type = typename type::view_type;

	std::size_t operator()(const type& s) const noexcept(noexcept(std::hash<sv_type>{}(s.view()))) {
		return std::hash<sv_type>{}(s.view());
	}
};

} // std

#endif // INCLUDE_CTP_TOOLS_SMALL_STRING_HPP
