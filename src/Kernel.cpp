#include <dos.h>
#include <iostream.h>
#include "Kernel.h"
#include "Timer.h"
#include "PutNGet.h"
#include "Thread.h"

#define MAX_STACK_SIZE 65535

//pomocni
volatile PCB* help;
volatile SignalHandlerWrapper* helpSHW;
volatile SignalHandlerWrapper* tmpSHW;
volatile SignalWrapper* helpSW;
volatile SignalHandler tmpSH;

volatile int flg1 = 0; // za swap
volatile int flg2 = 0;


extern volatile PCB* allThreads;

extern PCB MAIN;

extern volatile PCB** PomPointer;

//staticka polja klase PCB
volatile int PCB::AliveThreads = 0;
volatile PCB* PCB::running;
volatile ID PCB::currID = 1;
volatile unsigned int PCB::signalBlockedGlobal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};




PCB::PCB() {  // za main nit, nema potrebe za asm pushf/popf u konstruktorima, sem u delu za inicijalizaciju steka
	stack = 0;
	time = 0;
	timeLeftBlocked = 0;

	next = 0;
	allThreadsNext = 0;

	PCB_ID = 1;

	done = 0;
	expired = 0;
	noTimeLimit = 0;

	threadsForWTC = 0;
	myThread = new Thread(0,0);
	myThread->myPCB = this;
	signalHandlersHead = 0;
	signalHandlersTail = 0;
	signalsHead = 0;
	signalsTail = 0;

	parentPCB = 0;

	for (int i = 0; i < 16; i++) signalBlocked[i] = 0;
}

PCB::PCB(Thread* t, Time tm, StackSize stackSize) { // inace za niti
	if (stackSize > 0) {
		time = tm;
		timeLeftBlocked = 0;

		next = 0;
		allThreadsNext = 0;

		PCB_ID = ++currID;

		done = 0;
		expired = 0;
		noTimeLimit = 0;

		threadsForWTC = 0;
		myThread = t;
		signalHandlersHead = 0;
		signalHandlersTail = 0;
		signalsHead = 0;
		signalsTail = 0;

		parentPCB = (PCB*)PCB::running;

		asm pushf;
		asm cli;

		for (int i = 0; i < 16; i++) {
			if (parentPCB) signalBlocked[i] = parentPCB->signalBlocked[i];
			else signalBlocked[i] = 0;
		}
		if (parentPCB) inheritSignals();
		getReady(PCB::wrapper, stackSize);
		addToListOfThreads();

		asm popf;
	}
}

PCB::PCB(Time t, void (*fun)()) {  // za IDLE nit

	time = t;
	timeLeftBlocked = 0;

	next = 0;
	allThreadsNext = 0;

	PCB_ID = 0;

	done = 1; // isto ko i za main, samo sto njen stack nece biti 0 (poziva se getReady), pa zapravo treba nekako da se obezbedi da ona ne udje u eThread, a moze to ovako jer se ona ne stavlja u Scheduler pa ni ne moze da dodje do greske prilikom 			if(help->done == 0) threadPut((PCB*)help); u eThread
	expired = 0;
	noTimeLimit = 0;

	threadsForWTC = 0;
	myThread = 0;
	signalHandlersHead = 0;
	signalHandlersTail = 0;
	signalsHead = 0;
	signalsTail = 0;

	parentPCB = 0;

	for (int i = 0; i < 16; i++) {
		signalBlocked[i] = 0;
	}

	asm pushf;
	asm cli;

	getReady(fun, 1024);

	asm popf;
}

void PCB::waitToComplete() {
	softLock(); // trebalo bi da moze softlock jer se nigde u onom dole ifu u timeru pristupa PomPointeru, niti se on menja bilo gde u timeru ako je SysCall_ID = 4(sto bi i bio da se desi
	if (done == 0) { // tick tip prekida i tad pozove nas timer
		PomPointer = &threadsForWTC;
		systemBlock();
	}
	else softUnlock();
}


