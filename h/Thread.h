#ifndef _thread_h_
#define _thread_h_
#define lock asm cli
#define unlock asm sti

typedef unsigned long StackSize;
const StackSize defaultStackSize = 4096;
typedef unsigned int Time; // time, x 55ms
const Time defaultTimeSlice = 2; // default = 2*55ms
typedef int ID;

typedef void (*SignalHandler)();
typedef unsigned SignalId;

class PCB; // Kernel's implementation of a user's thread

class Thread {
public:

	void start();
	void waitToComplete();
	virtual ~Thread();

	ID getID();
	static ID getRunningId();
	static Thread* getThreadById(ID id);

	//2. ZAD

	void signal(SignalId signal);

	void registerHandler(SignalId signal, SignalHandler handler); 
	void unregisterAllHandlers(SignalId id);
	void swap(SignalId id, SignalHandler hand1, SignalHandler hand2);

	void blockSignal(SignalId signal);
	static void blockSignalGlobal(SignalId signal);
	void unblockSignal(SignalId signal); 
	static void unblockSignalGlobal(SignalId signal);

protected:
	friend class PCB;
	Thread (StackSize stackSize = defaultStackSize, Time timeSlice =
			defaultTimeSlice);
	virtual void run() {}
private:
	PCB* myPCB;
};

void dispatch ();
#endif
