#include "PIDController.h"

PIDController pid_create(double p, double i, double d) {
	PIDController std;

	std.pGain = p;
	std.iGain = i;
	std.dGain = d;

	std.errorSum = 0.0;
	std.lastError = 0.0;
	std.output = 0.0;

	return std;
}

void pid_calc(PIDController *pid, double setpoint, double current) {
	double error = setpoint - current;

	double pOut = pid->pGain * error;

	pid->errorSum += error;
	double iOut = pid->iGain * pid->errorSum;

	double changeInError = error - pid->lastError;
	double dOut = pid->dGain * changeInError;

	pid->lastError = error;

	pid->output = pOut + iOut + dOut;
}
