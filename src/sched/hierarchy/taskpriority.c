#include <float.h>
#include "taskpriority.h"

/****************************************************************
 * 12 different function to calculate task priority based on
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
 *
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
