#include "KernelS.h"
#include "PutNGet.h"
#include "Timer.h"
#include <iostream.h>

extern volatile PCB** PomPointer;
extern volatile KernelSem* allSemaphores;


KernelSem::KernelSem(int init): Threads(0), value(init) {
	asm pushf;  //ne moze ovde softlockovanje jer se menja allSemaphores koji se koristi u timeru kad se implicitno desi prekid (tick), pa just in case ostavi ovako
	asm cli;
	next = allSemaphores;
	allSemaphores = this;
	asm popf;
}

KernelSem::~KernelSem() {
	volatile KernelSem* currS;
	volatile KernelSem* lastS;
	volatile PCB* pom;
	asm pushf; // isto menja listu allSemaphores
	asm cli;
	currS = allSemaphores; // izbaci ga iz lste
	lastS = 0;
	while(currS != this) {
		lastS = currS;
		currS = currS->next;
	}
	if (!lastS && currS) {
		allSemaphores = allSemaphores->next;
		currS->next = 0;
	}
	else if (currS) {
		lastS->next = currS->next;
		currS->next = 0;
	}
	pom = Threads;
	while(Threads != 0) {
		Threads = Threads->next;
		pom->next = 0;
		if (pom->done == 0) threadPut((PCB*)pom); // ako nije zavrsila nit, a i dalje je blokirana, a neko je pametno krenuo da brise semaphor, onda vrati tu nit u scheduler
		pom = Threads;
	}
	asm popf;
}

int KernelSem::val() {
	return value;
}


int KernelSem::wait(Time maxTimeToWait) {
	int retVal;
	asm pushf; // opet ne moze softLock jer ako prosledimo trenutni threads u pompointer, i sad bas pre nego sto pozovemo block, desi se tick, i timer udje u ono dole sto prolazi kroz sve niti
	asm cli;  // svih semafora i gleda da li im je isteklo vreme blokiranja, pa recimo da je odmah prvoj niti na ovom semaforu isteklo, nas Threads ovog semafora ce se onda promeniti
	// na next ove niti sto se odblokira jer joj je isteklo vreme, a Threads koji je prosledjen u PomPointer je onaj prosli threads, tu je vec gg
	if (--value < 0) {
		PCB::running->timeLeftBlocked = maxTimeToWait;
		if(maxTimeToWait == 0) PCB::running->noTimeLimit = 1;
		PomPointer = &Threads;
		systemBlock();
		PCB::running->noTimeLimit = 0;
		retVal = ((PCB::running->expired) ? 0:1);
		if (!retVal) {
			value++;
			PCB::running->expired = 0;
		}
		asm popf;
		return retVal;
	}
	else {
		asm popf;
		return 1;
	}
}

int KernelSem::signal(int n) {
	volatile PCB* pom;
	int cnt;
	asm pushf; // ovde nema sile da igde moze softlock jer se cacka Threads i ovde i u timeru
	asm cli;
	if (n < 0) {
		asm popf;
		return n;
	}
	else if (n == 0) {
		if (value++ < 0) {
			pom = Threads;
			Threads = Threads->next;
			pom->next = 0;
			if(pom->done == 0) threadPut((PCB*)pom);
		}
		asm popf;
		return 0;
	}
	else {
		cnt = 0;
		pom = Threads;
		while(Threads && (cnt < n)) {
			Threads = Threads->next;
			cnt++;
			pom->next = 0;
			if(pom->done == 0) threadPut((PCB*)pom);
			pom = Threads;
		}
		value += n;
		asm popf;
		return cnt;
	}
}
