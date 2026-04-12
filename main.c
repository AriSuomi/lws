
#include <stdio.h>

#include "lws.h"

static void myTaskFn1(void);
static void myTaskFn2(void);

static lws_Tcb myTask1;
static lws_Tcb myTask2;

static lws__Sema mySema;

int main() {
    lws_init();

	lws_initTask(&myTask1, myTaskFn1, LWS_PRIO_NORMAL);
	lws_initTask(&myTask2, myTaskFn2, LWS_PRIO_NORMAL);

	lws_semaInit(&mySema);

    lws_runScheduler();
}

static void myTaskFn1(
	void
) {
	LWS_TASK_START();

    while (1) {
        printf("Hello1\n");

		LWS_SEMA_SIGNAL(&mySema);
        LWS_WAIT(100);
    }

	LWS_TASK_END();
}

static void myTaskFn2(
	void
) {
	LWS_TASK_START();

	while (1) {
		LWS_SEMA_WAIT(&mySema);
		printf("Hello2\n");
	}

	LWS_TASK_END();
}

