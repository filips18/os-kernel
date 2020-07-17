#include <iostream.h>
#include <dos.h>
#include "Kernel.h"
#include "PutNGet.h"
#include "KernelS.h"
#include "KernelEv.h"

volatile PCB* allThreads = 0; // pokazivac na listu svih niti
volatile KernelSem* allSemaphores = 0;
volatile PCB* signaled0 = 0;


volatile PCB *curr;
volatile PCB *tmp;
volatile PCB *last;

volatile PCB* help1ForTimer;
volatile PCB* help2ForTimer;
volatile KernelSem* saveMe;

volatile SignalWrapper* help1ForSW;
volatile SignalWrapper* help2ForSW;
volatile SignalHandlerWrapper* helpForSHW;
volatile unsigned int flgSig0;
volatile unsigned int contextChanged = 0;


volatile PCB** PomPointer = 0; // pom pokazivac za blokiranje niti na semaforu i pri pozivanju waitToComplete, ne moze da bude samo pokazivac na PCB jer bi tako (npr za waitToComplete):
							   // PomPointer bio jednak threadsForWTC, ali bi se u PomPointer samo kopirala vrednost iz threadsForWTC, pa posle kad bi menjali PomPointer menjali bi samo tu
							   // kopiranu vrednost, a ne i originalni threadsForWTC, sto nam je cilj. Dakle poenta je da imamo pokazivac na pokazivac kao globalni param, da bi mogli da
							   // menjamo i referenciramo vrednosti pokazivaca threadsForWTC i Threads(KernelS)

volatile unsigned int SysCall_ID = 4; // ID za manevrisanje kroz niz sis poziva, vrednost 4 znaci da nije nijedan sis poziv ( jer idu od 0 - 3)
volatile unsigned int LockFlag = 0; // zabrana preuzimanja bez zabrane prekida
volatile unsigned long RemainingTime = defaultTimeSlice; // bilo 20

extern void tick();

typedef void (*SysCall) ();

unsigned  SysTimerSEGMENT, SysTimerOFFSET;

volatile unsigned tmpSS; // za pristup SS,SP,BP u timeru
volatile unsigned tmpSP;
volatile unsigned tmpBP;

//sis pozivi koji se nalaze u nizu funkcija SysCallArr, one zapravo rade nesto u timeru (tipa stavljanje u scheduler, blokiranje...)

void Dispatch() {
	threadPut((PCB*)PCB::running);
}

void Block() {
	// koristim PomPointer da preko njega prenesem adresu glave liste blokiranih niti na semaforu, tj da ovde povezem PCB::running nit u listu blokiranih niti na semafor; koristi se i za waitToComplete
	curr = PCB::running;
	curr->next = *PomPointer;
	*PomPointer = curr;
}

void Shutdown() {
	curr = PCB::running;
	tmp = allThreads;
	last = 0;
	while(tmp != 0 && (((PCB*)tmp)->getID() != (((PCB*)curr)->getID()))) {
		last = tmp;
		tmp = tmp->allThreadsNext;
	}
	if (!last && tmp) {
		allThreads = allThreads->allThreadsNext;
		tmp->allThreadsNext = 0;
	}
	else if (tmp) {
		last->allThreadsNext = tmp->allThreadsNext;
		tmp->allThreadsNext = 0;
	}
	if (flgSig0) {
		PCB::running->next = signaled0;
		signaled0 = PCB::running;
		flgSig0 = 0;
	}
}

void Stop() {
	// treba mi jedan sis poziv koji ne radi nista, za wait na Eventu
}

//niz sis poziva
SysCall SysCallArr[4] = {Dispatch, Block, Shutdown, Stop};

