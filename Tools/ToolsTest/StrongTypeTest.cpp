#include <Tools/StrongType.hpp>
#include <cstdint>

namespace ctp::detail {
namespace {
template <typename T, typename U> using test_eq = decltype(std::declval<T>() == std::declval<U>());
template <typename T, typename U> using test_neq = decltype(std::declval<T>() != std::declval<U>());
template <typename T, typename U> using test_lt = decltype(std::declval<T>() < std::declval<U>());
template <typename T, typename U> using test_lteq = decltype(std::declval<T>() <= std::declval<U>());
template <typename T, typename U> using test_gt = decltype(std::declval<T>() > std::declval<U>());
template <typename T, typename U> using test_gteq = decltype(std::declval<T>() >= std::declval<U>());

template <typename T> using test_incr = decltype(++std::declval<T>());
template <typename T> using test_post_incr = decltype(std::declval<T>()++);
template <typename T> using test_decr = decltype(--std::declval<T>());
template <typename T> using test_post_decr = decltype(std::declval<T>()--);

template <typename T, typename U> using test_add = decltype(std::declval<T>() + std::declval<U>());
template <typename T, typename U> using test_addeq = decltype(std::declval<T>() += std::declval<U>());
template <typename T, typename U> using test_minus = decltype(std::declval<T>() - std::declval<U>());
template <typename T, typename U> using test_minuseq = decltype(std::declval<T>() -= std::declval<U>());

template <typename T, typename U> using test_mult = decltype(std::declval<T>()* std::declval<U>());
template <typename T, typename U> using test_multeq = decltype(std::declval<T>() *= std::declval<U>());
template <typename T, typename U> using test_div = decltype(std::declval<T>() / std::declval<U>());
template <typename T, typename U> using test_diveq = decltype(std::declval<T>() /= std::declval<U>());
template <typename T, typename U> using test_mod = decltype(std::declval<T>() % std::declval<U>());
template <typename T, typename U> using test_modeq = decltype(std::declval<T>() %= std::declval<U>());

struct TestIndexOperator { constexpr std::size_t operator[](std::size_t i) { return i; } };

template <typename Derived, typename Other>
constexpr bool is_strong_type_convertible_v = conversion_type_t<Derived, Other>::value;

namespace basic_test {
struct TypeOne : StrongType<TypeOne, int> { using StrongType::StrongType, StrongType::operator=; };
struct TypeTwo : StrongType<TypeTwo, int> { using StrongType::StrongType, StrongType::operator=;

