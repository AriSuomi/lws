/** ****************************************************************************
 *
 * 	\file	lws.c
 *
 * 	\brief	LwS implementation file.
 *
 ******************************************************************************/
/*
 *  LwS version 0.1
 *
 *  2026-03-18
 *
 *  Copyright 2026 Ari Suomi
 *
 *------------------------------------------------------------------------------
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 *  EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/*******************************************************************************
;
;	I N C L U D E S
;
;-----------------------------------------------------------------------------*/

#include "lws.h"

/*******************************************************************************
;
;	D E F I N E S
;
;-----------------------------------------------------------------------------*/

/*******************************************************************************
;
;	T Y P E S
;
;-----------------------------------------------------------------------------*/

/*******************************************************************************
;
;	S T A T I C   F U N C T I O N S
;
;-----------------------------------------------------------------------------*/

static bool lws__taskPrioCompare(
	const lwo_DlListNode * pNode1,
	const lwo_DlListNode * pNode2,
	const void *		   pParam
);

static bool lws__timerListCompare(
	const lwo_DlListNode * pNode1,
	const lwo_DlListNode * pNode2,
	const void *		   pParam
);

static lws_Tcb * lws_popFirstReady(
	void
);

/*******************************************************************************
;
;	I N L I N E   F U N C T I O N S
;
;-----------------------------------------------------------------------------*/

/*******************************************************************************
;
;	D A T A
;
;-----------------------------------------------------------------------------*/

lws_Tcb * lwo__currTcb_p = NULL;

static volatile lws_Ticks lws__timestamp = 0;

static lwo_DlList lws__readyTasks;
static lwo_DlList lws__timers;

static const lwo_DlListCmp lws__taskCompare = {
	lws__taskPrioCompare,
	NULL
};

/*******************************************************************************
;
;	F U N C T I O N   D E F I N I T I O N S
;
;-----------------------------------------------------------------------------*/

/** ****************************************************************************
 *
 * 	\brief		Initialize the scheduler.
 *
 * 	\note		Must be called before other LWS functions.
 *
 ******************************************************************************/
void lws_init(
	void
) {
	lwo_dlListInit(&lws__readyTasks);
	lwo_dlListInit(&lws__timers);
}

/** ****************************************************************************
 *
 * 	\brief		Initialize a TCB of a task.
 *
 * 	\param		pTcb		Pointer to the TCB of the task.
 * 	\param		pTaskFn		Pointer to the task function.
 *  \param		priority	The desired task priority.
 *
 * 	\note		May not be called from an ISR.
 *
 ******************************************************************************/
void lws_initTask(
	lws_Tcb *	 pTcb,
	lws_TaskFn * pTaskFn,
	lws_Priority priority
) {
	if (pTcb) {
		lwo_dlListNodeInit(&pTcb->listNode);
		pTcb->location = 0U;
		pTcb->pTaskFn = pTaskFn;
		pTcb->currPrio = priority;

		lwo_dlListInsert(&lws__readyTasks, &pTcb->listNode, &lws__taskCompare);
	}
}

/** ****************************************************************************
 *
 * 	\brief		Run the scheduler.
 *
 * 	\note		This function never returns.
 *
 ******************************************************************************/
void lws_runSceduler(
	void
) {
	lwo__currTcb_p = lws_popFirstReady();

	while (1) {
		lwo__currTcb_p->pTaskFn();
		lwo__currTcb_p = lws_popFirstReady();
	}
}

/** ****************************************************************************
 *
 * 	\brief		Put the supplied task in ready state.
 *
 * 	\param		pTcb		Pointer to the TCB of the task.
 *
 * 	\note		May not be called from an ISR.
 *
 ******************************************************************************/
bool lws_makeReady(
	lws_Tcb * pTcb
) {
	bool doSwitch = false;

	if (pTcb) {
		lwo_dlListInsert(&lws__readyTasks, &pTcb->listNode, &lws__taskCompare);

		/*
		 * Check if the current task should yield because of the supplied
		 * ready task.
		 */
		if (pTcb->currPrio > lwo__currTcb_p->currPrio) {
			/*
			 * The task has a higher priority than the current task. The
			 * current task should yield.
			 */
			lwo_dlListInsert(
				&lws__readyTasks,
				&lwo__currTcb_p->listNode,
				&lws__taskCompare
			);

			doSwitch = true;
		}
	}

	return doSwitch;
}

