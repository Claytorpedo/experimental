#define CTP_ASSERTS_ENABLED 1

#include <catch.hpp>
#include <Tools/small_storage.hpp>

#include <Tools/test/catch_test_helpers.hpp>

#include "small_storage_test_helpers.hpp"

#include <string>

using namespace std::literals;

namespace {

template <typename S>
class input_iterator_t : public ctp::iterator_t<input_iterator_t<S>, S, std::input_iterator_tag> {
	std::size_t i_;
	S* data_;

	friend ctp::iterator_accessor;
	constexpr void pre_increment() noexcept { ++i_; }
	constexpr bool equals(const input_iterator_t& o) const noexcept { return i_ == o.i_; }
	constexpr auto peek() noexcept { return &data_[i_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(input_iterator_t, S)
		: i_{other.i_}
		, data_{other.data_}
	{}

public:
	constexpr input_iterator_t(std::size_t i, S* data) noexcept : i_{i}, data_{data} {};
};

template <typename S>
class loop_iterator_t : public ctp::iterator_t<loop_iterator_t<S>, S> {
	std::size_t i_ = 0;
	std::size_t size_ = 0;
	S* data_ = nullptr;

	friend ctp::iterator_accessor;
	constexpr std::size_t& get_index() { return i_; }
	constexpr auto peek() noexcept { return &data_[i_ % size_]; }

	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(loop_iterator_t, S)
		: i_{other.i_}
		, size_{other.size_}
		, data_{other.data_}
	{}

public:
	constexpr loop_iterator_t(std::size_t i, std::size_t size, S* data) noexcept
		: i_{i}, size_{size}, data_{data} {};
	constexpr loop_iterator_t() noexcept = default;
};

template <typename ContainerType>
constexpr bool BasicSmallTests() {
	constexpr std::size_t Capacity = ContainerType::SmallCapacity;
	ContainerType container;

	ctpAssert(container.size() == 0);
	ctpAssert(container.empty());
	ctpAssert(container.is_empty());
	ctpAssert(container.max_size() == Capacity);
	ctpAssert(container.capacity() == Capacity);

	ctpAssert(container.push_back(0) == 0);

	ctpAssert(*container.data() == 0);
	ctpAssert(container.size() == 1);
	ctpAssert(!container.empty());
	ctpAssert(!container.is_empty());
	ctpAssert(container.max_size() == Capacity);
	ctpAssert(container.capacity() == Capacity);
	ctpAssert(container[0] == 0);
	ctpAssert(container.front() == 0);
	ctpAssert(container.back() == 0);

	ctpAssert(container.emplace_back(1) == 1);

	ctpAssert(*container.data() == 0);

	// We can use the raw data pointer if T is a trivial type, or if we're at runtime.
	if constexpr (ctp::small_storage::can_trivially_construct_and_assign_v<typename ContainerType::value_type>) {
		ctpAssert(*(container.data() + 1) == 1);
	}
	if CTP_NOT_CONSTEVAL {
		ctpAssert(*(container.data() + 1) == 1);
	}

	ctpAssert(container.size() == 2);
	ctpAssert(!container.empty());
	ctpAssert(!container.is_empty());
	ctpAssert(container.max_size() == Capacity);
	ctpAssert(container.capacity() == Capacity);
	ctpAssert(container[0] == 0);
	ctpAssert(container[1] == 1);
	ctpAssert(container.front() == 0);
	ctpAssert(container.back() == 1);

	// Some iterator tests.
	ctpAssert(*container.begin() == 0);
	ctpAssert(*container.last() == 1);
	{
		int i = 0;
		for (const auto& val : container)
			ctpAssert(val == i++);
	}

	container.clear();

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == Capacity);

	container.reset();

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == Capacity);

	return true;
}

template <typename ContainerType>
constexpr bool BasicSmallTestsNonTrivial() {
	constexpr std::size_t Capacity = ContainerType::SmallCapacity;
	ContainerType container;
	ctpAssert(container.size() == 0);
	ctpAssert(container.empty());
	ctpAssert(container.is_empty());
	ctpAssert(container.max_size() == Capacity);
	ctpAssert(container.capacity() == Capacity);

	ctpAssert(container.push_back("one") == "one"sv);

	ctpAssert(*container.data() == "one"sv);
	ctpAssert(container.size() == 1);
	ctpAssert(!container.empty());
	ctpAssert(!container.is_empty());
	ctpAssert(container.max_size() == Capacity);
	ctpAssert(container.capacity() == Capacity);
	ctpAssert(container[0] == "one"sv);
	ctpAssert(container.front() == "one"sv);
	ctpAssert(container.back() == "one"sv);

	ctpAssert(container.emplace_back("two") == "two"sv);

	ctpAssert(*container.data() == "one"sv);

	// We can use the raw data pointer if T is a trivial type, or if we're at runtime.
	if constexpr (std::is_trivially_default_constructible_v<typename ContainerType::value_type>) {
		ctpAssert(*(container.data() + 1) == "two"sv);
	}
	if CTP_NOT_CONSTEVAL {
		ctpAssert(*(container.data() + 1) == "two"sv);
	}

	ctpAssert(container.size() == 2);
	ctpAssert(!container.empty());
	ctpAssert(!container.is_empty());
	ctpAssert(container.max_size() == Capacity);
	ctpAssert(container.capacity() == Capacity);
	ctpAssert(container[0] == "one"sv);
	ctpAssert(container[1] == "two"sv);
	ctpAssert(container.front() == "one"sv);
	ctpAssert(container.back() == "two"sv);

	// Some iterator tests.
	ctpAssert(*container.begin() == "one"sv);
	ctpAssert(*container.last() == "two"sv);
	{
		std::string_view strs[] = {"one"sv, "two"sv};
		int i = 0;
		for (const auto& val : container)
			ctpAssert(val == strs[i++]);
	}

	container.clear();

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == Capacity);

	container.reset();

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == Capacity);

	container.reserve(2);

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == Capacity);

	return true;
}

