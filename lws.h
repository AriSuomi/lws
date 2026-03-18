/** ****************************************************************************
 *
 * 	\file	lws.h
 *
 * 	\brief	Header file with the public LwS interface.
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

#ifndef LWO_H_INCLUDED
#define LWO_H_INCLUDED

/*******************************************************************************
;
;	I N C L U D E S
;
;-----------------------------------------------------------------------------*/

#include <stdint.h>
#include "lwo_dlList.h"

/*******************************************************************************
;
;	M A C R O S
;
;-----------------------------------------------------------------------------*/

#define LWS_TASK_START()                \
	switch (lwo__currTcb_p->location) { \
		case 0:

#define LWS_TASK_END() \
	default:           \
		break;         \
		}

#define LWS_SUSPEND()                                \
	lwo__currTcb_p->location = (uint16_t)(__LINE__); \
	return;                                          \
	case __LINE__:

#define LWS_YIELD()                \
	lws_makeReady(lwo__currTcb_p); \
	LWS_SUSPEND()

#define LWS_RESUME(pTcb_)           \
	do {                            \
		if (lws_makeReady(pTcb_)) { \
			LWS_SUSPEND();          \
		}                           \
	} while (0)

#define LWS_WAIT(ticks_)                                       \
	do {                                                       \
		lws_start_timer(&lwo__currTcb_p->taskTimer, (ticks_)); \
		LWS_SUSPEND();                                         \
	} while (0)

/*******************************************************************************
;
;	T Y P E S
;
;-----------------------------------------------------------------------------*/

typedef void lws_TaskFn(void);

typedef uint32_t lws_Ticks;

typedef enum {
	LWS_PRIO_IDLE,
	LWS_PRIO_LOW,
	LWS_PRIO_NORMAL,
	LWS_PRIO_HIGH,
} lws_Priority;

typedef struct lws_timer_tag lws_Timer;

typedef void lws__TimerExpFn(
	lws_Timer * pTimer
);

struct lws_timer_tag {
	lwo_DlListNode	  listNode;
	lws_Ticks		  expTime;
	lws__TimerExpFn * pExpFn;
};

typedef struct {
	lwo_DlListNode listNode;
	uint16_t	   location;
	lws_TaskFn *   pTaskFn;
	lws_Priority   currPrio;
	lws_Timer	   taskTimer;
} lws_Tcb;

typedef struct {
	lwo_DlList waitingTasks;
} lws_Semaphore;

/*******************************************************************************
;
;	D A T A
;
;-----------------------------------------------------------------------------*/

extern lws_Tcb * lwo__currTcb_p;

/*******************************************************************************
;
;	F U N C T I O N S
;
;-----------------------------------------------------------------------------*/

void lws_init(
	void
);

void lws_initTask(
	lws_Tcb *	 pTcb,
	lws_TaskFn * pTaskFn,
	lws_Priority priority
);

void lws_runSceduler(
	void
);

void lws_startTimer(
	lws_Timer * pTimer,
	lws_Ticks	timeout
);

void lws_tickIsr(
	void
);

bool lws_makeReady(
	lws_Tcb * pTcb
);

#endif
