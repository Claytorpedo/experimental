#ifndef INCLUDE_CTP_TOOLS_TEST_SMALL_STORAGE_HELPERS_HPP
#define INCLUDE_CTP_TOOLS_TEST_SMALL_STORAGE_HELPERS_HPP

#include <Tools/config.hpp>
#include <Tools/small_storage.hpp>

#include <array>

// Option sets.

struct small_only_options : ctp::small_storage::default_options {
	using growth_policy = typename ctp::small_storage::medium_growth_policy;
	using large_size_type = std::size_t;
	static constexpr bool has_large_mode = false;
	static constexpr bool force_constexpr_friendliness = true;
	static constexpr bool allow_default_construction_in_constant_expressions = true;
};

struct small_only_non_constexpr_options : ctp::small_storage::default_options {
	using growth_policy = typename ctp::small_storage::medium_growth_policy;
	using large_size_type = std::size_t;
	static constexpr bool has_large_mode = false;
	static constexpr bool force_constexpr_friendliness = false;
	static constexpr bool allow_default_construction_in_constant_expressions = true;
};

struct can_grow_options : ctp::small_storage::default_options {
	using growth_policy = typename ctp::small_storage::medium_growth_policy;
	using large_size_type = std::size_t;
	static constexpr bool has_large_mode = true;
	static constexpr bool force_constexpr_friendliness = true;
	static constexpr bool allow_default_construction_in_constant_expressions = true;
};

struct can_grow_non_constexpr_options : ctp::small_storage::default_options {
	using growth_policy = typename ctp::small_storage::medium_growth_policy;
	using large_size_type = std::size_t;
	static constexpr bool has_large_mode = true;
	static constexpr bool force_constexpr_friendliness = false;
	static constexpr bool allow_default_construction_in_constant_expressions = true;
};

// Data sets

class NoDefaultConstruct {
	int i_;
public:
	NoDefaultConstruct() = delete;
	constexpr NoDefaultConstruct(int i) noexcept : i_{i} {}
	constexpr auto operator<=>(const NoDefaultConstruct&) const noexcept = default;
};

