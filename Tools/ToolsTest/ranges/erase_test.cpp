#include <catch.hpp>
#include <Tools/ranges/erase.hpp>

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

TEST_CASE("stable_erase") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto numErased = ctp::stable_erase(vec, 1);
			CHECK(numErased == 1);
			const std::vector expected = {2, 3, 4, 5};
			CHECK(vec == expected);
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto numErased = ctp::stable_erase(vec, 3);
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 4, 5};
			CHECK(vec == expected);
		}

		THEN("Removing the last element preserves order.")
		{
			const auto numErased = ctp::stable_erase(vec, 5);
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 3, 4};
			CHECK(vec == expected);
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto numErased = ctp::stable_erase(dupes, 9);
			CHECK(numErased == 9);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(vec == expected);
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto numErased = ctp::stable_erase(vec, 9);
			CHECK(numErased == 0);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(vec == expected);
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto numErased = ctp::stable_erase(vec, aggregate{1, "one"});
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(vec == expected);
		}

		THEN("We can use a projection on the num field.") {
			const auto numErased = ctp::stable_erase(vec, 2, &aggregate::num);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(vec == expected);
		}
		THEN("We can use a projection on the id field.") {
			const auto numErased = ctp::stable_erase(vec, "three", &aggregate::id);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(vec == expected);
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto numErased = ctp::stable_erase(dupes, "nine", &aggregate::id);
			CHECK(numErased == 3);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(vec == expected);
		}
	}
}

TEST_CASE("stable_erase_if") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto numErased = ctp::stable_erase_if(vec, [](int i) { return i == 1; });
			CHECK(numErased == 1);
			const std::vector expected = {2, 3, 4, 5};
			CHECK(vec == expected);
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto numErased = ctp::stable_erase_if(vec, [](int i) { return i == 3; });
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 4, 5};
			CHECK(vec == expected);
		}

		THEN("Removing the last element preserves order.")
		{
			const auto numErased = ctp::stable_erase_if(vec, [](int i) { return i == 5; });
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 3, 4};
			CHECK(vec == expected);
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto numErased = ctp::stable_erase_if(dupes, [](int i) { return i == 9; });
			CHECK(numErased == 9);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(vec == expected);
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto numErased = ctp::stable_erase_if(vec, [](int i) { return i == 9; });
			CHECK(numErased == 0);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(vec == expected);
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto numErased = ctp::stable_erase_if(vec, [](const auto& a) { return a == aggregate{1, "one"}; } );
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(vec == expected);
		}

		THEN("We can use a projection on the num field.") {
			const auto numErased = ctp::stable_erase_if(vec, [](int i) { return i == 2; }, &aggregate::num);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(vec == expected);
		}
		THEN("We can use a projection on the id field.") {
			const auto numErased = ctp::stable_erase_if(vec, [](std::string_view id) { return id == "three"; }, &aggregate::id);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(vec == expected);
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto numErased = ctp::stable_erase_if(dupes, [](std::string_view id) { return id == "nine"; }, &aggregate::id);
			CHECK(numErased == 3);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(vec == expected);
		}
	}
}

TEST_CASE("unstable_erase") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto numErased = ctp::unstable_erase(vec, 1);
			CHECK(numErased == 1);
			const std::vector expected = {2, 3, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto numErased = ctp::unstable_erase(vec, 3);
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the last element preserves order.")
		{
			const auto numErased = ctp::unstable_erase(vec, 5);
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 3, 4};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto numErased = ctp::unstable_erase(dupes, 9);
			CHECK(numErased == 9);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto numErased = ctp::unstable_erase(vec, 9);
			CHECK(numErased == 0);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto numErased = ctp::unstable_erase(vec, aggregate{1, "one"});
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("We can use a projection on the num field.") {
			const auto numErased = ctp::unstable_erase(vec, 2, &aggregate::num);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
		THEN("We can use a projection on the id field.") {
			const auto numErased = ctp::unstable_erase(vec, "three", &aggregate::id);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto numErased = ctp::unstable_erase(dupes, "nine", &aggregate::id);
			CHECK(numErased == 3);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
	}
}

TEST_CASE("unstable_erase_if") {
	GIVEN("A vector of ints.") {
		std::vector vec = {1, 2, 3, 4, 5};

		THEN("Removing the first element preserves order.")
		{
			const auto numErased = ctp::unstable_erase_if(vec, [](int i) { return i == 1; });
			CHECK(numErased == 1);
			const std::vector expected = {2, 3, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the middle element preserves order.")
		{
			const auto numErased = ctp::unstable_erase_if(vec, [](int i) { return i == 3; });
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing the last element preserves order.")
		{
			const auto numErased = ctp::unstable_erase_if(vec, [](int i) { return i == 5; });
			CHECK(numErased == 1);
			const std::vector expected = {1, 2, 3, 4};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing duplicate elements preserves order.")
		{
			std::vector dupes = {9, 9, 1, 9, 2, 9, 3, 9, 9, 4, 9, 5, 9, 9};

			const auto numErased = ctp::unstable_erase_if(dupes, [](int i) { return i == 9; });
			CHECK(numErased == 9);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("Removing no elements leaves the vector unchanged.")
		{
			const auto numErased = ctp::unstable_erase_if(vec, [](int i) { return i == 9; });
			CHECK(numErased == 0);
			const std::vector expected = {1, 2, 3, 4, 5};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
	}

	GIVEN("A struct with an id field.") {
		std::vector<aggregate> vec = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

		THEN("We can remove based on full struct equality.") {
			const auto numErased = ctp::unstable_erase_if(vec, [](const auto& a) { return a == aggregate{1, "one"}; });
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}

		THEN("We can use a projection on the num field.") {
			const auto numErased = ctp::unstable_erase_if(vec, [](int i) { return i == 2; }, &aggregate::num);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
		THEN("We can use a projection on the id field.") {
			const auto numErased = ctp::unstable_erase_if(vec, [](std::string_view id) { return id == "three"; }, &aggregate::id);
			CHECK(numErased == 1);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {4, "four"}};
			CHECK(std::is_permutation(vec.begin(), vec.end(), expected.begin(), expected.end()));
		}
		THEN("Projections can remove multiple elements.") {
			std::vector<aggregate> dupes = {{9, "nine"}, {1, "one"}, {9, "nine"}, {2, "two"}, {3, "three"}, {4, "four"}, {9, "nine"}};
			const auto numErased = ctp::unstable_erase_if(dupes, [](std::string_view id) { return id == "nine"; }, &aggregate::id);
			CHECK(numErased == 3);
			const std::vector<aggregate> expected = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
			CHECK(std::is_permutation(dupes.begin(), dupes.end(), expected.begin(), expected.end()));
		}
	}
}
