#ifndef INCLUDE_CTP_TOOLS_VECTORIZER_HPP
#define INCLUDE_CTP_TOOLS_VECTORIZER_HPP

#include "config.hpp"
#include "concepts.hpp"
#include "CrtpHelper.hpp"
#include "debug.hpp"
#include "iterator.hpp"
#include "reverse_iterator.hpp"
#include "type_traits.hpp"

#include <array>

namespace ctp {

// Treat a class like an iterable vector.
// Derived must be a standard layout type and may only contain members of type T with no padding, and
// (for constexpr operations) must define:
//		static constexpr auto get_vectorizer_list() noexcept
// that should return the result of passing pointers-to-members in-order to make_vectorizer_list
// struct Vec : Vectorizer<Vec, int, 2> {
//    int x, y;
//    static constexpr auto get_vectorizer_list() noexcept { return make_vectorizer_list(&Vec::x, &Vec::y); }
// };
// Derived classes should also implement, declare as defaulted, or explicitly delete the comparison operators.
template <typename Derived, typename ValueType, std::size_t N>
	requires (N > 0)
class Vectorizer;

template <typename T>
concept HasMemberVectorizerList = requires { T::get_vectorizer_list(); };

consteval auto make_vectorizer_list(concepts::AllExactlySame auto... MemberPtrs) noexcept
	requires std::is_member_object_pointer_v<first_element_t<decltype(MemberPtrs)...>>
{
	return std::array<first_element_t<decltype(MemberPtrs)...>, sizeof...(MemberPtrs)>{{MemberPtrs...}};
}

// Declare get_vectorizer_list with given pointer-to-member list.
#define CTP_MAKE_VECTORIZER_LIST(...) \
	static constexpr auto get_vectorizer_list() noexcept { return make_vectorizer_list(__VA_ARGS__); }

template <typename Derived, typename ValueType>
class vectorizer_iterator : public iterator_t<vectorizer_iterator<Derived, ValueType>, ValueType> {
	Derived* d_ = nullptr;
	std::size_t i_ = 0;

	friend class iterator_accessor;
	constexpr auto& get_index() noexcept { return i_; }

	template <typename D>
	struct validate_get_vector_list {
		using derived_type = std::remove_const_t<D>;

		static_assert(HasMemberVectorizerList<derived_type>,
			"Must define static function get_vectorizer_list for use in constant expressions.");

		using MemberPtrType = std::remove_reference_t<decltype(derived_type::get_vectorizer_list()[0])>;

		template <class T>
		struct member_types;
		template <class D_, class T>
		struct member_types<T D_::*> {
			using type = T;
			using derived = D_;
		};

		using MemberTypes = member_types<MemberPtrType>;
		using MemberType = typename MemberTypes::type;

		static_assert(std::same_as<typename MemberTypes::derived, derived_type>,
			"get_vectorizer_list returned a pointer-to-member type to an unexpected derived class.");
		static_assert(std::same_as<MemberType, typename derived_type::value_type> ||
			std::same_as<std::remove_extent_t<MemberType>, typename derived_type::value_type> ||
			concepts::HasData<MemberType, ValueType*>,
			"get_vectorizer_list returned a pointer-to-member type different from value_type.");
	};

	constexpr ValueType* consteval_peek() noexcept {
		using VectorListTypes = validate_get_vector_list<Derived>;

		constexpr auto member_list = Derived::get_vectorizer_list();

		if constexpr (member_list.size() != Derived::Size) {
			// If we have a mismatch of get_vectorizer_list size to N, we expect there to be one
			// member that behaves like an array.
			ctpAssert(member_list.size() == 1);

			if constexpr (concepts::HasData<typename VectorListTypes::MemberType, ValueType*>) {
				// Assume some kind of contiguous container like std::array.
				return (d_->*member_list[0]).data() + i_;
			} else {
				// Assume plain array.

				// The below funky syntax coerces MSVC into recognizing that we have an N-sized array
				// and it is valid to choose a non-zero index.
				if constexpr (std::is_const_v<ValueType>) {
					std::add_const_t<typename VectorListTypes::MemberType>* data = &(d_->*member_list[0]);
					return std::addressof((*data)[i_]);
				} else {
					typename VectorListTypes::MemberType* data = &(d_->*member_list[0]);
					return std::addressof((*data)[i_]);
				}
			}
		} else {
			return std::addressof(d_->*member_list[i_]);
		}
	}

	constexpr ValueType* peek() noexcept {
		ctpAssert(d_ != nullptr); // invalid iterator
		ctpAssert(i_ <= d_->size()); // peeking out of range

		if CTP_IS_CONSTEVAL{
			return consteval_peek();
		} else {
			return reinterpret_cast<ValueType*>(d_) + i_;
		}
	}

	// conversion from non-const iterator to const iterator.
	friend class vectorizer_iterator<Derived, std::add_const_t<ValueType>>;
	using nonconst_t = std::conditional_t<
		std::is_same_v<ValueType, std::remove_const_t<ValueType>>,
		nonesuch,
		vectorizer_iterator<Derived, std::remove_const_t<ValueType>>>;
public:
	constexpr vectorizer_iterator() noexcept = default;
	constexpr vectorizer_iterator(Derived& d, std::size_t i = 0) noexcept : d_{&d}, i_{i} {}