constexpr std::size_t DataSize = 7;
constexpr auto IntData = std::array<int, DataSize>{0, 1, 2, 3, 4, 5, 6};
constexpr auto StringData = std::array<const char*, DataSize>{
	"very long string to avoid small string optimization",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six"};
constexpr auto NoDefaultConstructData = std::array<NoDefaultConstruct, DataSize>{0, 1, 2, 3, 4, 5, 6};

// Allocators.

// Object that allocates some storage. Can be used for testing number of allocation-related operations
// being performed.
template <typename Alloc = std::allocator<int>>
class allocating_object {
	using rebind_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<int>;
	using alloc_traits = std::allocator_traits<rebind_alloc>;
	CTP_NO_UNIQUE_ADDRESS rebind_alloc alloc_;
	int* pointer_ = nullptr;

public:
	using allocator_type = Alloc;

	allocating_object() {
		pointer_ = alloc_traits::allocate(alloc_, 1);
	}
	allocating_object(const Alloc& alloc) : alloc_{alloc} {
		pointer_ = alloc_traits::allocate(alloc_, 1);
	}
	~allocating_object() {
		if (pointer_)
			alloc_traits::deallocate(alloc_, pointer_, 1);
	}

	// Copy

	allocating_object(const allocating_object& o)
		: alloc_{alloc_traits::select_on_container_copy_construction(o.alloc_)}
		, pointer_{alloc_traits::allocate(alloc_, 1)}
	{}

	allocating_object& operator=(const allocating_object& o) {
		if (pointer_)
			alloc_traits::deallocate(alloc_, pointer_, 1);

		if constexpr (alloc_traits::propagate_on_container_copy_assignment::value) {
			alloc_ = o.alloc_;
		}

		pointer_ = alloc_traits::allocate(alloc_, 1);
		return *this;
	}

	allocating_object(const allocating_object& o, const Alloc& alloc)
		: alloc_{alloc}
		, pointer_{alloc_traits::allocate(alloc_, 1)}
	{}

	// Move

	allocating_object(allocating_object&& o)
		: alloc_{std::move(o.alloc_)}
		, pointer_{std::exchange(o.pointer_, nullptr)}
	{}

	allocating_object& operator=(allocating_object&& o) {
		if (pointer_)
			alloc_traits::deallocate(alloc_, pointer_, 1);
		alloc_ = std::move(o.alloc_);
		pointer_ = std::exchange(o.pointer_, nullptr);
		return *this;
	}

	allocating_object(allocating_object&& o, const Alloc& alloc)
		: alloc_{alloc}
		, pointer_(std::exchange(o.pointer_, nullptr))
	{}
};

struct allocator_stats {
	int constructions = 0;
	int destructions = 0;
	int allocations = 0;
	int deletions = 0;
	int allocator_copies = 0;
	int allocator_moves = 0;
};

template <typename T, class Alloc = std::allocator<T>>
// Inherit for default trait definitions.
struct tracker_alloc {
	using value_type = T;

	allocator_stats* stats = nullptr;
	Alloc alloc;

	static constexpr tracker_alloc select_on_container_copy_construction(const tracker_alloc& o) noexcept {
		return o;
	}

	friend constexpr bool operator==(const tracker_alloc&, const tracker_alloc&) noexcept {
		return true; // Rhs can always deallocate Lhs's memory.
	}
	friend constexpr bool operator!=(const tracker_alloc&, const tracker_alloc&) noexcept {
		return false; // Rhs can always deallocate Lhs's memory.
	}

	constexpr tracker_alloc() noexcept = default;
	constexpr tracker_alloc(allocator_stats* ptr) noexcept : stats{ptr} {}
	constexpr tracker_alloc(tracker_alloc&& o) noexcept : stats{o.stats} {
		if (stats)
			++stats->allocator_moves;
	}
	constexpr tracker_alloc& operator=(tracker_alloc&& o) noexcept {
		stats = o.stats;
		if (stats)
			++stats->allocator_moves;
		return *this;
	}
	constexpr tracker_alloc(const tracker_alloc& o) noexcept : stats{o.stats} {
		if (stats)
			++stats->allocator_copies;
	}
	constexpr tracker_alloc& operator=(const tracker_alloc& o) noexcept {
		stats = o.stats;
		if (stats)
			++stats->allocator_copies;
		return *this;
	}

	template <typename U>
	constexpr tracker_alloc(const tracker_alloc<U>& o) noexcept : stats{o.stats} {
		if (stats)
			++stats->allocator_copies;
	}

	template <typename U>
	constexpr tracker_alloc(tracker_alloc<U>&& o) noexcept : stats{o.stats} {
		if (stats)
			++stats->allocator_moves;
	}

	constexpr auto allocate(std::size_t count) noexcept {
		if (stats)
			++stats->allocations;
		return std::allocator_traits<Alloc>::allocate(alloc, count);
	}

#ifdef __cpp_lib_allocate_at_least
	constexpr auto allocate_at_least(std::size_t count) noexcept {
		if (stats)
			++stats->allocations;
		return std::allocator_traits<Alloc>::allocate_at_least(alloc, count);
	}
#endif

	constexpr void deallocate(T* ptr, std::size_t count) noexcept {
		if (stats)
			++stats->deletions;

		std::allocator_traits<Alloc>::deallocate(alloc, ptr, count);
	}

	template <typename U, typename... Args>
	constexpr void construct(U* ptr, Args&&... args) {
		if (stats)
			++stats->constructions;

		std::allocator_traits<Alloc>::construct(alloc, ptr, std::forward<Args>(args)...);
	}

	template <typename U>
	constexpr void destroy(U* ptr) {
		if (stats)
			++stats->destructions;

		std::allocator_traits<Alloc>::destroy(alloc, ptr);
	}
};

namespace std {

template <typename T>
struct allocator_traits<tracker_alloc<T>> { // traits for std::allocator_type
	using propagate_on_container_copy_assignment = std::false_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_swap = std::false_type;
	using is_always_equal = std::false_type;

	using allocator_type = tracker_alloc<T>;
	using value_type = typename allocator_type::value_type;

	using pointer = value_type*;
	using const_pointer = const value_type*;
	using void_pointer = void*;
	using const_void_pointer = const void*;

	using size_type = size_t;
	using difference_type = ptrdiff_t;

	template <class Other>
	using rebind_alloc = tracker_alloc<Other>;

	template <class Other>
	using rebind_traits = allocator_traits<tracker_alloc<Other>>;

	static constexpr pointer allocate(allocator_type& alloc, const size_type count) {
		return alloc.allocate(count);
	}

	static constexpr pointer allocate(allocator_type& alloc, const size_type count, const_void_pointer) {
		return alloc.allocate(count);
	}

#ifdef __cpp_lib_allocate_at_least
	static constexpr allocation_result<pointer, size_type> allocate_at_least(allocator_type& alloc, const size_type count) {
		return alloc.allocate_at_least(count);
	}
#endif

	static constexpr void deallocate(allocator_type& alloc, const pointer ptr, const size_type count) {
		alloc.deallocate(ptr, count);
	}

	template <class U, class... Args>
	static constexpr void construct(allocator_type& alloc, U* const ptr, Args&&... args) {
		alloc.construct(ptr, std::forward<Args>(args)...);
	}

	template <class U>
	static constexpr void destroy(allocator_type& alloc, U* const ptr) {
		alloc.destroy(ptr);
	}

	static constexpr size_type max_size(const allocator_type&) noexcept {
		return static_cast<size_t>(-1) / sizeof(value_type);
	}

	static constexpr allocator_type select_on_container_copy_construction(const allocator_type& alloc) {
		return alloc;
	}
};

} // std

#endif //INCLUDE_CTP_TOOLS_TEST_SMALL_STORAGE_HELPERS_HPP
