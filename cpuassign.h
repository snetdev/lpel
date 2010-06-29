
#ifndef _CPUASSIGN_H_
#define _CPUASSIGN_H_


#define CPUASSIGN_USE_CAPABILITIES

extern bool CpuAssignCanExclusively(void);
extern int  CpuAssignQueryNumCpus(void);
extern int  CpuAssignToCore(int coreid);


#endif /* _CPUASSIGN_H_ */