void PCB::getReady(void (*fun)(), StackSize stackSize) {
	
	if(stackSize > MAX_STACK_SIZE) stackSize = MAX_STACK_SIZE;
	stackSize /= sizeof(unsigned); // stackSize se zadaje kao broj bajtova (long je jer bi unsigned int najvise mogao da prikaze 65535 (max stack size), a fora je da kao
								   //nekad moze da dodje i veci broj da bi ovaj if od malopre imao sta da radi. Elem, posto je ovaj stackSize broj bajtova, a jedan unsigned
								   // (unsigned == unsigned int) zauzima 2B, samo ga podelis sa sizeof(unsigned), sto je 2
	unsigned* st = new unsigned[stackSize];
	stack = st;
	st[stackSize - 1] = 0x200; //PSW
	
	#ifndef BCC_BLOCK_IGNORE
	st[stackSize - 2] = FP_SEG(fun);
	st[stackSize - 3] = FP_OFF(fun);
	SP = FP_OFF(st + stackSize - 12); // DS,ES,SI,DI,AX,BX,CX,DX,BP
	SS = FP_SEG(st + stackSize - 12); //
	BP = SP; //
	#endif

}

void PCB::start() {
	softLock();
	PCB::AliveThreads++;
	done = 0;
	threadPut(this);
	softUnlock();
}

void PCB::setTimeSlice(unsigned int t) {
	softLock(); // moze soft, jeste da se menja timeSlice al svakako onaj RemainingTime ostaje na bivsem timesliceu sve dok se ne promeni kontekst, sto svakako nece moci sa ovim lockom, pa nema problema
	time = t;
	softUnlock();
}

Thread* PCB::getThreadByID(ID id) {
	softLock(); // allThreads se nigde ne cacka u onom dole ifu u timeru pa moze valjda softLock
	help = allThreads;
	if (id == 1) {
		softUnlock();
		return (&MAIN)->myThread;
	}
	else {
		while(help != 0) {
			if (((PCB*)help)->getID() == id ) {
				softUnlock();
				return help->myThread;
			}
			help = help->allThreadsNext;
		}
	}
	softUnlock();
	return 0;
}


ID PCB::getRunningID() {
	return ((PCB*)PCB::running)->getID();
}

ID PCB::getID() {
	return PCB_ID;
}

void PCB::exitThread() {
	softLock();
	if (done == 0 && this != &MAIN) { // da ne bi MAIN nit ulazila u eThread, tu moze da udje samo ako je zavrsen main i compiler je poceo da je brise(tj iz destruktora pozvan eThread),
									//a ako ona udje ovde u eThread ona ce pozvati sis pozvi Shutdown koji ulazi u Timer i pokusava da uzme nit iz Schedulera,
									//u kom nema niti, pa ce on vratiti 0, pa ce se vratiti ili ta nula ili adresa IDLE niti, koja nzm ni da li postoji tu vise, mozda pukne 0ptr negde
									//i nasilno ce se zavrsiti program jer se poziva terminate onda, pa se nikad nece osloboditi stack MAIN niti( ma ni ne mora, stack main niti svakako ima vrednost 0)
		while(threadsForWTC) {
			help = threadsForWTC;
			threadsForWTC = threadsForWTC->next;
			help->next = 0;
			if(!(help->done)) threadPut((PCB*)help);
		}
		done = 1;
		parentPCB->signal(1);
		signal(2);
		systemShutdown();
	}
	done = 1;
	if (this == &MAIN) signal(2);
	softUnlock();
}

void PCB::addToListOfThreads() {
	allThreadsNext = allThreads;
	allThreads = this;
}

void PCB::wrapper() {
	PCB::running->myThread->run();
	softLock();
	PCB::AliveThreads--;
	softUnlock();
	((PCB*)PCB::running)->exitThread();
}

PCB::~PCB() {
	exitThread();

	asm pushf;
	asm cli;

	while(signalHandlersHead) {
		tmpSHW = signalHandlersHead;
		signalHandlersHead = signalHandlersHead->next;
		tmpSHW->next = 0;
		delete tmpSHW;
	}
	signalHandlersTail = 0;
	while(signalsHead) {
		helpSW = signalsHead;
		signalsHead = signalsHead->next;
		helpSW->next = 0;
		delete helpSW;
	}
	signalsTail = 0;

	delete[] stack;
	stack = 0;

	asm popf;
}

void PCB::blockSignal(SignalId signal) {
	signalBlocked[signal] = 1;
}

void PCB::unblockSignal(SignalId signal) {
	signalBlocked[signal] = 0;
}

void PCB::blockSignalGlobal(SignalId signal) {
	PCB::signalBlockedGlobal[signal] = 1;
}

void PCB::unblockSignalGlobal(SignalId signal) {
	PCB::signalBlockedGlobal[signal] = 0;
}

