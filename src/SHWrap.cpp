#include "SHWrap.h"

SignalHandlerWrapper::SignalHandlerWrapper(SignalHandler _sh, SignalId _id): sh(_sh), id(_id), next(0) {
}

SignalHandlerWrapper::~SignalHandlerWrapper() {
	next = 0;
}
