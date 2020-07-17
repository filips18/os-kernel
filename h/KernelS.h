#ifndef kernel_sem_h
#define kernel_sem_h
#include "Kernel.h"

class KernelSem {
private:
	volatile int value;
public:
	volatile PCB* Threads;
	volatile KernelSem* next;

	KernelSem(int init);

	int wait(Time maxTimeToWait);
	int signal(int n);
	int val();

	~KernelSem();
};


#endif
