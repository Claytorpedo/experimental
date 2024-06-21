#include <catch.hpp>

//#define CTP_CONSTEXPR_ASSERTS_ENABLED 0
#include <Tools/small_string.hpp>
#include <Tools/test/catch_test_helpers.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace ctp;
using namespace std::literals;

#define CHECK_NULL_TERMINATED(str) CTP_CHECK(*(str.c_str() + str.size()) == '\0')

TEST_CASE("fixed_string basic tests", "[Tools][fixed_string]") {
	auto test = [] {
		constexpr std::string_view expected = "hello world";
		fixed_string<11> str = expected;

		CTP_CHECK(str.size() == expected.size());
		CTP_CHECK(str.length() == expected.size());

		{
			std::size_t i = 0;
			for (const char c : str)
				CTP_CHECK(c == expected[i++]);
		}

		for (std::size_t i = 0; i < str.size(); ++i)
			CTP_CHECK(str[i] == expected[i]);

		return true;
	};

	[[maybe_unused]] constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("fixed_string constructors", "[Tools][fixed_string]") {
	auto default_construct = [] {
		fixed_string<10> str1;
		fixed_zstring<10> str2;
		CTP_CHECK(str1.empty());
		CTP_CHECK(str2.empty());
		CTP_CHECK(*(str2.c_str() + str2.size()) == '\0');

		return true;
	};

	[[maybe_unused]] constexpr bool RunDefaultConstructConstexpr = default_construct();
	default_construct();

	auto count_construct = [] {
		fixed_string<10> str1(5, 'a');
		fixed_zstring<10> str2(10, 'b');

		CTP_CHECK(str1.size() == 5);
		for (char c : str1)
			CTP_CHECK(c == 'a');
		CTP_CHECK(str2.size() == 10);
		CHECK_NULL_TERMINATED(str2);
		for (char c : str2)
			CTP_CHECK(c == 'b');

		fixed_string<10> str3(0, 'c');
		fixed_zstring<10> str4(0, 'd');
		CTP_CHECK(str3.empty());
		CTP_CHECK(str4.empty());
		CHECK_NULL_TERMINATED(str4);

		return true;
	};

	[[maybe_unused]] constexpr bool RunCountConstructConstexpr = count_construct();
	count_construct();

	auto substr_construct = [] {
		constexpr std::string_view expected = "helloworld";
		fixed_string<10> str1 = expected;

		fixed_string<10> str2(str1, 0);
		fixed_string<10> str3(str1, 5);

		CTP_CHECK(str2.size() == expected.size());
		CTP_CHECK(str3.size() == expected.size() - 5);
		CTP_CHECK(str2 == expected);
		CTP_CHECK(str3 == "world");

		fixed_zstring<10> zstr1 = expected;
		fixed_zstring<10> zstr2(zstr1, 0);
		fixed_zstring<10> zstr3(zstr1, 5);

		CTP_CHECK(zstr2.size() == expected.size());
		CTP_CHECK(zstr3.size() == expected.size() - 5);
		CTP_CHECK(zstr2 == expected);
		CTP_CHECK(zstr3 == "world");
		CHECK_NULL_TERMINATED(zstr2);
		CHECK_NULL_TERMINATED(zstr3);

		fixed_string<10> from_zstr1(zstr1, 0);
		fixed_string<10> from_zstr2(zstr1, 5);

		CTP_CHECK(from_zstr1.size() == expected.size());
		CTP_CHECK(from_zstr2.size() == expected.size() - 5);
		CTP_CHECK(from_zstr1 == expected);
		CTP_CHECK(from_zstr2 == "world");

		fixed_zstring<10> to_zstr1(str1, 0);
		fixed_zstring<10> to_zstr2(str1, 5);

		CTP_CHECK(to_zstr1.size() == expected.size());
		CTP_CHECK(to_zstr2.size() == expected.size() - 5);
		CTP_CHECK(to_zstr1 == expected);
		CTP_CHECK(to_zstr2 == "world");
		CHECK_NULL_TERMINATED(to_zstr1);
		CHECK_NULL_TERMINATED(to_zstr2);

		return true;
	};

	[[maybe_unused]] constexpr bool RunSubstrConstructConstexpr = substr_construct();
	substr_construct();

	auto substr_count_construct = [] {
		constexpr std::string_view expected = "helloworld";
		fixed_string<10> str1 = expected;

		fixed_string<10> str2(str1, 0, 2);
		fixed_string<10> str3(str1, 5, 5);
		fixed_string<10> str4(str1, 5, 6); // one too many should truncate

		CTP_CHECK(str2.size() == 2);
		CTP_CHECK(str3.size() == 5);
		CTP_CHECK(str4.size() == 5);
		CTP_CHECK(str2 == "he");
		CTP_CHECK(str3 == "world");
		CTP_CHECK(str4 == "world");

		fixed_zstring<10> zstr1 = expected;
		fixed_zstring<10> zstr2(zstr1, 0, fixed_zstring<10>::npos);
		fixed_zstring<10> zstr3(zstr1, 5, 0);

		CTP_CHECK(zstr2.size() == expected.size());
		CTP_CHECK(zstr3.size() == 0);
		CTP_CHECK(zstr2 == expected);
		CTP_CHECK(zstr3 == "");
		CHECK_NULL_TERMINATED(zstr2);
		CHECK_NULL_TERMINATED(zstr3);

		fixed_string<10> from_zstr1(zstr1, 1, 7);
		fixed_string<10> from_zstr2(zstr1, 5, fixed_string<10>::npos);

		CTP_CHECK(from_zstr1.size() == 7);
		CTP_CHECK(from_zstr2.size() == 5);
		CTP_CHECK(from_zstr1 == "ellowor");
		CTP_CHECK(from_zstr2 == "world");

		fixed_zstring<10> to_zstr1(str1, 9, 2);
		fixed_zstring<10> to_zstr2(str1, 5, 4);

		CTP_CHECK(to_zstr1.size() == 1);
		CTP_CHECK(to_zstr2.size() == 4);
		CTP_CHECK(to_zstr1 == "d");
		CTP_CHECK(to_zstr2 == "worl");
		CHECK_NULL_TERMINATED(to_zstr1);
		CHECK_NULL_TERMINATED(to_zstr2);

		return true;
	};

	[[maybe_unused]] constexpr bool RunSubstrCountConstructConstexpr = substr_count_construct();
	substr_count_construct();

	auto move_substr_construct = [] {
		constexpr std::string_view expected = "helloworld";
		fixed_string<10> str1 = expected;

		fixed_string<10> str2(std::move(str1), 0);
		CTP_CHECK(str1.empty());

		str1 = expected;
		fixed_string<10> str3(std::move(str1), 5);

		CTP_CHECK(str2.size() == expected.size());
		CTP_CHECK(str3.size() == expected.size() - 5);
		CTP_CHECK(str2 == expected);
		CTP_CHECK(str3 == "world");

		fixed_zstring<10> zstr1 = expected;
		fixed_zstring<10> zstr2(std::move(zstr1), 0);
		CTP_CHECK(zstr1.empty());
		CHECK_NULL_TERMINATED(zstr1);

		zstr1 = expected;
		fixed_zstring<10> zstr3(std::move(zstr1), 5);

		CTP_CHECK(zstr2.size() == expected.size());
		CTP_CHECK(zstr3.size() == expected.size() - 5);
		CTP_CHECK(zstr2 == expected);
		CTP_CHECK(zstr3 == "world");
		CHECK_NULL_TERMINATED(zstr2);
		CHECK_NULL_TERMINATED(zstr3);

		zstr1 = expected;
		fixed_string<10> from_zstr1(std::move(zstr1), 0);
		zstr1 = expected;
		fixed_string<10> from_zstr2(std::move(zstr1), 5);

		CTP_CHECK(from_zstr1.size() == expected.size());
		CTP_CHECK(from_zstr2.size() == expected.size() - 5);
		CTP_CHECK(from_zstr1 == expected);
		CTP_CHECK(from_zstr2 == "world");

		str1 = expected;
		fixed_zstring<10> to_zstr1(std::move(str1), 0);
		str1 = expected;
		fixed_zstring<10> to_zstr2(std::move(str1), 5);

		CTP_CHECK(to_zstr1.size() == expected.size());
		CTP_CHECK(to_zstr2.size() == expected.size() - 5);
		CTP_CHECK(to_zstr1 == expected);
		CTP_CHECK(to_zstr2 == "world");
		CHECK_NULL_TERMINATED(to_zstr1);
		CHECK_NULL_TERMINATED(to_zstr2);

		return true;
	};

	[[maybe_unused]] constexpr bool RunMoveSubstrConstructConstexpr = move_substr_construct();
	move_substr_construct();

	auto move_substr_count_construct = [] {
		constexpr std::string_view expected = "helloworld";
		fixed_string<10> str1 = expected;

		fixed_string<10> str2(std::move(str1), 0, 2);
		CTP_CHECK(str1.empty());
		str1 = expected;
		fixed_string<10> str3(std::move(str1), 5, 5);
		str1 = expected;
		fixed_string<10> str4(std::move(str1), 5, 6); // one too many should truncate

		CTP_CHECK(str2.size() == 2);
		CTP_CHECK(str3.size() == 5);
		CTP_CHECK(str4.size() == 5);
		CTP_CHECK(str2 == "he");
		CTP_CHECK(str3 == "world");
		CTP_CHECK(str4 == "world");

		fixed_zstring<10> zstr1 = expected;
		fixed_zstring<10> zstr2(std::move(zstr1), 0, fixed_zstring<10>::npos);
		CTP_CHECK(zstr1.empty());
		CHECK_NULL_TERMINATED(zstr1);
		zstr1 = expected;
		fixed_zstring<10> zstr3(std::move(zstr1), 5, 0);

		CTP_CHECK(zstr2.size() == expected.size());
		CTP_CHECK(zstr3.size() == 0);
		CTP_CHECK(zstr2 == expected);
		CTP_CHECK(zstr3 == "");
		CHECK_NULL_TERMINATED(zstr2);
		CHECK_NULL_TERMINATED(zstr3);

		zstr1 = expected;
		fixed_string<10> from_zstr1(std::move(zstr1), 1, 7);
		zstr1 = expected;
		fixed_string<10> from_zstr2(std::move(zstr1), 5, fixed_string<10>::npos);

		CTP_CHECK(from_zstr1.size() == 7);
		CTP_CHECK(from_zstr2.size() == 5);
		CTP_CHECK(from_zstr1 == "ellowor");
		CTP_CHECK(from_zstr2 == "world");

		str1 = expected;
		fixed_zstring<10> to_zstr1(std::move(str1), 9, 2);
		str1 = expected;
		fixed_zstring<10> to_zstr2(std::move(str1), 5, 4);

		CTP_CHECK(to_zstr1.size() == 1);
		CTP_CHECK(to_zstr2.size() == 4);
		CTP_CHECK(to_zstr1 == "d");
		CTP_CHECK(to_zstr2 == "worl");
		CHECK_NULL_TERMINATED(to_zstr1);
		CHECK_NULL_TERMINATED(to_zstr2);

		return true;
	};

	[[maybe_unused]] constexpr bool RunMoveSubstrCountConstructConstexpr = move_substr_count_construct();
	move_substr_count_construct();

	auto cstr_construct = [] {
		constexpr const char* expected = "helloworld";
		const fixed_string<10> str1 = expected;
		const fixed_string<10> str2(expected, 0);
		const fixed_string<10> str3(expected, 6);
		const fixed_string<10> str4(expected);

		CTP_CHECK(str1.size() == 10);
		CTP_CHECK(str2.size() == 0);
		CTP_CHECK(str3.size() == 6);
		CTP_CHECK(str4.size() == 10);
		CTP_CHECK(str1 == expected);
		CTP_CHECK(str2 == "");
		CTP_CHECK(str3 == "hellow");
		CTP_CHECK(str4 == expected);

		const fixed_zstring<10> zstr1 = expected;
		const fixed_zstring<10> zstr2(expected, 0);
		const fixed_zstring<10> zstr3(expected, 4);
		const fixed_zstring<10> zstr4(expected);

		CTP_CHECK(zstr1.size() == 10);
		CTP_CHECK(zstr2.size() == 0);
		CTP_CHECK(zstr3.size() == 4);
		CTP_CHECK(zstr4.size() == 10);
		CTP_CHECK(zstr1 == expected);
		CTP_CHECK(zstr2 == "");
		CTP_CHECK(zstr3 == "hell");
		CTP_CHECK(zstr4 == expected);
		CHECK_NULL_TERMINATED(zstr1);
		CHECK_NULL_TERMINATED(zstr2);
		CHECK_NULL_TERMINATED(zstr3);
		CHECK_NULL_TERMINATED(zstr4);

		return true;
	};

	[[maybe_unused]] constexpr bool RunCStrConstructConstexpr = cstr_construct();
	cstr_construct();

	auto iterator_construct = [] {
		constexpr const char* expected = "helloworld";
		const fixed_string<10> str = expected;

		const fixed_string<10> empty(str.begin(), str.begin());
		CTP_CHECK(empty.size() == 0);

		const fixed_string<10> copy(str.begin(), str.end());
		CTP_CHECK(copy == str);

		const fixed_string<10> substr(str.begin() + 1, str.last());
		CTP_CHECK(substr == "elloworl");

		const fixed_zstring<10> zstr = expected;

		const fixed_zstring<10> zempty(zstr.begin(), zstr.begin());
		CTP_CHECK(zempty.size() == 0);
		CHECK_NULL_TERMINATED(zempty);

		const fixed_zstring<10> zcopy(zstr.begin(), zstr.end());
		CTP_CHECK(zcopy == str);
		CHECK_NULL_TERMINATED(zcopy);

		const fixed_zstring<10> zcopy_fromstr(str.begin(), str.end());
		CTP_CHECK(zcopy_fromstr == zcopy);
		CHECK_NULL_TERMINATED(zcopy_fromstr);

		const fixed_zstring<10> zsubstr(zstr.begin() + 1, zstr.last());
		CTP_CHECK(zsubstr == "elloworl");
		CHECK_NULL_TERMINATED(zsubstr);

		const fixed_zstring<10> zsubstr_fromstr(str.begin() + 1, str.last());
		CTP_CHECK(zsubstr_fromstr == "elloworl");
		CHECK_NULL_TERMINATED(zsubstr_fromstr);

		const fixed_string<10> empty_fromzstr(zstr.begin(), zstr.begin());
		CTP_CHECK(empty_fromzstr.size() == 0);

		const fixed_string<10> copy_fromzstr(zstr.begin(), zstr.end());
		CTP_CHECK(copy_fromzstr == str);

		const fixed_string<10> substr_fromzstr(zstr.begin() + 1, zstr.last());
		CTP_CHECK(substr_fromzstr.size() == 8);
		CTP_CHECK(substr_fromzstr == "elloworl");

		return true;
	};

	[[maybe_unused]] constexpr bool RunIteratorConstructConstexpr = iterator_construct();
	iterator_construct();

	auto copy_construct = [] {
		constexpr std::string_view expected = "helloworld";
		const fixed_string<10> str = expected;

		const fixed_string<10> copy(str);

		CTP_CHECK(copy == expected);

		const fixed_zstring<10> zstr = expected;
		const fixed_zstring<10> zcopy(zstr);
		CTP_CHECK(zcopy == expected);
		CHECK_NULL_TERMINATED(zcopy);

		const fixed_string<10> from_zstr(zstr);
		CTP_CHECK(from_zstr == expected);

		const fixed_zstring<10> to_zstr(str);
		CTP_CHECK(to_zstr == expected);
		CHECK_NULL_TERMINATED(to_zstr);

		return true;
	};
	[[maybe_unused]] constexpr bool RunCopyConstructConstexpr = copy_construct();
	copy_construct();

	auto move_construct = [] {
		constexpr std::string_view expected = "helloworld";
		fixed_string<10> str = expected;

		const fixed_string<10> moved(std::move(str));

		CTP_CHECK(moved == expected);
		CTP_CHECK(str.is_empty());

		fixed_zstring<10> zstr = expected;
		const fixed_zstring<10> zmoved(std::move(zstr));
		CTP_CHECK(zmoved == expected);
		CHECK_NULL_TERMINATED(zmoved);
		CTP_CHECK(zstr.is_empty());
		CHECK_NULL_TERMINATED(zstr);

		zstr = expected;
		const fixed_string<10> from_zstr(std::move(zstr));
		CTP_CHECK(from_zstr == expected);
		CTP_CHECK(zstr.is_empty());
		CHECK_NULL_TERMINATED(zstr);

		str = expected;
		const fixed_zstring<10> to_zstr(std::move(str));
		CTP_CHECK(to_zstr == expected);
		CHECK_NULL_TERMINATED(to_zstr);
		CTP_CHECK(str.is_empty());

		return true;
	};
	[[maybe_unused]] constexpr bool RunMoveConstructConstexpr = move_construct();
	move_construct();

	auto ilist_construct = [] {
		const fixed_string<10> str{'h','e','l','l','o','w','o','r','l','d'};
		CTP_CHECK(str == "helloworld"sv);

		const fixed_zstring<10> zstr{'h','e','l','l','o','w','o','r','l','d'};
		CTP_CHECK(zstr == "helloworld"sv);
		CHECK_NULL_TERMINATED(zstr);

		return true;
	};
	[[maybe_unused]] constexpr bool RunIlistConstructConstexpr = ilist_construct();
	ilist_construct();

	auto view_convertible_construct = [] {
		std::string stdstring = "helloworld";
		std::string_view view = stdstring;

		const fixed_string<10> str(stdstring);
		CTP_CHECK(str == stdstring);
		const fixed_string<10> str2(view);
		CTP_CHECK(str2 == stdstring);

		const fixed_zstring<10> zstr(stdstring);
		CTP_CHECK(zstr == stdstring);
		CHECK_NULL_TERMINATED(zstr);
		const fixed_zstring<10> zstr2(view);
		CTP_CHECK(zstr2 == stdstring);
		CHECK_NULL_TERMINATED(zstr2);

		return true;
	};
	[[maybe_unused]] constexpr bool RunViewConvertibleConstructConstexpr = view_convertible_construct();
	view_convertible_construct();


	auto range_construct = [] {
		std::vector<char> vec{'h','e','l','l','o','w','o','r','l','d'};
		const fixed_string<10> str(std::from_range_t{}, vec);
		CTP_CHECK(str == "helloworld"sv);
		
		const fixed_zstring<10> zstr(std::from_range_t{}, vec);
		CTP_CHECK(zstr == "helloworld"sv);
		CHECK_NULL_TERMINATED(zstr);

		return true;
	};
	[[maybe_unused]] constexpr bool RunRangeConstructConstexpr = range_construct();
	range_construct();
}

TEST_CASE("fixed_string operator=", "[Tools][fixed_string]") {
	const auto copy_assign = [] {
		constexpr std::string_view expected = "helloworld";
		const fixed_string<10> str = expected;

		fixed_string<10> copy;
		copy = str;
		CTP_CHECK(copy == expected);

		copy = "somestuff";
		copy = str;
		CTP_CHECK(copy == expected);

		const fixed_zstring<10> zstr = expected;
		fixed_zstring<10> zcopy;
		zcopy = zstr;
		CTP_CHECK(zcopy == expected);
		CHECK_NULL_TERMINATED(zcopy);
		zcopy = "somestuff";
		zcopy = zstr;
		CTP_CHECK(zcopy == expected);
		CHECK_NULL_TERMINATED(zcopy);

		fixed_string<10> from_zstr;
		from_zstr = zstr;
		CTP_CHECK(from_zstr == expected);
		from_zstr = "somestuff";
		from_zstr = zstr;
		CTP_CHECK(from_zstr == expected);

		fixed_zstring<10> to_zstr;
		to_zstr = str;
		CTP_CHECK(to_zstr == expected);
		CHECK_NULL_TERMINATED(to_zstr);
		to_zstr = "somestuff";
		to_zstr = str;
		CTP_CHECK(to_zstr == expected);
		CHECK_NULL_TERMINATED(to_zstr);

		return true;
	};
	[[maybe_unused]] constexpr bool RunCopyAssignConstexpr = copy_assign();
	copy_assign();

	const auto move_assign = [] {
		constexpr std::string_view expected = "helloworld";
		fixed_string<10> str = expected;

		fixed_string<10> moved;
		moved = std::move(str);
		CTP_CHECK(moved == expected);
		CTP_CHECK(str.is_empty());

		moved = "somestuff";
		str = expected;
		moved = std::move(str);
		CTP_CHECK(moved == expected);
		CTP_CHECK(str.is_empty());

		fixed_zstring<10> zstr = expected;
		fixed_zstring<10> zmoved;
		zmoved = std::move(zstr);
		CTP_CHECK(zmoved == expected);
		CHECK_NULL_TERMINATED(zmoved);
		CTP_CHECK(zstr.is_empty());
		zmoved = "somestuff";
		zstr = expected;
		zmoved = std::move(zstr);
		CTP_CHECK(zmoved == expected);
		CHECK_NULL_TERMINATED(zmoved);
		CTP_CHECK(zstr.is_empty());
		CHECK_NULL_TERMINATED(zstr);

		fixed_string<10> from_zstr;
		zstr = expected;
		from_zstr = std::move(zstr);
		CTP_CHECK(from_zstr == expected);
		CTP_CHECK(zstr.is_empty());
		CHECK_NULL_TERMINATED(zstr);
		from_zstr = "somestuff";
		zstr = expected;
		from_zstr = std::move(zstr);
		CTP_CHECK(from_zstr == expected);
		CTP_CHECK(zstr.is_empty());
		CHECK_NULL_TERMINATED(zstr);

		fixed_zstring<10> to_zstr;
		str = expected;
		to_zstr = std::move(str);
		CTP_CHECK(to_zstr == expected);
		CHECK_NULL_TERMINATED(to_zstr);
		CTP_CHECK(zstr.is_empty());
		CHECK_NULL_TERMINATED(zstr);
		to_zstr = "somestuff";
		str = expected;
		to_zstr = std::move(str);
		CTP_CHECK(to_zstr == expected);
		CHECK_NULL_TERMINATED(to_zstr);
		CTP_CHECK(str.is_empty());

		return true;
	};
	[[maybe_unused]] constexpr bool RunMoveAssignConstexpr = move_assign();
	move_assign();

	const auto cstr_assign = [] {
		constexpr std::string_view expected = "helloworld";
		const char* expected1 = "helloworld";
		const char* expected2 = "somestuff";

		fixed_string<10> copy;
		copy = expected1;
		CTP_CHECK(copy == expected1);
		copy = expected2;
		CTP_CHECK(copy == expected2);

		fixed_zstring<10> zcopy;
		zcopy = expected1;
		CTP_CHECK(zcopy == expected1);
		CHECK_NULL_TERMINATED(zcopy);
		zcopy = expected2;
		CTP_CHECK(zcopy == expected2);
		CHECK_NULL_TERMINATED(zcopy);

		return true;
	};
	[[maybe_unused]] constexpr bool RunCstrAssignConstexpr = cstr_assign();
	cstr_assign();

	const auto ilist_assign = [] {
		fixed_string<10> copy;
		copy = {'h','e','l','l','o','w','o','r','l','d'};
		CTP_CHECK(copy == "helloworld"sv);
		copy = {'s','o','m','e','s','t','u','f','f'};
		CTP_CHECK(copy == "somestuff");

		fixed_zstring<10> zcopy;
		zcopy = {'h','e','l','l','o','w','o','r','l','d'};
		CTP_CHECK(zcopy == "helloworld"sv);
		CHECK_NULL_TERMINATED(zcopy);
		zcopy = {'s','o','m','e','s','t','u','f','f'};
		CTP_CHECK(zcopy == "somestuff");
		CHECK_NULL_TERMINATED(zcopy);

		return true;
	};
	[[maybe_unused]] constexpr bool RunIlistAssignConstexpr = ilist_assign();
	ilist_assign();

	const auto view_convertible_assign = [] {
		fixed_string<10> copy;
		copy = "helloworld"sv; // std::string_view
		CTP_CHECK(copy == "helloworld"sv);
		copy = "somestuff"s; // std::string
		CTP_CHECK(copy == "somestuff");

		fixed_zstring<10> zcopy;
		zcopy = "helloworld"sv; // std::string_view
		CTP_CHECK(zcopy == "helloworld"sv);
		CHECK_NULL_TERMINATED(zcopy);
		zcopy = "somestuff"s; // std::string
		CTP_CHECK(zcopy == "somestuff");
		CHECK_NULL_TERMINATED(zcopy);

		return true;
	};
	[[maybe_unused]] constexpr bool RunVConvAssignConstexpr = view_convertible_assign();
	view_convertible_assign();
}

TEST_CASE("fixed_string assign", "[Tools][fixed_string]") {
	const auto count_char_assign = [] {
		{
			fixed_string<10> copy;
			copy.assign(10, 'a');
			CTP_CHECK(copy == "aaaaaaaaaa");
			copy.assign(5, 'b');
			CTP_CHECK(copy == "bbbbb");
			copy.assign(0, 'c');
			CTP_CHECK(copy == "");
		}
		{
			fixed_zstring<10> zcopy;
			zcopy.assign(10, 'a');
			CTP_CHECK(zcopy == "aaaaaaaaaa");
			CHECK_NULL_TERMINATED(zcopy);
			zcopy.assign(5, 'b');
			CTP_CHECK(zcopy == "bbbbb");
			CHECK_NULL_TERMINATED(zcopy);
			zcopy.assign(0, 'c');
			CTP_CHECK(zcopy == "");
			CHECK_NULL_TERMINATED(zcopy);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunCountCharAssignConstexpr = count_char_assign();
	count_char_assign();

	const auto small_string_move_assign = [] {
		const std::string_view expected1 = "helloworld";
		const std::string_view expected2 = "somestuff";
		{
			fixed_string<10> str = expected1;
			fixed_string<10> copy;
			copy.assign(std::move(str));
			CTP_CHECK(copy == expected1);
			CTP_CHECK(str.empty());
			str = expected2;
			copy.assign(std::move(str));
			CTP_CHECK(copy == expected2);
			CTP_CHECK(str.empty());
			copy.assign(std::move(str));
			CTP_CHECK(copy.empty());
		}
		{
			fixed_zstring<10> zstr = expected1;
			fixed_zstring<10> zcopy;
			zcopy.assign(std::move(zstr));
			CTP_CHECK(zcopy == expected1);
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zcopy);
			CHECK_NULL_TERMINATED(zstr);
			zstr = expected2;
			zcopy.assign(std::move(zstr));
			CTP_CHECK(zcopy == expected2);
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zcopy);
			CHECK_NULL_TERMINATED(zstr);
			zcopy.assign(std::move(zstr));
			CTP_CHECK(zcopy.empty());
			CHECK_NULL_TERMINATED(zcopy);
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunSSMvAssignConstexpr = small_string_move_assign();
	small_string_move_assign();

	const auto cstr_assign = [] {
		constexpr std::string_view expected = "helloworld";
		const char* expected1 = "helloworld";
		const char* expected2 = "somestuff";
		{
			fixed_string<10> copy;
			copy.assign(expected1);
			CTP_CHECK(copy == expected1);
			copy.assign(expected2);
			CTP_CHECK(copy == expected2);
		}
		{
			fixed_zstring<10> zcopy;
			zcopy.assign(expected1);
			CTP_CHECK(zcopy == expected1);
			CHECK_NULL_TERMINATED(zcopy);
			zcopy.assign(expected2);
			CTP_CHECK(zcopy == expected2);
			CHECK_NULL_TERMINATED(zcopy);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunCstrAssignConstexpr = cstr_assign();
	cstr_assign();

	const auto cstr_count_assign = [] {
		constexpr std::string_view expected = "helloworld";
		const char* expected1 = "helloworld";
		const char* expected2 = "somestuff";
		{
			fixed_string<10> copy;
			copy.assign(expected1, 10);
			CTP_CHECK(copy == expected1);
			copy.assign(expected2, 9);
			CTP_CHECK(copy == expected2);
			copy.assign(expected1, 2);
			CTP_CHECK(copy == "he");
			copy.assign(expected1, 0);
			CTP_CHECK(copy.empty());
		}
		{
			fixed_zstring<10> zcopy;
			zcopy.assign(expected1);
			CTP_CHECK(zcopy == expected1);
			CHECK_NULL_TERMINATED(zcopy);
			zcopy.assign(expected2);
			CTP_CHECK(zcopy == expected2);
			CHECK_NULL_TERMINATED(zcopy);
			zcopy.assign(expected1, 2);
			CTP_CHECK(zcopy == "he");
			CHECK_NULL_TERMINATED(zcopy);
			zcopy.assign(expected1, 0);
			CTP_CHECK(zcopy.empty());
			CHECK_NULL_TERMINATED(zcopy);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunCstrCountAssignConstexpr = cstr_count_assign();
	cstr_count_assign();

	const auto iterator_assign = [] {
		constexpr const char* expected = "helloworld";
		const fixed_string<10> str = expected;

		fixed_string<10> test;
		test.assign(str.begin(), str.begin());
		CTP_CHECK(test.size() == 0);

		test.assign(str.begin(), str.end());
		CTP_CHECK(test == str);

		test.assign(str.begin() + 1, str.last());
		CTP_CHECK(test == "elloworl");

		const fixed_zstring<10> zstr = expected;

		fixed_zstring<10> ztest(zstr.begin(), zstr.begin());
		CTP_CHECK(ztest.size() == 0);
		CHECK_NULL_TERMINATED(ztest);

		ztest.assign(zstr.begin(), zstr.end());
		CTP_CHECK(ztest == str);
		CHECK_NULL_TERMINATED(ztest);

		ztest.assign(str.begin(), str.end());
		CTP_CHECK(ztest == zstr);
		CHECK_NULL_TERMINATED(ztest);

		ztest.assign(zstr.begin() + 1, zstr.last());
		CTP_CHECK(ztest == "elloworl");
		CHECK_NULL_TERMINATED(ztest);

		ztest.assign(str.begin() + 1, str.last());
		CTP_CHECK(ztest == "elloworl");
		CHECK_NULL_TERMINATED(ztest);

		test.assign(zstr.begin(), zstr.begin());
		CTP_CHECK(test.size() == 0);

		test.assign(zstr.begin(), zstr.end());
		CTP_CHECK(test == str);

		test.assign(zstr.begin() + 1, zstr.last());
		CTP_CHECK(test == "elloworl");

		return true;
	};
	[[maybe_unused]] constexpr bool RunIteratorAssignConstexpr = iterator_assign();
	iterator_assign();

	const auto ilist_assign = [] {
		{
			fixed_string<10> str;
			str.assign({'h','e','l','l','o','w','o','r','l','d'});
			CTP_CHECK(str == "helloworld"sv);
			str.assign({'s','o','m','e','s','t','u','f','f'});
			CTP_CHECK(str == "somestuff");
		}
		{
			fixed_zstring<10> zstr;
			zstr.assign({'h','e','l','l','o','w','o','r','l','d'});
			CTP_CHECK(zstr == "helloworld"sv);
			CHECK_NULL_TERMINATED(zstr);
			zstr.assign({'s','o','m','e','s','t','u','f','f'});
			CTP_CHECK(zstr == "somestuff");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunIlistAssignConstexpr = ilist_assign();
	ilist_assign();

	// view convertible (string_view, basic_string)
	const auto view_convertible_assign = [] {
		{
			fixed_string<10> str;
			str.assign("helloworld"sv); // std::string_view
			CTP_CHECK(str == "helloworld"sv);
			str.assign("somestuff"s); // std::string
			CTP_CHECK(str == "somestuff");
		}
		{
			fixed_zstring<10> zstr;
			zstr.assign("helloworld"sv); // std::string_view
			CTP_CHECK(zstr == "helloworld"sv);
			CHECK_NULL_TERMINATED(zstr);
			zstr.assign("somestuff"s); // std::string
			CTP_CHECK(zstr == "somestuff");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunVConvAssignConstexpr = view_convertible_assign();
	view_convertible_assign();

	const auto assign_range = [] {
		const std::vector<char> vec{'h','e','l','l','o','w','o','r','l','d'};
		const std::string r2 = "somestuff";
		{
			fixed_string<10> str;
			str.assign_range(vec);
			CTP_CHECK(str == "helloworld"sv);
			str.assign_range(r2);
			CTP_CHECK(str == "somestuff");
		}
		{
			fixed_zstring<10> zstr;
			zstr.assign_range(vec);
			CTP_CHECK(zstr == "helloworld"sv);
			CHECK_NULL_TERMINATED(zstr);
			zstr.assign_range(r2);
			CTP_CHECK(zstr == "somestuff");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunAssignRangeConstexpr = assign_range();
	assign_range();
}

TEST_CASE("fixed_string clear", "[Tools][fixed_string]") {
	const auto clear = [] {
		{
			fixed_string<10> str = "helloworld";
			str.clear();
			CTP_CHECK(str.empty());
			str.clear();
			CTP_CHECK(str.empty());
		}
		{
			fixed_zstring<10> zstr = "helloworld";
			zstr.clear();
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
			zstr.clear();
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunClearConstexpr = clear();
	clear();
}

TEST_CASE("fixed_string c_str", "[Tools][fixed_string]") {
	static const auto streql = [] (const char* lhs, const char* rhs) {
		constexpr std::size_t Max = 12;

		for (std::size_t i = 0; i < Max; ++i) {
			if (lhs[i] != rhs[i])
				return false;
			if (lhs[i] == '\0')
				return rhs[i] == '\0';
			if (rhs[i] == '\0')
				return false;
		}

		return false;
	};

	const auto c_str = [] {
		// c_str disallowed for non-zstring

		fixed_zstring<10> zstr;
		CTP_CHECK(streql(zstr.c_str(), ""));
		zstr = "helloworld";
		CTP_CHECK(streql(zstr.c_str(), "helloworld"));
		return true;
	};
	[[maybe_unused]] constexpr bool RunCstrConstexpr = c_str();
	c_str();
}

TEST_CASE("fixed_string view", "[Tools][fixed_string]") {
	// check sizes of null vs non null
	const auto sview = [] {
		const fixed_string<10> str = "helloworl";
		const fixed_zstring<10> zstr = str;
		CHECK_NULL_TERMINATED(zstr);

		const std::string_view sview = str;
		const std::string_view zsview = zstr;

		CTP_CHECK(sview.size() == 9);
		CTP_CHECK(zsview.size() == 9);
		CTP_CHECK(sview == zsview);

		return true;
	};
	[[maybe_unused]] constexpr bool RunSviewConstexpr = sview();
	sview();
}

TEST_CASE("fixed_string append / operator+=", "[Tools][fixed_string]") {
	const auto char_append = [] {
		{
			fixed_string<10> str;
			str.append('a');
			str.append(5, 'b');
			str.append(0, 'c');
			str += 'd';
			CTP_CHECK(str == "abbbbbd");
		}
		{
			fixed_zstring<10> zstr;
			zstr.append('a');
			zstr.append(5, 'b');
			zstr.append(0, 'c');
			zstr += 'd';
			CTP_CHECK(zstr == "abbbbbd");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunCharAppendConstexpr = char_append();
	char_append();

	const auto view_append = [] {
		{
			fixed_string<10> str;
			str.append(""sv);
			str.append("a"sv);
			str.append("helloworld"sv, 5);
			str.append("helloworld"sv, 1, 2);
			str.append("helloworld"sv, 0, 0);
			str += "b"sv;
			str += fixed_string<1>("z");
			CTP_CHECK(str == "aworldelbz");
		}
		{
			fixed_zstring<10> zstr;
			zstr.append(""sv);
			zstr.append("a"sv);
			zstr.append("helloworld"sv, 5);
			zstr.append("helloworld"sv, 1, 2);
			zstr.append("helloworld"sv, 0, 0);
			zstr += "b"sv;
			zstr += fixed_string<1>("z");
			CTP_CHECK(zstr == "aworldelbz");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunViewAppendConstexpr = view_append();
	view_append();

	const auto iterator_append = [] {
		const fixed_string<20> range = "onetwothreefourfives";
		{
			fixed_string<10> str;
			str.append(range.begin(), range.begin() + 5);
			str.append(range.rbegin(), range.rbegin() + 5);
			CTP_CHECK(str == "onetwsevif");
		}
		{
			fixed_zstring<10> zstr;
			zstr.append(range.begin(), range.begin() + 5);
			zstr.append(range.rbegin(), range.rbegin() + 5);
			CTP_CHECK(zstr == "onetwsevif");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunIteratorAppendConstexpr = iterator_append();
	iterator_append();

	const auto ilist_append = [] {
		const fixed_string<20> range = "onetwothreefourfives";
		{
			fixed_string<10> str;
			str.append({'h','e','l','l','o'});
			str += {'w','o','r','l','d'};
			CTP_CHECK(str == "helloworld"sv);
		}
		{
			fixed_zstring<10> zstr;
			zstr.append({'h','e','l','l','o'});
			zstr += {'w','o','r','l','d'};
			CTP_CHECK(zstr == "helloworld"sv);
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunIlistAppendConstexpr = ilist_append();
	ilist_append();

	const auto append_range = [] {
		const std::vector<char> vec{'h','e','l','l','o'};
		const std::string r2 = "world";
		{
			fixed_string<10> str;
			str.append_range(vec);
			CTP_CHECK(str == "hello");
			str.append_range(r2);
			CTP_CHECK(str == "helloworld"sv);
		}
		{
			fixed_zstring<10> zstr;
			zstr.append_range(vec);
			CTP_CHECK(zstr == "hello");
			CHECK_NULL_TERMINATED(zstr);
			zstr.append_range(r2);
			CTP_CHECK(zstr == "helloworld"sv);
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunAppendRangeConstexpr = append_range();
	append_range();
}

TEST_CASE("fixed_string replace", "[Tools][fixed_string]") {
	const auto pos_count_view_replace = [] {
		{
			fixed_string<10> str;
			str.replace(0, 5, "aaaaaaaa"sv);
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(0, 2, "bb"sv);
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(0, 0, "cc"sv);
			CTP_CHECK(str == "ccbbaaaaaa");
			str.replace(8, fixed_string<10>::npos, "bb"sv);
			CTP_CHECK(str == "ccbbaaaabb");
			str.replace(1, 8, "d"sv);
			CTP_CHECK(str == "cdb");
			str.replace(1, 1, "eeeeee"sv);
			CTP_CHECK(str == "ceeeeeeb");
			str.replace(0, 99, ""sv);
			CTP_CHECK(str.empty());
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(0, 5, "aaaaaaaa"sv);
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 2, "bb"sv);
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 0, "cc"sv);
			CTP_CHECK(zstr == "ccbbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(8, fixed_zstring<10>::npos, "bb"sv);
			CTP_CHECK(zstr == "ccbbaaaabb");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(1, 8, "d"sv);
			CTP_CHECK(zstr == "cdb");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(1, 1, "eeeeee"sv);
			CTP_CHECK(zstr == "ceeeeeeb");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 99, ""sv);
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunPosCountViewReplaceConstexpr = pos_count_view_replace();
	pos_count_view_replace();

	const auto first_last_view_replace = [] {
		{
			fixed_string<10> str;
			str.replace(str.begin(), str.end(), "aaaaaaaa"sv);
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(str.begin(), str.begin() + 2, "bb"sv);
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(str.begin(), str.begin(), "cc"sv);
			CTP_CHECK(str == "ccbbaaaaaa");
			str.replace(str.begin() + 8, str.end(), "bb"sv);
			CTP_CHECK(str == "ccbbaaaabb");
			str.replace(str.begin() + 1, str.end() - 1, "d"sv);
			CTP_CHECK(str == "cdb");
			str.replace(str.begin() + 1, str.begin() + 2, "eeeeee"sv);
			CTP_CHECK(str == "ceeeeeeb");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(zstr.begin(), zstr.end(), "aaaaaaaa"sv);
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin() + 2, "bb"sv);
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin(), "cc"sv);
			CTP_CHECK(zstr == "ccbbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin() + 8, zstr.end(), "bb"sv);
			CTP_CHECK(zstr == "ccbbaaaabb");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin() + 1, zstr.end() - 1, "d"sv);
			CTP_CHECK(zstr == "cdb");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin() + 1, zstr.begin() + 2, "eeeeee"sv);
			CTP_CHECK(zstr == "ceeeeeeb");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunFirstLastViewReplaceConstexpr = first_last_view_replace();
	first_last_view_replace();

	const auto positions_replace = [] {
		{
			fixed_string<10> str;
			str.replace(0, 5, "aaaaaaaa"sv, 0, 8);
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(0, 2, "zbbz"sv, 1, 2);
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(0, 0, "zzcc"sv, 2 /*, npos*/);
			CTP_CHECK(str == "ccbbaaaaaa");
			str.replace(8, fixed_string<10>::npos, "bbzz"sv, 0, 2);
			CTP_CHECK(str == "ccbbaaaabb");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(0, 5, "aaaaaaaa"sv, 0, 8);
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 2, "zbbz"sv, 1, 2);
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 0, "zzcc"sv, 2 /*, npos*/);
			CTP_CHECK(zstr == "ccbbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(8, fixed_zstring<10>::npos, "bbzz"sv, 0, 2);
			CTP_CHECK(zstr == "ccbbaaaabb");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunPositionsReplaceConstexpr = positions_replace();
	positions_replace();

	const auto first_last_cstr_replace = [] {
		{
			fixed_string<10> str;
			str.replace(str.begin(), str.end(), "aaaaaaaa", 8);
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(str.begin(), str.begin() + 2, "bbzz", 2);
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(str.begin(), str.begin(), "cc", 0);
			CTP_CHECK(str == "bbaaaaaa");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(zstr.begin(), zstr.end(), "aaaaaaaa", 8);
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin() + 2, "bbzz", 2);
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin(), "cc", 0);
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunFirstLastCstrReplaceConstexpr = first_last_cstr_replace();
	first_last_cstr_replace();

	const auto pos_count_chars_replace = [] {
		{
			fixed_string<10> str;
			str.replace(0, 5, 8, 'a');
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(0, 2, 2, 'b');
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(0, 0, 0, 'c');
			CTP_CHECK(str == "bbaaaaaa");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(0, 5, 8, 'a');
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 2, 2, 'b');
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(0, 0, 0, 'c');
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunPosCountCharsReplaceConstexpr = pos_count_chars_replace();
	pos_count_chars_replace();

	const auto first_last_chars_replace = [] {
		{
			fixed_string<10> str;
			str.replace(str.begin(), str.end(), 8, 'a');
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(str.begin(), str.begin() + 2, 2, 'b');
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(str.begin(), str.begin(), 0, 'c');
			CTP_CHECK(str == "bbaaaaaa");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(zstr.begin(), zstr.end(), 8, 'a');
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin() + 2, 2, 'b');
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin(), 0, 'c');
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunFirstLastCharsReplaceConstexpr = first_last_chars_replace();
	first_last_chars_replace();

	const auto first_last_first2_last2_replace = [] {
		constexpr std::string_view one = "aaaaaaaa";
		constexpr std::string_view two = "zzbbzz";
		constexpr std::string_view three = "zzcc";
		{
			fixed_string<10> str;
			str.replace(str.begin(), str.end(), one.begin(), one.end());
			CTP_CHECK(str == "aaaaaaaa");
			str.replace(str.begin(), str.begin() + 2, two.begin() + 2, two.end() - 2);
			CTP_CHECK(str == "bbaaaaaa");
			str.replace(str.begin(), str.begin(), three.begin() + 2, three.end());
			CTP_CHECK(str == "ccbbaaaaaa");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(zstr.begin(), zstr.end(), one.begin(), one.end());
			CTP_CHECK(zstr == "aaaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin() + 2, two.begin() + 2, two.end() - 2);
			CTP_CHECK(zstr == "bbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin(), three.begin() + 2, three.end());
			CTP_CHECK(zstr == "ccbbaaaaaa");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunFirstLastFirst2Last2ReplaceConstexpr = first_last_first2_last2_replace();
	first_last_first2_last2_replace();

	const auto first_last_ilist_replace = [] {
		{
			fixed_string<10> str;
			str.replace(str.begin(), str.end(), {'h','e','l','l','o'});
			CTP_CHECK(str == "hello");
			str.replace(str.begin(), str.begin() + 2, {'b', 'b'});
			CTP_CHECK(str == "bbllo");
			str.replace(str.begin() + 1, str.end(), {'w','o','r','l','d'});
			CTP_CHECK(str == "bworld");
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace(zstr.begin(), zstr.end(), {'h','e','l','l','o'});
			CTP_CHECK(zstr == "hello");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin(), zstr.begin() + 2, {'b', 'b'});
			CTP_CHECK(zstr == "bbllo");
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace(zstr.begin() + 1, zstr.end(), {'w','o','r','l','d'});
			CTP_CHECK(zstr == "bworld");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunFirstLastIlistReplaceConstexpr = first_last_ilist_replace();
	first_last_ilist_replace();

	const auto replace_with_range = [] {
		std::string one = "helloworld";
		std::string two = "replace";
		{
			fixed_string<10> str;
			str.replace_with_range(str.begin(), str.end(), one);
			CTP_CHECK(str == "helloworld"sv);
			str.replace_with_range(str.begin() + 2, str.end() - 1, two);
			CTP_CHECK(str == "hereplaced"sv);
		}
		{
			fixed_zstring<10> zstr;
			zstr.replace_with_range(zstr.begin(), zstr.end(), one);
			CTP_CHECK(zstr == "helloworld"sv);
			CHECK_NULL_TERMINATED(zstr);
			zstr.replace_with_range(zstr.begin() + 2, zstr.end() - 1, two);
			CTP_CHECK(zstr == "hereplaced");
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunReplaceWithRangeConstexpr = replace_with_range();
	replace_with_range();
}

TEST_CASE("fixed_string resize", "[Tools][fixed_string]") {
	const auto default_resize = [] {
		fixed_string<10> str;
		str.resize(10);
		CTP_CHECK(str.size() == 10);
		for (std::size_t i = 0; i < 10; ++i)
			CTP_CHECK(str[i] == '\0');
		str.resize(2);
		CTP_CHECK(str.size() == 2);
		for (std::size_t i = 0; i < 2; ++i)
			CTP_CHECK(str[i] == '\0');

		fixed_zstring<10> zstr;
		zstr.resize(10);
		CTP_CHECK(zstr.size() == 10);
		for (std::size_t i = 0; i < 10; ++i)
			CTP_CHECK(zstr[i] == '\0');
		CHECK_NULL_TERMINATED(zstr);
		zstr.resize(2);
		CTP_CHECK(zstr.size() == 2);
		for (std::size_t i = 0; i < 2; ++i)
			CTP_CHECK(zstr[i] == '\0');
		CHECK_NULL_TERMINATED(zstr);

		return true;
	};
	[[maybe_unused]] constexpr bool RunDefaultResizeConstexpr = default_resize();
	default_resize();

	const auto resize_w_char = [] {
		fixed_string<10> str;
		str.resize(10, 'a');
		CTP_CHECK(str.size() == 10);
		CTP_CHECK(str == "aaaaaaaaaa");
		str.resize(2, 'b');
		CTP_CHECK(str.size() == 2);
		CTP_CHECK(str == "aa");
		str.resize(5, 'c');
		CTP_CHECK(str.size() == 5);
		CTP_CHECK(str == "aaccc");

		fixed_zstring<10> zstr;
		zstr.resize(10, 'a');
		CTP_CHECK(zstr.size() == 10);
		CTP_CHECK(zstr == "aaaaaaaaaa");
		CHECK_NULL_TERMINATED(zstr);
		zstr.resize(2, 'b');
		CTP_CHECK(zstr.size() == 2);
		CTP_CHECK(zstr == "aa");
		CHECK_NULL_TERMINATED(zstr);
		zstr.resize(5, 'c');
		CTP_CHECK(zstr.size() == 5);
		CTP_CHECK(zstr == "aaccc");
		CHECK_NULL_TERMINATED(zstr);

		return true;
	};
	[[maybe_unused]] constexpr bool RunResizeWCharConstexpr = resize_w_char();
	resize_w_char();
}

TEST_CASE("fixed_string swap", "[Tools][fixed_string]") {
	const auto do_swap = [] {
		fixed_string<10> str1 = "helloworld";
		fixed_string<10> str2 = "somestuff";

		std::swap(str1, str2);
		CTP_CHECK(str1 == "somestuff");
		CTP_CHECK(str2 == "helloworld"sv);

		fixed_zstring<10> zstr1 = "helloworld";
		fixed_zstring<10> zstr2 = "somestuff";

		std::swap(zstr1, zstr2);
		CTP_CHECK(zstr1 == "somestuff");
		CTP_CHECK(zstr2 == "helloworld"sv);

		str1.swap(zstr2);
		CTP_CHECK(str1 == "helloworld"sv);
		CTP_CHECK(zstr2 == "somestuff");

		return true;
	};
	[[maybe_unused]] constexpr bool RunSwapConstexpr = do_swap();
	do_swap();
}

TEST_CASE("fixed_string starts_with, ends_with, contains", "[Tools][fixed_string]") {
	const auto starts_with = [] {
		const fixed_string<10> str = "helloworld";
		CTP_CHECK(str.starts_with("hello"));
		CTP_CHECK(str.starts_with("helloworld"));
		CTP_CHECK_FALSE(str.starts_with("ello"));

		const fixed_zstring<10> zstr = "helloworld";
		CTP_CHECK(zstr.starts_with("hello"));
		CTP_CHECK(zstr.starts_with("helloworld"));
		CTP_CHECK_FALSE(zstr.starts_with("ello"));

		return true;
	};
	[[maybe_unused]] constexpr bool RunStartsWithConstexpr = starts_with();
	starts_with();

	const auto ends_with = [] {
		const fixed_string<10> str = "helloworld";
		CTP_CHECK(str.ends_with("world"));
		CTP_CHECK(str.ends_with("helloworld"));
		CTP_CHECK_FALSE(str.ends_with("worl"));

		const fixed_zstring<10> zstr = "helloworld";
		CTP_CHECK(zstr.ends_with("world"));
		CTP_CHECK(zstr.ends_with("helloworld"));
		CTP_CHECK_FALSE(zstr.ends_with("worl"));

		return true;
	};
	[[maybe_unused]] constexpr bool RunEndsWithConstexpr = ends_with();
	ends_with();

	const auto contains = [] {
		const fixed_string<10> str = "helloworld";
		CTP_CHECK(str.contains("hello"));
		CTP_CHECK(str.contains("world"));
		CTP_CHECK(str.contains("helloworld"));
		CTP_CHECK(str.contains("elloworl"));
		CTP_CHECK_FALSE(str.contains("hellob"));

		const fixed_zstring<10> zstr = "helloworld";
		CTP_CHECK(zstr.contains("hello"));
		CTP_CHECK(zstr.contains("world"));
		CTP_CHECK(zstr.contains("helloworld"));
		CTP_CHECK(zstr.contains("elloworl"));
		CTP_CHECK_FALSE(zstr.contains("hellob"));

		return true;
	};
	[[maybe_unused]] constexpr bool RunContainsConstexpr = contains();
	contains();
}

TEST_CASE("fixed_string substr", "[Tools][fixed_string]") {
	const auto copy_substr = [] {
		{
			const fixed_string<10> str = "helloworld";
			CTP_CHECK(str.substr(0) == "helloworld"sv);
			CTP_CHECK(str.substr(2) == "lloworld");
			CTP_CHECK(str.substr(0, 5) == "hello");
			CTP_CHECK(str.substr(0, 99) == "helloworld"sv);
			CTP_CHECK(str.substr(2, 5) == "llowo");
		}
		{
			const fixed_zstring<10> zstr = "helloworld";
			CTP_CHECK(zstr.substr(0) == "helloworld"sv);
			CTP_CHECK(zstr.substr(2) == "lloworld");
			CTP_CHECK(zstr.substr(0, 5) == "hello");
			CTP_CHECK(zstr.substr(0, 99) == "helloworld"sv);
			CTP_CHECK(zstr.substr(2, 5) == "llowo");
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunCopySubstrConstexpr = copy_substr();
	copy_substr();

	const auto move_stubstr = [] {
		const std::string_view expected = "helloworld";
		{
			fixed_string<10> str = expected;
			CTP_CHECK(std::move(str).substr(0) == "helloworld"sv);
			CTP_CHECK(str.empty());
			str = expected;
			CTP_CHECK(std::move(str).substr(2) == "lloworld");
			CTP_CHECK(str.empty());
			str = expected;
			CTP_CHECK(std::move(str).substr(0, 5) == "hello");
			CTP_CHECK(str.empty());
			str = expected;
			CTP_CHECK(std::move(str).substr(0, 99) == "helloworld"sv);
			CTP_CHECK(str.empty());
			str = expected;
			CTP_CHECK(std::move(str).substr(2, 5) == "llowo");
			CTP_CHECK(str.empty());
		}
		{
			fixed_zstring<10> zstr = expected;
			CTP_CHECK(std::move(zstr).substr(0) == "helloworld"sv);
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
			zstr = expected;
			CTP_CHECK(std::move(zstr).substr(2) == "lloworld");
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
			zstr = expected;
			CTP_CHECK(std::move(zstr).substr(0, 5) == "hello");
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
			zstr = expected;
			CTP_CHECK(std::move(zstr).substr(0, 99) == "helloworld"sv);
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
			zstr = expected;
			CTP_CHECK(std::move(zstr).substr(2, 5) == "llowo");
			CTP_CHECK(zstr.empty());
			CHECK_NULL_TERMINATED(zstr);
		}
		return true;
	};
	[[maybe_unused]] constexpr bool RunMoveSubstrConstexpr = move_stubstr();
	move_stubstr();
}

TEST_CASE("fixed_string operator==, operator<=>", "[Tools][fixed_string]") {
	const auto equality_test = [] {
		const fixed_string<10> str1 = "helloworld";
		const fixed_string<10> str2 = "somestuff";

		CTP_CHECK(str1 == str1);
		CTP_CHECK(str1 == "helloworld"sv);
		CTP_CHECK_FALSE(str1 == "elloworld");
		CTP_CHECK(str1 == "helloworld"sv);
		CTP_CHECK(str1 == "helloworld"s);
		CTP_CHECK(str1 != str2);

		const fixed_zstring<10> zstr1 = "helloworld";
		const fixed_zstring<10> zstr2 = "somestuff";

		CTP_CHECK(zstr1 == zstr1);
		CTP_CHECK(zstr1 == "helloworld"sv);
		CTP_CHECK_FALSE(zstr1 == "elloworld");
		CTP_CHECK(zstr1 == "helloworld"sv);
		CTP_CHECK(zstr1 == "helloworld"s);
		CTP_CHECK(zstr1 != zstr2);

		CTP_CHECK(str1 == zstr1);
		CTP_CHECK_FALSE(str1 == zstr2);

		return true;
	};
	[[maybe_unused]] constexpr bool RunEqualityTestConstexpr = equality_test();
	equality_test();

	const auto three_way_test = [] {
		const fixed_string<10> str1 = "helloworld";
		const fixed_string<10> str2 = "somestuff";

		CTP_CHECK((str1 <=> str1) == std::strong_ordering::equal);
		CTP_CHECK_FALSE(str1 > str2);
		CTP_CHECK(str1 < str2);
		CTP_CHECK_FALSE(str1 < "helloworld");
		CTP_CHECK(str1 > "elloworld");
		CTP_CHECK(str1 >= "helloworld"sv);
		CTP_CHECK(str1 < "zhelloworld"sv);
		CTP_CHECK(str1 <= "zhelloworld"s);

		const fixed_zstring<10> zstr1 = "helloworld";
		const fixed_zstring<10> zstr2 = "somestuff";

		CTP_CHECK((zstr1 <=> zstr1) == std::strong_ordering::equal);
		CTP_CHECK_FALSE(zstr1 > zstr1);
		CTP_CHECK(zstr1 < zstr2);
		CTP_CHECK_FALSE(zstr1 < "helloworld");
		CTP_CHECK(zstr1 > "elloworld");
		CTP_CHECK(zstr1 >= "helloworld"sv);
		CTP_CHECK(zstr1 < "zhelloworld"s);
		CTP_CHECK(zstr1 <= "zhelloworld"s);

		CTP_CHECK((str1 <=> zstr1) == std::strong_ordering::equal);
		CTP_CHECK(str1 >= zstr1);
		CTP_CHECK(str1 < zstr2);

		return true;
	};
	[[maybe_unused]] constexpr bool RunThreeWayTestConstexpr = three_way_test();
	three_way_test();
}

TEST_CASE("fixed_string operator>>, operator<<, getline", "[Tools][fixed_string]") {
	SECTION("Inputting or outputting with a stringstream.") {
		std::stringstream stream;

		const fixed_string<10> str = "helloworld";

		stream << str;

		fixed_string<10> strout;
		CHECK(stream >> strout);
		CHECK(strout == "helloworld"sv);

		stream << "space  toomanycharacters";

		CHECK(stream >> strout);
		CHECK(strout == "space");

		// Too many characters is fine for output stream.
		// It will write as many as possible.
		CHECK(stream >> strout);
		CHECK(strout == "toomanycha");
		CHECK(stream >> strout);
		CHECK(strout == "racters");
		CHECK_FALSE(stream >> strout);
	}

	SECTION("Extracting from a stringstream with getline.") {
		std::stringstream stream;
		stream << "one; two ;toomanycharacters";

		fixed_string<10> str;

		SECTION("Using ; as the delimiter.") {
			CHECK(std::getline(stream, str, ';'));
			CHECK(str == "one");
			CHECK(std::getline(stream, str, ';'));
			CHECK(str == " two ");

			// Too many characters so getline should fail.
			CHECK_FALSE(std::getline(stream, str, ';'));
			CHECK(str == "toomanycha");
			stream.clear(); // Clears fail bit.
			CHECK(std::getline(stream, str, ';'));
			CHECK(str == "racters");
		}
	}
}

TEST_CASE("small_string general", "[Tools][small_string]") {
	const auto test = [] {
		small_string<10> small = "helloworld";
		
		{
			const fixed_string<10> copy = small;
			CTP_CHECK(copy == "helloworld"sv);
		}
		{
			const fixed_zstring<10> zcopy = small;
			CTP_CHECK(zcopy == "helloworld"sv);
			CHECK_NULL_TERMINATED(zcopy);
		}

		small += "switchToBigMode";
		CTP_CHECK(small == "helloworldswitchToBigMode"sv);

		small_zstring<10> zsmall = small;
		CTP_CHECK(zsmall == "helloworldswitchToBigMode"sv);
		CHECK_NULL_TERMINATED(zsmall);

		small += 'z';
		zsmall = std::move(small);

		CTP_CHECK(small.empty());
		CTP_CHECK(zsmall == "helloworldswitchToBigModez"sv);

		small = std::move(zsmall);

		CTP_CHECK(zsmall.empty());
		CHECK_NULL_TERMINATED(zsmall);
		CTP_CHECK(small == "helloworldswitchToBigModez"sv);

		return true;
	};
	[[maybe_unused]] constexpr bool RunTestConstexpr = test();
	test();
}

#undef CHECK_NULL_TERMINATED
