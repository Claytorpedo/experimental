#ifndef INCLUDE_CTP_TOOLS_EXCEPTION_HPP
#define INCLUDE_CTP_TOOLS_EXCEPTION_HPP

#include "config.hpp"

#include <exception>

namespace ctp {

constexpr int uncaught_exceptions() {
	if CTP_IS_CONSTEVAL{
		return 0;
	} else {
		return std::uncaught_exceptions();
	}
}

} // ctp

#endif // INCLUDE_CTP_TOOLS_EXCEPTION_HPP
