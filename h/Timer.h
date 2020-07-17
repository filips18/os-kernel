#ifndef timer_h
#define timer_h

void systemDispatch(); // sis pozivi koji postavljaju sam ID za sis pozive: 0 - DISPATCH, 1 - BLOCK, 2 - SHUTDOWN, 3 - STOP
void systemBlock();
void systemShutdown();
void systemStop();

void softLock(); // lock i unlock za zabranu preuzimanja konteksta bez zabrane prekida, sa laba
void softUnlock();

void setID(int id);

void interrupt timer(); // prekidna rutina

void timerRestore(); // sa laba, vrati originalnu timer prekidnu rutinu na ulaz na kom je bila
void timerOverride(); // sa laba, postavi nasu timer prekidnu rutinu na ulaz na kom je bila sistemska timer rutina

#endif
