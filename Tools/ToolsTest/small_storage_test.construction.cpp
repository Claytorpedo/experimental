#define CTP_ASSERTS_ENABLED 1

#include <catch.hpp>
#include <Tools/small_storage.hpp>

#include "small_storage_test_helpers.hpp"

// TODO: benchmark and look at generated code to tune things a bit.

using namespace std::literals;

namespace {

template <typename T, typename DataT>
constexpr bool CopyConstructTests(const std::array<DataT, DataSize>& data) {
	using namespace ctp::small_storage;
	{
		// Copy construct empty.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		const container_type copy(container);
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 0);
	}
	{
		// Copy construct partially full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container.push_back(data[0]);

		const container_type copy(container);
		ctpAssert(container.size() == 1);
		ctpAssert(copy.size() == 1);
		ctpAssert(copy[0] == data[0]);
	}

	{
		// Copy construct full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);

		const container_type copy(container);
		ctpAssert(container.size() == 2);
		ctpAssert(copy.size() == 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
	}

	{
		// Can be large, but currently in small mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);

		const container_type copy(container);
		ctpAssert(container.size() == 2);
		ctpAssert(copy.size() == 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
	}

	{
		// Large mode copy.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]);

		const container_type copy(container);
		ctpAssert(container.size() == 3);
		ctpAssert(copy.size() == 3);
		ctpAssert(container[0] == data[0]);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(container[1] == data[1]);
		ctpAssert(copy[1] == data[1]);
		ctpAssert(container[2] == data[2]);
		ctpAssert(copy[2] == data[2]);
	}

	return true;
}

template <typename T, typename DataT>
constexpr bool MoveConstructTests(const std::array<DataT, DataSize>& data) {
	using namespace ctp::small_storage;

	{
		// Copy construct empty.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		const container_type copy(std::move(container));
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 0);
	}

	{
		// Copy construct partially full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container.push_back(data[0]);

		const container_type copy(std::move(container));
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 1);
		ctpAssert(copy[0] == data[0]);
	}

	{
		// Copy construct full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);

		const container_type copy(std::move(container));
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
	}

	{
		// Can be large, but currently in small mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);

		const container_type copy(std::move(container));
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
	}

	{
		// Large mode copy.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]);

		const container_type copy(std::move(container));
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 3);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
		ctpAssert(copy[2] == data[2]);
	}

	return true;
}

