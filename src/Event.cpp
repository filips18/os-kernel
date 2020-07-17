#include "KernelEv.h"
#include "Kernel.h"

Event::Event(IVTNo ivtNo) {
	myImpl = new KernelEv((PCB*)PCB::running,ivtNo);
}

Event::~Event() {
	delete myImpl;
	myImpl = 0;
}

void Event::wait() {
	myImpl->wait();
}

void Event::signal() {
	myImpl->signal();
}
