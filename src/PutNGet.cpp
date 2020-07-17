#include <iostream.h>
#include "Schedule.h"
#include "Kernel.h"
#include "Timer.h"
#include "PutNGet.h"

void idleRun() {
	while(PCB::AliveThreads) systemDispatch();
}

volatile PCB* eventThreads = 0;

PCB MAIN; // PCB za main nit
PCB IDLE(1, idleRun); // PCB za idle nit

//obe metode se pozivaju samo iz kriticnih sekcija pa ne mora da se radi lock/unlock
void threadPut(PCB *thread) {
	/*if (thread->eThread) { // "priority" put za event niti
		thread->next = eventThreads;
		eventThreads = thread;
	}
	else*/ if (thread != &IDLE) Scheduler::put(thread);
}

PCB* threadGet() {
	PCB* pom;
	/*if (eventThreads) { // "priority" get, za bolji odziv dogadjaja
		pom = (PCB*)eventThreads;
		eventThreads = eventThreads->next;
		pom->next = 0;
		pom->eThread = 0;
		return pom;
	}*/
	pom = Scheduler::get();
	if (pom) return pom;
	if (PCB::AliveThreads) return &IDLE;
	return 0;
}

