#include <catch.hpp>

#include <Tools/test/catch_test_helpers.hpp>

#include <Tools/enum_map.hpp>

#include <string>

using namespace ctp;
using namespace std::literals;

namespace {

enum class OneVal {
	One,
};

enum class E1 {
	One,
	Two,
	Three
};

enum class E2 {
	One = 1,
	Five = 5,
	Ten = 10,
	Twentry = 20
};

static_assert(!std::is_default_constructible_v<enum_map<E1, int>>);
static_assert(std::is_constructible_v<enum_map<E1, int>, int>);
static_assert(!std::is_constructible_v<enum_map<E1, int>, int, int>);
static_assert(std::is_constructible_v<enum_map<E1, int>, int, int, int>);
static_assert(!std::is_constructible_v<enum_map<E1, int>, int, int, int, int>);

static_assert(!std::is_default_constructible_v<enum_map<E2, int>>);
static_assert(std::is_constructible_v<enum_map<E2, int>, int>);
static_assert(!std::is_constructible_v<enum_map<E2, int>, int, int>);
static_assert(!std::is_constructible_v<enum_map<E2, int>, int, int, int>);
static_assert(std::is_constructible_v<enum_map<E2, int>, int, int, int, int>);
static_assert(!std::is_constructible_v<enum_map<E2, int>, int, int, int, int, int>);

}