	explicit constexpr operator TypeOne() noexcept { return TypeOne{value}; }
};

static_assert(std::is_default_constructible_v<TypeTwo>);

static_assert(std::is_constructible_v<TypeTwo, int>);
static_assert(std::is_constructible_v<TypeTwo, TypeTwo>);
static_assert(!std::is_constructible_v<TypeTwo, TypeOne>);
static_assert(std::is_constructible_v<TypeTwo, unsigned>);
static_assert(std::is_constructible_v<TypeTwo, float>);

static_assert(is_explicitly_convertible_v<TypeTwo, int>);
static_assert(is_explicitly_convertible_v<TypeTwo, float>);
static_assert(static_cast<int>(TypeTwo{1}) == 1);
static_assert(!std::is_convertible_v<TypeTwo, TypeOne>); // Requires explicit conversion.
static_assert(TypeTwo{1}.castTo<TypeOne>() == TypeOne{1});

static_assert(std::is_constructible_v<TypeOne, int>);
static_assert(std::is_constructible_v<TypeOne, unsigned>);
static_assert(std::is_constructible_v<TypeOne, float>);

static_assert(is_detected_v<test_eq, TypeTwo, TypeTwo>);
static_assert(TypeTwo{1} == TypeTwo{1});
static_assert(!(TypeTwo{1} == TypeTwo{2}));

static_assert(is_detected_v<test_neq, TypeTwo, TypeTwo>);
static_assert(TypeTwo{1} != TypeTwo{2});
static_assert(!(TypeTwo{1} != TypeTwo{1}));

static_assert(is_detected_v<test_lt, TypeTwo, TypeTwo>);
static_assert(TypeTwo{1} < TypeTwo{2});
static_assert(!(TypeTwo{1} < TypeTwo{1}));

static_assert(is_detected_v<test_lteq, TypeTwo, TypeTwo>);
static_assert(TypeTwo{1} <= TypeTwo{2});
static_assert(TypeTwo{2} <= TypeTwo{2});
static_assert(!(TypeTwo{1} <= TypeTwo{0}));

static_assert(is_detected_v<test_gt, TypeTwo, TypeTwo>);
static_assert(TypeTwo{1} > TypeTwo{0});
static_assert(!(TypeTwo{1} > TypeTwo{1}));

static_assert(is_detected_v<test_gteq, TypeTwo, TypeTwo>);
static_assert(TypeTwo{1} >= TypeTwo{0});
static_assert(TypeTwo{1} >= TypeTwo{1});
static_assert(!(TypeTwo{1} >= TypeTwo{2}));
} // basic_test

namespace valid_test {
struct InvOne : StrongType<InvOne, int>, InvalidValue { using StrongType::StrongType, StrongType::operator=; };
struct InvTwo : StrongType<InvTwo, int>, InvalidValue {
	static constexpr value_type invalid_value = -1;
	constexpr InvTwo() noexcept : StrongType{invalid_value} {}
	using StrongType::StrongType, StrongType::operator=;
};
struct NonInvalid : StrongType<NonInvalid, int> { using StrongType::StrongType, StrongType::operator=; };

template <typename T> using test_valid = decltype(std::declval<T>().valid());
template <typename T> using test_invalidate = decltype(std::declval<T>().invalidate());

// Detectable, but can't actually call valid without specifying invalid_value.
// static_asserts catch this to be more user-friendly.
static_assert(is_explicitly_convertible_v<InvOne, bool>);
static_assert(is_detected_v<test_valid, InvOne>);
static_assert(is_detected_v<test_invalidate, InvOne>);

static_assert(!InvTwo{}.valid());
static_assert(!InvTwo{InvTwo::invalid_value}.valid());
static_assert(InvTwo{1}.valid());
static_assert(static_cast<bool>(InvTwo{1}));
static_assert(!static_cast<bool>(InvTwo{-1}));
static_assert([]{ InvTwo i{1}; i.invalidate(); return i == InvTwo{}; }());

static_assert(!std::is_convertible_v<NonInvalid, bool>);
static_assert(!is_detected_v<test_valid, NonInvalid>);
static_assert(!is_detected_v<test_invalidate, NonInvalid>);
} // valid_test

namespace incr_test {
struct Incr : StrongType<Incr, int>, Incrementable { using StrongType::StrongType, StrongType::operator=; };
struct NonIncrementable : StrongType<NonIncrementable, int> { using StrongType::StrongType, StrongType::operator=; };

static_assert(is_detected_v<test_incr, Incr>);
static_assert(is_detected_v<test_post_incr, Incr>);
static_assert(is_detected_v<test_decr, Incr>);
static_assert(is_detected_v<test_post_decr, Incr>);
static_assert(++Incr{0} == Incr{1});
static_assert(Incr{0}++ == Incr{0});

static_assert(!is_detected_v<test_incr, NonIncrementable>);
static_assert(!is_detected_v<test_post_incr, NonIncrementable>);
static_assert(!is_detected_v<test_decr, NonIncrementable>);
static_assert(!is_detected_v<test_post_decr, NonIncrementable>);
static_assert(--Incr{1} == Incr{0});
static_assert(Incr{1}-- == Incr{1});
} // incr_test

namespace sum_test {
struct SumOne : StrongType<SumOne, int>, Summable { using StrongType::StrongType, StrongType::operator=; };
struct SumTwo : StrongType<SumTwo, int>, Summable { using StrongType::StrongType, StrongType::operator=; };
struct NonSummable : StrongType<NonSummable, int> { using StrongType::StrongType, StrongType::operator=; };

static_assert(SumOne{2} + SumOne{3} == SumOne{5});
static_assert((SumOne{2} += SumOne{3}) == SumOne{5});
static_assert(SumOne{2} - SumOne{3} == SumOne{-1});
static_assert((SumOne{2} -= SumOne{3}) == SumOne{-1});
} // sum_test

namespace mult_test {
struct MultOne : StrongType<MultOne, int>, Multipliable { using StrongType::StrongType, StrongType::operator=; };
struct MultTwo : StrongType<MultTwo, int>, Multipliable { using StrongType::StrongType, StrongType::operator=; };
struct NonMultmable : StrongType<NonMultmable, int> { using StrongType::StrongType, StrongType::operator=; };

static_assert(MultOne{2} * MultOne{2} == MultOne{4});
static_assert((MultOne{2} *= MultOne{2}) == MultOne{4});
static_assert(MultOne{2} / MultOne{2} == MultOne{1});
static_assert((MultOne{2} /= MultOne{2}) == MultOne{1});
static_assert(MultOne{4} % MultOne{2} == MultOne{0});
static_assert((MultOne{4} %= MultOne{2}) == MultOne{0});
} // mult_test

namespace arithmetic_test {
struct TypeArithmetic : StrongType<TypeArithmetic, int>, Arithmetic {
	using StrongType::StrongType, StrongType::operator=;
};

static_assert(TypeArithmetic{1}++ == TypeArithmetic{1});
static_assert(++TypeArithmetic{1} == TypeArithmetic{2});
static_assert(TypeArithmetic{1}-- == TypeArithmetic{1});
static_assert(--TypeArithmetic{1} == TypeArithmetic{0});

static_assert(TypeArithmetic{1} + TypeArithmetic{1} == TypeArithmetic{2});
static_assert((TypeArithmetic{1} += TypeArithmetic{1}) == TypeArithmetic{2});
static_assert(TypeArithmetic{1} - TypeArithmetic{1} == TypeArithmetic{0});
static_assert((TypeArithmetic{1} -= TypeArithmetic{1}) == TypeArithmetic{0});
static_assert(TypeArithmetic{1} *TypeArithmetic{2} == TypeArithmetic{2});
static_assert((TypeArithmetic{1} *= TypeArithmetic{2}) == TypeArithmetic{2});
static_assert(TypeArithmetic{4} / TypeArithmetic{2} == TypeArithmetic{2});
static_assert((TypeArithmetic{4} /= TypeArithmetic{2}) == TypeArithmetic{2});

} // arithmetic_test

namespace value_operable_test {
// Null value operable.
struct TypeOne : StrongType<TypeOne, int>, ValueOperable<strictness_relaxed> {
	using StrongType::StrongType, StrongType::operator=;
};

static_assert(TypeOne{1} == TypeOne{1});
static_assert(TypeOne{1} == 1);
static_assert(1 == TypeOne{1});
static_assert(TypeOne{1} != TypeOne{2});
static_assert(TypeOne{1} != 2);
static_assert(2 != TypeOne{1});
static_assert(TypeOne{1} < TypeOne{2});
static_assert(TypeOne{1} < 2);
static_assert(1 < TypeOne{2});
static_assert(TypeOne{2} > TypeOne{1});
static_assert(TypeOne{2} > 1);
static_assert(2 > TypeOne{1});
static_assert(TypeOne{1} <= TypeOne{2});
static_assert(TypeOne{2} <= TypeOne{2});
static_assert(TypeOne{1} <= 2);
static_assert(TypeOne{2} <= 2);
static_assert(1 <= TypeOne{2});
static_assert(2 <= TypeOne{2});
static_assert(TypeOne{2} >= TypeOne{1});
static_assert(TypeOne{2} >= TypeOne{2});
static_assert(TypeOne{2} >= 1);
static_assert(TypeOne{2} >= 2);
static_assert(2 >= TypeOne{1});
static_assert(2 >= TypeOne{2});

static_assert(!is_detected_v<test_add, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_add, TypeOne, int>);
static_assert(!is_detected_v<test_addeq, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_addeq, TypeOne, int>);
static_assert(!is_detected_v<test_minus, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_minus, TypeOne, int>);
static_assert(!is_detected_v<test_minuseq, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_minuseq, TypeOne, int>);
static_assert(!is_detected_v<test_mult, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_mult, TypeOne, int>);
static_assert(!is_detected_v<test_multeq, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_multeq, TypeOne, int>);
static_assert(!is_detected_v<test_div, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_div, TypeOne, int>);
static_assert(!is_detected_v<test_diveq, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_diveq, TypeOne, int>);
static_assert(!is_detected_v<test_mod, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_mod, TypeOne, int>);
static_assert(!is_detected_v<test_modeq, TypeOne, TypeOne>);
static_assert(!is_detected_v<test_modeq, TypeOne, int>);

// Sum value operable.
struct TypeSum : StrongType<TypeSum, int>, Summable, ValueOperable<strictness_relaxed> {
	using StrongType::StrongType, StrongType::operator=;
};

static_assert(is_detected_v<test_add, TypeSum, TypeSum>);
static_assert(is_detected_v<test_add, TypeSum, int>);
static_assert(is_detected_v<test_addeq, TypeSum, TypeSum>);
static_assert(is_detected_v<test_addeq, TypeSum, int>);
static_assert(is_detected_v<test_minus, TypeSum, TypeSum>);
static_assert(is_detected_v<test_minus, TypeSum, int>);
static_assert(is_detected_v<test_minuseq, TypeSum, TypeSum>);
static_assert(is_detected_v<test_minuseq, TypeSum, int>);

static_assert(TypeSum{1} + TypeSum{1} == TypeSum{2});
static_assert(TypeSum{1} + 1 == TypeSum{2});

static_assert((TypeSum{1} += TypeSum{1}) == TypeSum{2});
static_assert((TypeSum{1} += 1) == TypeSum{2});

static_assert(TypeSum{1} - TypeSum{1} == TypeSum{0});
static_assert(TypeSum{1} - 1 == TypeSum{0});

static_assert((TypeSum{1} -= TypeSum{1}) == TypeSum{0});
static_assert((TypeSum{1} -= 1) == TypeSum{0});

static_assert(!is_detected_v<test_mult, TypeSum, TypeSum>);
static_assert(!is_detected_v<test_mult, TypeSum, int>);
static_assert(!is_detected_v<test_multeq, TypeSum, TypeSum>);
static_assert(!is_detected_v<test_multeq, TypeSum, int>);
static_assert(!is_detected_v<test_div, TypeSum, TypeSum>);
static_assert(!is_detected_v<test_div, TypeSum, int>);
static_assert(!is_detected_v<test_diveq, TypeSum, TypeSum>);
static_assert(!is_detected_v<test_diveq, TypeSum, int>);
static_assert(!is_detected_v<test_mod, TypeSum, TypeSum>);
static_assert(!is_detected_v<test_mod, TypeSum, int>);
static_assert(!is_detected_v<test_modeq, TypeSum, TypeSum>);
static_assert(!is_detected_v<test_modeq, TypeSum, int>);

// Product value operable.
struct TypeMult : StrongType<TypeMult, int>, Multipliable, ValueOperable<strictness_relaxed> {
	using StrongType::StrongType, StrongType::operator=;
};

static_assert(!is_detected_v<test_add, TypeMult, TypeMult>);
static_assert(!is_detected_v<test_add, TypeMult, int>);
static_assert(!is_detected_v<test_addeq, TypeMult, TypeMult>);
static_assert(!is_detected_v<test_addeq, TypeMult, int>);
static_assert(!is_detected_v<test_minus, TypeMult, TypeMult>);
static_assert(!is_detected_v<test_minus, TypeMult, int>);
static_assert(!is_detected_v<test_minuseq, TypeMult, TypeMult>);
static_assert(!is_detected_v<test_minuseq, TypeMult, int>);
static_assert(is_detected_v<test_mult, TypeMult, TypeMult>);
static_assert(is_detected_v<test_mult, TypeMult, int>);
static_assert(is_detected_v<test_multeq, TypeMult, TypeMult>);
static_assert(is_detected_v<test_multeq, TypeMult, int>);
static_assert(is_detected_v<test_div, TypeMult, TypeMult>);
static_assert(is_detected_v<test_div, TypeMult, int>);
static_assert(is_detected_v<test_diveq, TypeMult, TypeMult>);
static_assert(is_detected_v<test_diveq, TypeMult, int>);
static_assert(is_detected_v<test_mod, TypeMult, int>);
static_assert(is_detected_v<test_mod, TypeMult, int>);
static_assert(is_detected_v<test_modeq, TypeMult, TypeMult>);
static_assert(is_detected_v<test_modeq, TypeMult, int>);

static_assert(TypeMult{1} * TypeMult{2} == TypeMult{2});
static_assert(TypeMult{1} * 2 == TypeMult{2});

static_assert((TypeMult{1} *= TypeMult{2}) == TypeMult{2});
static_assert((TypeMult{1} *= 2) == TypeMult{2});

static_assert(TypeMult{4} / TypeMult{2} == TypeMult{2});
static_assert(TypeMult{4} / 2 == TypeMult{2});

static_assert((TypeMult{4} /= TypeMult{2}) == TypeMult{2});
static_assert((TypeMult{4} /= 2) == TypeMult{2});

static_assert((TypeMult{4} % TypeMult{2}) == TypeMult{0});
static_assert((TypeMult{4} % 2) == TypeMult{0});

static_assert((TypeMult{5} %= TypeMult{2}) == TypeMult{1});
static_assert((TypeMult{5} %= 2) == TypeMult{1});

// Arithmetic value operable.
struct TypeArithmetic : StrongType<TypeArithmetic, int>, ValueArithmetic<strictness_relaxed> {
	using StrongType::StrongType, StrongType::operator=;
};

static_assert(TypeArithmetic{1} + TypeArithmetic{1} == TypeArithmetic{2});
static_assert(TypeArithmetic{1} + 1 == TypeArithmetic{2});

static_assert((TypeArithmetic{1} += TypeArithmetic{1}) == TypeArithmetic{2});
static_assert((TypeArithmetic{1} += 1) == TypeArithmetic{2});

static_assert(TypeArithmetic{1} - TypeArithmetic{1} == TypeArithmetic{0});
static_assert(TypeArithmetic{1} - 1 == TypeArithmetic{0});

static_assert((TypeArithmetic{1} -= TypeArithmetic{1}) == TypeArithmetic{0});
static_assert((TypeArithmetic{1} -= 1) == TypeArithmetic{0});

static_assert(TypeArithmetic{1} *TypeArithmetic{2} == TypeArithmetic{2});
static_assert(TypeArithmetic{1} *2 == TypeArithmetic{2});

static_assert((TypeArithmetic{1} *= TypeArithmetic{2}) == TypeArithmetic{2});
static_assert((TypeArithmetic{1} *= 2) == TypeArithmetic{2});

static_assert(TypeArithmetic{4} / TypeArithmetic{2} == TypeArithmetic{2});
static_assert(TypeArithmetic{4} / 2 == TypeArithmetic{2});

static_assert((TypeArithmetic{4} /= TypeArithmetic{2}) == TypeArithmetic{2});
static_assert((TypeArithmetic{4} /= 2) == TypeArithmetic{2});
} // value_operable_test

namespace custom_constructor_test {
struct CustomConstructor : StrongType<CustomConstructor, std::uint16_t>, InvalidValue, ValueArithmetic<strictness_relaxed> {
	static constexpr auto invalid_value = static_cast<std::uint16_t>(-1);
	constexpr CustomConstructor() noexcept : StrongType{invalid_value} {}
	using StrongType::StrongType, StrongType::operator=;
};

static_assert(std::is_default_constructible_v<CustomConstructor>);
static_assert(std::is_constructible_v<CustomConstructor, std::uint16_t>);
static_assert(std::is_constructible_v<CustomConstructor, int>);
static_assert(!CustomConstructor{}.valid());
static_assert(CustomConstructor{1} == CustomConstructor{1});
static_assert(CustomConstructor{1} + 1 == 2);
static_assert(CustomConstructor{2} - 1 == 1);
} // custom_constructor_test

namespace conversion_test {
struct TypeOne : StrongType<TypeOne, int> { using StrongType::StrongType, StrongType::operator=; };
struct TypeTwo : StrongType<TypeTwo, int> { using StrongType::StrongType, StrongType::operator=; };
struct TypeThree : StrongType<TypeThree, int> {
	using StrongType::StrongType, StrongType::operator=;
	using operable_with = TypeOne;
	static constexpr TypeThree convert(TypeOne t) noexcept { return TypeThree{t.value}; }
};
struct TypeDouble : StrongType<TypeDouble, int>, ValueArithmetic<strictness_relaxed> {
	using StrongType::StrongType, StrongType::operator=;
	using operable_with = operable_with_types<TypeOne, TypeTwo>;
	static constexpr TypeDouble convert(TypeOne t) noexcept { return TypeDouble{t.value * 2}; }
};

static_assert(!is_strong_type_convertible_v<TypeOne, TypeTwo>);
static_assert(!is_strong_type_convertible_v<TypeOne, int>);
static_assert(!is_strong_type_convertible_v<TypeOne, float>);
static_assert(!is_strong_type_convertible_v<TypeOne, unsigned>);

static_assert(is_strong_type_convertible_v<TypeThree, TypeOne>);
static_assert(!is_strong_type_convertible_v<TypeOne, TypeThree>);
static_assert(!is_strong_type_convertible_v<TypeThree, TypeTwo>);

static_assert(TypeThree{0} == TypeOne{0});
static_assert(TypeThree{1} < TypeOne{2});

static_assert(TypeDouble{2} == TypeOne{1}); // uses convert
static_assert(TypeDouble{2} == TypeTwo{2}); // just casts

static_assert(TypeDouble{TypeOne{1}} == TypeDouble{2});
static_assert(static_cast<TypeDouble>(TypeOne{1}) == TypeDouble{2});
static_assert(TypeDouble{1} + TypeOne{1} == TypeDouble{3});
static_assert(TypeDouble{3} - TypeOne{1} == TypeDouble{1});
static_assert(TypeDouble{2} * TypeOne{1} == TypeDouble{4});
static_assert(TypeDouble{4} / TypeOne{1} == TypeDouble{2});

static_assert(1 == TypeDouble{1});


struct TypeExact : StrongType<TypeExact, float>, ValueArithmetic<strictness_exact_match> { using StrongType::StrongType, StrongType::operator=; };

static_assert(!is_strong_type_convertible_v<TypeExact, TypeTwo>);
static_assert(!is_strong_type_convertible_v<TypeExact, int>);
static_assert(!is_strong_type_convertible_v<TypeExact, unsigned>);
static_assert(is_strong_type_convertible_v<TypeExact, float>);
static_assert(!is_strong_type_convertible_v<TypeExact, double>);

struct TypeSimilar : StrongType<TypeSimilar, int>, ValueArithmetic<strictness_similar> { using StrongType::StrongType, StrongType::operator=; };

static_assert(!is_strong_type_convertible_v<TypeSimilar, TypeTwo>);
static_assert(is_strong_type_convertible_v<TypeSimilar, int>);
static_assert(is_strong_type_convertible_v<TypeSimilar, short>);
static_assert(is_strong_type_convertible_v<TypeSimilar, long>);
static_assert(!is_strong_type_convertible_v<TypeSimilar, unsigned>);
static_assert(!is_strong_type_convertible_v<TypeSimilar, float>);
static_assert(!is_strong_type_convertible_v<TypeSimilar, double>);

struct TypeRelaxed : StrongType<TypeRelaxed, int>, ValueArithmetic<strictness_relaxed> { using StrongType::StrongType, StrongType::operator=; };

static_assert(!is_strong_type_convertible_v<TypeRelaxed, TypeTwo>);
static_assert(is_strong_type_convertible_v<TypeRelaxed, int>);
static_assert(is_strong_type_convertible_v<TypeRelaxed, short>);
static_assert(is_strong_type_convertible_v<TypeRelaxed, long>);
static_assert(is_strong_type_convertible_v<TypeRelaxed, unsigned>);
static_assert(!is_strong_type_convertible_v<TypeRelaxed, float>);
static_assert(!is_strong_type_convertible_v<TypeRelaxed, double>);

} // conversion_test

namespace implicit_conversion_test {
struct TypeOne : StrongType<TypeOne, int> { using StrongType::StrongType, StrongType::operator=; };
struct TypeImplicit : StrongType<TypeImplicit, int>, ImplicitlyConvertible<TypeImplicit> { using StrongType::StrongType, StrongType::operator=; };

static_assert(!std::is_constructible_v<TypeOne, TypeImplicit>);
static_assert(!std::is_convertible_v<TypeOne, TypeImplicit>);
static_assert(!std::is_convertible_v<TypeImplicit, TypeOne>);

static_assert(std::is_convertible_v<TypeImplicit, int>);

static_assert(TypeOne{static_cast<int>(TypeImplicit{2})} == TypeOne{2});
static_assert(TypeImplicit{2}.castTo<TypeOne>() == TypeOne{2});

constexpr int func(int i) { return i; }
static_assert(func(TypeImplicit{2}) == 2);

static_assert(TestIndexOperator{}[TypeImplicit{2}] == 2);

} // implicit_conversion_test

namespace index_type_test {
struct IndexOne : IndexType<IndexOne, int> { using IndexType::IndexType; };
struct IndexTwo : IndexType<IndexTwo, int> { using IndexType::IndexType; };

static_assert(!std::is_constructible_v<IndexOne, IndexTwo>);
static_assert(!std::is_constructible_v<IndexTwo, IndexOne>);

constexpr int func(int i) { return i; }
static_assert(func(IndexOne{2}) == 2);

static_assert(TestIndexOperator{}[IndexOne{2}] == 2);

} // index_type_test
} // namespace
} // ctp::detail
