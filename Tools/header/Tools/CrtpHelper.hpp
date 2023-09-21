#ifndef INCLUDE_CTP_TOOLS_CRTPHELPER_HPP
#define INCLUDE_CTP_TOOLS_CRTPHELPER_HPP

namespace ctp {

template <typename Derived, typename CrtpType>
struct CrtpHelper {
protected:
	constexpr Derived& derived() noexcept { return static_cast<Derived&>(*this); }
	constexpr const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }
private:
	friend CrtpType;
	constexpr CrtpHelper() noexcept = default;
};

} // ctp

#endif // INCLUDE_CTP_TOOLS_CRTPHELPER_HPP
