#include <catch.hpp>

#include <Tools/enum_reflection.hpp>

#include <Tools/test/catch_test_helpers.hpp>

#include <array>

using namespace ctp;
using namespace std::literals;

namespace {

enum class Contiguous {
	NegOne = -1,
	Zero = 0,
	One = 1,
	Two = 2,
	Three = 3,
};

enum class NonContiguous {
	NegOne = -1,
	One = 1,
	Three = 3,
};

enum class Overlap : unsigned {
	Zero = 0,
	One = 1,
	Two = 2,
	TwoNumberTwo = Two,
};

enum class RangeLimited {
	One = 1,
	Two = 2,
	Three = 3,
	Four = 4,
};

} // namespace

CTP_CUSTOM_ENUM_MIN_MAX(RangeLimited, 2, 3)


TEST_CASE("enum_reflection type_name", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK("Contiguous"_zv == enums::type_name<Contiguous>());
		CTP_CHECK("NonContiguous"_zv == enums::type_name<NonContiguous>());
		CTP_CHECK("Overlap"_zv == enums::type_name<Overlap>());
		CTP_CHECK("RangeLimited"_zv == enums::type_name<RangeLimited>());
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection name", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK("NegOne"_zv == enums::name(Contiguous::NegOne));
		CTP_CHECK("Zero"_zv == enums::name(Contiguous::Zero));
		CTP_CHECK("One"_zv == enums::name(Contiguous::One));
		CTP_CHECK("Two"_zv == enums::name(Contiguous::Two));
		CTP_CHECK("Three"_zv == enums::name(Contiguous::Three));

		CTP_CHECK("NegOne"_zv == enums::name(NonContiguous::NegOne));
		CTP_CHECK("One"_zv == enums::name(NonContiguous::One));
		CTP_CHECK("Three"_zv == enums::name(NonContiguous::Three));

		CTP_CHECK("Zero"_zv == enums::name(Overlap::Zero));
		CTP_CHECK("One"_zv == enums::name(Overlap::One));
		CTP_CHECK("Two"_zv == enums::name(Overlap::Two));
		CTP_CHECK("Two"_zv == enums::name(Overlap::TwoNumberTwo));

		CTP_CHECK(""_zv == enums::name(RangeLimited::One));
		CTP_CHECK("Two"_zv == enums::name(RangeLimited::Two));
		CTP_CHECK("Three"_zv == enums::name(RangeLimited::Three));
		CTP_CHECK(""_zv == enums::name(RangeLimited::Four));

		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection size", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(5 == enums::size<Contiguous>());
		CTP_CHECK(3 == enums::size<NonContiguous>());
		CTP_CHECK(3 == enums::size<Overlap>());
		CTP_CHECK(2 == enums::size<RangeLimited>());
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection values", "[Tools][enum_reflection]") {
	constexpr auto test = [] {
		{
			constexpr std::array expected = {
				Contiguous::NegOne,
				Contiguous::Zero,
				Contiguous::One,
				Contiguous::Two,
				Contiguous::Three
			};
			CTP_CHECK(expected == enums::values<Contiguous>());
		}
		{
			constexpr std::array expected = {
				NonContiguous::NegOne,
				NonContiguous::One,
				NonContiguous::Three
			};
			CTP_CHECK(expected == enums::values<NonContiguous>());
		}
		{
			constexpr std::array expected = {
				Overlap::Zero,
				Overlap::One,
				Overlap::Two,
			};
			CTP_CHECK(expected == enums::values<Overlap>());
		}
		{
			constexpr std::array expected = {
				RangeLimited::Two,
				RangeLimited::Three,
			};
			CTP_CHECK(expected == enums::values<RangeLimited>());
		}
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection min_val", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(Contiguous::NegOne == enums::min_val<Contiguous>());
		CTP_CHECK(NonContiguous::NegOne == enums::min_val<NonContiguous>());
		CTP_CHECK(Overlap::Zero == enums::min_val<Overlap>());
		CTP_CHECK(RangeLimited::Two == enums::min_val<RangeLimited>());
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection max_val", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(Contiguous::Three == enums::max_val<Contiguous>());
		CTP_CHECK(NonContiguous::Three == enums::max_val<NonContiguous>());
		CTP_CHECK(Overlap::Two == enums::max_val<Overlap>());
		CTP_CHECK(RangeLimited::Three == enums::max_val<RangeLimited>());
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection names", "[Tools][enum_reflection]") {
	auto test = [] {
		{
			constexpr std::array expected = {
				"NegOne"_zv,
				"Zero"_zv,
				"One"_zv,
				"Two"_zv,
				"Three"_zv,
			};
			CTP_CHECK(expected == enums::names<Contiguous>());

			static_assert(
				std::is_same_v<
				decltype(enums::names<Contiguous>()),
				const std::array<zstring_view, 5>&>);
		}
		{
			constexpr std::array expected = {
				"NegOne"_zv,
				"One"_zv,
				"Three"_zv,
			};
			CTP_CHECK(expected == enums::names<NonContiguous>());
		}
		{
			constexpr std::array expected = {
				"Zero"_zv,
				"One"_zv,
				"Two"_zv,
			};
			CTP_CHECK(expected == enums::names<Overlap>());
		}
		{
			constexpr std::array expected = {
				"Two"_zv,
				"Three"_zv,
			};
			CTP_CHECK(expected == enums::names<RangeLimited>());
		}
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection try_cast<E>", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(std::nullopt == enums::try_cast<Contiguous>(-2));
		CTP_CHECK(Contiguous::NegOne == enums::try_cast<Contiguous>(-1));
		CTP_CHECK(Contiguous::Zero == enums::try_cast<Contiguous>(0));
		CTP_CHECK(Contiguous::One == enums::try_cast<Contiguous>(1));
		CTP_CHECK(Contiguous::Two == enums::try_cast<Contiguous>(2));
		CTP_CHECK(Contiguous::Three == enums::try_cast<Contiguous>(3));
		CTP_CHECK(std::nullopt == enums::try_cast<Contiguous>(4));

		CTP_CHECK(NonContiguous::NegOne == enums::try_cast<NonContiguous>(-1));
		CTP_CHECK(std::nullopt == enums::try_cast<NonContiguous>(0));
		CTP_CHECK(NonContiguous::One == enums::try_cast<NonContiguous>(1));
		CTP_CHECK(std::nullopt == enums::try_cast<NonContiguous>(2));
		CTP_CHECK(NonContiguous::Three == enums::try_cast<NonContiguous>(3));
		CTP_CHECK(std::nullopt == enums::try_cast<NonContiguous>(4));

		CTP_CHECK(Overlap::Zero == enums::try_cast<Overlap>(0));
		CTP_CHECK(Overlap::One == enums::try_cast<Overlap>(1));
		CTP_CHECK(Overlap::Two == enums::try_cast<Overlap>(2));
		CTP_CHECK(std::nullopt == enums::try_cast<Overlap>(3));

		CTP_CHECK(std::nullopt == enums::try_cast<RangeLimited>(1));
		CTP_CHECK(RangeLimited::Two == enums::try_cast<RangeLimited>(2));
		CTP_CHECK(RangeLimited::Three == enums::try_cast<RangeLimited>(3));
		CTP_CHECK(std::nullopt == enums::try_cast<RangeLimited>(4));

		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection try_cast<string_view>", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(Contiguous::NegOne == enums::try_cast<Contiguous>("NegOne"sv));
		CTP_CHECK(Contiguous::Zero == enums::try_cast<Contiguous>("Zero"sv));
		CTP_CHECK(Contiguous::One == enums::try_cast<Contiguous>("One"sv));
		CTP_CHECK(Contiguous::Two == enums::try_cast<Contiguous>("Two"sv));
		CTP_CHECK(Contiguous::Three == enums::try_cast<Contiguous>("Three"sv));
		CTP_CHECK(std::nullopt == enums::try_cast<Contiguous>("Unknown"sv));

		CTP_CHECK(NonContiguous::NegOne == enums::try_cast<NonContiguous>("NegOne"sv));
		CTP_CHECK(NonContiguous::One == enums::try_cast<NonContiguous>("One"sv));
		CTP_CHECK(NonContiguous::Three == enums::try_cast<NonContiguous>("Three"sv));
		CTP_CHECK(std::nullopt == enums::try_cast<NonContiguous>("Unknown"sv));

		CTP_CHECK(Overlap::Zero == enums::try_cast<Overlap>("Zero"sv));
		CTP_CHECK(Overlap::One == enums::try_cast<Overlap>("One"sv));
		CTP_CHECK(Overlap::Two == enums::try_cast<Overlap>("Two"sv));
		CTP_CHECK(std::nullopt == enums::try_cast<Overlap>("TwoNumberTwo"sv));
		CTP_CHECK(std::nullopt == enums::try_cast<Overlap>("Unknown"sv));

		CTP_CHECK(std::nullopt == enums::try_cast<RangeLimited>("One"sv));
		CTP_CHECK(RangeLimited::Two == enums::try_cast<RangeLimited>("Two"sv));
		CTP_CHECK(RangeLimited::Three == enums::try_cast<RangeLimited>("Three"sv));
		CTP_CHECK(std::nullopt == enums::try_cast<RangeLimited>("Four"sv));

		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection try_cast_icase<string_view>", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(Contiguous::NegOne == enums::try_cast_icase<Contiguous>("NegOne"sv));
		CTP_CHECK(Contiguous::Zero == enums::try_cast_icase<Contiguous>("Zero"sv));
		CTP_CHECK(Contiguous::One == enums::try_cast_icase<Contiguous>("One"sv));
		CTP_CHECK(Contiguous::Two == enums::try_cast_icase<Contiguous>("Two"sv));
		CTP_CHECK(Contiguous::Three == enums::try_cast_icase<Contiguous>("Three"sv));
		CTP_CHECK(Contiguous::NegOne == enums::try_cast_icase<Contiguous>("negone"sv));
		CTP_CHECK(Contiguous::Zero == enums::try_cast_icase<Contiguous>("zero"sv));
		CTP_CHECK(Contiguous::One == enums::try_cast_icase<Contiguous>("one"sv));
		CTP_CHECK(Contiguous::Two == enums::try_cast_icase<Contiguous>("two"sv));
		CTP_CHECK(Contiguous::Three == enums::try_cast_icase<Contiguous>("three"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<Contiguous>("Unknown"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<Contiguous>("unknown"sv));

		CTP_CHECK(NonContiguous::NegOne == enums::try_cast_icase<NonContiguous>("NegOne"sv));
		CTP_CHECK(NonContiguous::One == enums::try_cast_icase<NonContiguous>("One"sv));
		CTP_CHECK(NonContiguous::Three == enums::try_cast_icase<NonContiguous>("Three"sv));
		CTP_CHECK(NonContiguous::NegOne == enums::try_cast_icase<NonContiguous>("negone"sv));
		CTP_CHECK(NonContiguous::One == enums::try_cast_icase<NonContiguous>("one"sv));
		CTP_CHECK(NonContiguous::Three == enums::try_cast_icase<NonContiguous>("three"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<NonContiguous>("Unknown"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<NonContiguous>("unknown"sv));

		CTP_CHECK(Overlap::Zero == enums::try_cast_icase<Overlap>("Zero"sv));
		CTP_CHECK(Overlap::One == enums::try_cast_icase<Overlap>("One"sv));
		CTP_CHECK(Overlap::Two == enums::try_cast_icase<Overlap>("Two"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<Overlap>("TwoNumberTwo"sv));
		CTP_CHECK(Overlap::Zero == enums::try_cast_icase<Overlap>("zero"sv));
		CTP_CHECK(Overlap::One == enums::try_cast_icase<Overlap>("one"sv));
		CTP_CHECK(Overlap::Two == enums::try_cast_icase<Overlap>("two"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<Overlap>("twonumbertwo"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<Overlap>("Unknown"sv));
		CTP_CHECK(std::nullopt == enums::try_cast_icase<Overlap>("unknown"sv));

		CTP_CHECK(RangeLimited::Two == enums::try_cast_icase<RangeLimited>("two"sv));
		CTP_CHECK(RangeLimited::Three == enums::try_cast_icase<RangeLimited>("three"sv));

		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

namespace {

enum class LookupTableEnum {
	NegOne = -1,
	One = 1,
	Two = 2,
	Three = 3,
	Five = 5,
	Seven = 7,
	Ten = 10,
};

// Should also use lookup table.
enum class WideRange {
	NegThirtyTwo = -32,
	NegOne = -1,
	One = 1,
	Two = 2,
	Ten = 10,
	Twenty = 20,
	Thirty = 30,
	Fourty = 40,
	Fifty = 50,
	Sixty = 60,
	Seventy = 70,
	Eighty = 80,
	Ninety = 90,
	OneHundred = 100,
	OneTen = 110,
	OneTwenty = 120,
	OneTwentyEight = 128,
};

enum class PositiveOffset {
	Thousand = 1000,
	ThousandOne = 1001,
	ThousandTwo = 1002,
};

enum class NegativeOffset {
	NegThousand = -1000,
	NegThousandOne = -1001,
	NegThousandTwo = -1002,
};

// Contiguous
enum class CustomHuge {
	NegOneHundred = -100,
	NegNinetyNine = -99,
	NegNinetyEight = -98,
	NegNinetySeven = -97,
	NegNinetySix = -96,
	NegNinetyFive = -95,
	NegNinetyFour = -94,
	NegNinetyThree = -93,
	NegNinetyTwo = -92,
	NegNinetyOne = -91,
	NegNinety = -90,
	NegEightyNine = -89,
	NegEightyEight = -88,
	NegEightySeven = -87,
	NegEightySix = -86,
	NegEightyFive = -85,
	NegEightyFour = -84,
	NegEightyThree = -83,
	NegEightyTwo = -82,
	NegEightyOne = -81,
	NegEighty = -80,
	NegSeventyNine = -79,
	NegSeventyEight = -78,
	NegSeventySeven = -77,
	NegSeventySix = -76,
	NegSeventyFive = -75,
	NegSeventyFour = -74,
	NegSeventyThree = -73,
	NegSeventyTwo = -72,
	NegSeventyOne = -71,
	NegSeventy = -70,
	NegSixtyNine = -69,
	NegSixtyEight = -68,
	NegSixtySeven = -67,
	NegSixtySix = -66,
	NegSixtyFive = -65,
	NegSixtyFour = -64,
	NegSixtyThree = -63,
	NegSixtyTwo = -62,
	NegSixtyOne = -61,
	NegSixty = -60,
	NegFiftyNine = -59,
	NegFiftyEight = -58,
	NegFiftySeven = -57,
	NegFiftySix = -56,
	NegFiftyFive = -55,
	NegFiftyFour = -54,
	NegFiftyThree = -53,
	NegFiftyTwo = -52,
	NegFiftyOne = -51,
	NegFifty = -50,
	NegFortyNine = -49,
	NegFortyEight = -48,
	NegFortySeven = -47,
	NegFortySix = -46,
	NegFortyFive = -45,
	NegFortyFour = -44,
	NegFortyThree = -43,
	NegFortyTwo = -42,
	NegFortyOne = -41,
	NegForty = -40,
	NegThirtyNine = -39,
	NegThirtyEight = -38,
	NegThirtySeven = -37,
	NegThirtySix = -36,
	NegThirtyFive = -35,
	NegThirtyFour = -34,
	NegThirtyThree = -33,
	NegThirtyTwo = -32,
	NegThirtyOne = -31,
	NegThirty = -30,
	NegTwentyNine = -29,
	NegTwentyEight = -28,
	NegTwentySeven = -27,
	NegTwentySix = -26,
	NegTwentyFive = -25,
	NegTwentyFour = -24,
	NegTwentyThree = -23,
	NegTwentyTwo = -22,
	NegTwentyOne = -21,
	NegTwenty = -20,
	NegNineteen = -19,
	NegEighteen = -18,
	NegSeventeen = -17,
	NegSixteen = -16,
	NegFifteen = -15,
	NegFourteen = -14,
	NegThirteen = -13,
	NegTwelve = -12,
	NegEleven = -11,
	NegTen = -10,
	NegNine = -9,
	NegEight = -8,
	NegSeven = -7,
	NegSix = -6,
	NegFive = -5,
	NegFour = -4,
	NegThree = -3,
	NegTwo = -2,
	NegOne = -1,
	Zero = 0,
	One = 1,
	Two = 2,
	Three = 3,
	Four = 4,
	Five = 5,
	Six = 6,
	Seven = 7,
	Eight = 8,
	Nine = 9,
	Ten = 10,
	Eleven = 11,
	Twelve = 12,
	Thirteen = 13,
	Fourteen = 14,
	Fifteen = 15,
	Sixteen = 16,
	Seventeen = 17,
	Eighteen = 18,
	Nineteen = 19,
	Twenty = 20,
	TwentyOne = 21,
	TwentyTwo = 22,
	TwentyThree = 23,
	TwentyFour = 24,
	TwentyFive = 25,
	TwentySix = 26,
	TwentySeven = 27,
	TwentyEight = 28,
	TwentyNine = 29,
	Thirty = 30,
	ThirtyOne = 31,
	ThirtyTwo = 32,
	ThirtyThree = 33,
	ThirtyFour = 34,
	ThirtyFive = 35,
	ThirtySix = 36,
	ThirtySeven = 37,
	ThirtyEight = 38,
	ThirtyNine = 39,
	Forty = 40,
	FortyOne = 41,
	FortyTwo = 42,
	FortyThree = 43,
	FortyFour = 44,
	FortyFive = 45,
	FortySix = 46,
	FortySeven = 47,
	FortyEight = 48,
	FortyNine = 49,
	Fifty = 50,
	FiftyOne = 51,
	FiftyTwo = 52,
	FiftyThree = 53,
	FiftyFour = 54,
	FiftyFive = 55,
	FiftySix = 56,
	FiftySeven = 57,
	FiftyEight = 58,
	FiftyNine = 59,
	Sixty = 60,
	SixtyOne = 61,
	SixtyTwo = 62,
	SixtyThree = 63,
	SixtyFour = 64,
	SixtyFive = 65,
	SixtySix = 66,
	SixtySeven = 67,
	SixtyEight = 68,
	SixtyNine = 69,
	Seventy = 70,
	SeventyOne = 71,
	SeventyTwo = 72,
	SeventyThree = 73,
	SeventyFour = 74,
	SeventyFive = 75,
	SeventySix = 76,
	SeventySeven = 77,
	SeventyEight = 78,
	SeventyNine = 79,
	Eighty = 80,
	EightyOne = 81,
	EightyTwo = 82,
	EightyThree = 83,
	EightyFour = 84,
	EightyFive = 85,
	EightySix = 86,
	EightySeven = 87,
	EightyEight = 88,
	EightyNine = 89,
	Ninety = 90,
	NinetyOne = 91,
	NinetyTwo = 92,
	NinetyThree = 93,
	NinetyFour = 94,
	NinetyFive = 95,
	NinetySix = 96,
	NinetySeven = 97,
	NinetyEight = 98,
	NinetyNine = 99,
	OneHundred = 100,
	OneHundredOne = 101,
	OneHundredTwo = 102,
	OneHundredThree = 103,
	OneHundredFour = 104,
	OneHundredFive = 105,
	OneHundredSix = 106,
	OneHundredSeven = 107,
	OneHundredEight = 108,
	OneHundredNine = 109,
	OneHundredTen = 110,
	OneHundredEleven = 111,
	OneHundredTwelve = 112,
	OneHundredThirteen = 113,
	OneHundredFourteen = 114,
	OneHundredFifteen = 115,
	OneHundredSixteen = 116,
	OneHundredSeventeen = 117,
	OneHundredEighteen = 118,
	OneHundredNineteen = 119,
	OneHundredTwenty = 120,
	OneHundredTwentyOne = 121,
	OneHundredTwentyTwo = 122,
	OneHundredTwentyThree = 123,
	OneHundredTwentyFour = 124,
	OneHundredTwentyFive = 125,
	OneHundredTwentySix = 126,
	OneHundredTwentySeven = 127,
	OneHundredTwentyEight = 128,
	OneHundredTwentyNine = 129,
	OneHundredThirty = 130,
	OneHundredThirtyOne = 131,
	OneHundredThirtyTwo = 132,
	OneHundredThirtyThree = 133,
	OneHundredThirtyFour = 134,
	OneHundredThirtyFive = 135,
	OneHundredThirtySix = 136,
	OneHundredThirtySeven = 137,
	OneHundredThirtyEight = 138,
	OneHundredThirtyNine = 139,
	OneHundredForty = 140,
	OneHundredFortyOne = 141,
	OneHundredFortyTwo = 142,
	OneHundredFortyThree = 143,
	OneHundredFortyFour = 144,
	OneHundredFortyFive = 145,
	OneHundredFortySix = 146,
	OneHundredFortySeven = 147,
	OneHundredFortyEight = 148,
	OneHundredFortyNine = 149,
	OneHundredFifty = 150,
	OneHundredFiftyOne = 151,
	OneHundredFiftyTwo = 152,
	OneHundredFiftyThree = 153,
	OneHundredFiftyFour = 154,
	OneHundredFiftyFive = 155,
	OneHundredFiftySix = 156,
	OneHundredFiftySeven = 157,
	OneHundredFiftyEight = 158,
	OneHundredFiftyNine = 159,
	OneHundredSixty = 160,
	OneHundredSixtyOne = 161,
	OneHundredSixtyTwo = 162,
	OneHundredSixtyThree = 163,
	OneHundredSixtyFour = 164,
	OneHundredSixtyFive = 165,
	OneHundredSixtySix = 166,
	OneHundredSixtySeven = 167,
	OneHundredSixtyEight = 168,
	OneHundredSixtyNine = 169,
	OneHundredSeventy = 170,
	OneHundredSeventyOne = 171,
	OneHundredSeventyTwo = 172,
	OneHundredSeventyThree = 173,
	OneHundredSeventyFour = 174,
	OneHundredSeventyFive = 175,
	OneHundredSeventySix = 176,
	OneHundredSeventySeven = 177,
	OneHundredSeventyEight = 178,
	OneHundredSeventyNine = 179,
	OneHundredEighty = 180,
	OneHundredEightyOne = 181,
	OneHundredEightyTwo = 182,
	OneHundredEightyThree = 183,
	OneHundredEightyFour = 184,
	OneHundredEightyFive = 185,
	OneHundredEightySix = 186,
	OneHundredEightySeven = 187,
	OneHundredEightyEight = 188,
	OneHundredEightyNine = 189,
	OneHundredNinety = 190,
	OneHundredNinetyOne = 191,
	OneHundredNinetyTwo = 192,
	OneHundredNinetyThree = 193,
	OneHundredNinetyFour = 194,
	OneHundredNinetyFive = 195,
	OneHundredNinetySix = 196,
	OneHundredNinetySeven = 197,
	OneHundredNinetyEight = 198,
	OneHundredNinetyNine = 199,
	TwoHundred = 200,
	TwoHundredOne = 201,
	TwoHundredTwo = 202,
	TwoHundredThree = 203,
	TwoHundredFour = 204,
	TwoHundredFive = 205,
	TwoHundredSix = 206,
	TwoHundredSeven = 207,
	TwoHundredEight = 208,
	TwoHundredNine = 209,
	TwoHundredTen = 210,
	TwoHundredEleven = 211,
	TwoHundredTwelve = 212,
	TwoHundredThirteen = 213,
	TwoHundredFourteen = 214,
	TwoHundredFifteen = 215,
	TwoHundredSixteen = 216,
	TwoHundredSeventeen = 217,
	TwoHundredEighteen = 218,
	TwoHundredNineteen = 219,
	TwoHundredTwenty = 220,
	TwoHundredTwentyOne = 221,
	TwoHundredTwentyTwo = 222,
	TwoHundredTwentyThree = 223,
	TwoHundredTwentyFour = 224,
	TwoHundredTwentyFive = 225,
	TwoHundredTwentySix = 226,
	TwoHundredTwentySeven = 227,
	TwoHundredTwentyEight = 228,
	TwoHundredTwentyNine = 229,
	TwoHundredThirty = 230,
	TwoHundredThirtyOne = 231,
	TwoHundredThirtyTwo = 232,
	TwoHundredThirtyThree = 233,
	TwoHundredThirtyFour = 234,
	TwoHundredThirtyFive = 235,
	TwoHundredThirtySix = 236,
	TwoHundredThirtySeven = 237,
	TwoHundredThirtyEight = 238,
	TwoHundredThirtyNine = 239,
	TwoHundredForty = 240,
	TwoHundredFortyOne = 241,
	TwoHundredFortyTwo = 242,
	TwoHundredFortyThree = 243,
	TwoHundredFortyFour = 244,
	TwoHundredFortyFive = 245,
	TwoHundredFortySix = 246,
	TwoHundredFortySeven = 247,
	TwoHundredFortyEight = 248,
	TwoHundredFortyNine = 249,
	TwoHundredFifty = 250,
	TwoHundredFiftyOne = 251,
	TwoHundredFiftyTwo = 252,
	TwoHundredFiftyThree = 253,
	TwoHundredFiftyFour = 254,
	TwoHundredFiftyFive = 255,
	TwoHundredFiftySix = 256,
	TwoHundredFiftySeven = 257,
	TwoHundredFiftyEight = 258,
	TwoHundredFiftyNine = 259,
	TwoHundredSixty = 260,
	TwoHundredSixtyOne = 261,
	TwoHundredSixtyTwo = 262,
	TwoHundredSixtyThree = 263,
	TwoHundredSixtyFour = 264,
	TwoHundredSixtyFive = 265,
	TwoHundredSixtySix = 266,
	TwoHundredSixtySeven = 267,
	TwoHundredSixtyEight = 268,
	TwoHundredSixtyNine = 269,
	TwoHundredSeventy = 270,
	TwoHundredSeventyOne = 271,
	TwoHundredSeventyTwo = 272,
	TwoHundredSeventyThree = 273,
	TwoHundredSeventyFour = 274,
	TwoHundredSeventyFive = 275,
	TwoHundredSeventySix = 276,
	TwoHundredSeventySeven = 277,
	TwoHundredSeventyEight = 278,
	TwoHundredSeventyNine = 279,
	TwoHundredEighty = 280,
	TwoHundredEightyOne = 281,
	TwoHundredEightyTwo = 282,
	TwoHundredEightyThree = 283,
	TwoHundredEightyFour = 284,
	TwoHundredEightyFive = 285,
	TwoHundredEightySix = 286,
	TwoHundredEightySeven = 287,
	TwoHundredEightyEight = 288,
	TwoHundredEightyNine = 289,
	TwoHundredNinety = 290,
	TwoHundredNinetyOne = 291,
	TwoHundredNinetyTwo = 292,
	TwoHundredNinetyThree = 293,
	TwoHundredNinetyFour = 294,
	TwoHundredNinetyFive = 295,
	TwoHundredNinetySix = 296,
	TwoHundredNinetySeven = 297,
	TwoHundredNinetyEight = 298,
	TwoHundredNinetyNine = 299,
	ThreeHundred = 300,
};

enum class TooSparseForLookupTable {
	Zero = 0,
	Pad1,
	Pad2,
	Pad3,
	Pad4,
	Pad5,
	Pad6,
	// Too big of a gap. Lookup table turned off by default...
	TwoFiftyFour = 14
};

// ... but we can force it on.
enum class ForceEnabledLookupTable {
	Zero = 0,
	Pad1,
	Pad2,
	Pad3,
	Pad4,
	Pad5,
	Pad6,
	TwoFiftyFour = 14
};

} // namespace

CTP_CUSTOM_ENUM_MIN_MAX(PositiveOffset, 1000, 1002)
CTP_CUSTOM_ENUM_MIN_MAX(NegativeOffset, -1002, -1000)
CTP_CUSTOM_ENUM_MIN_MAX(CustomHuge, -100, 300)

template<>
struct ctp::enums::enum_traits<ForceEnabledLookupTable> {
	static constexpr bool force_enable_lookup_table = true;
};


TEST_CASE("enum_reflection index(static)", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(0 == enums::index<Contiguous::NegOne>());
		CTP_CHECK(1 == enums::index<Contiguous::Zero>());
		CTP_CHECK(2 == enums::index<Contiguous::One>());
		CTP_CHECK(3 == enums::index<Contiguous::Two>());
		CTP_CHECK(4 == enums::index<Contiguous::Three>());

		CTP_CHECK(0 == enums::index<NonContiguous::NegOne>());
		CTP_CHECK(1 == enums::index<NonContiguous::One>());
		CTP_CHECK(2 == enums::index<NonContiguous::Three>());

		CTP_CHECK(0 == enums::index<Overlap::Zero>());
		CTP_CHECK(1 == enums::index<Overlap::One>());
		CTP_CHECK(2 == enums::index<Overlap::Two>());
		CTP_CHECK(2 == enums::index<Overlap::TwoNumberTwo>());

		//CTP_CHECK(0 == enums::index<RangeLimited::One>()); static asserts
		CTP_CHECK(0 == enums::index<RangeLimited::Two>());
		CTP_CHECK(1 == enums::index<RangeLimited::Three>());
		//CTP_CHECK(1 == enums::index<RangeLimited::Four>()); static asserts

		CTP_CHECK(0 == enums::index<LookupTableEnum::NegOne>());
		CTP_CHECK(4 == enums::index<LookupTableEnum::Five>());
		CTP_CHECK(6 == enums::index<LookupTableEnum::Ten>());

		CTP_CHECK(0 == enums::index<WideRange::NegThirtyTwo>());
		CTP_CHECK(1 == enums::index<WideRange::NegOne>());
		CTP_CHECK(8 == enums::index<WideRange::Fifty>());
		CTP_CHECK(13 == enums::index<WideRange::OneHundred>());
		CTP_CHECK(16 == enums::index<WideRange::OneTwentyEight>());

		CTP_CHECK(0 == enums::index<PositiveOffset::Thousand>());
		CTP_CHECK(1 == enums::index<PositiveOffset::ThousandOne>());
		CTP_CHECK(2 == enums::index<PositiveOffset::ThousandTwo>());

		CTP_CHECK(2 == enums::index<NegativeOffset::NegThousand>());
		CTP_CHECK(1 == enums::index<NegativeOffset::NegThousandOne>());
		CTP_CHECK(0 == enums::index<NegativeOffset::NegThousandTwo>());

		CTP_CHECK(0 == enums::index<CustomHuge::NegOneHundred>());
		CTP_CHECK(1 == enums::index<CustomHuge::NegNinetyNine>());
		CTP_CHECK(100 == enums::index<CustomHuge::Zero>());
		CTP_CHECK(399 == enums::index<CustomHuge::TwoHundredNinetyNine>());
		CTP_CHECK(400 == enums::index<CustomHuge::ThreeHundred>());
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection index(value)", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(0 == enums::index(Contiguous::NegOne));
		CTP_CHECK(1 == enums::index(Contiguous::Zero));
		CTP_CHECK(2 == enums::index(Contiguous::One));
		CTP_CHECK(3 == enums::index(Contiguous::Two));
		CTP_CHECK(4 == enums::index(Contiguous::Three));

		// Peek implementation detail to ensure test coverage.
		static_assert(!enums::detail::is_lookup_table_enabled_v<NonContiguous>);
		CTP_CHECK(0 == enums::index(NonContiguous::NegOne));
		CTP_CHECK(1 == enums::index(NonContiguous::One));
		CTP_CHECK(2 == enums::index(NonContiguous::Three));

		CTP_CHECK(0 == enums::index(Overlap::Zero));
		CTP_CHECK(1 == enums::index(Overlap::One));
		CTP_CHECK(2 == enums::index(Overlap::Two));
		CTP_CHECK(2 == enums::index(Overlap::TwoNumberTwo));

		//CTP_CHECK(0 == enums::index(RangeLimited::One)); static asserts
		CTP_CHECK(0 == enums::index(RangeLimited::Two));
		CTP_CHECK(1 == enums::index(RangeLimited::Three));
		//CTP_CHECK(1 == enums::index(RangeLimited::Four)); static asserts

		// Peek implementation detail to ensure test coverage.
		static_assert(enums::detail::is_lookup_table_enabled_v<LookupTableEnum>);
		CTP_CHECK(0 == enums::index(LookupTableEnum::NegOne));
		CTP_CHECK(4 == enums::index(LookupTableEnum::Five));
		CTP_CHECK(6 == enums::index(LookupTableEnum::Ten));

		CTP_CHECK(0 == enums::index(WideRange::NegThirtyTwo));
		CTP_CHECK(1 == enums::index(WideRange::NegOne));
		CTP_CHECK(8 == enums::index(WideRange::Fifty));
		CTP_CHECK(13 == enums::index(WideRange::OneHundred));
		CTP_CHECK(16 == enums::index(WideRange::OneTwentyEight));

		CTP_CHECK(0 == enums::index(PositiveOffset::Thousand));
		CTP_CHECK(1 == enums::index(PositiveOffset::ThousandOne));
		CTP_CHECK(2 == enums::index(PositiveOffset::ThousandTwo));

		CTP_CHECK(2 == enums::index(NegativeOffset::NegThousand));
		CTP_CHECK(1 == enums::index(NegativeOffset::NegThousandOne));
		CTP_CHECK(0 == enums::index(NegativeOffset::NegThousandTwo));

		CTP_CHECK(0 == enums::index(CustomHuge::NegOneHundred));
		CTP_CHECK(1 == enums::index(CustomHuge::NegNinetyNine));
		CTP_CHECK(100 == enums::index(CustomHuge::Zero));
		CTP_CHECK(399 == enums::index(CustomHuge::TwoHundredNinetyNine));
		CTP_CHECK(400 == enums::index(CustomHuge::ThreeHundred));

		static_assert(!enums::detail::is_lookup_table_enabled_v<TooSparseForLookupTable>);
		CTP_CHECK(0 == enums::index(TooSparseForLookupTable::Zero));
		CTP_CHECK(7 == enums::index(TooSparseForLookupTable::TwoFiftyFour));

		static_assert(enums::detail::is_lookup_table_enabled_v<ForceEnabledLookupTable>);
		CTP_CHECK(0 == enums::index(ForceEnabledLookupTable::Zero));
		CTP_CHECK(7 == enums::index(ForceEnabledLookupTable::TwoFiftyFour));

		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_reflection try_get_index(value)", "[Tools][enum_reflection]") {
	auto test = [] {
		CTP_CHECK(std::nullopt == enums::try_get_index(RangeLimited::One));
		CTP_CHECK(0 == enums::try_get_index(RangeLimited::Two));
		CTP_CHECK(1 == enums::try_get_index(RangeLimited::Three));
		CTP_CHECK(std::nullopt == enums::try_get_index(RangeLimited::Four));
		return true;
	};

	TEST_CONSTEXPR bool RunConstexpr = test();
	test();
}
