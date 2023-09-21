#include <catch.hpp>
#include <Tools/BitEnum.hpp>
#include <cstdint>

namespace ctp {
namespace {
enum class TestEnum : std::uint32_t {
	none = 0,
	one = 0b0001,
	two = 0b0010,
	three = 0b0100,
	four = 0b1000,
};

static_assert(std::is_constructible_v<BitEnum<TestEnum>, TestEnum>);
static_assert(std::is_constructible_v<BitEnum<TestEnum>, TestEnum, BitEnum<TestEnum>>);
static_assert(std::is_constructible_v<BitEnum<TestEnum>, BitEnum<TestEnum>>);
static_assert(std::is_constructible_v<BitEnum<TestEnum>, BitEnum<TestEnum>, TestEnum>);
static_assert(is_explicitly_convertible_v<BitEnum<TestEnum>, std::uint32_t>);

static_assert(BitEnum{TestEnum::one}.val == TestEnum::one);
static_assert(BitEnum{BitEnum{TestEnum::one}}.val == TestEnum::one);

static_assert((BitEnum{TestEnum::one} | TestEnum::two) == BitEnum{TestEnum::one, TestEnum::two});
static_assert((BitEnum{TestEnum::one} &TestEnum::two) == TestEnum::none);

static_assert(BitEnum<TestEnum>{}.set(TestEnum::one).underlying() == 0b0001);
static_assert(BitEnum<TestEnum>{}.set(TestEnum::two).underlying() == 0b0010);
static_assert(BitEnum<TestEnum>{}.set(TestEnum::two, TestEnum::three).underlying() == 0b0110);
static_assert(BitEnum<TestEnum>{TestEnum::one}.set(TestEnum::three).underlying() == 0b0101);

static_assert(BitEnum<TestEnum>{TestEnum::one}.unset(TestEnum::two).underlying() == 0b0001);
static_assert(BitEnum<TestEnum>{TestEnum::one}.unset(TestEnum::one).underlying() == 0b0000);
static_assert(BitEnum<TestEnum>{TestEnum::one, TestEnum::two, TestEnum::three}.unset(TestEnum::two).underlying() == 0b0101);

static_assert(BitEnum<TestEnum>{TestEnum::one}.flip(TestEnum::two).underlying() == 0b0011);
static_assert(BitEnum<TestEnum>{TestEnum::one}.flip(TestEnum::one).underlying() == 0b0000);
static_assert(BitEnum<TestEnum>{TestEnum::one, TestEnum::two, TestEnum::three}.flip(TestEnum::two, TestEnum::four).underlying() == 0b1101);

static_assert(!BitEnum<TestEnum>{}.any_of(TestEnum::one));
static_assert(BitEnum{TestEnum::one}.any_of(TestEnum::one));
static_assert(BitEnum{TestEnum::one, TestEnum::two}.any_of(TestEnum::one));
static_assert(BitEnum{TestEnum::one}.any_of(TestEnum::one, TestEnum::two));

static_assert(!BitEnum{TestEnum::one}.all_of(TestEnum::one, TestEnum::two));
static_assert(BitEnum{TestEnum::one}.all_of(TestEnum::one));
static_assert(BitEnum{TestEnum::one, TestEnum::two}.all_of(TestEnum::one, TestEnum::two));
static_assert(BitEnum{TestEnum::one, TestEnum::two}.all_of(TestEnum::one));
static_assert(BitEnum{TestEnum::one, TestEnum::two}.all_of(BitEnum{TestEnum::one}, TestEnum::two));
static_assert((~BitEnum<TestEnum>{}).all_of(TestEnum::one, TestEnum::two, TestEnum::three));

static_assert(BitEnum{BitEnum{TestEnum::one} |= TestEnum::two}.all_of(TestEnum::one, TestEnum::two));
static_assert(BitEnum{BitEnum{TestEnum::one} &= TestEnum::two}.none_of(TestEnum::one, TestEnum::two));
static_assert(BitEnum{BitEnum{TestEnum::one, TestEnum::two, TestEnum::three} ^= TestEnum::two}.all_of(TestEnum::one, TestEnum::three));
static_assert(BitEnum{BitEnum{TestEnum::one, BitEnum{TestEnum::two}, TestEnum::three} ^= TestEnum::two}.all_of(TestEnum::one, TestEnum::three));

// Ensure that using an incompatible enum type fails to compile.
enum class Wrong { compilation_failure };
static_assert(!std::is_constructible_v<BitEnum<TestEnum>, TestEnum, Wrong>);

template <typename T> using test_bitor_fail = decltype(std::declval<T>() | Wrong::compilation_failure);
static_assert(!is_detected_v<test_bitor_fail, BitEnum<TestEnum>>);

template <typename T> using test_bitand_fail = decltype(std::declval<T>() & Wrong::compilation_failure);
static_assert(!is_detected_v<test_bitand_fail, BitEnum<TestEnum>>);

template <typename T> using test_bitxor_fail = decltype(std::declval<T>() ^ Wrong::compilation_failure);
static_assert(!is_detected_v<test_bitxor_fail, BitEnum<TestEnum>>);

template <typename T> using test_anyof_fail = decltype(std::declval<T>().anyof(Wrong::compilation_failure));
static_assert(!is_detected_v<test_anyof_fail, BitEnum<TestEnum>>);

template <typename T> using test_allof_fail = decltype(std::declval<T>().allof(Wrong::compilation_failure));
static_assert(!is_detected_v<test_allof_fail, BitEnum<TestEnum>>);

template <typename T> using test_noneof_fail = decltype(std::declval<T>().noneof(Wrong::compilation_failure));
static_assert(!is_detected_v<test_noneof_fail, BitEnum<TestEnum>>);

template <typename T> using test_exactly_fail = decltype(std::declval<T>().exactly(Wrong::compilation_failure));
static_assert(!is_detected_v<test_exactly_fail, BitEnum<TestEnum>>);
} // namespace
} // ctp

TEST_CASE("BitEnum operations.", "[BitEnum]")
{
	using namespace ctp;
	GIVEN("A default-initialized BitEnum.") {
		BitEnum<TestEnum> empty;
		THEN("It defaults to no bits set.") {
			CHECK(empty == TestEnum::none);
			CHECK(empty.val == TestEnum::none);
			CHECK(empty.value() == TestEnum::none);
			CHECK(empty.underlying() == 0);
			CHECK(empty.none_of(TestEnum::one, TestEnum::two, TestEnum::three));
			CHECK(empty.all_of(TestEnum::none));
			CHECK_FALSE(empty.any_of(TestEnum::one, TestEnum::two, TestEnum::three));
			// None is special in that it indicates nothing is set.
			CHECK_FALSE(empty.any_of(TestEnum::none, TestEnum::one, TestEnum::two, TestEnum::three));
			CHECK_FALSE(empty.any_of(TestEnum::none));
			CHECK(empty.exactly(TestEnum::none));
		}
		WHEN("It is negated.") {
			empty = ~empty;
			THEN("It is the maximum value of its underlying type.") {
				CHECK(empty != TestEnum::none);
				CHECK(empty.underlying() == std::numeric_limits<std::uint32_t>::max());
				CHECK(empty.none_of(TestEnum::none));
				CHECK(empty.all_of(TestEnum::one, TestEnum::two, TestEnum::three));
				CHECK(empty.any_of(TestEnum::two));
				CHECK(empty.exactly(TestEnum{std::numeric_limits<std::uint32_t>::max()}));
			}
		}
	}
}
