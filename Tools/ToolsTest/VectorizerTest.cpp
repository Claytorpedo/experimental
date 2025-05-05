#define CTP_ASSERTS_ENABLED 1

#include <catch.hpp>
#include <Tools/Vectorizer.hpp>

#include <Tools/debug.hpp>

#include <algorithm>
#include <functional>
#include <numeric>

using namespace ctp;

namespace {

namespace conversions {
struct Vec2 : Vectorizer<Vec2, unsigned, 2> {
	unsigned a, b;
	using Vectorizer::Vectorizer;
	CTP_MAKE_VECTORIZER_LIST(&Vec2::a, &Vec2::b)
};

struct Vec3 : Vectorizer<Vec3, int, 3> {
	int d[3];
	using Vectorizer::Vectorizer;
	CTP_MAKE_VECTORIZER_LIST(&Vec3::d)
};

// Test constructor constraints.

template <typename V, typename T = typename V::value_type>
using constructible_with_1 = decltype(V{std::declval<T>()});
template <typename V, typename T = typename V::value_type>
using constructible_with_2 = decltype(V{std::declval<T>(), std::declval<T>()});
template <typename V, typename T = typename V::value_type>
using constructible_with_3 = decltype(V{std::declval<T>(), std::declval<T>(), std::declval<T>()});
template <typename V, typename T = typename V::value_type>
using constructible_with_4 = decltype(V{std::declval<T>(), std::declval<T>(), std::declval<T>(), std::declval<T>()});

static_assert(is_detected_v<constructible_with_1, Vec2>);
static_assert(is_detected_v<constructible_with_2, Vec2>);
static_assert(!is_detected_v<constructible_with_3, Vec2>);
static_assert(!is_detected_v<constructible_with_4, Vec2>);

static_assert(is_detected_v<constructible_with_1, Vec3>);
static_assert(!is_detected_v<constructible_with_2, Vec3>);
static_assert(is_detected_v<constructible_with_3, Vec3>);
static_assert(!is_detected_v<constructible_with_4, Vec3>);

} // conversions

template <typename Vec>
constexpr auto basic_vec4_tests() {
	ctpAssert(Vec{0} == Vec{0,0,0,0});
	ctpAssert(Vec{1, 2, 3, 4} == Vec{1, 2, 3, 4});
	ctpAssert(Vec{1, 2, 3, -4} != Vec{1, 2, 3, 4});
	ctpAssert(Vec{0} != Vec{0, 0, 1, 0});

	// Indexing
	{
		const auto test = Vec{4, 3, 2, 1};
		ctpAssert(test[0] == test[0]);
		ctpAssert(test[0] == 4);
		ctpAssert(test[1] == test[1]);
		ctpAssert(test[1] == 3);
		ctpAssert(test[2] == test[2]);
		ctpAssert(test[2] == 2);
		ctpAssert(test[3] == test[3]);
		ctpAssert(test[3] == 1);
	}

	// Index assignment
	{
		auto test = Vec{0, -1, 2, 3};
		test[0] = -44;
		test[2] = 10;
		test[3] = 123;

		ctpAssert(test[0] == -44);
		ctpAssert(test[1] == -1);
		ctpAssert(test[2] == 10);
		ctpAssert(test[3] == 123);
	}

	// Iterating
	{
		const auto test1 = Vec{1, 2, 3, 4};
		const auto test2 = Vec{4, 3, 2, 1};

		ctpAssert(test1.size() == 4);
		ctpAssert(test2.size() == 4);

		auto it = std::rbegin(test2);
		for (const auto& val : test1) {
			ctpAssert(it != std::rend(test2));
			ctpAssert(val == *it);
			++it;
		}
		ctpAssert(it == std::rend(test2));
	}

	// Use in STL algorithms.
	{
		const auto test = Vec{10, -2, 6, -4};
		const auto BiggerThanFiveOrLessThanOne = [](const auto& v)
		{
			return v > 5 || v < 1;
		};

		ctpAssert(std::all_of(test.cbegin(), test.cend(), BiggerThanFiveOrLessThanOne));
		ctpAssert(std::ranges::all_of(test, BiggerThanFiveOrLessThanOne));

		const bool result = std::none_of(test.crbegin(), test.crend(), [](const auto& v)
		{
			return v != -2;
		});
		ctpAssert(!result);
	}


	// Mutating STL algorithm.
	{
		auto test = Vec{3, 5, 2, -1};
		std::sort(test.begin(), test.end());
		ctpAssert(test == Vec{-1, 2, 3, 5});
	}

	return 0;
}

struct VecArray4 : Vectorizer<VecArray4, int, 4> {
	int data[4];
	using Vectorizer::Vectorizer;
	constexpr auto operator<=>(const VecArray4& rhs) const noexcept = default;
	CTP_MAKE_VECTORIZER_LIST(&VecArray4::data)
};

struct VecStdArray4 : Vectorizer<VecStdArray4, int, 4> {
	std::array<int, 4> data;
	using Vectorizer::Vectorizer;
	constexpr auto operator<=>(const VecStdArray4& rhs) const noexcept = default;
	CTP_MAKE_VECTORIZER_LIST(&VecStdArray4::data)
};

struct VecInt4 : Vectorizer<VecInt4, int, 4> {
	int a, b, c, d;
	using Vectorizer::Vectorizer;
	constexpr auto operator<=>(const VecInt4& rhs) const noexcept = default;
	CTP_MAKE_VECTORIZER_LIST(&VecInt4::a, &VecInt4::b, &VecInt4::c, &VecInt4::d)
};

constexpr int conversion_tests() {
	struct Veci : Vectorizer<Veci, int, 5> {
		using Vectorizer::Vectorizer;
		int a, b, c, d, e;
		constexpr auto operator<=>(const Veci&) const noexcept = default;
		CTP_MAKE_VECTORIZER_LIST(&Veci::a, &Veci::b, &Veci::c, &Veci::d, &Veci::e)
	};
	struct Vecu : Vectorizer<Vecu, unsigned, 5> {
		using Vectorizer::Vectorizer;
		unsigned x, y, z, p, q;
		constexpr auto operator<=>(const Vecu&) const noexcept = default;
		CTP_MAKE_VECTORIZER_LIST(&Vecu::x, &Vecu::y, &Vecu::z, &Vecu::p, &Vecu::q)
	};

	// Convert to unsigned and back.
	const auto intVec = Veci{0, -1, 2, 3, 4};
	const auto test = static_cast<Vecu>(intVec);

	ctpAssert(intVec == Veci{0, -1, 2, 3, 4});
	ctpAssert(test == Vecu{0u, (std::numeric_limits<unsigned>::max)(), 2u, 3u, 4u});

	const auto reversed = static_cast<Veci>(test);
	ctpAssert(reversed == intVec);

	return 0;
}

constexpr int operation_tests() {
	struct Vec : Vectorizer<Vec, int, 3> {
		int x, y, z;
		using Vectorizer::Vectorizer;

		constexpr auto operator<=>(const Vec&) const noexcept = default;

		constexpr Vec operator+(int i) const { return reduce_to_vec(std::plus{}, i); }
		constexpr Vec operator+(Vec v) const { return reduce_to_vec(std::plus{}, v); }
		constexpr Vec operator-(int i) const { return reduce_to_vec(std::minus{}, i); }
		constexpr Vec operator-(Vec v) const { return reduce_to_vec(std::minus{}, v); }

		constexpr Vec& operator+=(int i) { return apply([](int& v, int o) { v += o; }, i); }
		constexpr Vec& operator*=(int i) { return apply([](int& v, int o) { v *= o; }, i); }
		constexpr Vec& operator*=(Vec v) { return apply([](int& v, int o) { v *= o; }, v); }

		CTP_MAKE_VECTORIZER_LIST(&Vec::x, &Vec::y, &Vec::z)
	};

	// Addition.
	{
		ctpAssert(Vec{0} + 0 == Vec{0});
		ctpAssert(Vec{0} + Vec{0} == Vec{0});

		ctpAssert(Vec{1} + 2 == Vec{3});
		ctpAssert(Vec{1} + Vec{1} == Vec{2});

		ctpAssert(Vec{0, 1, -2} + 10 == Vec{10, 11, 8});
		ctpAssert(Vec{0, 1, -2} + Vec{-1, 2, 3} == Vec{-1, 3, 1});
	}
	// Subtraction.
	{
		ctpAssert(Vec{0} - 0 == Vec{0});
		ctpAssert(Vec{0} - Vec{0} == Vec{0});

		ctpAssert(Vec{1} - 2 == Vec{-1});
		ctpAssert(Vec{1} - Vec{1} == Vec{0});

		ctpAssert(Vec{0, 1, -2} - 10 == Vec{-10, -9, -12});
		ctpAssert(Vec{0, 1, -2} - Vec{-1, 2, 3} == Vec{1, -1, -5});
	}
	// Addition assignment.
	{
		auto test = Vec{0};
		ctpAssert((test += 0) == Vec{0});
		ctpAssert(test == Vec{0});


		test = Vec{1};
		ctpAssert((test += 2) == Vec{3});
		ctpAssert(test == Vec{3});

		test = Vec{1, 2, 3};
		ctpAssert((test += 4) == Vec{5, 6, 7});
		ctpAssert(test == Vec{5, 6, 7});
	}
	// Multiplication assignment.
	{
		{
			auto test = Vec{0};
			ctpAssert((test *= 0) == Vec{0});
			ctpAssert(test == Vec{0});


			test = Vec{2};
			ctpAssert((test *= 4) == Vec{8});
			ctpAssert(test == Vec{8});

			test = Vec{1, 2, 3};
			ctpAssert((test *= 4) == Vec{4, 8, 12});
			ctpAssert(test == Vec{4, 8, 12});
		}
		{
			auto test = Vec{0};
			ctpAssert((test *= Vec{1, 0, 1}) == Vec{0});
			ctpAssert(test == Vec{0});


			test = Vec{2};
			ctpAssert((test *= Vec{2, 4, 8}) == Vec{4, 8, 16});
			ctpAssert(test == Vec{4, 8, 16});

			test = Vec{1, 2, 3};
			ctpAssert((test *= Vec{4, 3, -2}) == Vec{4, 6, -6});
			ctpAssert(test == Vec{4, 6, -6});
		}
	}

	return 0;
}

constexpr int RunTests() {
	basic_vec4_tests<VecInt4>();
	basic_vec4_tests<VecArray4>();
	basic_vec4_tests<VecStdArray4>();

	conversion_tests();

	operation_tests();

	return 0;
}

constexpr int RunConstexprTests = RunTests();

} // namespace

TEST_CASE("Run-time vectorizer tests", "[Tools][Vectorizer]") {
	RunTests();
}
