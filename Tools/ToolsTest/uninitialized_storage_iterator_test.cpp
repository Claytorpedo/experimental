#include <catch.hpp>
#include <Tools/uninitialized_storage.hpp>

#include <Tools/debug.hpp>

#include <string>

using namespace std::literals;

namespace {

constexpr bool UninitializedArrayIteratorTests() {
	{
		ctp::uninitialized_item<int> storage[5];
		std::construct_at(&storage[0].item, 0);
		std::construct_at(&storage[1].item, 1);
		std::construct_at(&storage[2].item, 2);

		ctp::uninitialized_item_iterator<int> iterator{storage};
		const ctp::const_uninitialized_item_iterator<int> end{storage + 3};

		for (int i = 0;iterator < end; ++iterator, ++i)
			ctpAssert(*iterator == i);
	}

	{
		ctp::uninitialized_item<std::string> storage[5];
		std::construct_at(&storage[0].item, "one"sv);
		std::construct_at(&storage[1].item, "two"sv);
		std::construct_at(&storage[2].item, "very long string to avoid small string optimization"sv);

		ctp::uninitialized_item_iterator<std::string> iterator{storage};
		ctpAssert(*iterator++ == "one"sv);
		ctpAssert(*iterator++ == "two"sv);
		ctpAssert(*iterator++ == "very long string to avoid small string optimization"sv);

		// Must destroy non trivial types.
		std::destroy_at(&storage[0].item);
		std::destroy_at(&storage[1].item);
		std::destroy_at(&storage[2].item);
	}

	return true;
}


constexpr auto RunUninitializedArrayIteratorTests = UninitializedArrayIteratorTests();

} // namespace

TEST_CASE("Basic tests.", "[Tools][uninitialized_item_iterator]") {
	UninitializedArrayIteratorTests();
}
