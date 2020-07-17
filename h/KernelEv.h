#ifndef KERNELEV_H_
#define KERNELEV_H_
#include "Event.h"
#include "Kernel.h"

class KernelEv {
private:
	PCB* myThread;
	IVTNo id;
	int value;
public:

	KernelEv(PCB* owner, IVTNo ivtNo);
	~KernelEv();

	void wait();
	void signal();
};



#endif /* KERNELEV_H_ */