template <typename ContainerType>
constexpr bool BasicLargeTests() {
	constexpr std::size_t SmallCapacity = ContainerType::SmallCapacity;
	// Cheat a bit to grab the size we want.
	constexpr auto LargeMaxCapacity =
		ctp::small_storage::detail::bits_max(sizeof(typename ContainerType::size_type) * 8 - 1);

	ContainerType container;

	ctpAssert(container.size() == 0);
	ctpAssert(container.empty());
	ctpAssert(container.is_empty());
	ctpAssert(container.max_size() == LargeMaxCapacity);
	ctpAssert(container.capacity() == SmallCapacity);

	ctpAssert(container.push_back(0) == 0);
	ctpAssert(container.emplace_back(1) == 1);

	ctpAssert(*container.data() == 0);
	ctpAssert(*(container.data() + 1) == 1);
	ctpAssert(container.size() == 2);
	ctpAssert(!container.empty());
	ctpAssert(!container.is_empty());
	ctpAssert(container.max_size() == LargeMaxCapacity);
	ctpAssert(container.capacity() == SmallCapacity);
	ctpAssert(container[0] == 0);
	ctpAssert(container[1] == 1);
	ctpAssert(container.front() == 0);
	ctpAssert(container.back() == 1);

	// Some iterator tests.
	ctpAssert(*container.begin() == 0);
	ctpAssert(*container.last() == 1);
	{
		int i = 0;
		for (const auto& val : container)
			ctpAssert(val == i++);
	}

	// Switch to large mode.

	ctpAssert(container.emplace_back(2) == 2);

	ctpAssert(container[0] == 0);
	ctpAssert(container[1] == 1);
	ctpAssert(container[2] == 2);

	{
		const auto currentCapacity = container.capacity();
		container.clear();

		ctpAssert(container.size() == 0);
		ctpAssert(container.capacity() == currentCapacity);
	}

	container.reset();

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == SmallCapacity);

	container.reserve(2);

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == SmallCapacity);

	container.reserve(10);

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == 10);

	return true;
}

template <typename ContainerType>
constexpr bool BasicLargeTestsNonTrivial() {
	constexpr std::size_t SmallCapacity = ContainerType::SmallCapacity;
	constexpr auto LargeMaxCapacity =
		ctp::small_storage::detail::bits_max(sizeof(typename ContainerType::size_type) * 8 - 1);

	ContainerType container;

	ctpAssert(container.size() == 0);
	ctpAssert(container.empty());
	ctpAssert(container.is_empty());
	ctpAssert(container.max_size() == LargeMaxCapacity);
	ctpAssert(container.capacity() == SmallCapacity);

	ctpAssert(container.push_back("one") == "one"sv);
	ctpAssert(container.emplace_back("two") == "two"sv);

	ctpAssert(*container.data() == "one"sv);
	ctpAssert(*(container.data() + 1) == "two"sv);
	ctpAssert(container.size() == 2);
	ctpAssert(!container.empty());
	ctpAssert(!container.is_empty());
	ctpAssert(container.max_size() == LargeMaxCapacity);
	ctpAssert(container.capacity() == SmallCapacity);
	ctpAssert(container[0] == "one"sv);
	ctpAssert(container[1] == "two"sv);
	ctpAssert(container.front() == "one"sv);
	ctpAssert(container.back() == "two"sv);

	// Some iterator tests.
	ctpAssert(*container.begin() == "one"sv);
	ctpAssert(*container.last() == "two"sv);
	{
		std::string_view strs[] = {"one"sv, "two"sv};
		int i = 0;
		for (const auto& val : container)
			ctpAssert(val == strs[i++]);
	}

	// Switch to large mode.
	ctpAssert(container.emplace_back("three") == "three"sv);

	ctpAssert(container[0] == "one"sv);
	ctpAssert(container[1] == "two"sv);
	ctpAssert(container[2] == "three"sv);

	{
		const auto currentCapacity = container.capacity();
		container.clear();

		ctpAssert(container.size() == 0);
		ctpAssert(container.capacity() == currentCapacity);
	}

	container.reset();

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == SmallCapacity);

	container.reserve(2);

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == SmallCapacity);

	container.reserve(10);

	ctpAssert(container.size() == 0);
	ctpAssert(container.capacity() == 10);

	return true;
}

template <typename T, typename ContainerOptions, typename DataT>
constexpr bool ResizeTests(const std::array<DataT, DataSize>& data) {
	using container_type = ctp::small_storage::container<T, 2, std::allocator<T>, ContainerOptions>;

	{
		// Null resize.
		container_type container;
		container.resize(0);
		ctpAssert(container.size() == 0);
	}

	{
		// resize 1 from empty
		container_type container;
		container.resize(1);
		ctpAssert(container.size() == 1);
		ctpAssert(container[0] == T{});
	}

	{
		// resize to fit small capacity
		container_type container;
		container.push_back(data[0]);
		container.resize(2, data[1]);

		ctpAssert(container.size() == 2);
		ctpAssert(container[0] == data[0]);
		ctpAssert(container[1] == data[1]);

		// shrink
		container.resize(1, data[2]);
		ctpAssert(container.size() == 1);
		ctpAssert(container[0] == data[0]);

		// shrink to empty

		container.resize(0, data[2]);
		ctpAssert(container.size() == 0);

		// fill in one go
		container.resize(2, data[2]);
		ctpAssert(container.size() == 2);
		ctpAssert(container[0] == data[2]);
		ctpAssert(container[1] == data[2]);
	}

	if constexpr (container_type::HasLargeMode) {
		container_type container;

		// resize to large mode
		container.resize(4, data[0]);

		ctpAssert(container.size() == 4);
		ctpAssert(container[0] == data[0]);
		ctpAssert(container[1] == data[0]);
		ctpAssert(container[2] == data[0]);
		ctpAssert(container[3] == data[0]);

		// shrink below large threshold (stays in large mode).

		const auto capacity = container.capacity();
		container.resize(1, data[1]);

		ctpAssert(container.size() == 1);
		ctpAssert(container.capacity() == capacity);
		ctpAssert(container[0] == data[0]);
	}

	return true;
}

template <typename T, typename ContainerOptions, typename DataT>
constexpr bool ShrinkToFitTests(const std::array<DataT, DataSize>& data) {
	using container_type = ctp::small_storage::container<T, 2, std::allocator<T>, ContainerOptions>;

	{
		// Null shrink.
		container_type container;
		container.shrink_to_fit();
		ctpAssert(container.size() == 0);
		ctpAssert(container.capacity() == 2);
	}

	{
		container_type container;
		container.resize(2, data[0]);
		container.shrink_to_fit();
		ctpAssert(container.size() == 2);
		ctpAssert(container.capacity() == 2);

		if constexpr (container_type::HasLargeMode) {
			// Switch to large mode.
			container.resize(10, data[0]);

			const auto largeCapacity = container.capacity();
			container.resize(3);
			container.shrink_to_fit();

			ctpAssert(container.size() == 3);
			// Should be smaller, but not guaranteed to be 3 due to allocate_at_least.
			ctpAssert(container.capacity() < largeCapacity);

			container.pop_back();
			container.shrink_to_fit();

			ctpAssert(container.size() == 2);
			// Should be back to small mode.
			ctpAssert(container.capacity() == 2);
		}
	}

	return true;
}