//nas timer koji stavljamo u IVT
void interrupt timer() {
	if (RemainingTime > 0 && SysCall_ID == 4) RemainingTime--; // krpa, jer ako nema if, moze da se desi da se ovo smanji na 0, ali ne dodje do promene konteksta jer
															//je npr LockFlag na 1, pa bi se u sledecem pozivu ove prekidne rutine opet smanjio RemainingTime i bio bog zna sta,
															//pa ta nit nikad ne bi izgubila procesor usled timeslicea, iako bi trebalo
	if (SysCall_ID != 4 || ( LockFlag == 0 && RemainingTime == 0 && (((PCB*)PCB::running)->time > 0))) {
		#ifndef BCC_BLOCK_IGNORE
			asm {
					mov tmpSS, ss
					mov tmpSP, sp
					mov tmpBP, bp
			}
		#endif

		PCB::running->SP = tmpSP;
		PCB::running->SS = tmpSS;
		PCB::running->BP = tmpBP;

		if (SysCall_ID != 4) (*(SysCallArr[SysCall_ID]))();
		else (*(SysCallArr[0]))(); // ako je SysCall_ID na svojoj default vrednosti onda treba samo da se odradi Dispatch jer je iscurrlo vreme izvrsavanja dodeljeno lasthodnoj niti

		PCB::running = threadGet();
		tmpSP = PCB::running->SP;
		tmpSS = PCB::running->SS;
		tmpBP = PCB::running->BP;
		RemainingTime = PCB::running->time;

		contextChanged = 1;
		if (LockFlag) LockFlag = 0;

		#ifndef BCC_BLOCK_IGNORE
			asm {
					mov ss, tmpSS
					mov sp, tmpSP
					mov bp, tmpBP
			}
		#endif
	}
	if (SysCall_ID == 4) {
		saveMe = allSemaphores;
		while(allSemaphores) {
			help1ForTimer = allSemaphores->Threads;
			help2ForTimer = help1ForTimer;
			while(help1ForTimer) {
				if (--(help1ForTimer->timeLeftBlocked) == 0 && help1ForTimer->noTimeLimit == 0) {
					if (allSemaphores->Threads == help1ForTimer) {
						help1ForTimer = help1ForTimer->next;
						allSemaphores->Threads = help1ForTimer;
						help2ForTimer->expired = 1;
						help2ForTimer->next = 0;
						if (!(help2ForTimer->done)) threadPut((PCB*)help2ForTimer);
						help2ForTimer = help1ForTimer;
						continue;
					}
					else {
						help1ForTimer = help1ForTimer->next;
						help2ForTimer->next->next = 0;
						help2ForTimer->next->expired = 1;
						if (!(help2ForTimer->next->done)) threadPut((PCB*)(help2ForTimer->next));
						help2ForTimer->next = help1ForTimer;
						continue;
					}
				}
				else {
					help2ForTimer = help1ForTimer;
					help1ForTimer = help1ForTimer->next;
				}
			}
			allSemaphores = allSemaphores->next;
		}
		allSemaphores = saveMe;
		tick();
		#ifndef BCC_BLOCK_IGNORE
			asm int 60h;
		#endif
	}
	else SysCall_ID = 4;
	if (contextChanged) {
		contextChanged = 0;
		while(signaled0 != 0) {
			tmp = signaled0;
			signaled0 = signaled0->next;
			tmp->next = 0;
			((PCB*)tmp)->signalHandler0();
		}
		help1ForSW = PCB::running->signalsHead;
		help2ForSW = 0;
		while(help1ForSW) {
			if (PCB::running->signalBlocked[help1ForSW->id] || PCB::signalBlockedGlobal[help1ForSW->id]) {
				help2ForSW = help1ForSW;
				help1ForSW = help1ForSW->next;
				continue;
			}
			helpForSHW = PCB::running->signalHandlersHead;
			LockFlag = 1;
			asm pushf;
			asm sti;
			while(helpForSHW != 0) {
				if(help1ForSW->id == 0) {
					flgSig0 = 1;
					asm popf;
					LockFlag = 0;
					((PCB*)PCB::running)->exitThread();
				}
				if((helpForSHW->id) == (help1ForSW->id)) (*(helpForSHW->sh))();
				helpForSHW = helpForSHW->next;
			}
			asm popf;
			LockFlag = 0;
			if (help1ForSW == PCB::running->signalsHead) { // ako ne postoji nijedan signal handler onda ovde ja u principu samo izbacujem signal iz liste, ako treba da ostane onda mora da se menja ovo sa nekim flagom
				if (help1ForSW == PCB::running->signalsTail) PCB::running->signalsTail = 0;
				PCB::running->signalsHead = PCB::running->signalsHead->next;
				help2ForSW = help1ForSW;
				help1ForSW = PCB::running->signalsHead;
				help2ForSW->next = 0;
				delete help2ForSW;
				help2ForSW = 0;
			}
			else {
				if (help1ForSW == PCB::running->signalsTail) PCB::running->signalsTail = (SignalWrapper*)help2ForSW;
				help2ForSW->next = help1ForSW->next;
				help1ForSW->next = 0;
				delete help1ForSW;
				help1ForSW = help2ForSW->next;
			}
		}
	}
}

