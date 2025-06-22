#include <catch.hpp>

#include <Tools/test/catch_test_helpers.hpp>

#include <Tools/zstring_view.hpp>

#include <string>

using namespace ctp;
using namespace ctp::literals;
using namespace std::literals;

TEST_CASE("zstring_view construction and basic ops", "[Tools][zstring_view]") {
	auto test = [] {
		CTP_CHECK(""_zv == ""sv);
		CTP_CHECK(zstring_view{} == "");
		CTP_CHECK(zstring_view{"hi"_zv} == "hi");

		// size checks
		CTP_CHECK(zstring_view{}.is_empty());
		CTP_CHECK_FALSE("hi"_zv.is_empty());
		CTP_CHECK("hi"_zv.size() == 2);
		CTP_CHECK(""_zv.length() == 0);

		// equality
		CTP_CHECK("one_two" == "one_two"sv);
		CTP_CHECK("one_two" != "one_twoo"sv);
		CTP_CHECK("one_two" < "one_twoo"sv);
		CTP_CHECK_FALSE("one_two" > "one_twoo"sv);

		// can construct from std::string as it's null terminated
		CTP_CHECK(zstring_view{"onetwothreefourfive"s} == "onetwothreefourfive"_zv);

		// string_view
		static_assert(!std::is_constructible_v<zstring_view, std::string_view>);
		CTP_CHECK(zstring_view{zstring_view::null_terminated, "test"sv} == "test"sv);

		// literal + size is valid if it is null terminated at size
		CTP_CHECK(zstring_view{"test", 4} == "test");

		CTP_CHECK(*"testo"_zv.last() == 'o');

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}
