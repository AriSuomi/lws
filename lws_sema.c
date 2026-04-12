/** ****************************************************************************
*
* 	\file	lws.c
*
* 	\brief	LwS semaphore handling.
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

lws_Tcb * lws__semaSignalInternal(
	lws__Sema * pSema
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

/*******************************************************************************
;
;	F U N C T I O N   D E F I N I T I O N S
;
;-----------------------------------------------------------------------------*/

/** ****************************************************************************
*
* 	\brief		Initialize a semaphore.
*
* 	\param		pSema		Pointer to the semaphore.
*
******************************************************************************/
void lws_semaInit(
	lws__Sema * pSema
) {
	lwo_dlListInit(&pSema->waitingTasks);
}

/** ****************************************************************************
*
* 	\brief		Wait for a semaphore.
*
* 	\param		pSema		Pointer to the semaphore.
*
* 	\details	This function will add the current task to the semaphore's
* 				waiting list if the semaphore is not available.
*
******************************************************************************/
bool lws__semaWait(
	lws__Sema * pSema
) {
	bool should_suspend = true;
	/*-------------------------- Critical section ------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntr(&intState);

		const lwo_DlListNode * pFirstNode = lwo_dlListPeekFirst(
			&pSema->waitingTasks
		);

		if (pFirstNode == (lwo_DlListNode *)pSema) {
			/*
			 * The semaphore has been signaled when no task was waiting for it.
			 * In this case a pointer to the semaphore itself has been added to
			 * the waiting list. We just clear the pointer and continue
			 * execution.
			 */
			lwo_dlListSneekFirst(&pSema->waitingTasks, NULL);
			should_suspend = false;

		} else {
			/*
			* The semaphore has not been signaled.
			* Add the current task to the waiting list.
			*/
			lwo_dlListInsert(
				&pSema->waitingTasks,
				&lws__currTcb_p->listNode,
				&lws__taskCompare
			);
		}

		lws__portRestoreIntr(intState);
	}
	/*-------------------------- Critical section ------------------------*/
	return should_suspend;
}

/** ****************************************************************************
*
* 	\brief		Signal a semaphore.
*
* 	\param		pSema		Pointer to the semaphore.
*
* 	\retval		true		A higher priority task was made ready.
* 	\retval		false		No task switch is needed.
*
******************************************************************************/
bool lws__semaSignal(
	lws__Sema * pSema
) {
	bool doSwitch = false;

	/*-------------------------- Critical section ------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntr(&intState);

		lws_Tcb * pTcb = lws__semaSignalInternal(pSema);
		if (pTcb != NULL) {
			doSwitch = lws__makeReady(pTcb);
		}

		lws__portRestoreIntr(intState);
	}
	/*-------------------------- Critical section ------------------------*/

	return doSwitch;
}

/** ****************************************************************************
*
* 	\brief		Signal a semaphore from an ISR.
*
* 	\param		pSema		Pointer to the semaphore.
*
******************************************************************************/
void lws_semaSignalIsr(
	lws__Sema * pSema
) {
	/*-------------------------- Critical section ------------------------*/
	{
		lws__PortIntState intState = 0;
		lws__portDisableIntrIsr(&intState);

		lws_Tcb * pTcb = lws__semaSignalInternal(pSema);
		if (pTcb != NULL) {
			lws_resumeIsr(pTcb);
		}

		lws__portRestoreIntrIsr(intState);
	}
	/*-------------------------- Critical section ------------------------*/
}

/** ****************************************************************************
*
* 	\brief		Signal a semaphore.
*
* 	\param		pSema		Pointer to the semaphore.
*
* 	\return		Pointer to the task that should be set in ready state. NULL if
*				no task should be set in ready state.
*
******************************************************************************/
lws_Tcb * lws__semaSignalInternal(
	lws__Sema * pSema
) {
	lws_Tcb * pReadyTcb = NULL;
	lwo_DlListNode * pNode = lwo_dlListPeekFirst(&pSema->waitingTasks);

	if (pNode != (lwo_DlListNode *)pSema) {
		if (pNode == NULL) {
			/*
			* No task is waiting for the semaphore. Store a pointer to the
			* semaphore itself to mark it as signaled.
			*/
			lwo_dlListSneekFirst(
				&pSema->waitingTasks,
				(lwo_DlListNode *)pSema
			);

		} else {
			/*
			* A task was waiting for the semaphore.
			* Make it ready.
			*/
			lwo_dlListRemove(pNode);

			pReadyTcb = lws__getStructPtr(lws_Tcb, listNode, pNode);
		}
	}

	return pReadyTcb;
}