void timerOverride(){
#ifndef BCC_BLOCK_IGNORE
	asm{
		pushf
		cli
		push es
		push ax

		mov ax,0
		mov es,ax

		mov ax, word ptr es:0x0022 // 4 * N + 2, gde je N = 8
		mov word ptr SysTimerSEGMENT, ax
		mov ax, word ptr es:0x0020 // 4 * N, gde je N = 8
		mov word ptr SysTimerOFFSET, ax

		mov word ptr es:0x0022, seg timer // seg deo za ulaz broj 8 (sto je zapravo adresa 0x0022) zameni sa  seg naseg timera
		mov word ptr es:0x0020, offset timer // off deo za ulaz broj 8 (sto je zapravo adresa 0x0020) zameni sa off naseg timera

		mov ax, SysTimerSEGMENT
		mov word ptr es:0x0182, ax // prebaci seg deo sistemskog timera na slobodni ulaz u IV tabeli (seg deo za broj 60 je 0x0182)
		mov ax, SysTimerOFFSET
		mov word ptr es:0x0180, ax // prebaci off deo sis timera na slobodni ulaz u IV tabeli (off deo za broj 60 je 0x0180)

		pop ax
		pop es
		popf
	}
#endif
}

void timerRestore(){
#ifndef BCC_BLOCK_IGNORE
	asm {
		pushf
		cli
		push es
		push ax
		
		mov ax,0
		mov es,ax


		mov ax, word ptr SysTimerSEGMENT
		mov word ptr es:0x0022, ax // vraca seg deo sistemskog timera na prvobitno mesto
		mov ax, word ptr SysTimerOFFSET
		mov word ptr es:0x0020, ax // isto za off deo

		pop ax
		pop es
		popf
	}
#endif
}


//zabrana promene konteksta bez zabranjivanja prihvatanja prekida

void softLock() {
#ifndef BCC_BLOCK_IGNORE
	asm pushf;
	asm cli;
	LockFlag = 1;
	asm popf;
#endif
}

void softUnlock() {
#ifndef BCC_BLOCK_IGNORE
	asm pushf;
	asm cli;
	LockFlag = 0;
	asm popf;
#endif
}
void setID(int id) {
	SysCall_ID = id;
}
//funkcije koje se pozivaju iz delova koda van klase Timer, samo postavljaju SysCall_ID

void systemDispatch() {
#ifndef BCC_BLOCK_IGNORE
	asm pushf;
	asm cli;
	setID(0);
	asm int 8h;
	asm popf;
#endif
}

void systemBlock() {
	setID(1);
#ifndef BCC_BLOCK_IGNORE
	asm int 8h;
#endif
}

void systemShutdown() {
	setID(2);
#ifndef BCC_BLOCK_IGNORE
	asm int 8h;
#endif
}

void systemStop(){
#ifndef BCC_BLOCK_IGNORE
	asm pushf;
	asm cli;
	setID(3);
	asm int 8h;
	asm popf;
#endif
}