void PCB::unregisterAllHandlers(SignalId id) {
	asm pushf;
	asm cli;
	if (id > 15) {
		asm popf;
		return;
	}
	helpSHW = signalHandlersHead;
	tmpSHW = 0;
	while(helpSHW) {
		if (helpSHW->id == id) {
			if (helpSHW == signalHandlersHead) {
				if (helpSHW == signalHandlersTail) signalHandlersTail = 0;
				signalHandlersHead = signalHandlersHead->next;
				tmpSHW = helpSHW;
				helpSHW = helpSHW->next;
				tmpSHW->next = 0;
				delete tmpSHW;
			}
			else {
				if (helpSHW == signalHandlersTail) signalHandlersTail = (SignalHandlerWrapper*)tmpSHW;
					helpSHW = helpSHW->next;
					tmpSHW->next->next = 0;
					delete tmpSHW->next;
					tmpSHW->next = (SignalHandlerWrapper*)helpSHW;
			}
		}
		else {
			tmpSHW = helpSHW;
			helpSHW = helpSHW->next;
		}
	}

	asm popf;
}

void PCB::swap(SignalId id, SignalHandler hand1, SignalHandler hand2) {
	asm pushf;
	asm cli;
	if (id > 15) {
		asm popf;
		return;
	}
	helpSHW = signalHandlersHead;
	tmpSHW = helpSHW;
	flg1 = 0;
	flg2 = 0;

	while(!flg1 || !flg2) {
		if (!tmpSHW || !helpSHW) {
			asm popf;
			return;
		}
		if (!flg1 && helpSHW->sh == hand1 && helpSHW->id == id) flg1 = 1;
		if (!flg2 && tmpSHW->sh == hand2 && tmpSHW->id == id) flg2 = 1;
		if (!flg1) helpSHW = helpSHW->next;
		if (!flg2) tmpSHW = tmpSHW->next;
	}
	if (flg1 && flg2) {
		tmpSH = helpSHW->sh;
		helpSHW->sh = tmpSHW->sh;
		tmpSHW->sh = (SignalHandler)tmpSH;
	}
	asm popf;
	return;
}

void PCB::registerHandler(SignalId signal, SignalHandler handler) {
	asm pushf;
	asm cli;
	if (signal > 15) {
		asm popf;
		return;
	}
	if (signalHandlersHead == 0) {
		signalHandlersHead = new SignalHandlerWrapper(handler, signal);;
		signalHandlersTail = signalHandlersHead;
	}
	else {
		signalHandlersTail->next = new SignalHandlerWrapper(handler, signal);
		signalHandlersTail = signalHandlersTail->next;
	}

	asm popf;
}

void PCB::signal(SignalId signal) {
	asm pushf;
	asm cli;
	if (signal > 15) {
		asm popf;
		return;
	}
	helpSHW = signalHandlersHead;
	while(helpSHW && (helpSHW->id != signal)) helpSHW = helpSHW->next;
	if (helpSHW || (signal == 0)) {
		if(signalsHead == 0 ) {
			signalsHead = new SignalWrapper(signal);
			signalsTail = signalsHead;
		}
		else {
			signalsTail->next = new SignalWrapper(signal);
			signalsTail = signalsTail->next;
		}
	}
	asm popf;
}

void PCB::inheritSignals() {
	helpSHW = parentPCB->signalHandlersHead;
	while(helpSHW) {
		if (signalHandlersHead == 0) {
			signalHandlersHead = new SignalHandlerWrapper(helpSHW->sh , helpSHW->id);
			signalHandlersTail = signalHandlersHead;
		}
		else {
			signalHandlersTail->next = new SignalHandlerWrapper(helpSHW->sh , helpSHW->id);
			signalHandlersTail = signalHandlersTail->next;
		}
		helpSHW = helpSHW->next;
	}
	helpSW = parentPCB->signalsHead;
	while(helpSW) {
		if (signalsHead == 0) {
			signalsHead = new SignalWrapper(helpSW->id);;
			signalsTail = signalsHead;
		}
		else {
			signalsTail->next = new SignalWrapper(helpSW->id);;
			signalsTail = signalsTail->next;
		}
		helpSW = helpSW->next;
	}
}

void PCB::signalHandler0() {
	while(signalHandlersHead) {
		tmpSHW = signalHandlersHead;
		signalHandlersHead = signalHandlersHead->next;
		tmpSHW->next = 0;
		delete tmpSHW;
	}
	signalHandlersTail = 0;
	while(signalsHead) {
		helpSW = signalsHead;
		signalsHead = signalsHead->next;
		helpSW->next = 0;
		delete helpSW;
	}
	signalsTail = 0;

	delete[] stack;
	stack = 0;
}
