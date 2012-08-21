#ifndef _LPEL_HWLOC_H_
#define _LPEL_HWLOC_H_

#ifdef HAVE_HWLOC
#include <hwloc.h>

typedef struct {
    int socket, core, pu;
} lpel_hw_place_t;

int LpelHwLocToWorker(lpel_hw_place_t place);
lpel_hw_place_t LpelWorkerToHwLoc(int i);
#endif

void LpelHwLocInit(lpel_config_t *cfg);
int LpelHwLocCheckConfig(lpel_config_t *cfg);
void LpelHwLocStart(lpel_config_t *cfg);
int LpelThreadAssign(int core);
void LpelHwLocCleanup(void);
#endif
