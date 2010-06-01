/**
 * Main LPEL module
 * contains:
 * - startup and shutdown routines
 * - worker thread code
 * - (stream and task management)
 *
 *
 */


struct workerdata {
  /* ID */
  unsigned int id;
  /* init queue */
  /* ready queue(s) */
  void *readyQ;
  /* waiting queue */
  /* current task */
  /* monitoring output info */
};



/**
 * Initialise the LPEL
 * - if num_workers==-1, determine the number of worker threads automatically
 * - create the data structures for each worker thread
 * - create necessary TSD
 */
void LpelInit(int num_workers)
{
  // TODO
}

/**
 * Create and execute the worker threads
 * - joins on the worker threads
 */
void LpelRun(void)
{
  // TODO
}


/**
 * Cleans the LPEL up
 * - free the data structures of worker threads
 * - free the TSD
 */
void LpelCleanup(void)
{
  // TODO
}


/**
 * Worker thread code
 */
void *LpelWorker(void *data)
{
  /* data contains id, set as thread specific worker_id */
  /* set affinity to id=CPU */

  /* set scheduling policy */


  /* MAIN LOOP */
  /* fetch new tasks from InitQ, insert into ReadyQ (sched) */
  /* select a task from the ReadyQ (sched) */
  /* set current_task */
  /* start timing (mon) */
  /* context switch */
  /* end timing (mon) */
  /* output accounting info (mon) */
  /* check state of task, place into appropriate queue */
  /* iterate through waiting queue, check r/w events */
  /* (iterate through nap queue, check alert-time) */
  /* MAIN LOOP END */
  
  /* exit thread */
}
