
#include <stdio.h>

#include "lws.h"

static void myTaskFn1(void);
static void myTaskFn2(void);
static void idleTask(void);

static lws_Tcb myTask1;
static lws_Tcb myTask2;
static lws_Tcb idleTaskTcb;

int main() {
    lws_init();

	lws_initTask(&myTask1, myTaskFn1, LWS_PRIO_NORMAL);
	lws_initTask(&myTask2, myTaskFn2, LWS_PRIO_NORMAL);
	lws_initTask(&idleTaskTcb, idleTask, LWS_PRIO_IDLE);

    lws_runSceduler();
}

static void myTaskFn1(
	void
) {
	LWS_TASK_START();

    while (1) {
        printf("Hello1\n");

        LWS_WAIT(10);
    }

	LWS_TASK_END();
}

static void myTaskFn2(
	void
) {
	LWS_TASK_START();

	while (1) {
		printf("Hello2\n");

		LWS_WAIT(20);
	}

	LWS_TASK_END();
}

static void idleTask(
	void
) {
	LWS_TASK_START();

	while (1) {
		LWS_YIELD();
	}

	LWS_TASK_END();
}