/** ****************************************************************************
 *
 * 	\brief		Start a software timer.
 *
 * 	\param		pTimer		Pointer to the timer.
 * 	\param		timeout		Number of ticks before the timer should expire.
 *
 * 	\note		May not be called from an ISR.
 *
 ******************************************************************************/
void lws_startTimer(
	lws_Timer * pTimer,
	lws_Ticks	timeout
) {
	const lwo_DlListCmp cmp = {
		lws__timerListCompare,
		NULL
	};

	lwo_dlListNodeInit(&pTimer->listNode);
	pTimer->expTime = lws__timestamp + timeout;
	lwo_dlListInsert(&lws__timers, &pTimer->listNode, &cmp);
}

/** ****************************************************************************
 *
 * 	\brief		Handle timed events.
 *
 * 	\details	Increments the internal tick counter and handles software
 * 				timers.
 *
 * 	\note		Must be called from an ISR.
 *
 ******************************************************************************/
void lws_tickIsr(
	void
) {
	lws__timestamp++;

	lws_Timer * pTimer = (lws_Timer *)lwo_dlListPeekFirst(&lws__timers);
	while (pTimer != NULL && pTimer->expTime == lws__timestamp) {
		lwo_dlListRemove(pTimer);
		lws_makeReady(pTimer->pTcb);
		pTimer = (lws_Timer *)lwo_dlListPeekFirst(&lws__timers);
	}
}

/** ****************************************************************************
 *
 * 	\brief		Pop the first task from the ready tasks lists.
 *
 * 	\details	The first task is the task with the highest priority.
 *
 * 	\note		May not be called from an ISR.
 *
 ******************************************************************************/
static lws_Tcb * lws_popFirstReady(
	void
) {
	lwo_DlListNode * pNode = lwo_dlListPopFirst(&lws__readyTasks);
	return (lws_Tcb *)pNode;
}

/** ****************************************************************************
 *
 * 	\brief		Task priority comparison function.
 *
 * 	\param		pNode1	Pointer to the first node. This is the node already in
 * 						the list when inserting new nodes into a list
 * 	\param		pNode2 	Pointer to the second node. This is the new node when
 * 						inserting new nodes in lists.
 * 	\param		pParam	Pointer to comparison parameter.
 *
 * 	\details	This function makes sure that the tasks on the ready tasks list
 * 				are sorted in priority order with the first task having the
 * 				highest priority.
 *
 * 				Tasks with the same priority is added in the insertion order
 * 				i.e. tasks added first are popped first.
 *
 ******************************************************************************/
static bool lws__taskPrioCompare(
	const lwo_DlListNode * pNode1,
	const lwo_DlListNode * pNode2,
	const void *		   pParam
) {
	const lws_Tcb * pTask1 = (lws_Tcb *)pNode1;
	const lws_Tcb * pTask2 = (lws_Tcb *)pNode2;
	(void)pParam;

	return pTask1->currPrio >= pTask2->currPrio;
}

/** ****************************************************************************
 *
 * 	\brief		Timer expiration time comparison function.
 *
 * 	\param		pNode1	Pointer to the first node. This is the node already in
 * 						the list when inserting new nodes into a list
 * 	\param		pNode2 	Pointer to the second node. This is the new node when
 * 						inserting new nodes in lists.
 * 	\param		pParam	Pointer to comparison parameter.
 *
 * 	\details	This function makes sure that the timers on the running timers
 * 				list are sorted in expiration order.
 *
 ******************************************************************************/
static bool lws__timerListCompare(
	const lwo_DlListNode * pNode1,
	const lwo_DlListNode * pNode2,
	const void *		   pParam
) {
	const lws_Timer * pTimer1 = (lws_Timer *)pNode1;
	const lws_Timer * pTimer2 = (lws_Timer *)pNode2;

	lws_Ticks currTime = lws__timestamp;

	/*
	 * Subtract the current tick count from the timer expiration timestamp to
	 * get remaining time until expiration.
	 */
	lws_Ticks timer1RemainingTicks = (lws_Ticks)(pTimer1->expTime - currTime);
	lws_Ticks timer2RemainingTicks = (lws_Ticks)(pTimer2->expTime - currTime);

	return timer1RemainingTicks >= timer2RemainingTicks;
}