template <typename T, typename ContainerOptions, typename DataT>
constexpr bool InsertTests(const std::array<DataT, DataSize>& data) {
	constexpr int SmallSize = 10;
	using container_type = ctp::small_storage::container<T, SmallSize, std::allocator<T>, ContainerOptions>;

	// Single itemn insert.
	{
		container_type container;
		auto it = container.insert(container.begin(), data[0]);

		ctpAssert(container.size() == 1);
		ctpAssert(container[0] == data[0]);
		ctpAssert(it == container.begin());
		ctpAssert(*it == data[0]);

		// Insert to begin by move.
		T copy = data[1];
		it = container.insert(container.begin(), std::move(copy));

		if constexpr (!std::is_trivially_move_assignable_v<T>) {
			ctpAssert(copy == T{});
		}

		ctpAssert(container.size() == 2);
		ctpAssert(container[0] == data[1]);
		ctpAssert(container[1] == data[0]);
		ctpAssert(it == container.begin());
		ctpAssert(*it == data[1]);

		// Insert to end.
		it = container.insert(container.end(), data[2]);

		ctpAssert(container.size() == 3);
		ctpAssert(container[0] == data[1]);
		ctpAssert(container[1] == data[0]);
		ctpAssert(container[2] == data[2]);
		ctpAssert(*it == data[2]);

		// Insert in middle
		it = container.insert(container.begin() + 1, data[3]);

		ctpAssert(container.size() == 4);
		ctpAssert(container[0] == data[1]);
		ctpAssert(container[1] == data[3]);
		ctpAssert(container[2] == data[0]);
		ctpAssert(container[3] == data[2]);
		ctpAssert(*it == data[3]);
	}


	// Multi item insert.
	{
		container_type container;

		auto it = container.insert(container.begin(), 0, data[0]);

		ctpAssert(container.size() == 0);
		ctpAssert(it == container.end());

		it = container.insert(container.end(), 3, data[0]);

		ctpAssert(container.size() == 3);
		ctpAssert(it == container.begin());

		it = container.insert(container.begin(), 2, data[1]);

		ctpAssert(container.size() == 5);
		ctpAssert(it == container.begin());

		it = container.insert(container.end(), 2, data[2]);

		ctpAssert(container.size() == 7);
		ctpAssert(it == container.end() - 2);

		// Inserting near the end, where we will overlap it.
		it = container.insert(container.end() - 2, 3, data[3]);

		ctpAssert(container.size() == 10);
		ctpAssert(it == container.end() - 2 - 3); // -2 for insert pos, -3 for num inserted.

		it = container.begin();
		ctpAssert(it[0] == data[1]);
		ctpAssert(it[1] == data[1]);
		ctpAssert(it[2] == data[0]);
		ctpAssert(it[3] == data[0]);
		ctpAssert(it[4] == data[0]);
		ctpAssert(it[5] == data[3]);
		ctpAssert(it[6] == data[3]);
		ctpAssert(it[7] == data[3]);
		ctpAssert(it[8] == data[2]);
		ctpAssert(it[9] == data[2]);
	}

	// Insert via first/last iterators
	{
		container_type container;

		auto it = container.insert(container.begin(), data.begin(), data.begin());

		ctpAssert(container.size() == 0);
		ctpAssert(it == container.end());

		it = container.insert(container.end(), data.begin(), data.begin() + 3);

		ctpAssert(container.size() == 3);
		ctpAssert(it == container.begin());

		it = container.insert(container.begin(), data.begin(), data.begin() + 2);

		ctpAssert(container.size() == 5);
		ctpAssert(it == container.begin());

		it = container.insert(container.end(), data.end() - 2, data.end());

		ctpAssert(container.size() == 7);
		ctpAssert(it == container.end() - 2);

		// Inserting near the end, where we will overlap it.
		it = container.insert(container.end() - 2, data.end() - 3, data.end());

		ctpAssert(container.size() == 10);
		ctpAssert(it == container.end() - 2 - 3); // -2 for insert pos, -3 for num inserted.

		it = container.begin();
		ctpAssert(it[0] == data[0]);
		ctpAssert(it[1] == data[1]);
		ctpAssert(it[2] == data[0]);
		ctpAssert(it[3] == data[1]);
		ctpAssert(it[4] == data[2]);
		ctpAssert(it[5] == data[DataSize - 3]);
		ctpAssert(it[6] == data[DataSize - 2]);
		ctpAssert(it[7] == data[DataSize - 1]);
		ctpAssert(it[8] == data[DataSize - 2]);
		ctpAssert(it[9] == data[DataSize - 1]);
	}

	// Insert via initializer list.
	{
		container_type container;

		auto it = container.insert(container.begin(), std::initializer_list<T>{});

		ctpAssert(container.size() == 0);
		ctpAssert(it == container.end());

		it = container.insert(container.end(), std::initializer_list<T>{data[0], data[1], data[2]});

		ctpAssert(container.size() == 3);
		ctpAssert(it == container.begin());

		it = container.insert(container.begin(), std::initializer_list<T>{data[0], data[1]});

		ctpAssert(container.size() == 5);
		ctpAssert(it == container.begin());

		it = container.insert(container.end(), std::initializer_list<T>{*(data.end() - 2), * (data.end() - 1)});

		ctpAssert(container.size() == 7);
		ctpAssert(it == container.end() - 2);

		// Inserting near the end, where we will overlap it.
		it = container.insert(
			container.end() - 2,
			std::initializer_list<T>{*(data.end() - 3), * (data.end() - 2), * (data.end() - 1)});

		ctpAssert(container.size() == 10);
		ctpAssert(it == container.end() - 2 - 3); // -2 for insert pos, -3 for num inserted.

		it = container.begin();
		ctpAssert(it[0] == data[0]);
		ctpAssert(it[1] == data[1]);
		ctpAssert(it[2] == data[0]);
		ctpAssert(it[3] == data[1]);
		ctpAssert(it[4] == data[2]);
		ctpAssert(it[5] == data[DataSize - 3]);
		ctpAssert(it[6] == data[DataSize - 2]);
		ctpAssert(it[7] == data[DataSize - 1]);
		ctpAssert(it[8] == data[DataSize - 2]);
		ctpAssert(it[9] == data[DataSize - 1]);
	}

	// Insert via range.
	{
		container_type container;
		container_type range(5, data[0]);

		auto it = container.insert_range(container.begin(), range);

		ctpAssert(container.size() == 5);
		ctpAssert(range.size() == 5);
		ctpAssert(it == container.begin());

		ctpAssert(range[0] == data[0]);
		ctpAssert(container[0] == data[0]);
	}

	if constexpr (container_type::HasLargeMode) {
		// Switching to large with no pre-exiting items.
		{
			container_type container;

			// No pre-existing items.
			auto it = container.insert(container.begin(), 11, data[0]);

			ctpAssert(container.size() == 11);
			ctpAssert(it == container.begin());

			for (const auto& item : container)
				ctpAssert(item == data[0]);

			// Inserting an extra element in large mode.

			it = container.insert(container.end(), 1, data[1]);

			ctpAssert(container.size() == 12);
			ctpAssert(it == container.end() - 1);

			for (typename ContainerOptions::large_size_type i = 0; i < 11; ++i)
				ctpAssert(container[i] == data[0]);
			ctpAssert(container[11] == data[1]);
		}

		// Switching to large mode from a container with existing elements.
		// Switch via insert to begin.
		{
			container_type container;

			container.insert(container.begin(), 5, data[0]);
			container.insert(container.begin(), 4, data[1]);
			container.insert(container.begin(), 2, data[2]); // switch

			ctpAssert(container[0] == data[2]);
			ctpAssert(container[1] == data[2]);
			for (typename ContainerOptions::large_size_type i = 2; i < 6; ++i)
				ctpAssert(container[i] == data[1]);
			for (typename ContainerOptions::large_size_type i = 6; i < 11; ++i)
				ctpAssert(container[i] == data[0]);
		}

		// Switching to large mode from a container with existing elements.
		// Switch via insert to middle.
		{
			container_type container;

			container.insert(container.begin(), 5, data[0]);
			container.insert(container.begin(), 4, data[1]);
			container.insert(container.begin() + 4, 2, data[2]); // switch

			for (typename ContainerOptions::large_size_type i = 0; i < 4; ++i)
				ctpAssert(container[i] == data[1]);
			ctpAssert(container[4] == data[2]);
			ctpAssert(container[5] == data[2]);
			for (typename ContainerOptions::large_size_type i = 6; i < 11; ++i)
				ctpAssert(container[i] == data[0]);
		}

		// Switching to large mode from a container with existing elements.
		// Switch via insert to end.
		{
			container_type container;

			container.insert(container.begin(), 5, data[0]);
			container.insert(container.begin(), 4, data[1]);
			container.insert(container.end(), 2, data[2]); // switch

			for (typename ContainerOptions::large_size_type i = 0; i < 4; ++i)
				ctpAssert(container[i] == data[1]);
			for (typename ContainerOptions::large_size_type i = 4; i < 9; ++i)
				ctpAssert(container[i] == data[0]);
			ctpAssert(container[9] == data[2]);
			ctpAssert(container[10] == data[2]);
		}

		// Inserting when container is exactly full in small mode.
		{
			container_type container;

			container.insert(container.begin(), 5, data[0]);
			container.insert(container.begin(), 5, data[1]);
			container.insert(container.end(), 2, data[2]); // switch

			for (typename ContainerOptions::large_size_type i = 0; i < 5; ++i)
				ctpAssert(container[i] == data[1]);
			for (typename ContainerOptions::large_size_type i = 5; i < 10; ++i)
				ctpAssert(container[i] == data[0]);
			ctpAssert(container[10] == data[2]);
			ctpAssert(container[11] == data[2]);
		}

		// Insert that triggers a realloc in large mode.
		{
			container_type container;

			container.insert(container.begin(), 11, data[0]);
			const auto capacity = container.capacity();
			container.insert(container.begin() + 5, capacity, data[1]);

			typename ContainerOptions::large_size_type i = 0;
			for (; i < 5; ++i)
				ctpAssert(container[i] == data[0]);
			for (; i < 5 + capacity; ++i)
				ctpAssert(container[i] == data[1]);
			for (; i < 11 + capacity; ++i)
				ctpAssert(container[i] == data[0]);
		}

		//---------------------------------------------------
		// With iterators.

		const container_type data0(11, data[0]);
		const container_type data1(11, data[1]);
		const container_type data2(11, data[2]);
		const container_type data3(11, data[3]);

		// Switching to large with no pre-exiting items.
		{
			container_type container;

			// No pre-existing items.
			auto it = container.insert(container.begin(), data0.begin(), data0.end());

			ctpAssert(container.size() == 11);
			ctpAssert(it == container.begin());

			for (const auto& item : container)
				ctpAssert(item == data[0]);

			// Inserting an extra element in large mode.

			it = container.insert(container.end(), data1.begin(), data1.begin() + 1);

			ctpAssert(container.size() == 12);
			ctpAssert(it == container.end() - 1);

			for (typename ContainerOptions::large_size_type i = 0; i < 11; ++i)
				ctpAssert(container[i] == data[0]);
			ctpAssert(container[11] == data[1]);
		}

		// Switching to large mode from a container with existing elements.
		// Switch via insert to begin.
		{
			container_type container;

			container.insert(container.begin(), data0.begin(), data0.begin() + 5);
			container.insert(container.begin(), data1.begin(), data1.begin() + 4);
			container.insert(container.begin(), data2.begin(), data2.begin() + 2); // switch

			ctpAssert(container[0] == data[2]);
			ctpAssert(container[1] == data[2]);
			for (typename ContainerOptions::large_size_type i = 2; i < 6; ++i)
				ctpAssert(container[i] == data[1]);
			for (typename ContainerOptions::large_size_type i = 6; i < 11; ++i)
				ctpAssert(container[i] == data[0]);
		}

		// Switching to large mode from a container with existing elements.
		// Switch via insert to middle.
		{
			container_type container;

			container.insert(container.begin(), data0.begin(), data0.begin() + 5);
			container.insert(container.begin(), data1.begin(), data1.begin() + 4);
			container.insert(container.begin() + 4, data2.begin(), data2.begin() + 2); // switch

			for (typename ContainerOptions::large_size_type i = 0; i < 4; ++i)
				ctpAssert(container[i] == data[1]);
			ctpAssert(container[4] == data[2]);
			ctpAssert(container[5] == data[2]);
			for (typename ContainerOptions::large_size_type i = 6; i < 11; ++i)
				ctpAssert(container[i] == data[0]);
		}

		// Switching to large mode from a container with existing elements.
		// Switch via insert to end.
		{
			container_type container;

			container.insert(container.begin(), data0.begin(), data0.begin() + 5);
			container.insert(container.begin(), data1.begin(), data1.begin() + 4);
			container.insert(container.end(), data2.begin(), data2.begin() + 2); // switch

			for (typename ContainerOptions::large_size_type i = 0; i < 4; ++i)
				ctpAssert(container[i] == data[1]);
			for (typename ContainerOptions::large_size_type i = 4; i < 9; ++i)
				ctpAssert(container[i] == data[0]);
			ctpAssert(container[9] == data[2]);
			ctpAssert(container[10] == data[2]);
		}

		// Inserting when container is exactly full in small mode.
		{
			container_type container;

			container.insert(container.begin(), data0.begin(), data0.begin() + 5);
			container.insert(container.begin(), data1.begin(), data1.begin() + 5);
			container.insert(container.end(), data2.begin(), data2.begin() + 2); // switch

			for (typename ContainerOptions::large_size_type i = 0; i < 5; ++i)
				ctpAssert(container[i] == data[1]);
			for (typename ContainerOptions::large_size_type i = 5; i < 10; ++i)
				ctpAssert(container[i] == data[0]);
			ctpAssert(container[10] == data[2]);
			ctpAssert(container[11] == data[2]);
		}

		// Insert that triggers a realloc in large mode.
		{
			container_type container;

			container.insert(container.begin(), data0.begin(), data0.end());
			container.insert(container.begin() + 5, data1.begin(), data1.end());

			typename ContainerOptions::large_size_type i = 0;
			for (; i < 5; ++i)
				ctpAssert(container[i] == data[0]);
			for (; i < 5 + data1.size(); ++i)
				ctpAssert(container[i] == data[1]);
			for (; i < 11 + data1.size(); ++i)
				ctpAssert(container[i] == data[0]);
		}

		// Input iterator test.
		{
			using iterator = input_iterator_t<std::remove_pointer_t<decltype(data0.data())>>;

			container_type container;

			container.insert(container.begin(), iterator{0, data0.data()}, iterator{data0.size(), data0.data()});
			container.insert(container.begin() + 5, iterator{0, data1.data()}, iterator{data1.size(), data1.data()});

			typename ContainerOptions::large_size_type i = 0;
			for (; i < 5; ++i)
				ctpAssert(container[i] == data[0]);
			for (; i < 5 + data1.size(); ++i)
				ctpAssert(container[i] == data[1]);
			for (; i < 11 + data1.size(); ++i)
				ctpAssert(container[i] == data[0]);
		}
	}

	return true;
}

