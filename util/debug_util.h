// debug_util.h

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#ifndef _WIN32
#include <signal.h>
#endif

namespace mongo {

    // get the value of the time-stamp counter
    static inline uint64_t rdtsc()
    {
        uint32_t upper, lower;
        asm volatile("rdtsc\n" : "=a"(lower), "=d"(upper));
        return (uint64_t)upper << 32 | lower;
    }

    // estiamte the number of cycles for a block of code
    // NOTE: this function will produce invalid results if the process
    //       is migrated to another CPU during execution.  it also includes
    //       the number of cycles taken to allocate 2 uint32_t's on the stack.
    #define EST_CYCLE_COUNT(name, ...)          \
        {                                       \
            uint64_t l_count = 0;               \
            uint64_t l_start_time = rdtsc();    \
            /* run the code to count */         \
            __VA_ARGS__;                        \
            l_count = (rdtsc() - l_start_time); \
            log(1) << "PERFORMANCE:  Cycles take to execute " << name << ": " << l_count << endl; \
        }

// for debugging
    typedef struct _Ints {
        int i[100];
    } *Ints;
    typedef struct _Chars {
        char c[200];
    } *Chars;

    typedef char CHARS[400];

    typedef struct _OWS {
        int size;
        char type;
        char string[400];
    } *OWS;

#if defined(_DEBUG)
    enum {DEBUG_BUILD = 1};
#else
    enum {DEBUG_BUILD = 0};
#endif

#define MONGO_DEV if( DEBUG_BUILD )
#define DEV MONGO_DEV

#define MONGO_DEBUGGING if( 0 )
#define DEBUGGING MONGO_DEBUGGING

// The following declare one unique counter per enclosing function.
// NOTE The implementation double-increments on a match, but we don't really care.
#define MONGO_SOMETIMES( occasion, howOften ) for( static unsigned occasion = 0; ++occasion % howOften == 0; )
#define SOMETIMES MONGO_SOMETIMES

#define MONGO_OCCASIONALLY SOMETIMES( occasionally, 16 )
#define OCCASIONALLY MONGO_OCCASIONALLY

#define MONGO_RARELY SOMETIMES( rarely, 128 )
#define RARELY MONGO_RARELY

#define MONGO_ONCE for( static bool undone = true; undone; undone = false )
#define ONCE MONGO_ONCE

#if defined(_WIN32)
    inline int strcasecmp(const char* s1, const char* s2) {return _stricmp(s1, s2);}
#endif

    // Sets SIGTRAP handler to launch GDB
    // Noop unless on *NIX and compiled with _DEBUG
    void setupSIGTRAPforGDB();

    extern int tlogLevel;

    inline void breakpoint() {
        if ( tlogLevel < 0 )
            return;
#ifdef _WIN32
        //DEV DebugBreak();
#endif
#ifndef _WIN32
        // code to raise a breakpoint in GDB
        ONCE {
            //prevent SIGTRAP from crashing the program if default action is specified and we are not in gdb
            struct sigaction current;
            sigaction(SIGTRAP, NULL, &current);
            if (current.sa_handler == SIG_DFL) {
                signal(SIGTRAP, SIG_IGN);
            }
        }

        raise(SIGTRAP);
#endif
    }


    // conditional breakpoint
    inline void breakif(bool test) {
        if (test)
            breakpoint();
    }

} // namespace mongo
