#include <sched.h>

#include "mongo/util/counters/core_counter.h"

namespace mongo {

    uint16_t CoreCounter::getCurrentCore() const {
#if defined(_WIN32)
        // todo:
        return ::GetCurrentProcessorNumberEx();
#elif defined(__linux__) || defined(__sunos__)
        return ::sched_getcpu();
#elif defined(__APPLE__)
        // todo:
        return 0;
#endif

    }

}