template <typename ContainerOptions>
constexpr bool EmplaceTests() {
	using int_container = ctp::small_storage::container<int, 3, std::allocator<int>, ContainerOptions>;
	using str_container = ctp::small_storage::container<std::string, 3, std::allocator<std::string>, ContainerOptions>;

	// Emplace internally uses insert, so only need a few tests.

	// emplace a few things with an int container
	{
		int_container container;

		auto it = container.emplace(container.begin(), 5);

		ctpAssert(container.size() == 1);
		ctpAssert(container[0] == 5);
		ctpAssert(it == container.begin());

		it = container.emplace(container.end(), 6);

		ctpAssert(container.size() == 2);
		ctpAssert(container[0] == 5);
		ctpAssert(container[1] == 6);
		ctpAssert(it == container.last());

		it = container.emplace(container.begin(), 4);

		ctpAssert(container.size() == 3);
		ctpAssert(container[0] == 4);
		ctpAssert(container[1] == 5);
		ctpAssert(container[2] == 6);
		ctpAssert(it == container.begin());

		if constexpr (container.HasLargeMode) {
			// switch to large mode
			it = container.emplace(container.begin() + 1, 8);

			ctpAssert(container.size() == 4);
			ctpAssert(container[0] == 4);
			ctpAssert(container[1] == 8);
			ctpAssert(container[2] == 5);
			ctpAssert(container[3] == 6);
			ctpAssert(it == container.begin() + 1);

			// in large mode.
			it = container.emplace(container.begin() + 1, 7);

			ctpAssert(container.size() == 5);
			ctpAssert(container[0] == 4);
			ctpAssert(container[1] == 7);
			ctpAssert(container[2] == 8);
			ctpAssert(container[3] == 5);
			ctpAssert(container[4] == 6);
			ctpAssert(it == container.begin() + 1);
		}
	}

	{
		str_container container;

		auto it = container.emplace(container.begin(), "five");

		ctpAssert(container.size() == 1);
		ctpAssert(container[0] == "five"sv);
		ctpAssert(it == container.begin());

		it = container.emplace(container.end(), "six"sv);

		ctpAssert(container.size() == 2);
		ctpAssert(container[0] == "five"sv);
		ctpAssert(container[1] == "six"sv);
		ctpAssert(it == container.last());

		{
			std::string long_moved = "fouuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuur";
			it = container.emplace(container.begin(), std::move(long_moved));
			ctpAssert(long_moved.empty());
		}

		ctpAssert(container.size() == 3);
		ctpAssert(container[0] == "fouuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuur"sv);
		ctpAssert(container[1] == "five"sv);
		ctpAssert(container[2] == "six"sv);
		ctpAssert(it == container.begin());

		if constexpr (container.HasLargeMode) {
			{
				std::string copied = "eiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiight";
				// switch to large mode
				it = container.emplace(container.begin() + 1, copied);
				ctpAssert(copied == "eiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiight"sv);
			}

			ctpAssert(container.size() == 4);
			ctpAssert(container[0] == "fouuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuur"sv);
			ctpAssert(container[1] == "eiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiight"sv);
			ctpAssert(container[2] == "five");
			ctpAssert(container[3] == "six");
			ctpAssert(it == container.begin() + 1);

			// in large mode.
			it = container.emplace(container.begin() + 1, std::string{"seeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeven"});

			ctpAssert(container.size() == 5);
			ctpAssert(container[0] == "fouuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuur"sv);
			ctpAssert(container[1] == "seeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeven"sv);
			ctpAssert(container[2] == "eiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiight"sv);
			ctpAssert(container[3] == "five");
			ctpAssert(container[4] == "six");
			ctpAssert(it == container.begin() + 1);
		}
	}

	return true;
}

