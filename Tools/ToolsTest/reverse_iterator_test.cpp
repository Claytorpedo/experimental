#include <catch2/catch_test_macros.hpp>
#include <Tools/reverse_iterator.hpp>

#include <string_view>

using namespace std::literals;

namespace {
class test_iterator : public ctp::iterator_t<test_iterator, const std::string_view> {
public:
	static constexpr int Size = 5;

	int i_ = 0;
	static constexpr std::string_view data_[Size] = {"zero"sv, "one"sv, "two"sv, "three"sv, "four"sv};

	constexpr test_iterator() noexcept = default;
	constexpr test_iterator(int i) noexcept : i_{i} {}

	static constexpr auto begin() noexcept { return test_iterator{0}; }
	static constexpr auto end() noexcept { return test_iterator{Size}; }

private:
	friend ctp::iterator_accessor;
	constexpr auto& get_index() noexcept { return i_; }
	constexpr auto* peek() noexcept { return &data_[i_]; }
};
} // namespace

TEST_CASE("reverse_iterator", "[Tools][iterator]") {
	using reverse_it = ctp::reverse_iterator<test_iterator>;
	static_assert(std::contiguous_iterator<reverse_it>);

	GIVEN("A reverse iterator from the end of a collection.") {
		reverse_it it{test_iterator::end()};

		CHECK(*it == "four"sv);
		CHECK(*++it == "three"sv);
		CHECK(*--it == "four"sv);
		CHECK(*it++ == "four"sv);
		CHECK(*it == "three"sv);
		CHECK(*it-- == "three"sv);
		CHECK(*it == "four"sv);
		it += 2;
		CHECK(*it == "two"sv);
		it -= 2;
		CHECK(*it == "four"sv);

		CHECK(it->size() == 4); // test ->

		CHECK(it[2] == "two"sv);

		const reverse_it start{test_iterator::begin()};

		CHECK(it != start);
		CHECK(it < start);
		CHECK(it <= start);
		CHECK(start > it);
		CHECK(start >= it);

		it = it + 4;
		CHECK(*it == "zero"sv);
		++it; // one past the end for reverse.
		CHECK(it == start);

		it = it - 2;
		CHECK(*it == "one"sv);
	}
}
