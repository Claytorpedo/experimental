#include <catch.hpp>
#include <Tools/uninitialized_storage.hpp>

#include <Tools/debug.hpp>

#include <string>

using namespace std::literals;

namespace {

constexpr bool UninitializedArrayTests() {
	{
		ctp::uninitialized_item<int> storage[5];
		std::construct_at(&storage[0].item, 0);
		std::construct_at(&storage[1].item, 1);
		std::construct_at(&storage[2].item, 2);
		ctpAssert(storage[0].item == 0);
		ctpAssert(storage[1].item == 1);
		ctpAssert(storage[2].item == 2);

		// Don't need to destroy trivial types.
	}

	{
		ctp::uninitialized_item<std::string> storage[5];
		std::construct_at(&storage[0].item, "one"sv);
		std::construct_at(&storage[1].item, "two"sv);
		std::construct_at(&storage[2].item, "very long string to avoid small string optimization"sv);

		ctpAssert(storage[0].item == "one"sv);
		ctpAssert(storage[1].item == "two"sv);
		ctpAssert(storage[2].item == "very long string to avoid small string optimization"sv);

		// Must destroy non trivial types.
		std::destroy_at(&storage[0].item);
		std::destroy_at(&storage[1].item);
		std::destroy_at(&storage[2].item);
	}

	return true;
}


constexpr auto RunUninitializedArrayTests = UninitializedArrayTests();

} // namespace

TEST_CASE("Basic tests.", "[Tools][uninitialized_storage]") {
	UninitializedArrayTests();
}
