
#define _GNU_SOURCE

#include <sched.h>
#include <errno.h>
#include <unistd.h>  /* sysconf() */
#include <sys/types.h> /* pid_t */

#include <linux/unistd.h>
#include <sys/syscall.h> 



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
 * On success, returns 0; otherwise errno
 */
int CpuAssignToCore(int coreid)
{
  int res;
  cpu_set_t cpuset; 
  pid_t tid;
  struct sched_param param;
  tid = gettid();
  CPU_ZERO(&cpuset);

  /*TODO: check for range? or as precond */
  CPU_SET( coreid , &cpuset);
  
  res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
  if (res == -1) {
    /*TODO handle_error_en(errno, "sched_setaffinity"); */
    return errno;
  }

  /*TODO check permissions */
  param.sched_priority = 1; /* lowest real-time */

  /* following can also be SCHED_FIFO */
  res = sched_setscheduler(tid, SCHED_RR, &param);
  if (res == -1) {
    /*TODO handle_error_en(errno, "sched_setscheduler"); */
    return errno;
  }

  return 0;
}
