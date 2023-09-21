#define BENCHMARK_STATIC_DEFINE
#include <benchmark/benchmark.h>

#include <Tools/ranges/erase.hpp>

#include <string>
#include <vector>

namespace {

class EraseFixture : public benchmark::Fixture {
protected:
	std::vector<int> vec_;
public:
	void SetUp(benchmark::State& state) override {
		for (std::int64_t i = 0; i < state.range(0); ++i) {
			vec_.push_back(i % 10);
			vec_.push_back(i % 77);
		}
	}
};

class EraseFixtureString : public benchmark::Fixture {
protected:
	std::vector<std::string> vec_;
public:
	constexpr static std::string_view Alphabet = "abcdefghijklmnopqrstuvwxyz";
	void SetUp(benchmark::State& state) override {
		for (std::int64_t i = 0; i < state.range(0); ++i) {
			std::string str{Alphabet};
			std::rotate(str.begin(), str.begin() + i % 26, str.end());
			vec_.push_back(std::move(str));
		}
	}
};

struct LargeClass {
	std::string str;
	char buff[128]{0};
};
class EraseFixtureLargeClass : public benchmark::Fixture {
protected:
	std::vector<LargeClass> vec_;
public:
	constexpr static std::string_view Alphabet = "abcdefghijklmnopqrstuvwxyz";
	void SetUp(benchmark::State& state) override {
		for (std::int64_t i = 0; i < state.range(0); ++i) {
			std::string str{Alphabet};
			std::rotate(str.begin(), str.begin() + i % 26, str.end());
			vec_.push_back(LargeClass{std::move(str)});
		}
	}
};

} // namespace

//#define DO_RANGE() Range(128, 2 << 12)
#define DO_RANGE() Range(8, 128)

// ints

BENCHMARK_DEFINE_F(EraseFixture, StdErase_EraseInt)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		auto numErased = vec.erase(std::remove(vec.begin(), vec.end(), 4), vec.end());

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixture, StdErase_EraseInt)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixture, CTPStableErase_EraseInt)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		auto numErased = ctp::stable_erase(vec, 4);

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixture, CTPStableErase_EraseInt)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixture, CTPErase_EraseInt)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		auto numErased = ctp::unstable_erase(vec, 4);

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixture, CTPErase_EraseInt)->DO_RANGE();

// strings

BENCHMARK_DEFINE_F(EraseFixtureString, StdErase_EraseStr)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		[[maybe_unused]] auto numErased = vec.erase(std::remove(vec.begin(), vec.end(), EraseFixtureString::Alphabet), vec.end());

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixtureString, StdErase_EraseStr)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixtureString, CTPStableErase_EraseStr)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		[[maybe_unused]] auto numErased = ctp::stable_erase(vec, EraseFixtureString::Alphabet);

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixtureString, CTPStableErase_EraseStr)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixtureString, CTPErase_EraseStr)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		[[maybe_unused]] auto numErased = ctp::unstable_erase(vec, EraseFixtureString::Alphabet);

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixtureString, CTPErase_EraseStr)->DO_RANGE();

// Large classes

BENCHMARK_DEFINE_F(EraseFixtureLargeClass, StdErase_EraseLargeClass)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		[[maybe_unused]] auto numErased = vec.erase(std::remove_if(vec.begin(), vec.end(), [](const auto& t) { return t.str == EraseFixtureString::Alphabet; }), vec.end());

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixtureLargeClass, StdErase_EraseLargeClass)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixtureLargeClass, CTPStableErase_EraseLargeClass)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		[[maybe_unused]] auto numErased = ctp::stable_erase(vec, EraseFixtureString::Alphabet, &LargeClass::str);

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixtureLargeClass, CTPStableErase_EraseLargeClass)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixtureLargeClass, CTPErase_EraseLargeClass)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;

		[[maybe_unused]] auto numErased = ctp::unstable_erase(vec, EraseFixtureString::Alphabet, &LargeClass::str);

		benchmark::DoNotOptimize(numErased);
	}
}
BENCHMARK_REGISTER_F(EraseFixtureLargeClass, CTPErase_EraseLargeClass)->DO_RANGE();

// Measure the vector copy.

BENCHMARK_DEFINE_F(EraseFixture, BaseTime_EraseInt)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;
		benchmark::DoNotOptimize(vec.data());
	}
}
BENCHMARK_REGISTER_F(EraseFixture, BaseTime_EraseInt)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixtureString, BaseTime_EraseStr)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;
		benchmark::DoNotOptimize(vec.data());
	}
}
BENCHMARK_REGISTER_F(EraseFixtureString, BaseTime_EraseStr)->DO_RANGE();

BENCHMARK_DEFINE_F(EraseFixtureLargeClass, BaseTime_EraseLargeClass)(benchmark::State& state) {
	for (auto _ : state) {
		auto vec = vec_;
		benchmark::DoNotOptimize(vec.data());
	}
}
BENCHMARK_REGISTER_F(EraseFixtureLargeClass, BaseTime_EraseLargeClass)->DO_RANGE();
