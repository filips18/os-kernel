#include <iostream.h>
#include "Timer.h"
#include "Thread.h"
#include "PutNGet.h"
#include "Kernel.h"
#include "semaphor.h"
#include "Event.h"
#include "IVTEntry.h"
#include "user.h"

extern PCB MAIN;

int ret;

extern int userMain (int argc, char* argv[]);


int main(int argc, char* argv[]){
	PCB::running = &MAIN;
	timerOverride();
	ret = userMain(argc, argv);
	timerRestore();
	cout << "Function userMain returned " << ret;
	return ret;
}
