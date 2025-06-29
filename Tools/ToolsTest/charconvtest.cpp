#include <catch.hpp>

#include <Tools/test/catch_test_helpers.hpp>
#include <Tools/charconv.hpp>

using namespace std::literals;

namespace ctp {

static_assert(max_char_digits_10_v<char> == 4);
static_assert(max_char_digits_10_v<signed char> == 4);
static_assert(max_char_digits_10_v<unsigned char> == 3);
static_assert(max_char_digits_10_v<std::int32_t> == 11);
static_assert(max_char_digits_10_v<std::uint32_t> == 10);
static_assert(max_char_digits_10_v<std::int64_t> == 20);
static_assert(max_char_digits_10_v<std::uint64_t> == 20);

} // ctp

TEST_CASE("Charconv conversion helpers.", "[charconv]")
{
	using namespace ctp;

	auto test = [] {
		constexpr auto str = ToConstantString<1234567>();
		CTP_CHECK(str == "1234567"sv);

		CTP_CHECK(ToCharsConverter(5) == "5"sv);
		CTP_CHECK(ToCharsConverter(123456789) == "123456789"sv);
		CTP_CHECK(ToCharsConverter(-123456789) == "-123456789"sv);
		CTP_CHECK(ToCharsConverter(std::numeric_limits<std::int32_t>::max()) == "2147483647"sv);
		CTP_CHECK(ToCharsConverter(std::numeric_limits<std::int32_t>::min()) == "-2147483648"sv);

		auto converter = ToCharsConverter{std::int32_t{12}};
		CTP_CHECK(converter == "12"sv);
		CTP_CHECK(converter.view() == "12"sv);

		// It can convert types that can be represented in the same number of digits.
		CTP_CHECK(converter(std::uint32_t{100}) == "100"sv);
		CTP_CHECK(converter(4294967295u) == "4294967295"sv);

		// It can convert types that can be represented in fewer digits.

		CTP_CHECK(converter('a') == "97"sv);

		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}