template <typename T, typename ContainerOptions, typename DataT>
constexpr bool EraseTests(const std::array<DataT, DataSize>& data) {
	using container_type = ctp::small_storage::container<T, DataSize, std::allocator<T>, ContainerOptions>;

	// Erase single items.
	{
		container_type container{data.begin(), data.end()};
		std::size_t remaining = DataSize;

		// Erase from start.
		auto it = container.erase(container.begin());

		ctpAssert(container.size() == --remaining);
		ctpAssert(it == container.begin());

		for (std::size_t i = 0; i < remaining; ++i)
			ctpAssert(container[i] == data[i + 1]);

		// Erase from end.
		it = container.erase(container.last());

		ctpAssert(container.size() == --remaining);
		ctpAssert(it == container.end());

		for (std::size_t i = 0; i < remaining; ++i)
			ctpAssert(container[i] == data[i + 1]);

		// Erase from middle.
		it = container.erase(container.begin() + 2);

		ctpAssert(container.size() == --remaining);
		ctpAssert(it == container.begin() + 2);

		for (std::size_t i = 0; i < 2; ++i)
			ctpAssert(container[i] == data[i + 1]);

		for (std::size_t i = 2; i < remaining; ++i)
			ctpAssert(container[i] == data[i + 2]);
	}

	// Erase empty range.
	{
		container_type container{data.begin(), data.end()};

		auto it = container.erase(container.end(), container.end());
		ctpAssert(container.size() == DataSize);
		ctpAssert(it == container.end());

		it = container.erase(container.begin(), container.begin());
		ctpAssert(container.size() == DataSize);
		ctpAssert(it == container.begin());

		it = container.erase(container.begin() + 2, container.begin() + 2);
		ctpAssert(container.size() == DataSize);
		ctpAssert(it == container.begin() + 2);
	}

	// Erase range from start.
	{
		container_type container{data.begin(), data.end()};

		auto it = container.erase(container.begin(), container.begin() + 3);
		ctpAssert(container.size() == DataSize - 3);
		ctpAssert(it == container.begin());

		for (std::size_t i = 0; i < DataSize - 3; ++i)
			ctpAssert(container[i] == data[i + 3]);
	}

	// Erase range from end.
	{
		container_type container{data.begin(), data.end()};

		auto it = container.erase(container.last() - 2, container.end());
		ctpAssert(container.size() == DataSize - 3);
		ctpAssert(it == container.end());

		for (std::size_t i = 0; i < DataSize - 3; ++i)
			ctpAssert(container[i] == data[i]);
	}

	// Erase range from middle.
	{
		container_type container{data.begin(), data.end()};

		auto it = container.erase(container.begin() + 2, container.begin() + 5);
		ctpAssert(container.size() == DataSize - 3);
		ctpAssert(it == container.begin() + 2);

		for (std::size_t i = 0; i < 2; ++i)
			ctpAssert(container[i] == data[i]);
		for (std::size_t i = 2; i < DataSize - 3; ++i)
			ctpAssert(container[i] == data[i + 3]);
	}

	// Erase everything.
	{
		container_type container{data.begin(), data.end()};

		auto it = container.erase(container.begin(), container.end());
		ctpAssert(container.size() == 0);
		ctpAssert(it == container.end());
	}

	if constexpr (container_type::HasLargeMode) {
		// Force large mode by giving very small capacity.
		using smaller_container_type = ctp::small_storage::container<T, 1, std::allocator<T>, ContainerOptions>;

		// Erase single items.
		{
			smaller_container_type container{data.begin(), data.end()};
			std::size_t remaining = DataSize;

			// Erase from start.
			auto it = container.erase(container.begin());

			ctpAssert(container.size() == --remaining);
			ctpAssert(it == container.begin());

			for (std::size_t i = 0; i < remaining; ++i)
				ctpAssert(container[i] == data[i + 1]);

			// Erase from end.
			it = container.erase(container.last());

			ctpAssert(container.size() == --remaining);
			ctpAssert(it == container.end());

			for (std::size_t i = 0; i < remaining; ++i)
				ctpAssert(container[i] == data[i + 1]);

			// Erase from middle.
			it = container.erase(container.begin() + 2);

			ctpAssert(container.size() == --remaining);
			ctpAssert(it == container.begin() + 2);

			for (std::size_t i = 0; i < 2; ++i)
				ctpAssert(container[i] == data[i + 1]);

			for (std::size_t i = 2; i < remaining; ++i)
				ctpAssert(container[i] == data[i + 2]);
		}

		// Erase empty range.
		{
			smaller_container_type container{data.begin(), data.end()};

			auto it = container.erase(container.end(), container.end());
			ctpAssert(container.size() == DataSize);
			ctpAssert(it == container.end());

			it = container.erase(container.begin(), container.begin());
			ctpAssert(container.size() == DataSize);
			ctpAssert(it == container.begin());

			it = container.erase(container.begin() + 2, container.begin() + 2);
			ctpAssert(container.size() == DataSize);
			ctpAssert(it == container.begin() + 2);
		}

		// Erase range from start.
		{
			smaller_container_type container{data.begin(), data.end()};

			auto it = container.erase(container.begin(), container.begin() + 3);
			ctpAssert(container.size() == DataSize - 3);
			ctpAssert(it == container.begin());

			for (std::size_t i = 0; i < DataSize - 3; ++i)
				ctpAssert(container[i] == data[i + 3]);
		}

		// Erase range from end.
		{
			smaller_container_type container{data.begin(), data.end()};

			auto it = container.erase(container.last() - 2, container.end());
			ctpAssert(container.size() == DataSize - 3);
			ctpAssert(it == container.end());

			for (std::size_t i = 0; i < DataSize - 3; ++i)
				ctpAssert(container[i] == data[i]);
		}

		// Erase range from middle.
		{
			smaller_container_type container{data.begin(), data.end()};

			auto it = container.erase(container.begin() + 2, container.begin() + 5);
			ctpAssert(container.size() == DataSize - 3);
			ctpAssert(it == container.begin() + 2);

			for (std::size_t i = 0; i < 2; ++i)
				ctpAssert(container[i] == data[i]);
			for (std::size_t i = 2; i < DataSize - 3; ++i)
				ctpAssert(container[i] == data[i + 3]);
		}

		// Erase everything.
		{
			smaller_container_type container{data.begin(), data.end()};

			auto it = container.erase(container.begin(), container.end());
			ctpAssert(container.size() == 0);
			ctpAssert(it == container.end());
		}
	}

	return true;
}

