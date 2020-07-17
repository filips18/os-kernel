#include <iostream.h>
#include "Kernel.h"
#include "Timer.h"

Thread::Thread (StackSize defaultStackSize, Time defaultTimeSlice) :
myPCB(new PCB(this, defaultTimeSlice, defaultStackSize)) {}

void Thread::start(){
	myPCB->start();
}

void Thread::waitToComplete(){
	myPCB->waitToComplete();
}

Thread:: ~Thread(){
	myPCB->waitToComplete();
	delete myPCB;
	myPCB = 0;
}

ID Thread::getID(){
	return myPCB->getID();
}

ID Thread::getRunningId(){
	return PCB::getRunningID();
}

Thread* Thread::getThreadById(ID id){
	return PCB::getThreadByID(id);
}

void dispatch(){
	systemDispatch();
}

//DRUGI ZAD
void Thread::signal(SignalId signal) {
	myPCB->signal(signal);
}
void Thread::registerHandler(SignalId signal, SignalHandler handler) {
	myPCB->registerHandler(signal, handler);
}

void Thread::unregisterAllHandlers(SignalId id) {
	myPCB->unregisterAllHandlers(id);
}
void Thread::swap(SignalId id, SignalHandler hand1, SignalHandler hand2) {
	myPCB->swap(id, hand1, hand2);

}
void Thread::blockSignal(SignalId signal) {
	myPCB->blockSignal(signal);
}
void Thread::blockSignalGlobal(SignalId signal) {
	PCB::blockSignalGlobal(signal);
}
void Thread::unblockSignal(SignalId signal) {
	myPCB->unblockSignal(signal);
}
void Thread::unblockSignalGlobal(SignalId signal) {
	PCB::unblockSignalGlobal(signal);
}


