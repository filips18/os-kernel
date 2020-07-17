#include "SigWrap.h"

SignalWrapper::SignalWrapper(SignalId _id): id(_id), next(0) {
}

SignalWrapper::~SignalWrapper() {
	next = 0;
}
