// Trace.cpp

#include "Trace.h"

void Trace::Init() {}
void Trace::Shutdown() {
}

Trace& Trace::Report() {
    return s_globalTrace;
}

Trace::endlType Trace::EndRep() {
    return endl;
}

Trace Trace::s_globalTrace;
