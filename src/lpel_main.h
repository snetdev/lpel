
#ifndef _LPEL_MAIN_H_
#define _LPEL_MAIN_H_


#include <lpel.h>


#define _MON_CB_MEMBER(glob,member) (glob.member)
#define MON_CB(name) (_MON_CB_MEMBER(_lpel_global_config.mon,name))


int LpelThreadAssign( int core);


extern lpel_config_t    _lpel_global_config;

#endif /* _LPEL_MAIN_H_ */
