#include <catch.hpp>
#include <Tools/ranges/remove.hpp>

#include <forward_list>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace {
struct aggregate {
	int num = 0;
	std::string id;

	friend bool operator==(const aggregate& lhs, const aggregate& rhs) noexcept
	{
		return lhs.num == rhs.num && lhs.id == rhs.id;
	}
};
} // namespace

TEST_CASE("stable_remove") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto [ret, last] = ctp::stable_remove(vec, 1);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {2, 3, 4, 5};
			CHECK(result == expected);
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto [ret, last] = ctp::stable_remove(vec, 3);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 4, 5};
			CHECK(result == expected);
		}

		THEN("Removing the last element preserves order.")
		{
			const auto [ret, last] = ctp::stable_remove(vec, 5);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4};
			CHECK(result == expected);
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto [ret, last] = ctp::stable_remove(dupes, 9);
			const std::vector<int> result{dupes.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(result == expected);
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto [ret, last] = ctp::stable_remove(vec, 9);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(result == expected);
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto [ret, last] = ctp::stable_remove(vec, aggregate{1, "one"});
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(result == expected);
		}

		THEN("We can use a projection on the num field.") {
			const auto [ret, last] = ctp::stable_remove(vec, 2, &aggregate::num);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(result == expected);
		}
		THEN("We can use a projection on the id field.") {
			const auto [ret, last] = ctp::stable_remove(vec, "three", &aggregate::id);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(result == expected);
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto [ret, last] = ctp::stable_remove(dupes, "nine", &aggregate::id);
			const std::vector<aggregate> result{dupes.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(result == expected);
		}
	}
}

TEST_CASE("stable_remove_if") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto [ret, last] = ctp::stable_remove_if(vec, [](int i) { return i == 1; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {2, 3, 4, 5};
			CHECK(result == expected);
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto [ret, last] = ctp::stable_remove_if(vec, [](int i) { return i == 3; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 4, 5};
			CHECK(result == expected);
		}

		THEN("Removing the last element preserves order.")
		{
			const auto [ret, last] = ctp::stable_remove_if(vec, [](int i) { return i == 5; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4};
			CHECK(result == expected);
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto [ret, last] = ctp::stable_remove_if(dupes, [](int i) { return i == 9; });
			const std::vector<int> result{dupes.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(result == expected);
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto [ret, last] = ctp::stable_remove_if(vec, [](int i) { return i == 9; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(result == expected);
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto [ret, last] = ctp::stable_remove_if(vec, [](const auto& a) { return a == aggregate{1, "one"}; } );
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(result == expected);
		}

		THEN("We can use a projection on the num field.") {
			const auto [ret, last] = ctp::stable_remove_if(vec, [](int i) { return i == 2; }, &aggregate::num);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(result == expected);
		}
		THEN("We can use a projection on the id field.") {
			const auto [ret, last] = ctp::stable_remove_if(vec, [](std::string_view id) { return id == "three"; }, &aggregate::id);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(result == expected);
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto [ret, last] = ctp::stable_remove_if(dupes, [](std::string_view id) { return id == "nine"; }, &aggregate::id);
			const std::vector<aggregate> result{dupes.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(result == expected);
		}
	}
}

TEST_CASE("unstable_remove") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto ret = ctp::unstable_remove(vec, 1);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {2, 3, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto ret = ctp::unstable_remove(vec, 3);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the last element preserves order.")
		{
			const auto ret = ctp::unstable_remove(vec, 5);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto ret = ctp::unstable_remove(dupes, 9);
			const std::vector<int> result{dupes.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto ret = ctp::unstable_remove(vec, 9);
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto ret = ctp::unstable_remove(vec, aggregate{1, "one"});
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("We can use a projection on the num field.") {
			const auto ret = ctp::unstable_remove(vec, 2, &aggregate::num);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
		THEN("We can use a projection on the id field.") {
			const auto ret = ctp::unstable_remove(vec, "three", &aggregate::id);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto ret = ctp::unstable_remove(dupes, "nine", &aggregate::id);
			const std::vector<aggregate> result{dupes.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
	}
}

TEST_CASE("unstable_remove_if") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto ret = ctp::unstable_remove_if(vec, [](int i) { return i == 1; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {2, 3, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto ret = ctp::unstable_remove_if(vec, [](int i) { return i == 3; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the last element preserves order.")
		{
			const auto ret = ctp::unstable_remove_if(vec, [](int i) { return i == 5; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto ret = ctp::unstable_remove_if(dupes, [](int i) { return i == 9; });
			const std::vector<int> result{dupes.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto ret = ctp::unstable_remove_if(vec, [](int i) { return i == 9; });
			const std::vector<int> result{vec.begin(), ret};
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto ret = ctp::unstable_remove_if(vec, [](const auto& a) { return a == aggregate{1, "one"}; });
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}

		THEN("We can use a projection on the num field.") {
			const auto ret = ctp::unstable_remove_if(vec, [](int i) { return i == 2; }, &aggregate::num);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
		THEN("We can use a projection on the id field.") {
			const auto ret = ctp::unstable_remove_if(vec, [](std::string_view id) { return id == "three"; }, &aggregate::id);
			const std::vector<aggregate> result{vec.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto ret = ctp::unstable_remove_if(dupes, [](std::string_view id) { return id == "nine"; }, &aggregate::id);
			const std::vector<aggregate> result{dupes.begin(), ret};
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(result.begin(), result.end(), expected.begin(), expected.end()));
		}
	}
}
