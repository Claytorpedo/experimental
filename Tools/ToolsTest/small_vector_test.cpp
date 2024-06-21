#include <catch.hpp>
#include <Tools/small_vector.hpp>

#include "small_storage_test_helpers.hpp"

#include <algorithm>

using namespace ctp;

TEST_CASE("static_vector", "[Tools][static_vector]") {
	static_vector<int, 10> vec;
	CHECK(vec.size() == 0);
	CHECK(vec.empty());
	CHECK(vec.is_empty());

	vec.resize(10);
	CHECK(vec.size() == 10);

	std::iota(vec.begin(), vec.end(), 0);

	for (std::size_t i = 0; i < vec.size(); ++i)
		CHECK(vec[i] == i);

	vec.reset();
	CHECK(vec.size() == 0);
	CHECK(vec.empty());
	CHECK(vec.is_empty());
}

TEST_CASE("small_vector", "[Tools][small_vector]") {
	GIVEN("A small_vector of ints with local space for 10.") {
		small_vector<int, 10> vec;
		CHECK(vec.size() == 0);
		CHECK(vec.empty());
		CHECK(vec.is_empty());

		vec.resize(10);
		CHECK(vec.size() == 10);

		std::iota(vec.begin(), vec.end(), 0);

		for (std::size_t i = 0; i < vec.size(); ++i)
			CHECK(vec[i] == i);

		vec.reset();
		CHECK(vec.size() == 0);
		CHECK(vec.empty());
		CHECK(vec.is_empty());

		for (int i = 0; i < 20; ++i)
			vec.push_back(i);

		CHECK(vec.size() == 20);
		for (std::size_t i = 0; i < vec.size(); ++i)
			CHECK(vec[i] == i);
	}

	GIVEN("An allocator aware small_vector of allocating_objects with local space for 2.") {
		allocator_stats stats;
		{
			small_vector<allocating_object<tracker_alloc<int>>, 2, tracker_alloc<int>> vec{&stats};

			vec.emplace_back();

			CHECK(stats.allocations == 1);

			vec.emplace_back();

			CHECK(stats.allocations == 2);

			vec.emplace_back(); // Switch to large mode.

			CHECK(stats.allocations == 4);

			const auto capacity = vec.capacity();
			vec.emplace_back();

			const bool didResize = capacity != vec.capacity();
			CHECK(stats.allocations == (didResize ? 6 : 5));
			CHECK(stats.deletions == (didResize ? 1 : 0));
		}
		CHECK(stats.deletions == stats.allocations);
	}
}
