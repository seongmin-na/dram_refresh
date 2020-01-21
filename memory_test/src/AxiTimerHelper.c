/*
 * AxiTimerHelper.c

 *
 *  Created on: 2020. 1. 21.
 *      Author: Seongmin
 */


/*
 * AxiTimerHelper.c
 *
 *  Created on: 2019. 8. 6.
 *      Author: ÀÏ±Ç
 */

#include "AxiTimerHelper.h"

void timerInitialize(AxiTimerHelper* axi_timer_helper)
{
	XTmrCtr_Initialize((&axi_timer_helper->m_AxiTimer), XPAR_TMRCTR_0_DEVICE_ID);
	axi_timer_helper->m_timerClockFreq = (double) XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ;
	axi_timer_helper->m_clockPeriodSeconds = (double)1/(axi_timer_helper->m_timerClockFreq);
	return;
}

unsigned long getElapsedTicks(AxiTimerHelper* axi_timer_helper)
{
	return axi_timer_helper->m_tickCounter2 - axi_timer_helper->m_tickCounter1;
}

unsigned long startTimer(AxiTimerHelper *axi_timer_helper)
{
	XTmrCtr_Reset(&(axi_timer_helper->m_AxiTimer),0);
	axi_timer_helper->m_tickCounter1 = XTmrCtr_GetValue(&(axi_timer_helper->m_AxiTimer), 0);
	XTmrCtr_Start(&(axi_timer_helper->m_AxiTimer),0);
	return axi_timer_helper->m_tickCounter1;
}

unsigned long stopTimer(AxiTimerHelper *axi_timer_helper)
{
	XTmrCtr_Stop(&(axi_timer_helper->m_AxiTimer), 0);
	axi_timer_helper->m_tickCounter2 = XTmrCtr_GetValue(&(axi_timer_helper->m_AxiTimer), 0);
	return axi_timer_helper->m_tickCounter2 - axi_timer_helper->m_tickCounter1;
}

double getElapsedTimerInSeconds(AxiTimerHelper *axi_timer_helper)
{
	double elapsedTimeInSeconds = (double) (axi_timer_helper->m_tickCounter2 - axi_timer_helper->m_tickCounter1) * axi_timer_helper->m_clockPeriodSeconds;
	return elapsedTimeInSeconds;
}

unsigned getElapsedTimerInMiliSeconds(AxiTimerHelper *axi_timer_helper)
{
	unsigned elapsedTimeInMiliSeconds = (unsigned) (axi_timer_helper->m_tickCounter2 - axi_timer_helper->m_tickCounter1) * axi_timer_helper->m_clockPeriodSeconds * 1000;
	return elapsedTimeInMiliSeconds;
}

double getClockPeriod(AxiTimerHelper *axi_timer_helper)
{
	return axi_timer_helper->m_clockPeriodSeconds;
}

double getTimerClockFreq(AxiTimerHelper *axi_timer_helper)
{
	return axi_timer_helper->m_timerClockFreq;
}

