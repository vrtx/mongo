#include <sched.h>
#include <stdio.h>

#include "mongo/util/counters/core_counter.h"

namespace mongo {

    uint32_t CoreCounter::getCurrentCore() const {
#if defined(_WIN32)
        // todo:
        return ::GetCurrentProcessorNumberEx();

#elif defined(__linux__) || defined(__sunos__)
        return ::sched_getcpu();

#elif defined(__APPLE__)
        uint32_t core = 0;
        // read the processor id from ecx
        asm volatile ("rdtscp"
                      : "=c"(core)
                      :
                      : "%rax", "%rcx", "%rdx");
        // todo: always returns core 0...  wtf?
        printf("current core: %u\n", core);
        return core;
#endif

    }

}