TEST_CASE("Enum map basic ops", "[Tools][enum_map]") {
	auto test = [] {
		const enum_map<OneVal, int> oneval{1};
		CTP_CHECK(oneval[OneVal::One] == 1);

		enum_map<E1, int> map(1, 2, 3);
		CTP_CHECK(map[E1::One] == 1);
		CTP_CHECK(map[E1::Two] == 2);
		CTP_CHECK(map[E1::Three] == 3);

		map[E1::One] = 3;
		map[E1::Two] = 4;
		map[E1::Three] = 1;

		CTP_CHECK(map[E1::One] == 3);
		CTP_CHECK(map[E1::Two] == 4);
		CTP_CHECK(map[E1::Three] == 1);

		const enum_map<E1, int> copy{map};
		const enum_map<E1, int> mv{std::move(map)};

		CTP_CHECK(copy[E1::One] == 3);
		CTP_CHECK(copy[E1::Two] == 4);
		CTP_CHECK(copy[E1::Three] == 1);

		CTP_CHECK(mv[E1::One] == 3);
		CTP_CHECK(mv[E1::Two] == 4);
		CTP_CHECK(mv[E1::Three] == 1);

		// One arg gets copied.
		enum_map<E1, int> copy_assign(7);
		CTP_CHECK(copy_assign[E1::One] == 7);
		CTP_CHECK(copy_assign[E1::Two] == 7);
		CTP_CHECK(copy_assign[E1::Three] == 7);

		// Copy assignment
		copy_assign = copy;
		CTP_CHECK(copy_assign[E1::One] == 3);
		CTP_CHECK(copy_assign[E1::Two] == 4);
		CTP_CHECK(copy_assign[E1::Three] == 1);

		// Move assignment
		enum_map<E1, int> move_assign(0, 0, 0);
		move_assign = std::move(copy_assign);
		CTP_CHECK(move_assign[E1::One] == 3);
		CTP_CHECK(move_assign[E1::Two] == 4);
		CTP_CHECK(move_assign[E1::Three] == 1);
		CTP_CHECK(copy_assign[E1::One] == 3);
		CTP_CHECK(copy_assign[E1::Two] == 4);
		CTP_CHECK(copy_assign[E1::Three] == 1);

		// Copy construction
		enum_map<E1, int> copy_constructed(copy);
		CTP_CHECK(copy_constructed[E1::One] == 3);
		CTP_CHECK(copy_constructed[E1::Two] == 4);
		CTP_CHECK(copy_constructed[E1::Three] == 1);

		// Move construction
		enum_map<E1, int> move_constructed(std::move(move_assign));
		CTP_CHECK(move_constructed[E1::One] == 3);
		CTP_CHECK(move_constructed[E1::Two] == 4);
		CTP_CHECK(move_constructed[E1::Three] == 1);
		CTP_CHECK(move_assign[E1::One] == 3);
		CTP_CHECK(move_assign[E1::Two] == 4);
		CTP_CHECK(move_assign[E1::Three] == 1);

		{
			// swap
			auto a = enum_map<E1, int>{1,2,3};
			auto b = enum_map<E1, int>{4,5,6};
			a.swap(b);
			for (E1 e : a.keys()) {
				CTP_CHECK(a[e] == std::to_underlying(e) + 4);
				CTP_CHECK(b[e] == std::to_underlying(e) + 1);
			}
			std::swap(a, b);
			for (E1 e : a.keys()) {
				CTP_CHECK(a[e] == std::to_underlying(e) + 1);
				CTP_CHECK(b[e] == std::to_underlying(e) + 4);
			}
		}

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Enum map basic ops non-trivial", "[Tools][enum_map]") {
	auto test = [] {
		const enum_map<OneVal, std::string> oneval{"one"};
		CTP_CHECK(oneval[OneVal::One] == "one"sv);

		enum_map<E1, std::string> map("one", "two", "three");
		CTP_CHECK(map[E1::One] == "one");
		CTP_CHECK(map[E1::Two] == "two");
		CTP_CHECK(map[E1::Three] == "three");

		map[E1::One] = "three";
		map[E1::Two] = "fourextraextraextralong";
		map[E1::Three] = "one";

		CTP_CHECK(map[E1::One] == "three"sv);
		CTP_CHECK(map[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(map[E1::Three] == "one"sv);

		const enum_map<E1, std::string> copy{map};
		const enum_map<E1, std::string> mv{std::move(map)};

		CTP_CHECK(copy[E1::One] == "three"sv);
		CTP_CHECK(copy[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(copy[E1::Three] == "one"sv);

		CTP_CHECK(mv[E1::One] == "three"sv);
		CTP_CHECK(mv[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(mv[E1::Three] == "one"sv);

		// Copy assignment
		enum_map<E1, std::string> copy_assign("a", "b", "c");
		copy_assign = copy;
		CTP_CHECK(copy_assign[E1::One] == "three"sv);
		CTP_CHECK(copy_assign[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(copy_assign[E1::Three] == "one"sv);

		// Move assignment
		enum_map<E1, std::string> move_assign("x", "y", "z");
		move_assign = std::move(copy_assign);
		CTP_CHECK(move_assign[E1::One] == "three"sv);
		CTP_CHECK(move_assign[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(move_assign[E1::Three] == "one"sv);
		CTP_CHECK(copy_assign[E1::One].empty());
		CTP_CHECK(copy_assign[E1::Two].empty());
		CTP_CHECK(copy_assign[E1::Three].empty());

		// Copy construction
		enum_map<E1, std::string> copy_constructed(copy);
		CTP_CHECK(copy_constructed[E1::One] == "three"sv);
		CTP_CHECK(copy_constructed[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(copy_constructed[E1::Three] == "one"sv);

		// Move construction
		enum_map<E1, std::string> move_constructed(std::move(move_assign));
		CTP_CHECK(move_constructed[E1::One] == "three"sv);
		CTP_CHECK(move_constructed[E1::Two] == "fourextraextraextralong"sv);
		CTP_CHECK(move_constructed[E1::Three] == "one"sv);
		CTP_CHECK(move_assign[E1::One].empty());
		CTP_CHECK(move_assign[E1::Two].empty());
		CTP_CHECK(move_assign[E1::Three].empty());

		{
			// swap
			auto a = enum_map<E1, std::string>{"hi"};
			auto b = enum_map<E1, std::string>{"bye"};
			a.swap(b);
			for (E1 e : a.keys()) {
				CTP_CHECK(a[e] == "bye"sv);
				CTP_CHECK(b[e] == "hi"sv);
			}
			std::swap(a, b);
			for (E1 e : a.keys()) {
				CTP_CHECK(a[e] == "hi"sv);
				CTP_CHECK(b[e] == "bye"sv);
			}
		}

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("indexible_enum_map", "[Tools][enum_map]") {
	auto test = [] {
		const indexible_enum_map<OneVal, int> oneval(1);
		CTP_CHECK(oneval[OneVal::One] == 1);
		CTP_CHECK(oneval[0] == 1);

		indexible_enum_map<E1, int> e(1, 2, 3);
		CTP_CHECK(e[E1::One] == 1);
		CTP_CHECK(e[E1::Two] == 2);
		CTP_CHECK(e[E1::Three] == 3);

		CTP_CHECK(e[0] == 1);
		CTP_CHECK(e[1] == 2);
		CTP_CHECK(e[2] == 3);

		e[0] = 3;
		e[1] = 4;
		e[2] = 1;

		CTP_CHECK(e[E1::One] == 3);
		CTP_CHECK(e[E1::Two] == 4);
		CTP_CHECK(e[E1::Three] == 1);

		const indexible_enum_map<E1, int> copy{e};
		const indexible_enum_map<E1, int> mv{std::move(e)};

		CTP_CHECK(copy[0] == 3);
		CTP_CHECK(copy[E1::Two] == 4);
		CTP_CHECK(copy[2] == 1);

		CTP_CHECK(mv[E1::One] == 3);
		CTP_CHECK(mv[1] == 4);
		CTP_CHECK(mv[E1::Three] == 1);
		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}


TEST_CASE("Enum map iterators", "[Tools][enum_map]") {
	auto test = [] {
		enum_map<E1, int> map = {1,2,3};
		{
			int i = 0;
			for (int e : map.values())
				CTP_CHECK(++i == e);

			i = 0;
			for (E1 e : map.keys())
				CTP_CHECK(enums::values<E1>()[i++] == e);

			i = 0;
			for (const auto& [key, value] : map) {
				CTP_CHECK(enums::values<E1>()[i] == key);
				CTP_CHECK(++i == value);
			}
		}

		CTP_CHECK(*map.begin() == std::pair{E1::One, 1});
		CTP_CHECK(*map.cbegin() == std::pair{E1::One, 1});
		CTP_CHECK(*(--map.end()) == std::pair{E1::Three, 3});
		CTP_CHECK(*(--map.cend()) == std::pair{E1::Three, 3});
		CTP_CHECK(*map.last() == std::pair{E1::Three, 3});

		const enum_map<E1, int> cmap = {1,2,3};

		CTP_CHECK(*cmap.begin() == std::pair{E1::One, 1});
		CTP_CHECK(*cmap.cbegin() == std::pair{E1::One, 1});
		CTP_CHECK(*(--cmap.end()) == std::pair{E1::Three, 3});
		CTP_CHECK(*(--cmap.cend()) == std::pair{E1::Three, 3});
		CTP_CHECK(*cmap.last() == std::pair{E1::Three, 3});

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_map.at", "[Tools][enum_map]") {
	auto test = [] {
		enum_map<E1, int> map = {1,2,3};

		CTP_CHECK(map.at(E1::One) == 1);
		CTP_CHECK(map.at(E1::Three) == 3);
		map.at(E1::Three) = 4;
		CTP_CHECK(map.at(E1::Three) == 4);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("enum_map.fill", "[Tools][enum_map]") {
	auto test = [] {
		enum_map<E1, int> map = {1,2,3};

		map.fill(7);
		for (auto [key, val] : map)
			CTP_CHECK(val == 7);

		map.fill(0);
		for (int e : map.values())
			CTP_CHECK(e == 0);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Enum map non-default construct", "[Tools][enum_map]") {
	// Given a type that can't be default constructed.
	struct nondefault {
		int i;
		constexpr nondefault(int in) noexcept { i = in; }
	};

	// as usual, the map is not default constructible
	static_assert(!std::is_default_constructible_v<enum_map<E1, nondefault>>);

	auto test = [] {
		// but if provided a default value, it's copied like normal
		const nondefault val{18};
		enum_map<E1, nondefault> map(val);
		for (const auto& [key, value] : map)
			CTP_CHECK(value.i == 18);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Enum map comparison", "[Tools][enum_map]") {
	auto test = [] {
		const enum_map<E1, int> a = {1,2,3};
		enum_map<E1, int> b = {1,2,3};

		CTP_CHECK(a == b);
		CTP_CHECK(a <= b);
		CTP_CHECK_FALSE(a < b);
		CTP_CHECK_FALSE(a > b);
		CTP_CHECK(a >= b);
		CTP_CHECK_FALSE(a != b);

		b[E1::Three] = 4;

		CTP_CHECK_FALSE(a == b);
		CTP_CHECK(a <= b);
		CTP_CHECK(a < b);
		CTP_CHECK_FALSE(a > b);
		CTP_CHECK_FALSE(a >= b);
		CTP_CHECK(a != b);

		CTP_CHECK_FALSE(b == a);
		CTP_CHECK_FALSE(b <= a);
		CTP_CHECK_FALSE(b < a);
		CTP_CHECK(b > a);
		CTP_CHECK(b >= a);
		CTP_CHECK(b != a);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Enum map with an enum that has gaps", "[Tools][enum_map]") {
	auto test = [] {
		enum_map<E2, int> map = {1,2,3,4};

		int i = 0;
		for (auto [key, val] : map)
			CTP_CHECK(val == ++i);

		map.fill(7);
		for (auto [key, val] : map)
			CTP_CHECK(val == 7);

		enum_map<E2, int> map2{8};
		map = map2;
		for (auto [key, val] : map)
			CTP_CHECK(val == 8);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}