#ifdef _MSC_VER
// Seems that it needs to be disabled outside this function for it to take effect.
// The issue is with the initializer_list in a constexpr context.
// https://developercommunity.visualstudio.com/t/MSVC-incorrectly-reports-warning-C4702:/10720847
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
template <typename T, typename DataT>
constexpr bool CopyAssignTests(const std::array<DataT, DataSize>& data) {
	using namespace ctp::small_storage;

	{
		// Copy assign empty.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container_type copy;
		copy = container;
		ctpAssert(container.size() == 0);
		ctpAssert(copy.size() == 0);
	}
	{
		// Copy assign partially full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container.push_back(data[0]);

		container_type copy;
		copy = container;
		ctpAssert(container.size() == 1);
		ctpAssert(copy.size() == 1);
		ctpAssert(copy[0] == data[0]);
	}

	{
		// Copy assign full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type larger;
		larger.push_back(data[0]);
		larger.push_back(data[1]);

		// Copy a larger container into a smaller container.
		container_type copy;
		copy.push_back(data[3]);
		copy = larger;

		ctpAssert(larger.size() == 2);
		ctpAssert(copy.size() == 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);

		// Copy a smaller container into a larger one.
		container_type smaller;
		smaller.push_back(data[3]);
		copy = smaller;

		ctpAssert(smaller.size() == 1);
		ctpAssert(copy.size() == 1);
		ctpAssert(copy[0] == data[3]);
	}

	{
		// Can be large, but currently in small mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type larger;
		larger.push_back(data[0]);
		larger.push_back(data[1]);

		// Copy a larger container into a smaller container.
		container_type copy;
		copy.push_back(data[3]);
		copy = larger;

		ctpAssert(larger.size() == 2);
		ctpAssert(copy.size() == 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);

		// Copy a smaller container into a larger one.
		container_type smaller;
		smaller.push_back(data[3]);
		copy = smaller;

		ctpAssert(smaller.size() == 1);
		ctpAssert(copy.size() == 1);
		ctpAssert(copy[0] == data[3]);
	}

	{
		// Copy from large mode into empty, such that it resizes to large mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]); // switch to large mode

		container_type copy;
		copy = container;

		ctpAssert(container.size() == 3);
		ctpAssert(container[0] == data[0]);
		ctpAssert(container[1] == data[1]);
		ctpAssert(container[2] == data[2]);

		ctpAssert(copy.size() == 3);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
		ctpAssert(copy[2] == data[2]);
	}

	{
		// Copy from large mode into small mode without needing to switch to large.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]); // switch to large mode
		container.clear(); // Clear items but stay in large mode.
		container.push_back(data[3]);

		container_type copy;
		copy.push_back(data[0]);

		copy = container;

		ctpAssert(container.size() == 1);
		ctpAssert(container.capacity() > 2);
		ctpAssert(container[0] == data[3]);

		ctpAssert(copy.size() == 1);
		ctpAssert(copy.capacity() == 2);
		ctpAssert(copy[0] == data[3]);
	}

	{
		// Copy from large mode into a container that must switch to large mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]); // switch to large mode

		container_type copy;
		copy.push_back(data[3]);

		copy = container;

		ctpAssert(container.size() == 3);
		ctpAssert(container.capacity() > 2);
		ctpAssert(container[0] == data[0]);
		ctpAssert(container[1] == data[1]);
		ctpAssert(container[2] == data[2]);

		ctpAssert(copy.size() == 3);
		ctpAssert(copy.capacity() > 2);
		ctpAssert(copy[0] == data[0]);
		ctpAssert(copy[1] == data[1]);
		ctpAssert(copy[2] == data[2]);
	}

	{
		// Copy from small mode into large mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);

		container_type copy;
		copy.push_back(data[1]);
		copy.push_back(data[2]);
		copy.push_back(data[3]); // switch to large mode

		copy = container;

		ctpAssert(container.size() == 1);
		ctpAssert(container.capacity() == 2);
		ctpAssert(container[0] == data[0]);

		ctpAssert(copy.size() == 1);
		ctpAssert(copy.capacity() > 2); // still large mode
		ctpAssert(copy[0] == data[0]);
	}

	{
		// Copy from an initializer list.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;

		const auto ilist = std::initializer_list<T>{data[0], data[1], data[2]};
// The issue is specifically with the above initializer_list.
// The test is working fine via debugger inspection.
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		container = ilist;

		ctpAssert(container.size() == 3);
		ctpAssert(container[0] == data[0]);
		ctpAssert(container[1] == data[1]);
		ctpAssert(container[2] == data[2]);
	}

	return true;
}

