#ifndef MACROS_H
#define MACROS_H

#ifndef __always_inline
#define __always_inline	inline __attribute__((__always_inline__))
#endif
#define	__packed	__attribute__((__packed__))	
#define __aligned(x)	__attribute__((__aligned(x)__))
#define __counted_by(x)	__attribute__((__counted_by(x)))
#define __may_alias	__attribute__((__may_alias__))

#define PROMPT	"calc> "
#define	ALIGN(x, b)		(((x) + (b) - 1)&(~((b) - 1)))
#define ALIGN_DOWN(x, b)	((x)&(~((b) - 1)))
#define MIN(a, b)		((a) < (b) ? (a) : (b))
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define CLAMP_TOP(a, x)		MIN(a, x)
#define CLAMP_BOT(x, b)		MAX(x, b)
#define ERROR(line, err)	fprintf(stderr, "[line %d] Error %s\n", line, err)

#endif
