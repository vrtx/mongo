#pragma once

#include <stdint.h>

#include "mongo/bson/util/atomic_int.h"

namespace mongo {

    class CoreCounter;

    class CoreCounterPool {
    public:
        static void init();
        static CoreCounter& createCounter();
        static inline unsigned getNumCores() { return ncores; }
        static inline unsigned getCoreOffsetBits() { return coreOffsetBits; }
    private:
        static inline uint64_t roundUpToPow2(uint64_t n);

        static uint64_t* counterPool;
        static AtomicUInt nextCounterSlot;
        static const uint32_t slotsPerCore = 8192;
        static const uint8_t coreOffsetBits = 16;   // (8192 * sizeof(uint64_t) = 2^16)
        static unsigned ncores;
        static unsigned allocSize;
        // todo: dynamically allocate and deallocate slots
        // todo: static initializer should be called early in the dependancy graph, but
        //       after ProcessInfo
    };

} // namespace mongo