template <typename T, typename DataT>
constexpr bool MoveAssignTests(const std::array<DataT, DataSize>& data) {
	using namespace ctp::small_storage;

	{
		// Move assign empty.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container_type toMove;
		toMove = std::move(container);
		ctpAssert(container.size() == 0);
		ctpAssert(toMove.size() == 0);
	}
	{
		// Move assign partially full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type container;
		container.push_back(data[0]);

		container_type toMove;
		toMove = std::move(container);
		ctpAssert(container.size() == 0);
		ctpAssert(toMove.size() == 1);
		ctpAssert(toMove[0] == data[0]);
	}

	{
		// Move assign full small mode.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type larger;
		larger.push_back(data[0]);
		larger.push_back(data[1]);

		// Move a larger container into a smaller container.
		container_type toMove;
		toMove.push_back(data[3]);
		toMove = std::move(larger);

		ctpAssert(larger.size() == 0);
		ctpAssert(toMove.size() == 2);
		ctpAssert(toMove[0] == data[0]);
		ctpAssert(toMove[1] == data[1]);

		// Move a smaller container into a larger one.
		container_type smaller;
		smaller.push_back(data[3]);
		toMove = std::move(smaller);

		ctpAssert(smaller.size() == 0);
		ctpAssert(toMove.size() == 1);
		ctpAssert(toMove[0] == data[3]);
	}

	{
		// Can be large, but currently in small mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type larger;
		larger.push_back(data[0]);
		larger.push_back(data[1]);

		// Move a larger container into a smaller container.
		container_type toMove;
		toMove.push_back(data[3]);
		toMove = std::move(larger);

		ctpAssert(larger.size() == 0);
		ctpAssert(toMove.size() == 2);
		ctpAssert(toMove[0] == data[0]);
		ctpAssert(toMove[1] == data[1]);

		// Move a smaller container into a larger one.
		container_type smaller;
		smaller.push_back(data[3]);
		toMove = std::move(smaller);

		ctpAssert(smaller.size() == 0);
		ctpAssert(toMove.size() == 1);
		ctpAssert(toMove[0] == data[3]);
	}

	{
		// Move from large mode into empty.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]); // switch to large mode

		container_type toMove;
		toMove = std::move(container);

		ctpAssert(container.size() == 0);

		ctpAssert(toMove.size() == 3);
		ctpAssert(toMove[0] == data[0]);
		ctpAssert(toMove[1] == data[1]);
		ctpAssert(toMove[2] == data[2]);
	}

	{
		// Move from large mode into small mode without needing to switch to large.
		// We should prefer the large pointer anyway.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]); // switch to large mode
		container.clear(); // Clear items but stay in large mode.
		container.push_back(data[3]);

		container_type toMove;
		toMove.push_back(data[0]);

		toMove = std::move(container);

		ctpAssert(container.size() == 0);
		ctpAssert(container.capacity() == 2);

		ctpAssert(toMove.size() == 1);
		ctpAssert(toMove.capacity() >= 3);
		ctpAssert(toMove[0] == data[3]);
	}

	{
		// Move from large mode into a container that must switch to large mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);
		container.push_back(data[1]);
		container.push_back(data[2]); // switch to large mode

		container_type toMove;
		toMove.push_back(data[3]);

		toMove = std::move(container);

		ctpAssert(container.size() == 0);
		ctpAssert(container.capacity() == 2); // back to small mode

		ctpAssert(toMove.size() == 3);
		ctpAssert(toMove.capacity() > 2);
		ctpAssert(toMove[0] == data[0]);
		ctpAssert(toMove[1] == data[1]);
		ctpAssert(toMove[2] == data[2]);
	}

	{
		// Move from small mode into large mode.
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type container;
		container.push_back(data[0]);

		container_type toMove;
		toMove.push_back(data[1]);
		toMove.push_back(data[2]);
		toMove.push_back(data[3]); // switch to large mode

		toMove = std::move(container);

		ctpAssert(container.size() == 0);
		ctpAssert(container.capacity() == 2);

		ctpAssert(toMove.size() == 1);
		ctpAssert(toMove.capacity() >= 3); // still large mode
		ctpAssert(toMove[0] == data[0]);
	}

	return true;
}