template <typename T, typename ContainerOptions, typename DataT>
constexpr bool AssignTests(const std::array<DataT, DataSize>& data) {
	using container_type = ctp::small_storage::container<T, 10, std::allocator<T>, ContainerOptions>;

	static_assert(container_type::SmallCapacity == 10);

	{
		container_type container;

		// Assign from empty.
		container.assign(5, data[0]);

		ctpAssert(container.size() == 5);
		for (const auto& item : container)
			ctpAssert(item == data[0]);

		// Assign to smaller amount
		container.assign(3, data[1]);

		ctpAssert(container.size() == 3);
		for (const auto& item : container)
			ctpAssert(item == data[1]);

		// Assign to a larger amount
		container.assign(6, data[2]);

		ctpAssert(container.size() == 6);
		for (const auto& item : container)
			ctpAssert(item == data[2]);

		// Assign to full.
		container.assign(10, data[3]);

		ctpAssert(container.size() == 10);
		for (const auto& item : container)
			ctpAssert(item == data[3]);

		// Assign to empty
		container.assign(0, data[3]);
		ctpAssert(container.size() == 0);

		if constexpr (container.HasLargeMode) {
			// Switch to large mode.

			ctpAssert(container.size() == 0);
			ctpAssert(container.capacity() == 10);

			container.assign(container_type::SmallCapacity + 1, data[4]);
			ctpAssert(container.size() == container_type::SmallCapacity + 1);
			for (const auto& item : container)
				ctpAssert(item == data[4]);

			const auto capacity = container.capacity();
			// Assign allocates for exact fit.
			ctpAssert(capacity == container_type::SmallCapacity + 1);

			// Assigning smaller does not switch back to small mode.
			container.assign(5, data[5]);
			ctpAssert(container.size() == 5);
			ctpAssert(container.capacity() == capacity);
			for (const auto& item : container)
				ctpAssert(item == data[5]);

			// Realloc
			container.assign(capacity + 1, data[6]);
			ctpAssert(container.size() == capacity + 1);
			for (const auto& item : container)
				ctpAssert(item == data[6]);
		}
	}

	// Iterator tests.
	{
		container_type container;

		// Assign from empty.
		container.assign(data.begin(), data.begin() + 5);

		ctpAssert(container.size() == 5);
		for (std::size_t i = 0; i < 5; ++i)
			ctpAssert(container[i] == data[i]);

		// Assign to smaller amount
		container.assign(data.begin(), data.begin() + 3);

		ctpAssert(container.size() == 3);
		for (std::size_t i = 0; i < 3; ++i)
			ctpAssert(container[i] == data[i]);

		// Assign to a larger amount
		container.assign(data.begin(), data.begin() + 6);

		ctpAssert(container.size() == 6);
		for (std::size_t i = 0; i < 6; ++i)
			ctpAssert(container[i] == data[i]);

		using loop_iterator = loop_iterator_t<const DataT>;
		static_assert(std::forward_iterator<loop_iterator>);
		const auto it = loop_iterator{0, data.size(), data.data()};

		// Assign to full.
		container.assign(it, it + 10);

		ctpAssert(container.size() == 10);
		for (std::size_t i = 0; i < 10; ++i)
			ctpAssert(container[i] == data[i % data.size()]);

		// Assign to empty
		container.assign(data.begin(), data.begin());
		ctpAssert(container.size() == 0);

		if constexpr (container.HasLargeMode) {
			// Switch to large mode.

			ctpAssert(container.size() == 0);
			ctpAssert(container.capacity() == 10);

			container.assign(it, it + (container_type::SmallCapacity + 1));
			ctpAssert(container.size() == container_type::SmallCapacity + 1);
			for (std::size_t i = 0; i < container_type::SmallCapacity + 1; ++i)
				ctpAssert(container[i] == data[i % data.size()]);

			const auto capacity = container.capacity();
			// Assign allocates for exact fit.
			ctpAssert(capacity == container_type::SmallCapacity + 1);

			// Assigning smaller does not switch back to small mode.
			container.assign(data.begin(), data.begin() + 5);
			ctpAssert(container.size() == 5);
			ctpAssert(container.capacity() == capacity);
			for (std::size_t i = 0; i < 5; ++i)
				ctpAssert(container[i] == data[i]);

			// Realloc
			container.assign(it, it + (capacity + 1));
			ctpAssert(container.size() == capacity + 1);
			for (std::size_t i = 0; i < capacity + 1; ++i)
				ctpAssert(container[i] == data[i % data.size()]);
		}
	}

	// Input iterator test.
	{
		using iterator = input_iterator_t<const DataT>;
		static_assert(std::input_iterator<iterator>);
		static_assert(!std::forward_iterator<iterator>);

		container_type container;
		container.assign(iterator{0, data.data()}, iterator{data.size(), data.data()});

		ctpAssert(container.size() == data.size());
		for (std::size_t i = 0; i < container.size(); ++i)
			ctpAssert(container[i] == data[i]);
	}

	// Assign via range.
	{
		container_type container;
		container_type range(5, data[0]);

		container.assign_range(range);

		ctpAssert(container.size() == 5);
		ctpAssert(range.size() == 5);

		ctpAssert(range[0] == data[0]);
		ctpAssert(container[0] == data[0]);
	}

	// Move-assign via range. Same as assign.
	{
		container_type container;
		container_type range(5, data[0]);

		container.assign_range(std::move(range));

		ctpAssert(container.size() == 5);
		ctpAssert(range.size() == 5);
		ctpAssert(container[0] == data[0]);
	}

	return true;
}

