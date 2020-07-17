#ifndef IVTENTRY_H_
#define IVTENTRY_H_

typedef void interrupt (*pInterrupt)(...);

typedef unsigned char IVTNo;

class KernelEv;

class IVTEntry {
private:
	IVTNo id;
	KernelEv* myEvent;
public:
	IVTEntry(IVTNo ivtNo, pInterrupt _newInt);
	~IVTEntry();

	static void Set(IVTNo ivtNo, pInterrupt newInt);
	static void Restore(IVTNo ivtNo);

	static IVTEntry* arrOfIVT[256];
	static pInterrupt oldInt[256];

	void signal();
	void callOld();

	friend class KernelEv;
};


#define PREPAREENTRY(numEntry,cO)\
void interrupt inter##numEntry(...);\
IVTEntry newEntry##numEntry(numEntry,inter##numEntry);\
void interrupt inter##numEntry(...){\
	newEntry##numEntry.signal();\
	if(cO == 1) newEntry##numEntry.callOld();\
}

#endif /* IVTENTRY_H_ */
