
#define _GNU_SOURCE

#include <sched.h>
#include <unistd.h>  /* sysconf() */
#include <sys/types.h> /* pid_t */

#include <linux/unistd.h>
#include <sys/syscall.h> 


#include "cpuassign.h" /* defines CPUASSIGN_USE_CAPABILITIES */



/*!! link with -lcap */
#ifdef CPUASSIGN_USE_CAPABILITIES
#  include <sys/capability.h>
#endif


/* macro using syscall for gettid, as glibc doesn't provide a wrapper */
#define gettid() syscall( __NR_gettid )

/*
 * Query the system for number of CPUs
 */
int CpuAssignQueryNumCpus(void)
{
  int num_cpus;
  /* query the number of CPUs */
  num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (num_cpus == -1) {
    /*TODO _SC_NPROCESSORS_ONLN not available! */
  }
  return num_cpus;
}

/*
 * Assigns the current OS thread to a CPU
 * Returns success.
 */
bool CpuAssignToCore(int coreid)
{
  int res;
  pid_t tid;
  cpu_set_t cpuset; 

  tid = gettid();
  CPU_ZERO(&cpuset);

  /*TODO: check for range? or as precond */
  CPU_SET( coreid , &cpuset);
  
  res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
  if (res == -1) {
    /*TODO check errno? */
    return false;
  }

  return true;

}


bool CpuAssignSetRealtime(int rt)
{
  int res;
  pid_t tid;
  struct sched_param param;

  tid = gettid();
  if ( rt == 0 ) {
    param.sched_priority = 0; /* default nice value */
    res = sched_setscheduler(tid, SCHED_OTHER, &param);
  } else {
    param.sched_priority = 1; /* lowest real-time, TODO other? */
    res = sched_setscheduler(tid, SCHED_FIFO, &param);
  }

  if (res == -1) {
    /*TODO check errno? */
    return false;
  }
  return true;
}

bool CpuAssignCanRealtime(void)
{
#ifdef CPUASSIGN_USE_CAPABILITIES
  cap_t caps;
  cap_flag_value_t cap;

  caps = cap_get_proc();
  if (caps == NULL) {
    /*TODO error msg*/
    return false;
  }

  cap_get_flag(caps, CAP_SYS_NICE, CAP_EFFECTIVE, &cap);

  return (cap == CAP_SET);
#else
  return false;
#endif

}