template <typename T, typename DataT>
constexpr bool SwapTests(const std::array<DataT, DataSize>& data) {
	using namespace ctp::small_storage;

	{
		// Empty swap.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type a;
		container_type b;
		a.swap(b);
		ctpAssert(a.size() == 0);
		ctpAssert(b.size() == 0);
	}

	{
		// Swap into empty.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type a;
		a.push_back(data[0]);
		a.push_back(data[1]);
		container_type b;
		a.swap(b);
		ctpAssert(a.size() == 0);
		ctpAssert(b.size() == 2);
		ctpAssert(b[0] == data[0]);
		ctpAssert(b[1] == data[1]);
	}

	{
		// two way swap.
		using container_type = container<T, 2, std::allocator<T>, small_only_options>;
		container_type a;
		a.push_back(data[0]);
		a.push_back(data[1]);
		container_type b;
		b.push_back(data[2]);
		b.push_back(data[3]);
		a.swap(b);

		ctpAssert(a.size() == 2);
		ctpAssert(b.size() == 2);
		ctpAssert(a[0] == data[2]);
		ctpAssert(a[1] == data[3]);
		ctpAssert(b[0] == data[0]);
		ctpAssert(b[1] == data[1]);
	}

	{
		// large and small
		using container_type = container<T, 2, std::allocator<T>, can_grow_options>;
		container_type a;
		a.push_back(data[0]);
		a.push_back(data[1]);
		container_type b;
		b.push_back(data[2]);
		b.push_back(data[3]);
		b.push_back(data[4]);

		a.swap(b);

		ctpAssert(a.size() == 3);
		ctpAssert(b.size() == 2);
		ctpAssert(a[0] == data[2]);
		ctpAssert(a[1] == data[3]);
		ctpAssert(a[2] == data[4]);
		ctpAssert(b[0] == data[0]);
		ctpAssert(b[1] == data[1]);

		a.swap(b);

		ctpAssert(a.size() == 2);
		ctpAssert(b.size() == 3);
		ctpAssert(a[0] == data[0]);
		ctpAssert(a[1] == data[1]);
		ctpAssert(b[0] == data[2]);
		ctpAssert(b[1] == data[3]);
		ctpAssert(b[2] == data[4]);
	}

	return true;
}

// TODO: test copy/move constructors taking separate allocator

// TODO: test range conversion constructor.

constexpr auto RunCopyConstructTestsTrivial = CopyConstructTests<int>(IntData);
constexpr auto RunCopyConstructTestsNonTrivial = CopyConstructTests<std::string>(StringData);
constexpr auto RunCopyConstructTestsNoDefault = CopyConstructTests<NoDefaultConstruct>(NoDefaultConstructData);

constexpr auto RunMoveConstructTestsTrivial = MoveConstructTests<int>(IntData);
constexpr auto RunMoveConstructTestsNonTrivial = MoveConstructTests<std::string>(StringData);
constexpr auto RunMoveConstructTestsNoDefault = MoveConstructTests<NoDefaultConstruct>(NoDefaultConstructData);

constexpr auto RunCopyAssignTestsTrivial = CopyAssignTests<int>(IntData);
constexpr auto RunCopyAssignTestsNonTrivial = CopyAssignTests<std::string>(StringData);
constexpr auto RunCopyAssignTestsNoDefault = CopyAssignTests<NoDefaultConstruct>(NoDefaultConstructData);

constexpr auto RunMoveAssignTestsTrivial = MoveAssignTests<int>(IntData);
constexpr auto RunMoveAssignTestsNonTrivial = MoveAssignTests<std::string>(StringData);
constexpr auto RunMoveAssignTestsNoDefault = MoveAssignTests<NoDefaultConstruct>(NoDefaultConstructData);

constexpr auto RunSwapTestsTrivial = SwapTests<int>(IntData);

} // namespace

TEST_CASE("Construction tests.", "[Tools][small_storage]") {
	using namespace ctp::small_storage;

	CopyConstructTests<int>(IntData);
	CopyConstructTests<std::string>(StringData);
	CopyConstructTests<NoDefaultConstruct>(NoDefaultConstructData);

	MoveConstructTests<int>(IntData);
	MoveConstructTests<std::string>(StringData);
	MoveConstructTests<NoDefaultConstruct>(NoDefaultConstructData);

	CopyAssignTests<int>(IntData);
	CopyAssignTests<std::string>(StringData);
	CopyAssignTests<NoDefaultConstruct>(NoDefaultConstructData);

	MoveAssignTests<int>(IntData);
	MoveAssignTests<std::string>(StringData);
	MoveAssignTests<NoDefaultConstruct>(NoDefaultConstructData);

	SwapTests<int>(IntData);
}
