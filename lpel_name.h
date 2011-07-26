#ifndef _LPEL_NAME_H_
#define _LPEL_NAME_H_



#ifndef LPEL_PREFIX
#define LPEL_PREFIX   Lpel
#endif /* LPEL_PREFIX */

#define _LP_PASTE(prefix,func)    prefix ## func
#define _LP_CONCAT(prefix,func)   _LP_PASTE(prefix,func)

#define LPEL_EXPORT(func)     _LP_CONCAT(LPEL_PREFIX,func)


#endif /* _LPEL_NAME_H_ */