	template <typename NonConstT = nonconst_t, std::enable_if_t<!std::is_same_v<NonConstT, ctp::nonesuch>, int> = 0>
	constexpr vectorizer_iterator(const nonconst_t& o) noexcept : d_{o.d_}, i_{o.i_} {}
};

template <typename Vec, typename ValueType, std::size_t N>
concept VectorizerCompatible = requires (Vec u) {
	// Rather than checking for a Vectorizer base, we can be slighly
	// more flexible by checking for Size and an iterator with a compatible
	// value type;

	u.Size;
	requires (u.Size == N);

	// Have some iterator whos values can be converted to the value type.
	{ *u.begin() } -> std::convertible_to<ValueType>;
};

namespace vectorizer_detail {

template <typename ValueType, std::size_t Size, typename... Ts>
concept MatchingConstructorValues = requires {
	requires (Size != 1);
	requires (sizeof...(Ts) == Size);
	requires concepts::AllExactlySame<Ts...>;
	requires concepts::AllConvertibleTo<ValueType, Ts...>;
};

} // vectorizer_detail

template <typename Derived, typename ValueType, std::size_t N>
	requires (N > 0)
class Vectorizer : public CrtpHelper<Derived, Vectorizer<Derived, ValueType, N>> {
public:
	using derived_type = Derived;
	using value_type = ValueType;
	using reference = value_type&;
	using const_reference = const value_type&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using iterator = vectorizer_iterator<derived_type, value_type>;
	using const_iterator = vectorizer_iterator<const derived_type, const value_type>;
	using reverse_iterator = ctp::reverse_iterator<iterator>;
	using const_reverse_iterator = ctp::reverse_iterator<const_iterator>;

	static constexpr size_type Size = N;

	constexpr reference operator[](std::size_t i) noexcept { return *iterator{this->derived(), i}; }
	constexpr const_reference operator[](std::size_t i) const noexcept { return *const_iterator{this->derived(), i}; }

	// Conversion operator.
	template <VectorizerCompatible<value_type, N> U>
	explicit constexpr operator U() const noexcept {
		U result{};
		auto it = cbegin();
		auto other = result.begin();
		for (std::size_t i = 0; i < N; ++i)
			*other++ = static_cast<std::remove_reference_t<decltype(*other)>>(*it++);
		return result;
	}

	constexpr Vectorizer() noexcept = default;

	// Technically these helper constructors should probably not be able to be constexpr, (since
	// Derived, and hence its values, are not initialized yet) but MSVC seems to accept them...
	explicit constexpr Vectorizer(value_type arg) noexcept {
		for (auto& val : *this)
			val = arg;
	}
	template <class... Ts>
		requires vectorizer_detail::MatchingConstructorValues<ValueType, N, Ts...>
	constexpr Vectorizer(Ts&&... args) noexcept {
		auto it = begin();
		(..., void(*it++ = forward<Ts>(args)));
	}

	// Iterator operations.

	constexpr std::size_t size() const noexcept { return Size; }

	constexpr iterator begin() noexcept { return iterator{this->derived(), 0}; }
	constexpr const_iterator begin() const noexcept { return const_iterator{this->derived(), 0}; }
	constexpr const_iterator cbegin() const noexcept { return begin(); }
	constexpr iterator end() noexcept { return iterator{this->derived(), Size}; }
	constexpr const_iterator end() const noexcept { return const_iterator{this->derived(), Size}; }
	constexpr const_iterator cend() const noexcept { return end(); }

	constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
	constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{cend()}; }
	constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
	constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{cbegin()}; }
	constexpr const_reverse_iterator crend() const noexcept { return rend(); }

	// Allow derived classes to generate comparison operators.
	// Annoying that these (currently) can't be protected, or defaulted comparison operators can't be generated.
	friend constexpr auto operator<=>(const Vectorizer&, const Vectorizer&) noexcept {
		return std::strong_ordering::equal;
	}
	friend constexpr bool operator==(const Vectorizer&, const Vectorizer&) noexcept { return true; }
protected:
	// Helper function to apply an op to each element in this vector.
	// Op takes elements from this as the first argument, then any other values, and returns a value for the new vector.
	template <typename... Ts, typename Op>
		requires (!VectorizerCompatible<first_element_t<Ts...>, value_type, Size>)
	[[nodiscard]] constexpr auto reduce_to_vec(Op&& op, Ts&&... ts) const noexcept {
		derived_type result;
		auto it = this->begin();
		for (std::size_t i = 0; i < Size; ++i)
			result[i] = op(*it++, ts...);
		return result;
	}
	// Helper function to apply an op to each element in this vector.
	// Op takes elements from this as the first argument, then any other vectors, and returns a value for the new vector.
	template <VectorizerCompatible<value_type, Size>... Vs, typename Op>
	[[nodiscard]] constexpr auto reduce_to_vec(Op&& op, Vs&&... vs) const noexcept {
		derived_type result;
		auto it = this->begin();
		for (std::size_t i = 0; i < Size; ++i)
			result[i] = op(*it++, vs[i]...);
		return result;
	}

	// Helper function to apply an op to each element in this vector.
	// Op takes elements from this as the first argument, then any other vectors.
	template <VectorizerCompatible<value_type, Size>... Vs, typename Op>
	constexpr auto& apply(this auto& self, Op&& op, Vs&&... vs) noexcept {
		auto it = self.begin();
		for (std::size_t i = 0; i < Size; ++i)
			op(*it++, vs[i]...);
		return self.derived();
	}
	// Helper function to apply an op to each element in this vector, mutating it.
	// Op takes elements from this as the first argument, then any other arguments.
	template <typename... Ts, typename Op>
		requires(!VectorizerCompatible<first_element_t<Ts...>, value_type, Size>)
	constexpr auto& apply(this auto& self, Op&& op, Ts&&... ts) noexcept {
		auto it = self.begin();
		for (std::size_t i = 0; i < Size; ++i)
			op(*it++, ts...);
		return self.derived();
	}
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_VECTORIZER_HPP
