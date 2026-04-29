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
#include "lws_port.h"
#include "lws_internal.h"

/*******************************************************************************
;
;	M A C R O S
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

static void lws__taskTimerExpFn(
	lws_Timer * pTimer
);

static bool lws__timerListCompare(
	const lwl_DlListNode * pNode1,
	const lwl_DlListNode * pNode2,
	const void *		   pParam
);

static void idleTask(
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

lws_Tcb * lws__currTcb_p = NULL;

static volatile lws_Ticks lws__timestamp = 0;

static lwl_DlList lws__readyTasks;
static lwl_DlList lws__runningTimers;

static lws_Tcb idleTaskTcb;

const lwl_DlListCmp lws__taskCompare = {
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
	lws__portInit();

	lwl_dlListInit(&lws__readyTasks);
	lwl_dlListInit(&lws__runningTimers);

	lws_initTask(&idleTaskTcb, idleTask, LWS_PRIO_IDLE);
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
	if (pTcb != NULL) {
		lwl_dlListNodeInit(&pTcb->listNode);
		pTcb->location = 0U;
		pTcb->pTaskFn = pTaskFn;
		pTcb->currPrio = priority;

		pTcb->taskTimer.pExpFn = lws__taskTimerExpFn;

		lwl_dlListInsert(
			&lws__readyTasks,
			&pTcb->listNode,
			&lws__taskCompare
		);
	}
}

/** ****************************************************************************
 *
 * 	\brief		Run the scheduler.
 *
 * 	\note		This function never returns.
 *
 ******************************************************************************/
void lws_runScheduler(
	void
) {
	for (;;) {
		lwl_DlListNode * pNode = NULL;

		/*-------------------------- Critical section ------------------------*/
		{
			lws__PortIntState intState = 0;
			lws__portDisableIntr(&intState);

			pNode = lwl_dlListPopFirst(&lws__readyTasks);

			lws__portRestoreIntr(intState);
		}
		/*-------------------------- Critical section ------------------------*/

		lws__currTcb_p = lws__getStructPtr(lws_Tcb, listNode, pNode);

		lws__currTcb_p->pTaskFn();
	}
}

/** ****************************************************************************
 *
 * 	\brief		Put the supplied task in ready state.
 *
 * 	\param		pTcb		Pointer to the TCB of the task.
 *
 * 	\retval		true		The current task should yield.
 * 	\retval		false		The current task may continue.
 *
 * 	\note		May not be called from an ISR.
 *
 ******************************************************************************/
bool lws__makeReady(
	lws_Tcb * pTcb
) {
	bool doSwitch = false;

	/*-------------------------- Critical section ----------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntr(&intState);

		lwl_dlListInsert(&lws__readyTasks, &pTcb->listNode, &lws__taskCompare);

		/*
		 * Check if the current task should yield because of the new
		 * ready task.
		 */
		if (pTcb->currPrio > lws__currTcb_p->currPrio) {
			/*
			 * The task has a higher priority than the current task. The
			 * current task should yield.
			 */
			lwl_dlListInsert(
				&lws__readyTasks,
				&lws__currTcb_p->listNode,
				&lws__taskCompare
			);

			doSwitch = true;
		}

		lws__portRestoreIntr(intState);
	}
	/*-------------------------- Critical section ---------------------------*/

	return doSwitch;
}

/** ****************************************************************************
*
* 	\brief		Put the supplied task in ready state.
*
* 	\param		pTcb		Pointer to the TCB of the task.
*
* 	\retval		true		The current task should yield.
* 	\retval		false		The current task may continue.
*
* 	\note		May not be called from an ISR.
*
*******************************************************************************/
void lws_resumeIsr(
	lws_Tcb * pTcb
) {
	/*-------------------------- Critical section ----------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntrIsr(&intState);

		lwl_dlListInsert(&lws__readyTasks, &pTcb->listNode, &lws__taskCompare);

		lws__portRestoreIntrIsr(intState);
	}
	/*-------------------------- Critical section ---------------------------*/
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
	const lwl_DlListCmp cmp = {
		lws__timerListCompare,
		NULL
	};

	lwl_dlListNodeInit(&pTimer->listNode);

	/*-------------------------- Critical section ----------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntr(&intState);

		pTimer->expTime = lws__timestamp + timeout;
		lwl_dlListInsert(
			&lws__runningTimers,
			&pTimer->listNode,
			&cmp
		);

		lws__portRestoreIntr(intState);
	}
	/*-------------------------- Critical section ----------------------------*/
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
	/*-------------------------- Critical section ----------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntrIsr(&intState);

		lws__timestamp++;

		for (;;) {
			lwl_DlListNode * pTimerNode = lwl_dlListPeekFirst(
				&lws__runningTimers
			);
			if (pTimerNode == NULL) {
				break;
			}

			lws_Timer * pTimer = lws__getStructPtr(
				lws_Timer,
				listNode,
				pTimerNode
			);

			if (pTimer->expTime != lws__timestamp) {
				break;
			}

			/*
			 * The timer has expired.
			 */
			lwl_dlListRemove(&pTimer->listNode);

			pTimer->pExpFn(pTimer);
		}

		lws__portRestoreIntrIsr(intState);
	}
	/*-------------------------- Critical section ----------------------------*/
}

/** ****************************************************************************
 *
 * 	\brief		Task timer expiration function.
 *
 * 	\param		pTimer		Pointer to the expired task timer.
 *
 * 	\details	Called when a task timer expires.
 *
 * 	\note		Called from an ISR with interrupts disabled!
 *
 ******************************************************************************/
static void lws__taskTimerExpFn(
	lws_Timer * pTimer
) {
	lws_Tcb * pTcb = lws__getStructPtr(lws_Tcb, taskTimer, pTimer);

	lwl_dlListInsert(
		&lws__readyTasks,
		&pTcb->listNode,
		&lws__taskCompare
	);
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
 * \retval		true	Node1 should be before node2.
 * \retval		false	Node2 should be before node1.
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
	const lwl_DlListNode * pNode1,
	const lwl_DlListNode * pNode2,
	const void *		   pParam
) {
	const lws_Tcb * pTask1 = lws__getStructPtr(lws_Tcb, listNode, pNode1);
	const lws_Tcb * pTask2 = lws__getStructPtr(lws_Tcb, listNode, pNode2);
	(void)pParam;

	return pTask1->currPrio > pTask2->currPrio;
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
	const lwl_DlListNode * pNode1,
	const lwl_DlListNode * pNode2,
	const void *		   pParam
) {
	const lws_Timer * pTimer1 = lws__getStructPtr(lws_Timer, listNode, pNode1);
	const lws_Timer * pTimer2 = lws__getStructPtr(lws_Timer, listNode, pNode2);
	(void)pParam;

	lws_Ticks currTime = lws__timestamp;

	/*
	 * Subtract the current tick count from the timer expiration timestamp to
	 * get remaining time until expiration.
	 */
	lws_Ticks timer1RemainingTicks = (lws_Ticks)(pTimer1->expTime - currTime);
	lws_Ticks timer2RemainingTicks = (lws_Ticks)(pTimer2->expTime - currTime);

	return timer1RemainingTicks >= timer2RemainingTicks;
}

/** ****************************************************************************
*
* 	\brief		The idle task.
*
* 	\details	The scheduler will run this task if no other task is available.
* 
*	\note		This task may never use LWS_WAIT or LWS_SEMA_WAIT.
*
******************************************************************************/
static void idleTask(
	void
) {
	LWS_TASK_START();

	while (1) {
		LWS_YIELD();
	}

	LWS_TASK_END();
}

