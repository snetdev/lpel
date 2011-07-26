/**
 * Macros for prefixing types and functions.
 *
 */

#ifndef _LPEL_NAME_H_
#define _LPEL_NAME_H_



#ifndef LPEL_PREFIX
#define LPEL_PREFIX     Lpel
#endif /* LPEL_PREFIX */

#ifndef LPEL_PREFIX_T
#define LPEL_PREFIX_T   lpel_
#endif /* LPEL_PREFIX_T */

#define _LP_PASTE(prefix,func)    prefix ## func
#define _LP_CONCAT(prefix,func)   _LP_PASTE(prefix,func)

#define LPEL_FUNC(func)     _LP_CONCAT(LPEL_PREFIX,func)
#define LPEL_TYPE(func)     _LP_CONCAT(LPEL_PREFIX_T,func)


#endif /* _LPEL_NAME_H_ */
