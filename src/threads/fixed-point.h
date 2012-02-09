#ifndef __THREADS_FIXED_POINT_H
#define __THREADS_FIXED_POINT_H

#define FIXED int
#define __F (1 << 14)

#define F_TO_FIXED(n) 		((n) * __F)
#define F_TO_INT(x) 		((x) / __F)
#define F_TO_INT_NEAREST(x) (((x) >= 0) ? (((x) + (__F / 2)) / __F) : (((x) - (__F / 2 ) ) / __F) )
#define F_ADD(x, y)			((x) + (y))
#define F_SUB(x, y)			((x) - (y))
#define F_ADD_INT(x, n)		((x) + ((n) * __F))
#define F_SUB_INT(x, n)		((x) - ((n) * __F))
#define F_MUL(x, y)			(((int64_t)(x)) * (y) / __F)
#define F_MUL_INT(x, n)		((x) * n)
#define F_DIV(x, y)			(((int64_t)(x)) * __F / (y))
#define F_DIV_INT(x, n)		((x)/(n))


#endif
