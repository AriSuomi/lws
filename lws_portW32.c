

#include <Windows.h>

#include "lws_port.h"

static uint_fast32_t lwo__portIsrDisabled = 0;
static uint_fast32_t lwo__portInIsr = 0;

static CRITICAL_SECTION lwo__portCriticalSection;
static CRITICAL_SECTION lwo__portIsrDisable;

/** ****************************************************************************
*
* 	\brief		Get current interrupt state and disable interrupts.
*
* 	\param[out]	pIntState_	Pointer to variable where the current interrupt
* 							state will be stored.
*
* 	\note		May be called from an ISR.
*
******************************************************************************/
void lws__portDisableIntr(
	lwo__PortIntState * pIntState
) {
	EnterCriticalSection(&lwo__portIsrDisable);
	*pIntState = lwo__portIsrDisabled;
	lwo__portIsrDisabled = 1U;
}

/** ****************************************************************************
*
* 	\brief 		Restore interrupt state.
*
* 	\param		oldValue_	The interrupt state that should be restored.
*
* 	\details	No interrupt enabling is needed for targets without nested
* 				interrupts. For those targets this macro can be left empty.
*
* 	\note		May be called from an ISR.
*
******************************************************************************/
void lws__portRestoreIntr(
	lwo__PortIntState oldValue
) {
	lwo__portIsrDisabled = oldValue;
	LeaveCriticalSection(&lwo__portIsrDisable);
}

