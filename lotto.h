#ifndef __LOTTO_H__
#define __LOTTO_H__

#define LOTTO_NUM 6

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#define swap(a, b); {(a)^=(b);(b)^=(a);(a)^=(b);}
#define noprint(...)

#define __DEBUG__
#ifdef __DEBUG__
#define debug printf
#else
#define debug noprint
#endif /* DEBUG */

#endif /* LOTTO_H */
