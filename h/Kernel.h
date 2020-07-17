#ifndef kernel_h
#define kernel_h

#include "Thread.h"
#include "SHWrap.h"
#include "SigWrap.h"


class PCB {
public:
	volatile unsigned int time; // time-sharing

	unsigned* stack;// "stek" niti
	
	volatile unsigned SP; // cuvanje
	volatile unsigned SS; // konteksta
	volatile unsigned BP; // niti
	
	volatile PCB* next; // za ulancavanje kod npr blokiranih niti na semaforu
	volatile PCB* allThreadsNext; // za ulancavanje u listi svih niti ( pokazivac allThreads)
	
	volatile static int AliveThreads; // broj "zivih" niti
	volatile static ID currID; // "globalni ID" preko kojeg niti dobijaju svoj ID kad se kreiraju(krece od 0)
	volatile ID PCB_ID; // ID niti
	
	volatile int done; // flag, koristi se u exitThread
	volatile int expired; // flag, koristi se za semafore tj kad se nit blokira na odredjeno vreme
	volatile int noTimeLimit; // flag, ako se nit blokira na semaforu sa wait(0), da znam da je ne skidam zbog isteka vremena
	
	volatile Time timeLeftBlocked; // na koliko je vremena blokirana nit
	
	Thread* myThread; // pokazivac na objekat klase Thread povezanog sa ovim objektom klase PCB
	volatile PCB* threadsForWTC; // pokazivac na listu niti koje su blokirane pri pozivanju waitToComplete
	
	volatile static PCB* running; // staticki pokazivac na objekat klase PCB ciji se Thread trenutno izvrsava
	
	// 2. zad
	PCB* parentPCB;

	SignalWrapper* signalsHead;

	SignalWrapper* signalsTail;

	SignalHandlerWrapper* signalHandlersHead;

	SignalHandlerWrapper* signalHandlersTail;

	volatile unsigned int signalBlocked[16]; // 0 - NOT BLOCKED, 1 - BLOCKED

	volatile static unsigned int signalBlockedGlobal[16];

	// konstruktori
	PCB(); // za main nit
	PCB(Thread* t, Time tm, StackSize stackSize);// inace za niti
	PCB(Time t, void (*fun)());// za IDLE nit
	
	// destr
	~ PCB();

	void getReady( void (*fun)(), StackSize stackSize); // priprema steka
	void waitToComplete(); // poziva se ako neko zeli da obrise nit koja nije zavrsila sa radom
	void start(); // ubaci u Scheduler
	void setTimeSlice(unsigned int t); // podesi this->time na t
	static void wrapper(); // omotac, ovu funkciju zapravo prosledjujemo getReady da nju stavi na stek, a ova unutra poziva run Threada i posle se ubija
	
	static Thread* getThreadByID(ID id); // dohvati Thread koji ima odg id
	static ID getRunningID(); // dohvati PCB_ID tekuce niti
	ID getID(); // dohvati this->PCB_ID
	
	void exitThread(); // priprema za brisanje niti
	void addToListOfThreads(); // dodaj u listu niti

	//2. zad

	static void blockSignalGlobal(SignalId signal);

	void blockSignal(SignalId signal);

	static void unblockSignalGlobal(SignalId signal);

	void unblockSignal(SignalId signal);

	void unregisterAllHandlers(SignalId id);

	void registerHandler(SignalId signal, SignalHandler handler);

	void swap(SignalId id, SignalHandler hand1, SignalHandler hand2);

	void signal(SignalId signal);

	void inheritSignals();

	void signalHandler0();

};






#endif
