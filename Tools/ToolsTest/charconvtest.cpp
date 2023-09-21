#include <catch.hpp>
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

static_assert(ToCharsConverter(5) == "5"sv);
static_assert(ToCharsConverter(123456789) == "123456789"sv);
static_assert(ToCharsConverter(-123456789) == "-123456789"sv);
static_assert(ToCharsConverter(std::numeric_limits<std::int32_t>::max()) == "2147483647"sv);
static_assert(ToCharsConverter(std::numeric_limits<std::int32_t>::min()) == "-2147483648"sv);

} // ctp

TEST_CASE("Charconv conversion helpers.", "[charconv]")
{
	using namespace ctp;

	GIVEN("A ToChars object seeded with a 32 bit integer.") {
		auto converter = ToCharsConverter{std::int32_t{12}};

		CHECK(converter == "12"sv);

		THEN("It can convert types that can be represented in the same number of digits.") {
			CHECK(converter(std::uint32_t{100}) == "100"sv);
		}
		THEN("It can convert types that can be represented in fewer digits.") {

			CHECK(converter('a') == "97"sv);
		}
	}
}
