#include "KernelEv.h"
#include "IVTEntry.h"
#include "Timer.h"
#include "PutNGet.h"
#include <iostream.h>

KernelEv::KernelEv(PCB* owner,IVTNo ivtNo): id(ivtNo), myThread(owner), value(1) {
	asm pushf;
	asm cli;
	IVTEntry::arrOfIVT[id]->myEvent = this;
	asm popf;
}

KernelEv::~KernelEv() {
	asm pushf;
	asm cli;
	delete IVTEntry::arrOfIVT[id];
	IVTEntry::arrOfIVT[id] = 0;
	asm popf;
}

void KernelEv::wait() {
	asm pushf;
	asm cli;
	if (value && (myThread == PCB::running)) {
		value = 0;
		systemStop();
	}
	asm popf;
}

void KernelEv::signal() {
	if (value == 0) {
		threadPut(myThread);
		value = 1;
		systemDispatch();
	}
}

