#ifndef SHWRAP_H_
#define SHWRAP_H_
#include "Thread.h"

class SignalHandlerWrapper {
public:
	SignalHandler sh;
	SignalId id;
	SignalHandlerWrapper* next;

	SignalHandlerWrapper(SignalHandler _sh, SignalId _id);
	~SignalHandlerWrapper();

};



#endif /* SHWRAP_H_ */
