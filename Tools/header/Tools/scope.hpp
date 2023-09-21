#ifndef INCLUDE_CTP_TOOLS_SCOPE_HPP
#define INCLUDE_CTP_TOOLS_SCOPE_HPP

#include <exception>
#include <limits>
#include <type_traits>
#include <utility>

namespace ctp {
namespace detail {

struct ExitPolicy {
	bool should_execute_ = true;
	constexpr void release() noexcept { should_execute_ = false; }
	constexpr bool shouldExecute() const noexcept { return should_execute_; }
};

struct FailPolicy {
	int numExceptions = std::uncaught_exceptions();
	void release() noexcept { numExceptions = (std::numeric_limits<int>::max)(); }
	bool shouldExecute() const noexcept { return numExceptions < std::uncaught_exceptions(); }
};

struct SuccessPolicy {
	int numExceptions = std::uncaught_exceptions();
	void release() noexcept { numExceptions = -1; }
	bool shouldExecute() const noexcept { return numExceptions >= std::uncaught_exceptions(); }
};

template <typename F, typename Policy>
class [[nodiscard]] ScopeExit : Policy {
	static_assert(std::is_invocable_v<F>);

	F invocable_;
public:
	template <typename C>
	constexpr explicit ScopeExit(C&& c) noexcept(std::is_nothrow_constructible_v<F, C&&>) : invocable_{::std::forward<C>(c)} {}

	constexpr ~ScopeExit() noexcept(std::is_nothrow_invocable_v<F>) {
		if (this->shouldExecute())
			invocable_();
	}

	constexpr ScopeExit(ScopeExit&& other) noexcept(std::is_nothrow_move_constructible_v<F>)
		: Policy(other)
		, invocable_{::std::move(other.invocable_)}
	{}

	ScopeExit() = delete;
	ScopeExit(const ScopeExit&) = delete;
	ScopeExit& operator=(const ScopeExit&) = delete;
	ScopeExit& operator=(ScopeExit&&) = delete;

	using Policy::release;
};

} // detail


// ScopeExit calls a function upon leaving scope in which it was created, unless release() is called.
template <typename F>
struct ScopeExit : detail::ScopeExit<F, detail::ExitPolicy> {
	using Base = detail::ScopeExit<F, detail::ExitPolicy>;
	using Base::Base;
};
template<typename C> ScopeExit(C&& c) -> ScopeExit<C>;

// ScopeFail calls a function upon leaving scope in which it was created where an exception was thrown, unless release() is called.
template <typename F>
struct ScopeFail : detail::ScopeExit<F, detail::FailPolicy> {
	using Base = detail::ScopeExit<F, detail::FailPolicy>;
	using Base::Base;
};
template<typename C> ScopeFail(C&& c) -> ScopeFail<C>;

// ScopeSuccess calls a function upon leaving scope in which it was created if no exception was thrown, unless release() was called.
template <typename F>
struct ScopeSuccess : detail::ScopeExit<F, detail::SuccessPolicy>{
	using Base = detail::ScopeExit<F, detail::SuccessPolicy>;
	using Base::Base;
};
template<typename C> ScopeSuccess(C&& c) -> ScopeSuccess<C>;

/* TODO:
template <class Resource, class Deleter>
class [[nodiscard]] UniqueResource {
	//static_assert((std::is_move_constructible_v<Resource> && std::is_nothrow_move_constructible_v<Resource>) || std::is_copy_constructible_v<Resource>);
	//static_assert((std::is_move_constructible_v<Resource> && std::is_nothrow_move_constructible_v<Deleter>) || std::is_copy_constructible_v<Deleter>);

	Resource resource_;
	Deleter deleter_;
	bool needsCleanup = true;

};

template <class Resource, class Destructor, class S = std::decay_t<Resource>>
UniqueResource<std::decay_t<Resource>, std::decay_t<Destructor>>
	make_unique_resource_checked(R&& r, const S& invalid, D&& d) noexcept( false /*see below*//*)
{

}
*/
} // namespace ctp
#endif // INCLUDE_CTP_TOOLS_SCOPE_HPP
