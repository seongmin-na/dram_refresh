
 /*
 * AxiTimerHelper.h
 *
 *  Created on: 2019. 8. 6.
 *      Author: �ϱ�
 */

#ifndef AXITIMERHELPER_H_
#define AXITIMERHELPER_H_

#include "xil_types.h"
#include "xtmrctr.h"
#include "xparameters.h"

typedef struct
{
	XTmrCtr m_AxiTimer;
	unsigned long m_tickCounter1;
	unsigned long m_tickCounter2;
	double m_clockPeriodSeconds;
	double m_timerClockFreq;
}AxiTimerHelper;

void timerInitialize(AxiTimerHelper* axi_timer_helper);
unsigned long getElapsedTicks(AxiTimerHelper* axi_timer_helper);
double getElapsedTimerInSeconds(AxiTimerHelper *axi_timer_helper);
unsigned long startTimer(AxiTimerHelper *axi_timer_helper);
unsigned long stopTimer(AxiTimerHelper *axi_timer_helper);
double getClockPeriod(AxiTimerHelper *axi_timer_helper);
double getTimerClockFreq(AxiTimerHelper *axi_timer_helper);
unsigned getElapsedTimerInMiliSeconds(AxiTimerHelper *axi_timer_helper);



#endif /* AXITIMERHELPER_H_ */
