#include <float.h>
#include "taskpriority.h"

double priorfunc1(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return DBL_MAX;

	return (in +1.0)/(out + 1.0);
}

double priorfunc2(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return in + 1.0;

	return (in +1.0)/(out + 1.0);
}

double priorfunc3(int in, int out) {
	if (in == -1) {
		if (out == -1)
			out = 0;
		return (1.0/(1.0 + out));
	}
	if (out == -1)
		return DBL_MAX;

	return (in +1.0)/(out + 1.0);
}

double priorfunc4(int in, int out) {
	if (in == -1) {
			if (out == -1)
				out = 0;
			return (1.0/(1.0 + out));
		}
		if (out == -1)
			return in + 1.0;

		return (in +1.0)/(out + 1.0);
}


double priorfunc5(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return DBL_MAX;

	return (in +1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc6(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return (in + 1.0) * (in + 1.0);

	return (in +1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc7(int in, int out) {
	if (in == -1) {
		if (out == -1)
			out = 0;
		return (1.0/(1.0 + out));
	}
	if (out == -1)
		return DBL_MAX;

	return (in +1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc8(int in, int out) {
	if (in == -1) {
			if (out == -1)
				out = 0;
			return (1.0/(1.0 + out));
		}

		if (out == -1)
			return (in + 1.0) * (in + 1.0);

		return (in +1.0) * (in + 1.0)/(out + 1.0);
}

double priorfunc9(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return DBL_MAX;

	return (in +1.0)/(out + 1.0)/(out + 1.0);
}

double priorfunc10(int in, int out) {
	if (in == -1)
		return 0;
	if (out == -1)
		return (in + 1.0);

	return (in +1.0)/(out + 1.0)/(out + 1.0);
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

		return (in +1.0)/(out + 1.0)/(out + 1.0);
}
