
#ifndef _CPUASSIGN_H_
#define _CPUASSIGN_H_

#include "bool.h"

#define CPUASSIGN_USE_CAPABILITIES


extern bool CpuAssignCanExclusively(void);
extern int  CpuAssignQueryNumCpus(void);
extern bool CpuAssignToCore(int coreid);
extern bool CpuAssignSetRealtime(int rt);


#endif /* _CPUASSIGN_H_ */

