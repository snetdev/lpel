#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_PROC_CAPABILITIES
#include <sys/capability.h>
#endif

#include "lpel_common.h"
#include "lpelcfg.h"

#include "lpel_hwloc.h"

static int pu_count;

#ifdef HAVE_HWLOC
static hwloc_cpuset_t   *cpu_sets;
static lpel_hw_place_t  *hw_places;

static hwloc_topology_t topology = 0;

#elif defined(HAVE_PTHREAD_SETAFFINITY_NP)
/* cpuset for others-threads = [proc_workers, ...]
 * is used if FLAG_EXCLUSIVE is set & FLAG_PINNED is not set
 * */
static cpu_set_t cpuset_others;
static int offset_others = 0;
static int rot_others;
static int proc_others;

/*
 * cpuset for workers = [0,proc_workers-1]
 * is only used if not FLAG_PINNED is set
 */
static cpu_set_t cpuset_workers;
static int rot_workers;
static int proc_workers;
//static int offset_workers = 0;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */

static int LpelCanSetExclusive(int *result)
{
#ifdef HAVE_PROC_CAPABILITIES
  cap_t caps;
  cap_flag_value_t cap;
  /* obtain caps of process */
  caps = cap_get_proc();
  if (caps == NULL) {
    return LPEL_ERR_FAIL;
  }
  cap_get_flag(caps, CAP_SYS_NICE, CAP_EFFECTIVE, &cap);
  *result = (cap == CAP_SET);
  return 0;
#else
  return LPEL_ERR_EXCL;
#endif
}

#ifdef HAVE_HWLOC
inline static void traverse(hwloc_obj_t object)
{
  static int index = 0, socket = -1, core = -1, pu = -1;

  assert(index < pu_count);
  switch (object->type) {
    case HWLOC_OBJ_SOCKET:
        socket++;
        core = -1;
        pu = -1;
        break;
    case HWLOC_OBJ_CORE:
        core++;
        pu = -1;
        break;
    case HWLOC_OBJ_PU:
        pu++;
        hw_places[index].socket = socket;
        hw_places[index].core = core;
        hw_places[index].pu = pu;
        cpu_sets[index] = hwloc_bitmap_dup(object->cpuset);
        index++;
        break;
    default:
        break;
  }

  for (int i = 0; i < object->arity; i++) {
    traverse(object->children[i]);
  }
}

int LpelHwLocToWorker(lpel_hw_place_t place)
{
    for (int i = 0; i < pu_count; i++) {
        if (hw_places[i].socket == place.socket &&
                hw_places[i].core == place.core &&
                hw_places[i].pu == place.pu) return i;
    }

    return -1;
}

lpel_hw_place_t LpelWorkerToHwLoc(int wid) { return hw_places[wid]; }
#endif

void LpelHwLocInit(lpel_config_t *cfg)
{
#ifdef HAVE_HWLOC
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  pu_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

  cpu_sets = malloc(pu_count * sizeof(hwloc_cpuset_t));
  hw_places = malloc(pu_count * sizeof(lpel_hw_place_t));

  traverse(hwloc_get_root_obj(topology));
#elif defined(HAVE_SYSCONF)
  /* query the number of CPUs */
  pu_count = sysconf(_SC_NPROCESSORS_ONLN);
#else
  int res = LpelGetNumCores( &pu_count);
  if(res == 0)
  	pu_count = res;
#endif
}

int LpelHwLocCheckConfig(lpel_config_t *cfg)
{
	  /* input sanity checks */
	  if (cfg->type == DECEN_LPEL) {
	  	if ( cfg->num_workers <= 0 ||  cfg->proc_workers <= 0 )
	  		return LPEL_ERR_INVAL;
	  } else if (cfg->type == HRC_LPEL) {
	  	if ( cfg->num_workers <= 1 ||  cfg->proc_workers <= 0)
	  	  return LPEL_ERR_INVAL;
	  }

	  if ( cfg->proc_others < 0 ) {
	    return LPEL_ERR_INVAL;
	  }

	  /* check if there are enough processors (if we can check) */
	  if(pu_count != LPEL_ERR_FAIL) {		/* in case can not read the number of cores, no need to check */
	  	if (cfg->proc_workers + cfg->proc_others > pu_count) {
	  		return LPEL_ERR_INVAL;
	  	}
	  	/* check exclusive flag sanity */
	  	if ( LPEL_ICFG( LPEL_FLAG_EXCLUSIVE) ) {
	  		/* check if we can do a 1-1 mapping */
	  		if ( (cfg->proc_others== 0) || (cfg->num_workers > cfg->proc_workers) ) {
	  			return LPEL_ERR_INVAL;
	  		}
	  	}
	  }
	  /* additional flags for exclusive flag */
	  if ( LPEL_ICFG( LPEL_FLAG_EXCLUSIVE) ) {
	  	int can_rt;
	    /* pinned flag must also be set */
	    if ( !LPEL_ICFG( LPEL_FLAG_PINNED) ) {
	      return LPEL_ERR_INVAL;
	    }
	    /* check permissions to set exclusive (if we can check) */
	    if ( 0==LpelCanSetExclusive(&can_rt) && !can_rt ) {
	          return LPEL_ERR_EXCL;
	    }
	  }

	  return 0;
}

