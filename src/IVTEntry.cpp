#include <dos.h>
#include "IVTEntry.h"
#include "Event.h"
#include "KernelEv.h"

IVTEntry* IVTEntry::arrOfIVT[256];
pInterrupt IVTEntry::oldInt[256];

IVTEntry::IVTEntry(IVTNo ivtNo, pInterrupt _newInt): id(ivtNo), myEvent(0) {
	asm pushf;
	asm cli;
	IVTEntry::arrOfIVT[ivtNo] = this;
	IVTEntry::Set(ivtNo, _newInt);
	asm popf;
}

IVTEntry::~IVTEntry() {
	asm pushf;
	asm cli;
	IVTEntry::Restore(id);
	IVTEntry::arrOfIVT[id] = 0;
	asm popf;
}

void IVTEntry::Set(IVTNo ivtNo, pInterrupt newInt){
#ifndef BCC_BLOCK_IGNORE
	IVTEntry::oldInt[ivtNo] = getvect(ivtNo);
	setvect(ivtNo, newInt);
#endif
}

void IVTEntry::Restore(IVTNo ivtNo) {
#ifndef BCC_BLOCK_IGNORE
	setvect(ivtNo, IVTEntry::oldInt[ivtNo]);
	IVTEntry::oldInt[ivtNo] = 0;
#endif
}

void IVTEntry::signal() {
	myEvent->signal();
}

void IVTEntry::callOld() {
	(IVTEntry::oldInt[id])();
}
