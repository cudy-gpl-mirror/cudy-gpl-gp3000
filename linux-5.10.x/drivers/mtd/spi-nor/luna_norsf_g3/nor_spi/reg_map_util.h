#ifndef _REG_MAP_UTIL_H_
#define _REG_MAP_UTIL_H_

typedef volatile unsigned int *regval;

#define RLOAD(_var, _reg)  (((_reg##_T) (_var)) = { .v = _reg##rv })
#define RTYPE(_var, _reg)  (((_reg##_T) (_var)) = { .v = 0 })
#define RSTORE(_var, _reg) ((_reg##rv) = _var.v)

/* for __VA_NARG__ */
#define PP_NARG(...)  PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define rset(rtype, ival, ...) do {            \
		rtype##_T dummyr = { .v = ival };          \
		PP_NARG(__VA_ARGS__)(dummyr, __VA_ARGS__); \
		rtype##rv = dummyr.v;                      \
	} while (0)
#endif