template <typename T, typename ContainerOptions, typename DataT>
constexpr bool CompareTests(std::array<DataT, DataSize> data) {
	// Sort for comparison purposes.
	std::sort(data.begin(), data.end());

	// Comparisons with empty.
	{
		using container_type = ctp::small_storage::container<T, 10, std::allocator<T>, ContainerOptions>;

		const container_type a, b;

		ctpAssert(a == b);
		ctpAssert(a <= b);
		ctpAssert(a >= b);
		ctpAssert(!(a < b));
		ctpAssert(!(a > b));
		ctpAssert(!(a != b));
	}

	{
		using container_type = ctp::small_storage::container<T, 10, std::allocator<T>, ContainerOptions>;

		const auto a = container_type{data[1], data[2], data[3], data[4], data[5]};
		auto b = container_type{data[1], data[2], data[3], data[4], data[5]};

		ctpAssert(a == b);
		ctpAssert(a <= b);
		ctpAssert(a >= b);
		ctpAssert(!(a < b));
		ctpAssert(!(a > b));
		ctpAssert(!(a != b));

		b.push_back(data[6]);

		ctpAssert(a != b);
		ctpAssert(a <= b);
		ctpAssert(!(a >= b));
		ctpAssert(a < b);
		ctpAssert(!(a > b));
		ctpAssert(!(a == b));
	}

	return true;
}

// Small mode only.
using small_only_constexpr = ctp::small_storage::container<int, 2, std::allocator<int>, small_only_options>;
constexpr auto RunBasicSmallTests1 = BasicSmallTests<small_only_constexpr>();

using small_only_non_constexpr = ctp::small_storage::container<int, 2, std::allocator<int>, small_only_non_constexpr_options>;
constexpr auto RunBasicSmallTests2 = BasicSmallTests<small_only_non_constexpr>(); // Still constexpr friendly for trivial T.

using small_only_constexpr_non_trivial = ctp::small_storage::container<std::string, 2, std::allocator<std::string>, small_only_options>;
constexpr auto RunBasicSmallTests3 = BasicSmallTestsNonTrivial<small_only_constexpr_non_trivial>();


// Large mode.
using large_constexpr = ctp::small_storage::container<int, 2, std::allocator<int>, can_grow_options>;
constexpr auto RunBasicLargeTests1 = BasicLargeTests<large_constexpr>();

using large_non_constexpr = ctp::small_storage::container<int, 2, std::allocator<int>, can_grow_non_constexpr_options>;
constexpr auto RunBasicLargeTests2 = BasicLargeTests<large_non_constexpr>(); // Still constexpr friendly for trivial T.

using large_constexpr_non_trivial =
ctp::small_storage::container<std::string, 2, std::allocator<std::string>, can_grow_options>;
constexpr auto RunBasicLargeTestsNonTrivial1 = BasicLargeTestsNonTrivial<large_constexpr_non_trivial>();

using large_non_constexpr_non_trivial =
ctp::small_storage::container<std::string, 2, std::allocator<std::string>, can_grow_non_constexpr_options>;
constexpr auto RunBasicLargeTestsNontrivial2 = BasicLargeTestsNonTrivial<large_non_constexpr_non_trivial>();


constexpr auto RunResizeTests =
ResizeTests<int, small_only_options>(IntData) &&
ResizeTests<int, can_grow_options>(IntData) &&
ResizeTests<std::string, small_only_options>(StringData) &&
ResizeTests<std::string, can_grow_options>(StringData) &&
ResizeTests<std::string, small_only_non_constexpr_options>(StringData) &&
ResizeTests<std::string, can_grow_non_constexpr_options>(StringData);

