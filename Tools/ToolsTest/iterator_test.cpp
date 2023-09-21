#include <catch2/catch_template_test_macros.hpp>

#include <Tools/debug.hpp>
#include <Tools/iterator.hpp>

// Tests are defined with constexpr-friendly asserts so that they can be run both
// at compile time and run time.

namespace {

template <typename I> concept IsReadOnly = requires { typename I::read_only; };

template <typename Iterator, typename ConstIterator>
constexpr int iterator_conversion_tests() {
	char chars[] = "0123456789";
	Iterator non_const_it{1, chars};
	ConstIterator const_it{non_const_it}; // Conversion constructor

	static_assert(!std::constructible_from<Iterator, ConstIterator>, "Conversion should only go one way.");

	return 0;
}

template <typename Iterator, typename ConstIterator>
constexpr int input_iterator_tests() {
	iterator_conversion_tests<Iterator, ConstIterator>();

	char chars[] = "0123456789";
	{
		Iterator a{1, chars};

		ctpAssert(*a == '1');
		ctpAssert(*++a == '2');
		ctpAssert(*a == '2');
		ctpAssert(*a++ == '2');
		ctpAssert(*a == '3');
	}

	{
		Iterator a{0, chars};
		Iterator b{1, chars};

		ctpAssert(a == a);
		ctpAssert(a != b);
		ctpAssert(b != a);
		++a;
		ctpAssert(a == b);
		ctpAssert(b == a);
	}

	{
		Iterator a{0, chars};
		ConstIterator b{1, chars};

		ctpAssert(a != b);
		ctpAssert(b != a);
		++a;
		ctpAssert(a == b);
		ctpAssert(b == a);
		ctpAssert(b == a);
	}

	return 0;
}

// We don't specifically have output iterators.
// forward_iterator is basically a multipass enabled input iterator
// and so doesn't really need extra testing on the iterator_t level.
template <typename Iterator, typename ConstIterator>
constexpr int output_iterator_tests() {
	input_iterator_tests<Iterator, ConstIterator>();

	char chars[] = "0123456789";
	Iterator edit{0, chars};

	if constexpr (!IsReadOnly<Iterator>) {
		ctpAssert((*edit = 'a') == 'a');
		ctpAssert(*edit == 'a');
		ctpAssert((*++edit = 'b') == 'b');
		ctpAssert(*edit == 'b');
		ctpAssert((*edit++ = 'c') == 'c'); // overwrite b with c
		ctpAssert(*edit == '2');
	}

	return 0;
}

template <typename Iterator, typename ConstIterator>
constexpr int bidirectional_iterator_tests() {
	output_iterator_tests<Iterator, ConstIterator>();

	char chars[] = "0123456789";
	Iterator a{2, chars};

	ctpAssert(*--a == '1');
	ctpAssert(*a == '1');
	ctpAssert(*a-- == '1');
	ctpAssert(*a == '0');

	return 0;
}

template <typename Iterator, typename ConstIterator>
constexpr int random_access_iterator_tests() {
	bidirectional_iterator_tests<Iterator, ConstIterator>();

	char chars[] = "0123456789";
	{
		Iterator a{1, chars};

		ctpAssert(*a == '1');
		ctpAssert(*(a += 5) == '6');
		ctpAssert(*a == '6');
		ctpAssert(*(a -= 2) == '4');
		ctpAssert(*a == '4');
		ctpAssert(*(a + 3) == '7');
		ctpAssert(*a == '4');
		a = a + 3;
		ctpAssert(*a == '7');
		ctpAssert(*(a - 5) == '2');
		ctpAssert(*a == '7');
		a = a - 5;
		ctpAssert(*a == '2');
		ctpAssert(*(2 + a) == '4');
		ctpAssert(*a == '2');
		a = 2 + a;
		ctpAssert(*a == '4');
		ctpAssert(a[0] == '4');
		ctpAssert(a[5] == '9');
		ctpAssert(a[-2] == '2');

		const Iterator b{4, chars};

		ctpAssert(a == b);
		ctpAssert(--a != b);
		ctpAssert(a <= b);
		ctpAssert(a < b);
		ctpAssert(b > a);
		ctpAssert(b >= a);
		ctpAssert(b <=> a > 0);

		ctpAssert(a - b == -1);
		ctpAssert(b - a == 1);

		const ConstIterator c{b};

		++a;
		ctpAssert(a == c);
		ctpAssert(--a != c);
		ctpAssert(a <= c);
		ctpAssert(a < c);
		ctpAssert(c > a);
		ctpAssert(c >= a);
		ctpAssert(c <=> a > 0);

		ctpAssert(a - c == -1);
		ctpAssert(c - a == 1);
	}

	if constexpr (!IsReadOnly<Iterator>) {
		Iterator edit{0, chars};

		ctpAssert((*(edit + 2) = 'a') == 'a');
		ctpAssert(*edit == '0');
		ctpAssert(edit[2] == 'a');
		ctpAssert((*++edit = 'b') == 'b');
		ctpAssert(*edit == 'b');
		ctpAssert((edit[-1] = 'c') == 'c');
		ctpAssert(*edit == 'b');
		ctpAssert(*(edit - 1) == 'c');
	}

	return 0;
}

// ---------------------------------------------------------
// Simple random access iterator using get_index.
// ---------------------------------------------------------
template <typename S>
class random_access_get_index_iterator_t : public ctp::iterator_t<random_access_get_index_iterator_t<S>, S> {
public:
	using I = int; // Can toggle to test conversions in iterator_accessor::advance.
	I i_;
	S* str_;
	constexpr random_access_get_index_iterator_t(I a, char* str) noexcept : i_{a}, str_{str} {};

private:
	friend ctp::iterator_accessor;
	constexpr auto& get_index() noexcept { return i_; }
	constexpr auto peek() noexcept { return &str_[i_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(random_access_get_index_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

// ---------------------------------------------------------
// Simple random access iterator using get_index that returns a proxy type.
// ---------------------------------------------------------
template <typename S>
class random_access_get_index_proxy_iterator_t : public ctp::iterator_t<random_access_get_index_proxy_iterator_t<S>, S> {
public:
	int i_;
	S* str_;
	constexpr random_access_get_index_proxy_iterator_t(int a, char* str) noexcept : i_{a}, str_{str} {};

private:
	struct random_proxy {
		int& i;
		constexpr int& operator++() { return ++i; }
		constexpr int& operator--() { return --i; }
		constexpr int& operator+=(std::ptrdiff_t n) { return i += static_cast<int>(n); }
		constexpr int& operator=(std::ptrdiff_t n) { return i = static_cast<int>(n); }
		friend constexpr int operator-(const random_proxy& lhs, const random_proxy& rhs) { return lhs.i - rhs.i; }
		friend constexpr bool operator==(const random_proxy& lhs, const random_proxy& rhs) { return lhs.i == rhs.i; }
	};

	friend ctp::iterator_accessor;

	constexpr random_proxy get_index() noexcept { return {i_}; }
	constexpr auto peek() noexcept { return &str_[i_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(random_access_get_index_proxy_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

// ---------------------------------------------------------
// Simple random access iterator using other implementation functions.
// ---------------------------------------------------------
template <typename S>
class random_access_advance_iterator_t : public ctp::iterator_t<random_access_advance_iterator_t<S>, S> {
public:
	int i_;
	S* str_;
	constexpr random_access_advance_iterator_t(int a, char* str) noexcept : i_{a}, str_{str} {};

private:
	friend ctp::iterator_accessor;
	constexpr void advance(std::ptrdiff_t n) noexcept { i_ += static_cast<int>(n); }
	constexpr int distance_to(const random_access_advance_iterator_t& o) const noexcept { return o.i_ - i_; }
	constexpr auto peek() noexcept { return &str_[i_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(random_access_advance_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

constexpr int operator_arrow_test() {
	struct pair_t : public ctp::iterator_t<pair_t, std::pair<int, char>> {
		std::pair<int, char> p_;
		constexpr pair_t(std::pair<int, char> p) noexcept : p_{p} {};
		constexpr auto peek() noexcept { return &p_; }
	};

	pair_t pair{std::make_pair(5, 's')};
	ctpAssert(pair->first == 5);
	ctpAssert(pair->second == 's');

	pair->first = 4;
	pair->second = 'a';

	ctpAssert(pair->first == 4);
	ctpAssert(pair->second == 'a');

	// Operator arrow that needs an automatically-provided proxy.
	struct pair_proxy_t
		: public ctp::iterator_t<pair_proxy_t,
		                         std::pair<int, char>,
		                         std::random_access_iterator_tag,
		                         std::ptrdiff_t,
		                         std::pair<char, int>> { // reference is a value type, so arrow needs a proxy.
		constexpr auto peek() noexcept { return std::make_pair('s', 5); }
	};

	pair_proxy_t proxy;
	ctpAssert(proxy->first == 's');
	ctpAssert(proxy->second == 5);

	// The std::pair is being recreated on the fly, so assignment does nothing.
	proxy->first = 'a';
	proxy->second = 3;

	ctpAssert(proxy->first == 's');
	ctpAssert(proxy->second == 5);

	// But we can do:
	auto ptr = proxy.operator->();
	ptr->first = 'z';
	ctpAssert(ptr->first == 'z');

	return 0;
}

// ---------------------------------------------------------
// Input iterator.
// ---------------------------------------------------------
template <typename S>
class input_iterator_t : public ctp::iterator_t<input_iterator_t<S>, S> {
public:
	using I = int; // Can toggle to test conversions in iterator_accessor::advance.
	I i_;
	S* str_;
	constexpr input_iterator_t(I a, char* str) noexcept : i_{a}, str_{str} {};

private:
	friend ctp::iterator_accessor;
	constexpr void pre_increment() noexcept { ++i_; }
	constexpr bool equals(const input_iterator_t& o) const noexcept { return i_ == o.i_; }
	constexpr auto peek() noexcept { return &str_[i_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(input_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

// ---------------------------------------------------------
// Destructive input iterator.
// ---------------------------------------------------------
template <typename S>
class destructive_input_iterator_t : public ctp::iterator_t<destructive_input_iterator_t<S>, S> {
public:
	using I = int; // Can toggle to test conversions in iterator_accessor::advance.
	I i_;
	S* str_;
	constexpr destructive_input_iterator_t(I a, char* str) noexcept : i_{a}, str_{str} {};

	// Simulate expiring values by overwriting them.
	static constexpr char ReplaceChar = 'D';
private:
	friend ctp::iterator_accessor;
	constexpr void pre_increment() noexcept {
		str_[i_] = ReplaceChar;
		++i_;
	}
	constexpr bool equals(const destructive_input_iterator_t& o) const noexcept { return i_ == o.i_; }
	constexpr auto peek() noexcept { return &str_[i_]; }

	using iterator_postfix_values_expire = void;

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(destructive_input_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

template <typename Iterator>
constexpr int destructive_input_iterator_tests() {
	char chars[] = "0123456789";
	Iterator a{1, chars};

	ctpAssert(*a == '1');
	ctpAssert(*++a == '2');
	ctpAssert(*a == '2');
	ctpAssert(*a++ == '2');
	ctpAssert(*a == '3');

	// Ensure they were overwritten, as expected.
	ctpAssert(chars[0] = '0');
	ctpAssert(chars[1] = Iterator::ReplaceChar);
	ctpAssert(chars[2] = Iterator::ReplaceChar);
	ctpAssert(chars[3] = '3');

	return 0;
}

// ---------------------------------------------------------
// Bidirectional iterator.
// ---------------------------------------------------------
template <typename S>
class bidi_iterator_t : public ctp::iterator_t<bidi_iterator_t<S>, S> {
public:
	using I = int; // Can toggle to test conversions in iterator_accessor::advance.
	I i_;
	S* str_;
	constexpr bidi_iterator_t(I a, char* str) noexcept : i_{a}, str_{str} {};

private:
	friend ctp::iterator_accessor;
	constexpr void pre_increment() noexcept { ++i_; }
	constexpr void pre_decrement() noexcept { --i_; }
	constexpr bool equals(const bidi_iterator_t& o) const noexcept { return i_ == o.i_; }
	constexpr auto peek() noexcept { return &str_[i_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(bidi_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

template <typename S>
class random_access_brackets_proxy_iterator_t : public ctp::iterator_t<random_access_brackets_proxy_iterator_t<S>, S> {
public:
	int i_;
	S* str_;
	char val_;
	constexpr random_access_brackets_proxy_iterator_t(int a, char* str) noexcept : i_{a}, str_{str}, val_{str[a]} {};

	// Can't write directly back to val_
	using read_only = void;
private:
	using iterator_brackets_values_expire = void;

	friend ctp::iterator_accessor;
	constexpr auto& get_index() noexcept { return i_; }
	// Returns a reference that must be preserved by operator[]
	constexpr auto peek() noexcept {
		val_ = str_[i_];
		return &val_;
	}

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(random_access_brackets_proxy_iterator_t, S)
		: i_{other.i_}
		, str_{other.str_}
	{}
};

constexpr int RunTests() {
	random_access_iterator_tests<
		random_access_get_index_iterator_t<char>,
		random_access_get_index_iterator_t<const char>>();

	random_access_iterator_tests<
		random_access_get_index_proxy_iterator_t<char>,
		random_access_get_index_proxy_iterator_t<const char>>();

	random_access_iterator_tests<
		random_access_advance_iterator_t<char>,
		random_access_advance_iterator_t<const char>>();

	random_access_iterator_tests<
		random_access_brackets_proxy_iterator_t<char>,
		random_access_brackets_proxy_iterator_t<const char>>();

	input_iterator_tests<
		input_iterator_t<char>,
		input_iterator_t<const char>>();

	destructive_input_iterator_tests<destructive_input_iterator_t<char>>();

	bidirectional_iterator_tests<
		bidi_iterator_t<char>,
		bidi_iterator_t<const char>>();

	operator_arrow_test();

	return 0;
}

constexpr int RunConstexprTests = RunTests();

} // namespace

TEST_CASE("Run-time iterator tests", "[Tools][iterator]") {
	RunTests();
}
