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

		static void Init();
		static void Shutdown();

		// This trace does not work because the following format:
		// Report() << ... << EndRep()
		// which is the format that I want, actually executes EndRep()
		// first, unlocking the mutex that hasnt been locked, and then
		// locking the mutex with the report call. This makes all other
		// Report() calls unable to lock the mutex if they are on a
		// seperate thread, making this completely invalid.
        static Trace& Report();

        template <typename T>
        Trace& operator<<(T const& val){
            std::cout << val;
            return *this;
        }

        static endlType EndRep();

    private:
        static Trace s_globalTrace;
};

#endif