constexpr auto RunShrinkToFitTests =
ShrinkToFitTests<int, small_only_options>(IntData) &&
ShrinkToFitTests<int, can_grow_options>(IntData) &&
ShrinkToFitTests<std::string, small_only_options>(StringData) &&
ShrinkToFitTests<std::string, can_grow_options>(StringData) &&
ShrinkToFitTests<std::string, small_only_non_constexpr_options>(StringData) &&
ShrinkToFitTests<std::string, can_grow_non_constexpr_options>(StringData);

constexpr auto RunInsertTests =
InsertTests<int, small_only_options>(IntData) &&
InsertTests<int, can_grow_options>(IntData) &&
InsertTests<std::string, small_only_options>(StringData) &&
InsertTests<std::string, can_grow_options>(StringData) &&
InsertTests<std::string, small_only_non_constexpr_options>(StringData) &&
InsertTests<std::string, can_grow_non_constexpr_options>(StringData);

constexpr auto RunEmplaceTests =
EmplaceTests<small_only_options>() &&
EmplaceTests<can_grow_options>() &&
EmplaceTests<small_only_non_constexpr_options>() &&
EmplaceTests<can_grow_non_constexpr_options>();

constexpr auto RunEraseTests =
EraseTests<int, small_only_options>(IntData) &&
EraseTests<int, can_grow_options>(IntData) &&
EraseTests<std::string, small_only_options>(StringData) &&
EraseTests<std::string, can_grow_options>(StringData) &&
EraseTests<std::string, small_only_non_constexpr_options>(StringData) &&
EraseTests<std::string, can_grow_non_constexpr_options>(StringData);

constexpr auto RunAssignTests =
AssignTests<int, small_only_options>(IntData) &&
AssignTests<int, can_grow_options>(IntData) &&
AssignTests<std::string, small_only_options>(StringData) &&
AssignTests<std::string, can_grow_options>(StringData) &&
AssignTests<std::string, small_only_non_constexpr_options>(StringData) &&
AssignTests<std::string, can_grow_non_constexpr_options>(StringData);

constexpr auto RunCompareTests =
CompareTests<int, small_only_options>(IntData) &&
CompareTests<int, can_grow_options>(IntData) &&
CompareTests<std::string, small_only_options>(StringData) &&
CompareTests<std::string, can_grow_options>(StringData) &&
CompareTests<std::string, small_only_non_constexpr_options>(StringData) &&
CompareTests<std::string, can_grow_non_constexpr_options>(StringData);

} // namespace

TEST_CASE("Basic tests.", "[Tools][small_storage]") {
	using namespace ctp::small_storage;

	BasicSmallTests<small_only_constexpr>();
	BasicSmallTests<small_only_non_constexpr>();
	BasicSmallTestsNonTrivial<small_only_constexpr_non_trivial>();

	BasicLargeTests<large_constexpr>();
	BasicLargeTests<large_non_constexpr>();
	BasicLargeTestsNonTrivial<large_constexpr_non_trivial>();
	BasicLargeTestsNonTrivial<large_non_constexpr_non_trivial>();

	ResizeTests<int, small_only_options>(IntData);
	ResizeTests<int, can_grow_options>(IntData);
	ResizeTests<std::string, small_only_options>(StringData);
	ResizeTests<std::string, can_grow_options>(StringData);
	ResizeTests<std::string, small_only_non_constexpr_options>(StringData);
	ResizeTests<std::string, can_grow_non_constexpr_options>(StringData);

	ShrinkToFitTests<int, small_only_options>(IntData);
	ShrinkToFitTests<int, can_grow_options>(IntData);
	ShrinkToFitTests<std::string, small_only_options>(StringData);
	ShrinkToFitTests<std::string, can_grow_options>(StringData);
	ShrinkToFitTests<std::string, small_only_non_constexpr_options>(StringData);
	ShrinkToFitTests<std::string, can_grow_non_constexpr_options>(StringData);

	InsertTests<int, small_only_options>(IntData);
	InsertTests<int, can_grow_options>(IntData);
	InsertTests<std::string, small_only_options>(StringData);
	InsertTests<std::string, can_grow_options>(StringData);
	InsertTests<std::string, small_only_non_constexpr_options>(StringData);
	InsertTests<std::string, can_grow_non_constexpr_options>(StringData);

	EmplaceTests<small_only_options>();
	EmplaceTests<can_grow_options>();
	EmplaceTests<small_only_non_constexpr_options>();
	EmplaceTests<can_grow_non_constexpr_options>();

	EraseTests<int, small_only_options>(IntData);
	EraseTests<int, can_grow_options>(IntData);
	EraseTests<std::string, small_only_options>(StringData);
	EraseTests<std::string, can_grow_options>(StringData);
	EraseTests<std::string, small_only_non_constexpr_options>(StringData);
	EraseTests<std::string, can_grow_non_constexpr_options>(StringData);

	AssignTests<int, small_only_options>(IntData);
	AssignTests<int, can_grow_options>(IntData);
	AssignTests<std::string, small_only_options>(StringData);
	AssignTests<std::string, can_grow_options>(StringData);
	AssignTests<std::string, small_only_non_constexpr_options>(StringData);
	AssignTests<std::string, can_grow_non_constexpr_options>(StringData);

	CompareTests<int, small_only_options>(IntData);
	CompareTests<int, can_grow_options>(IntData);
	CompareTests<std::string, small_only_options>(StringData);
	CompareTests<std::string, can_grow_options>(StringData);
	CompareTests<std::string, small_only_non_constexpr_options>(StringData);
	CompareTests<std::string, can_grow_non_constexpr_options>(StringData);


	// TODO
	// for equality test, ensure I can both A <=> B and B <=> A with different small vector sizes.
	// in case I am accidentally generating ambiguous templates

	small_only_non_constexpr container;
	container.push_back(0);
	CHECK(container.at(0) == 0);
	container.at(0) = 1;
	CHECK(container.at(0) == 1);
	CHECK(const_cast<const small_only_non_constexpr&>(container).at(0) == 1);

#if CTP_USE_EXCEPTIONS
	CHECK_NOTHROW(container.at(0));
	CHECK_THROWS(container.at(1));
	CHECK_THROWS(container.at(2));
#endif
}

TEST_CASE("Small container small size with non-default constructible type.", "[Tools][small_storage]") {
	struct nondefault {
		int i;
		constexpr nondefault(int in) noexcept : i{in} {}
	};

	using container_type = ctp::small_storage::container<nondefault, 10, std::allocator<nondefault>, small_only_options>;

	auto test = [] {
		container_type container;

		container.emplace_back(1);
		container.emplace_back(2);
		container.emplace_back(3);

		// Test const_iterator to nonconst iterator conversion support for uninitialized_item_iterator.
		container.emplace(container.begin(), 7);

		CTP_CHECK(container.size() == 4);

		CTP_CHECK(container[0].i == 7);
		CTP_CHECK(container[1].i == 1);
		CTP_CHECK(container[2].i == 2);
		CTP_CHECK(container[3].i == 3);


		return true;
	};
	[[maybe_unused]] static constexpr bool RunConstexpr = test();
	test();
}
