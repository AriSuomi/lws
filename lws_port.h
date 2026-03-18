
#ifndef LWS_PORT_H_INCLUDED
#define LWS_PORT_H_INCLUDED

#include <stdint.h>

/**
 * Type for storing interrupt enable/disable state.
 */
typedef uint32_t lws__PortIntState;

void lws__portDisableIntr(
	lws__PortIntState * pIntState
);

void lws__portRestoreIntr(
	lws__PortIntState oldValue
);

void lws__portDisableIntrIsr(
	lws__PortIntState * pIntState
);

void lws__portRestoreIntrIsr(
	lws__PortIntState oldValue
);

#endif
