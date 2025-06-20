#include <catch.hpp>

#include <Tools/test/catch_test_helpers.hpp>

#include <Tools/array.hpp>

#include <string>

using namespace ctp;
using namespace std::literals;

static_assert(std::is_default_constructible_v<ctp::array<int, 5>>);

TEST_CASE("Array construction and basic ops", "[Tools][array]") {
	auto test = [] {
		{
			// default construction
			ctp::array<int, 5> arr;
			static_assert(arr.size() == 5);
			static_assert(arr.max_size() == 5);
			static_assert(arr.empty() == false);
			static_assert(arr.is_empty() == false);

			for (int i = 0; i < 5; ++i)
				arr[i] = i * 2;

			int iter = 0;
			for (int i : arr)
				CTP_CHECK(i == iter++ * 2);

			arr.fill(11);
			for (int i = 0; i < 5; ++i)
				CTP_CHECK(arr.data()[i] == 11);

		}

		{
			// deduction guide
			auto arr = ctp::array{0,1,2,3,4,5};
			static_assert(arr.size() == 6);
			for (std::size_t i = 0; i < arr.size(); ++i)
				CTP_CHECK(arr[i] == i);
		}

		{
			// explicit size
			auto arr = ctp::array<int, 6>{0,1,2,3,4,5};
			static_assert(arr.size() == 6);
			for (std::size_t i = 0; i < arr.size(); ++i)
				CTP_CHECK(arr[i] == i);
		}

		{
			// all with one value
			auto arr = ctp::array<int, 7>{5};
			static_assert(arr.size() == 7);
			for (std::size_t i = 0; i < arr.size(); ++i)
				CTP_CHECK(arr[i] == 5);
		}

		{
			// extra size
			auto arr = ctp::array<int, 7>{5};
			static_assert(arr.size() == 7);
			for (std::size_t i = 0; i < arr.size(); ++i)
				CTP_CHECK(arr[i] == 5);
		}

		{
			// copy
			auto arr = ctp::array<int, 5>{0,1,2,3,4};
			ctp::array b{arr};
			static_assert(b.size() == 5);
			for (int i = 0; i < 5; ++i) {
				CTP_CHECK(b[i] == i);
				CTP_CHECK(b[i] == arr[i]);
			}
		}

		{
			// move
			auto arr = ctp::array<int, 5>{0,1,2,3,4};
			ctp::array b{std::move(arr)};
			static_assert(b.size() == 5);
			for (int i = 0; i < 5; ++i) {
				CTP_CHECK(b[i] == i);
				// trivail move doesn't reset elements
				CTP_CHECK(b[i] == arr[i]);
			}
		}

		{
			// move non trivial
			auto arr = ctp::array<std::string, 5>{"hi"};
			ctp::array b{std::move(arr)};
			static_assert(b.size() == 5);
			for (std::size_t i = 0; i < b.size(); ++i) {
				CTP_CHECK(b[i] == "hi"sv);
				CTP_CHECK(arr[i].empty());
			}
		}

		{
			// swap
			auto a = ctp::array<std::string, 5>{"hi"};
			auto b = ctp::array<std::string, 5>{"bye"};
			a.swap(b);
			for (std::size_t i = 0; i < a.size(); ++i) {
				CTP_CHECK(a[i] == "bye"sv);
				CTP_CHECK(b[i] == "hi"sv);
			}
			std::swap(a, b);
			for (std::size_t i = 0; i < a.size(); ++i) {
				CTP_CHECK(a[i] == "hi"sv);
				CTP_CHECK(b[i] == "bye"sv);
			}
		}

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}


TEST_CASE("Array copy/move", "[Tools][array]") {
	auto test = [] {
		auto original = ctp::array<std::string, 3>{"zero", "one", "two"};
		ctp::array<std::string, 3> copy{original};
		ctp::array<std::string, 3> moved{std::move(original)};
		CTP_CHECK(original[0].empty());
		CTP_CHECK(original[1].empty());
		CTP_CHECK(original[2].empty());

		CTP_CHECK(copy[0] == "zero"sv);
		CTP_CHECK(copy[1] == "one"sv);
		CTP_CHECK(copy[2] == "two"sv);

		CTP_CHECK(moved[0] == "zero"sv);
		CTP_CHECK(moved[1] == "one"sv);
		CTP_CHECK(moved[2] == "two"sv);


		ctp::array<std::string, 3> copyAssign;
		copyAssign = copy;

		CTP_CHECK(copy[0] == "zero"sv);
		CTP_CHECK(copy[1] == "one"sv);
		CTP_CHECK(copy[2] == "two"sv);
		CTP_CHECK(copyAssign[0] == "zero"sv);
		CTP_CHECK(copyAssign[1] == "one"sv);
		CTP_CHECK(copyAssign[2] == "two"sv);

		ctp::array<std::string, 3> moveAssign;
		moveAssign = std::move(moved);

		CTP_CHECK(moved[0].empty());
		CTP_CHECK(moved[1].empty());
		CTP_CHECK(moved[2].empty());
		CTP_CHECK(moveAssign[0] == "zero"sv);
		CTP_CHECK(moveAssign[1] == "one"sv);
		CTP_CHECK(moveAssign[2] == "two"sv);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Array iterators", "[Tools][array]") {
	auto test = [] {
		ctp::array arr = {1,2,3};
		{
			int i = 0;
			for (int e : arr)
				CTP_CHECK(++i == e);
		}

		CTP_CHECK(*arr.begin() == 1);
		CTP_CHECK(*arr.cbegin() == 1);
		CTP_CHECK(*(--arr.end()) == 3);
		CTP_CHECK(*(--arr.cend()) == 3);
		CTP_CHECK(*arr.last() == 3);

		CTP_CHECK(arr.front() == 1);
		CTP_CHECK(arr.back() == 3);

		const ctp::array carr = {1,2,3,4,5};

		CTP_CHECK(*carr.begin() == 1);
		CTP_CHECK(*carr.cbegin() == 1);
		CTP_CHECK(*(--carr.end()) == 5);
		CTP_CHECK(*(--carr.cend()) == 5);
		CTP_CHECK(*carr.last() == 5);

		CTP_CHECK(carr.front() == 1);
		CTP_CHECK(carr.back() == 5);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Array.at", "[Tools][array]") {
	auto test = [] {
		ctp::array arr = {1,2,3};

		CTP_CHECK(arr.at(0) == 1);
		CTP_CHECK(arr.at(2) == 3);
		arr.at(0) = 4;
		CTP_CHECK(arr.at(0) == 4);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();

#if CTP_USE_EXCEPTIONS
	ctp::array arr = {1,2,3};
	CHECK_NOTHROW(arr.at(0));
	CHECK_THROWS(arr.at(3));
	CHECK_THROWS(arr.at(4));
#endif
}

TEST_CASE("Array tuple-like operations", "[Tools][array]") {
	auto test = [] {
		ctp::array arr = {1,2,3,4};

		static_assert(std::tuple_size_v<decltype(arr)> == 4);
		static_assert(std::is_same_v<std::tuple_element_t<0, decltype(arr)>, int>);
		static_assert(std::is_same_v<std::tuple_element_t<3, decltype(arr)>, int>);

		CTP_CHECK(std::get<0>(arr) == 1);
		CTP_CHECK(std::get<1>(arr) == 2);
		CTP_CHECK(std::get<2>(arr) == 3);
		CTP_CHECK(std::get<3>(arr) == 4);

		std::get<0>(arr) = 5;
		CTP_CHECK(std::get<0>(arr) == 5);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Array non-trivial construct", "[Tools][array]") {
	struct nontrivial {
		int i;
		constexpr nontrivial() noexcept { i = 18; }
	};

	auto test = [] {
		ctp::array<nontrivial, 4> arr;
		for (const auto& e : arr)
			CTP_CHECK(e.i == 18);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Array non-default construct", "[Tools][array]") {
	// Given a type that can't be default constructed.
	struct nondefault {
		int i;
		constexpr nondefault(int in) noexcept { i = in; }
	};

	// The array also becomes not default constructible.
	static_assert(!std::is_default_constructible_v<ctp::array<nondefault, 4>>);

	auto test = [] {
		// but if provided a default value, it's copied like normal
		const nondefault val{18};
		ctp::array<nondefault, 4> arr{val};
		for (const auto& e : arr)
			CTP_CHECK(e.i == 18);

		return true;
	};

	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}

TEST_CASE("Array comparison", "[Tools][array]") {
	auto test = [] {
		ctp::array a = {1,2,3,4};
		ctp::array b = {1,2,3,4};

		CTP_CHECK(a == b);
		CTP_CHECK(a <= b);
		CTP_CHECK_FALSE(a < b);
		CTP_CHECK_FALSE(a > b);
		CTP_CHECK(a >= b);
		CTP_CHECK_FALSE(a != b);

		b[3] = 5;

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
