
#ifndef _LPELCFG_H_
#define _LPELCFG_H_


#include <lpel_common.h>

#define MON_CB(name) (_MON_CB_MEMBER(_lpel_global_config.mon,name))
#define _MON_CB_MEMBER(glob,member) (glob.member)

#define PRIO_CFG(name) (_PRIO_CFG_MEMBER(_lpel_global_config.prio_config,name))
#define _PRIO_CFG_MEMBER(glob,member) (glob.member)

/* test if flags are set in lpel config */
#define LPEL_ICFG(f)   ( (_lpel_global_config.flags & (f)) == (f) )


extern lpel_config_t    _lpel_global_config;

#endif /* _LPELCFG_H_ */
