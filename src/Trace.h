#ifndef DW_TRACE_H
#define DW_TRACE_H
// Trace.h

#include <iostream>

#include <thread>
#include <future>
#include <mutex>

class Trace {
    public:
        static constexpr char endl = '\n';  
        using endlType = decltype(endl);

        static void Init() {}
        static void Shutdown() {}

        static Trace& Report();

        template <typename T>
        Trace& operator<<(T const& val){
            std::cout << val;
            return *this;
        }

        static endlType EndRep();

    private:
        static std::thread::id s_currentLockThread;
        static Trace s_globalTrace;
        static std::mutex s_mutex;
};

#endif
