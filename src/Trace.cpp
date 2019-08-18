// Trace.cpp

#include "Trace.h"

Trace& Trace::Report() {
    if(s_currentLockThread != std::this_thread::get_id()) {
        s_mutex.lock();
        s_currentLockThread = std::this_thread::get_id();
    }
    return s_globalTrace;
}

Trace::endlType Trace::EndRep() {
    if(s_currentLockThread == std::this_thread::get_id()) {
        s_currentLockThread = std::thread::id();
        s_mutex.unlock();
    }
    return endl;
}

Trace Trace::s_globalTrace;
std::mutex Trace::s_mutex;
std::thread::id Trace::s_currentLockThread;
