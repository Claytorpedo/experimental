#ifndef INCLUDE_CTP_TOOLS_RANGES_ERASE_HPP
#define INCLUDE_CTP_TOOLS_RANGES_ERASE_HPP

#include "remove.hpp"
#include <Tools/utility.hpp>

namespace ctp {

struct stable_erase_fn {
	template<std::ranges::forward_range Range, typename Value, typename Projection = identity>
		requires
			std::permutable<std::ranges::iterator_t<Range>> &&
			std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<Range>, Projection>, const Value*>
	constexpr auto operator()(Range&& range, const Value& value, Projection projection = Projection{}) const -> typename std::decay_t<Range>::size_type
	{
		auto [ret, last] = stable_remove(range, value, move(projection));
		const auto dist = static_cast<typename std::decay_t<Range>::size_type>(std::distance(ret, last));
		range.erase(move(ret), move(last));
		return dist;
	}
};
struct stable_erase_if_fn {
	template<std::ranges::forward_range Range,
		typename Projection = identity,
		std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Range>, Projection>> Predicate>
	constexpr auto operator()(Range&& range, Predicate&& predicate, Projection projection = Projection{}) const -> typename std::decay_t<Range>::size_type
	{
		auto [ret, last] = stable_remove_if(range, forward<Predicate>(predicate), move(projection));
		const auto dist = static_cast<typename std::decay_t<Range>::size_type>(std::distance(ret, last));
		range.erase(move(ret), move(last));
		return dist;
	}
};

struct unstable_erase_fn {
	template<std::ranges::bidirectional_range Range, typename Value, typename Projection = identity>
		requires
			std::permutable<std::ranges::iterator_t<Range>> &&
			std::ranges::common_range<Range> &&
			std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<Range>, Projection>, const Value*>
	constexpr auto operator()(Range&& range, const Value& value, Projection projection = Projection{}) const -> typename std::decay_t<Range>::size_type
	{
		auto it = unstable_remove(range, value, move(projection));
		auto end = std::end(range);
		const auto dist = static_cast<typename std::decay_t<Range>::size_type>(std::distance(it, end));
		range.erase(move(it), move(end));
		return dist;
	}
};
struct unstable_erase_if_fn {
	template<std::ranges::bidirectional_range Range,
			typename Projection = identity,
			std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Range>, Projection>> Predicate>
		requires std::permutable<std::ranges::iterator_t<Range>> && std::ranges::common_range<Range>
	constexpr auto operator()(Range&& range, Predicate&& predicate, Projection projection = Projection{}) const -> typename std::decay_t<Range>::size_type
	{
		auto it = unstable_remove_if(range, forward<Predicate>(predicate), move(projection));
		auto end = std::end(range);
		const auto dist = static_cast<typename std::decay_t<Range>::size_type>(std::distance(it, end));
		range.erase(move(it), move(end));
		return dist;
	}
};

inline constexpr stable_erase_fn stable_erase{};
inline constexpr stable_erase_if_fn stable_erase_if{};

inline constexpr unstable_erase_fn unstable_erase{};
inline constexpr unstable_erase_if_fn unstable_erase_if{};

inline constexpr unstable_erase_fn erase{};
inline constexpr unstable_erase_if_fn erase_if{};

} // ctp

#endif // INCLUDE_CTP_TOOLS_RANGES_ERASE_HPP
