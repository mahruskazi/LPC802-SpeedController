/*
 * PIDController.h
 *
 *  Created on: Apr. 4, 2020
 *      Author: mkazi
 */

#ifndef PIDCONTROLLER_H_
#define PIDCONTROLLER_H_

typedef struct {
	double pGain;
	double iGain;
	double dGain;

	double errorSum;
	double lastError;

	double output;
} PIDController;

//typedef struct PIDController * pid;

PIDController pid_create(double p, double i, double d);
void pid_calc(PIDController *pid, double setpoint, double current);

#endif /* PIDCONTROLLER_H_ */



