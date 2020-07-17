#ifndef SIGWRAP_H_
#define SIGWRAP_H_
#include "thread.h"

class SignalWrapper {
public:
	SignalId id;
	SignalWrapper* next;

	SignalWrapper(SignalId _id);

	~SignalWrapper();
};



#endif /* SIGWRAP_H_ */
