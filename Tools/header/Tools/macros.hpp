#ifndef INCLUDE_CTP_TOOLS_MACROS_HPP
#define INCLUDE_CTP_TOOLS_MACROS_HPP

#define CTP_EXPAND(x) x

#define CTP_MAKE_CLASS_PTR(CLASS_NAME, MEMBER) &CLASS_NAME::MEMBER

// ---- Argument count dispatcher ----
#define CTP_GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10, _11,_12,_13,_14,_15,_16,_17,_18,_19,_20,NAME,...) NAME

#define CTP_MACROS_FOREACH_1(M, C, A1) M(C, A1)
#define CTP_MACROS_FOREACH_2(M, C, A1, A2) M(C, A1), M(C, A2)
#define CTP_MACROS_FOREACH_3(M, C, A1, A2, A3) M(C, A1), M(C, A2), M(C, A3)
#define CTP_MACROS_FOREACH_4(M, C, A1, A2, A3, A4) M(C, A1), M(C, A2), M(C, A3), M(C, A4)
#define CTP_MACROS_FOREACH_5(M, C, A1, A2, A3, A4, A5) M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5)
#define CTP_MACROS_FOREACH_6(M, C, A1, A2, A3, A4, A5, A6) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6)
#define CTP_MACROS_FOREACH_7(M, C, A1, A2, A3, A4, A5, A6, A7) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7)
#define CTP_MACROS_FOREACH_8(M, C, A1, A2, A3, A4, A5, A6, A7, A8) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8)
#define CTP_MACROS_FOREACH_9(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9)
#define CTP_MACROS_FOREACH_10(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10)
#define CTP_MACROS_FOREACH_11(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), M(C, A11)
#define CTP_MACROS_FOREACH_12(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12)
#define CTP_MACROS_FOREACH_13(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13)
#define CTP_MACROS_FOREACH_14(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14)
#define CTP_MACROS_FOREACH_15(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14), M(C, A15)
#define CTP_MACROS_FOREACH_16(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14), M(C, A15), M(C, A16)
#define CTP_MACROS_FOREACH_17(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14), M(C, A15), M(C, A16), M(C, A17)
#define CTP_MACROS_FOREACH_18(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14), M(C, A15), M(C, A16), M(C, A17), M(C, A18)
#define CTP_MACROS_FOREACH_19(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, \
	A11, A12, A13, A14, A15, A16, A17, A18, A19) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14), M(C, A15), M(C, A16), M(C, A17), M(C, A18), M(C, A19)
#define CTP_MACROS_FOREACH_20(M, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, \
	A11, A12, A13, A14, A15, A16, A17, A18, A19, A20) \
	M(C, A1), M(C, A2), M(C, A3), M(C, A4), M(C, A5), M(C, A6), M(C, A7), M(C, A8), M(C, A9), M(C, A10), \
	M(C, A11), M(C, A12), M(C, A13), M(C, A14), M(C, A15), M(C, A16), M(C, A17), M(C, A18), M(C, A19), M(C, A20)

// Use to call a function on a sequence of va_args, like
// MACRO(FIRST_ARG, *first va_arg*), MACRO(FIRST_ARG, *second va_arg*), ...
#define CTP_MACRO_FUNC_TWO_PARAMS(MACRO_FUNC, FIRST_ARG, ...) \
	CTP_EXPAND( CTP_GET_MACRO(__VA_ARGS__, \
		CTP_MACROS_FOREACH_20, CTP_MACROS_FOREACH_19, CTP_MACROS_FOREACH_18, \
		CTP_MACROS_FOREACH_17, CTP_MACROS_FOREACH_16, CTP_MACROS_FOREACH_15, \
		CTP_MACROS_FOREACH_14, CTP_MACROS_FOREACH_13, CTP_MACROS_FOREACH_12, \
		CTP_MACROS_FOREACH_11, CTP_MACROS_FOREACH_10, CTP_MACROS_FOREACH_9, \
		CTP_MACROS_FOREACH_8, CTP_MACROS_FOREACH_7, CTP_MACROS_FOREACH_6, \
		CTP_MACROS_FOREACH_5, CTP_MACROS_FOREACH_4, CTP_MACROS_FOREACH_3, \
		CTP_MACROS_FOREACH_2, CTP_MACROS_FOREACH_1 \
	)(MACRO_FUNC, FIRST_ARG, __VA_ARGS__) )

#endif // INCLUDE_CTP_TOOLS_MACROS_HPP