void LpelHwLocStart(lpel_config_t *cfg)
{
#ifdef HAVE_HWLOC
    //FIXME
#elif defined(HAVE_PTHREAD_SETAFFINITY_NP)
	int  i;

	/* create the cpu_set for worker threads */
	proc_workers = cfg->proc_workers;
	CPU_ZERO( &cpuset_workers );
	for (i=0; i < proc_workers; i++) {
		CPU_SET(i, &cpuset_workers);
	}

	/* create the cpu_set for other threads */
	CPU_ZERO( &cpuset_others );
	if (cfg->proc_others == 0) {
		offset_others = 0;
		proc_others = proc_workers;
		/* distribute on the workers */
		for (i = 0; i< proc_workers; i++) {
			CPU_SET(i, &cpuset_others);
		}
	} else {
		offset_others = cfg->proc_workers;
		proc_others = cfg->proc_others;
		/* set to proc_others */
		for( i = cfg->proc_workers;
				i< cfg->proc_workers + cfg->proc_others;
				i++ ) {
			CPU_SET(i, &cpuset_others);
		}
	}
#endif
}

int LpelThreadAssign(int core)
{
  int res;
#ifdef HAVE_HWLOC
  //FIXME
  if (core < 0) return 0;

  res = hwloc_set_cpubind(topology, cpu_sets[core],
                          HWLOC_CPUBIND_THREAD | HWLOC_CPUBIND_STRICT);

  if (res == -1) return LPEL_ERR_ASSIGN;
#elif defined(HAVE_PTHREAD_SETAFFINITY_NP)
  lpel_config_t *cfg = &_lpel_global_config;
  pthread_t pt = pthread_self();
  cpu_set_t cpuset;

  if ( LPEL_ICFG(LPEL_FLAG_PINNED)) {
  	CPU_ZERO(&cpuset);
  	switch(core) {
  	case LPEL_MAP_OTHERS:	/* round robin pinned to cores in the set */
  		CPU_SET(rot_others + offset_others, &cpuset);
  		rot_others = (rot_others + 1) % proc_others;
  		break;

  	default:	// workers
  		/* assign to specified core */
  		assert( 0 <= core && core < cfg->num_workers );
  		CPU_SET( core % proc_workers, &cpuset);
  	}
  }
  else {
  	switch (core) {
  	case LPEL_MAP_OTHERS:
  		cpuset = cpuset_others;
  		break;
  	default: // workers
  		cpuset = cpuset_workers;
  	}
  }

  res = pthread_setaffinity_np(pt, sizeof(cpu_set_t), &cpuset);
  if( res != 0) return LPEL_ERR_ASSIGN;

  /* make non-preemptible for workers only */
  if ( LPEL_ICFG(LPEL_FLAG_EXCLUSIVE) && core != LPEL_MAP_OTHERS) {
  	struct sched_param param;
  	int sp = SCHED_FIFO;
  	/* highest real-time */
  	param.sched_priority = sched_get_priority_max(sp);
  	res = pthread_setschedparam(pt, sp, &param);
  	if ( res != 0) {
  		/* we do best effort at this point */
  		return LPEL_ERR_EXCL;
  	} else {
  		fprintf(stderr, "set realtime priority %d for worker %d.\n",
  				param.sched_priority, core);
  	}
  }

#endif
  return 0;
}

void LpelHwLocCleanup(void)
{
#ifdef HAVE_HWLOC
  if (topology) {
    free(cpu_sets);
    free(hw_places);
    hwloc_topology_destroy(topology);
  }
#endif
}
