#ifndef INCLUDE_CTP_TOOLS_RANGES_REMOVE_HPP
#define INCLUDE_CTP_TOOLS_RANGES_REMOVE_HPP

#include <Tools/utility.hpp>

#include <algorithm>
#include <concepts>
#include <iterator>

namespace ctp {

inline constexpr auto&& stable_remove = std::ranges::remove;
inline constexpr auto&& stable_remove_if = std::ranges::remove_if;

struct unstable_remove_fn {
	template<std::bidirectional_iterator I, typename Value, typename Projection = identity>
		requires std::permutable<I> && std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Projection>, const Value*>
	constexpr I operator()(I first, I last, const Value& value, Projection projection = Projection{}) const
	{
		for (;; std::advance(first, 1)) {
			first = std::ranges::find(move(first), last, value, std::ref(projection));

			if (first == last)
				return first;

			last = std::next(std::ranges::find_if_not(
				std::make_reverse_iterator(move(last)),
				std::make_reverse_iterator(std::next(first)),
				[&value](const auto& test) noexcept(noexcept(value == test)) { return value == test; },
				std::ref(projection)))
				.base();

			if (first == last)
				return first;

			*first = move(*last);
		}
	}
	template<std::ranges::bidirectional_range Range, typename Value, typename Projection = identity>
		requires
			std::permutable<std::ranges::iterator_t<Range>> &&
			std::ranges::common_range<Range> &&
			std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<Range>, Projection>, const Value*>
	constexpr auto operator()(Range&& range, const Value& value, Projection projection = Projection{}) const
	{
		return (*this)(std::begin(range), std::end(range), value, move(projection));
	}
};
struct unstable_remove_if_fn {
	template<std::bidirectional_iterator I,
			typename Projection = identity,
			std::indirect_unary_predicate<std::projected<I, Projection>> Predicate>
		requires std::permutable<I>
	constexpr I operator()(I first, I last, Predicate&& predicate, Projection projection = Projection{}) const
	{
		for (;; std::advance(first, 1)) {
			first = std::ranges::find_if(move(first), last, std::ref(predicate), std::ref(projection));

			if (first == last)
				return first;

			last = std::next(std::ranges::find_if_not(
				std::make_reverse_iterator(move(last)),
				std::make_reverse_iterator(std::next(first)),
				std::ref(predicate),
				std::ref(projection)))
				.base();

			if (first == last)
				return first;

			*first = move(*last);
		}
	}
	template<std::ranges::bidirectional_range Range,
			typename Projection = identity,
			std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Range>, Projection>> Predicate>
		requires std::permutable<std::ranges::iterator_t<Range>> && std::ranges::common_range<Range>
	constexpr auto operator()(Range&& range, Predicate&& predicate, Projection projection = Projection{}) const
	{
		return (*this)(std::begin(range), std::end(range), forward<Predicate>(predicate), move(projection));
	}
};

inline constexpr unstable_remove_fn unstable_remove{};
inline constexpr unstable_remove_if_fn unstable_remove_if{};

} // ctp

#endif // INCLUDE_CTP_TOOLS_RANGES_REMOVE_HPP
