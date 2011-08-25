
#ifndef _LPELCFG_H_
#define _LPELCFG_H_


#include <lpel.h>

#define MON_CB(name) (_MON_CB_MEMBER(_lpel_global_config.mon,name))
#define _MON_CB_MEMBER(glob,member) (glob.member)


extern lpel_config_t    _lpel_global_config;

#endif /* _LPELCFG_H_ */
