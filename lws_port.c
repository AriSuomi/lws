
#include <windows.h>

#include "lws.h"
#include "lws_port.h"

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

static void CALLBACK multimediaTimerCallback(
	UINT		uTimerID,
	UINT		uMsg, 
	DWORD_PTR	dwUser,
	DWORD_PTR	dw1,
	DWORD_PTR	dw2
);

lws__PortIntState disableCount = 0;
CRITICAL_SECTION interruptCritSect;

void lws__portInit(
	void
) {
	InitializeCriticalSection(&interruptCritSect);

	MMRESULT tiemerId = timeSetEvent(
		10,
		0,
		multimediaTimerCallback,
		0,
		TIME_PERIODIC
	);
}

static void CALLBACK multimediaTimerCallback(
	UINT		uTimerID,
	UINT		uMsg, 
	DWORD_PTR	dwUser,
	DWORD_PTR	dw1,
	DWORD_PTR	dw2
) {
	EnterCriticalSection(&interruptCritSect);
	lws_tickIsr();
	LeaveCriticalSection(&interruptCritSect);
} 

void lws__portDisableIntr(
	lws__PortIntState * pIntState
) {
	EnterCriticalSection(&interruptCritSect);
	*pIntState = disableCount++;
}

void lws__portRestoreIntr(
	lws__PortIntState oldValue
) {
	if (oldValue == 0) {
		lws__PortIntState localCount = disableCount;

		while (localCount != 0) {
			localCount--;
			disableCount--;
			LeaveCriticalSection(&interruptCritSect);
		}
	}
}

void lws__portDisableIntrIsr(
	lws__PortIntState * pIntState
) {
	EnterCriticalSection(&interruptCritSect);
	*pIntState = disableCount++;
}

void lws__portRestoreIntrIsr(
	lws__PortIntState oldValue
) {
	if (oldValue == 0) {
		lws__PortIntState localCount = disableCount;

		while (localCount != 0) {
			localCount--;
			disableCount--;
			LeaveCriticalSection(&interruptCritSect);
		}
	}
}

