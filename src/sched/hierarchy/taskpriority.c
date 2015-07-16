#include <stdlib.h>
#include <float.h>
#include <time.h>
#include "taskpriority.h"
#include "hrc_lpel.h"

/****************************************************************
 * 14 different function to calculate task priority based on
 * 			in: the number of records in input streams
 * 					-1 indicates entry task
 * 			out: the number of records in ouput streams
 *					-1 indicates exit task
 *
 * Task, which is not entry task nor exit task, is middle task
 *
 *
 * Priority Function		| 		Middle Task			 | 		Entry Task		| 		Exit Task
 * --------------------------------------------------------------------------------
 *   1									|  (I + 1) / (O + 1) 	 |				0						|			infinity
 *   2									|  (I + 1) / (O + 1) 	 |				0						|			I + 1
 *   3									|  (I + 1) / (O + 1) 	 |	   1/(O + 1)			|			infinity
 *   4									|  (I + 1) / (O + 1)   |	   1/(O + 1)			|			I + 1
 *
 *   5									|  (I + 1)^2 / (O + 1) |				0						|			infinity
 *   6									|  (I + 1)^2 / (O + 1) |				0						|			(I + 1)^2
 *   7									|  (I + 1)^2 / (O + 1) |	   1/(O + 1)			|			infinity
 *   8									|  (I + 1)^2 / (O + 1) |	   1/(O + 1)			|			(I + 1)^2
 *
 *   9									|  (I + 1) / (O + 1)^2 	|				0						|			infinity
 *   10									|  (I + 1) / (O + 1)^2 	|				0						|			I + 1
 *   11									|  (I + 1) / (O + 1)^2 	|	   1/(O + 1)^2		|			infinity
 *   12									|  (I + 1) / (O + 1)^2 	|	   1/(O + 1)^2		|			I + 1
 *	 13									DYNAMIC RANDOM
 *	 14										I - O											- O										I
 *	 15									LOCATION-BASED
 *	 16									STATIC RANDOM (use the 13 one but not update every time task stop)
 ****************************************************************************************/

double priorfunc1(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return DBL_MAX;

	return (in + 1.0)/(out + 1.0);
}

double priorfunc2(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return in + 1.0;

	return (in + 1.0)/(out + 1.0);
}

double priorfunc3(int in, int out) {
	if (in == -1) {
		if (out == -1)
			out = 0;
		return (1.0/(1.0 + out));
	}
	if (out == -1)
		return DBL_MAX;

	return (in + 1.0)/(out + 1.0);
}

double priorfunc4(int in, int out) {
	if (in == -1) {
			if (out == -1)
				out = 0;
			return (1.0/(1.0 + out));
		}
		if (out == -1)
			return in + 1.0;

		return (in + 1.0)/(out + 1.0);
}


double priorfunc5(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return DBL_MAX;

	return (in + 1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc6(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return (in + 1.0) * (in + 1.0);

	return (in + 1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc7(int in, int out) {
	if (in == -1) {
		if (out == -1)
			out = 0;
		return (1.0/(1.0 + out));
	}
	if (out == -1)
		return DBL_MAX;

	return (in + 1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc8(int in, int out) {
	if (in == -1) {
			if (out == -1)
				out = 0;
			return (1.0/(1.0 + out));
		}

		if (out == -1)
			return (in + 1.0) * (in + 1.0);

		return (in + 1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc9(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return DBL_MAX;

	return (in + 1.0)/(out + 1.0)/(out + 1.0);
}

double priorfunc10(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return (in + 1.0);

	return (in + 1.0)/(out + 1.0)/(out + 1.0);
}

double priorfunc11(int in, int out) {
	if (in == -1) {
		if (out == -1)
			out = 0;
		return 1.0/(1.0 + out)/(out + 1.0);
	}
	if (out == -1)
		return DBL_MAX;

	return (in + 1.0)/(out + 1.0)/(out + 1.0);
}

double priorfunc12(int in, int out) {
	if (in == -1) {
			if (out == -1)
				out = 0;
			return 1.0/(1.0 + out)/(out + 1.0);
		}

		if (out == -1)
			return (in + 1.0);

		return (in + 1.0)/(out + 1.0)/(out + 1.0);
}

/*
 * random priority
 */
double priorrandom(int in, int out) {
	return (double)rand();
}

double priorfunc14(int in, int out) {
	in = (in == -1 ? 0 : in);
	out = (out == -1 ? 0 : out);
	return (in - out);
}


void LpelTaskPrioInit(lpel_task_prio_conf *conf) {
		srand(time(NULL));
		conf->prio_type = LPEL_DYN_PRIO;
		conf->update_neigh_prio = 1;
		switch (conf->prio_index){
		case 1: conf->prio_func = priorfunc1;
						break;
		case 2: conf->prio_func = priorfunc2;
						break;
		case 3: conf->prio_func = priorfunc3;
						break;
		case 4: conf->prio_func = priorfunc4;
						break;
		case 5: conf->prio_func = priorfunc5;
						break;
		case 6: conf->prio_func = priorfunc6;
						break;
		case 7: conf->prio_func = priorfunc7;
						break;
		case 8: conf->prio_func = priorfunc8;
						break;
		case 9: conf->prio_func = priorfunc9;
						break;
		case 10: conf->prio_func = priorfunc10;
						break;
		case 11: conf->prio_func = priorfunc11;
						break;
		case 12: conf->prio_func = priorfunc12;
						break;
		case 13: conf->prio_func = priorrandom;	// dynamic random
						break;
		case 14: conf->prio_func = priorfunc14;
						break;
		case 15: conf->prio_func = NULL;	// location-based
						conf->prio_type = LPEL_STC_PRIO;
						conf->update_neigh_prio = 0;
						break;
		case 16: conf->prio_func = priorrandom;			// static random
						conf->prio_type = LPEL_STC_PRIO;
						conf->update_neigh_prio = 0;
						break;
		default: conf->prio_func = priorfunc14;
						break;
		}

		/* set call back to rts */
		if (conf->prio_index != 15)
			conf->rts_prio_cmp = NULL;
}
