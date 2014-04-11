#include <stdint.h>

#include "mongo/bson/util/atomic_int.h"

namespace mongo {

    class CoreCounter;

    class CoreCounterPool {
    public:
        static void init();
        static CoreCounter& createCounter();
    private:
        static inline uint64_t roundUpToPow2(uint64_t n);

        static uint64_t* counterPool;
        static AtomicUInt nextCounterSlot;
        static const uint64_t slotsPerCore = 8192;
        static unsigned ncores;
        static unsigned allocSize;
        // todo: dynamically allocate and deallocate slots
        // todo: static initializer should be called early in the dependancy graph, but
        //       after ProcessInfo
    };

} // namespace mongo
