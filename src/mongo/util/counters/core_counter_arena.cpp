#include <new>
#include <string.h>

#include "mongo/util/counters/core_counter_arena.h"
#include "mongo/util/counters/core_counter.h"
#include "mongo/util/processinfo.h"

namespace mongo {

    void CoreCounterArena::init() {
        ProcessInfo p;
        ncores = p.getNumCores();
        allocSize = ncores * slotsPerCore * sizeof(uint64_t);
        int err = ::posix_memalign(reinterpret_cast<void**>(&counterPool),
                                   roundUpToPow2(allocSize),
                                   allocSize);
        fassert(17431, err == 0);
        ::memset(counterPool, 0, allocSize);
    }

    CoreCounter& CoreCounterArena::createCounter() {
        uint64_t slot = nextCounterSlot++;
        return *(new (counterPool + slot) CoreCounter(slot));
    }

    // Round an integer up to the nearest power of 2 which is >= n
    uint64_t CoreCounterArena::roundUpToPow2(uint64_t n) {
        invariant(n > 0);
        // bitwise copy the highest set bits to all lower bits, then increment
        // n so the set bits carry over to the next highest bit.  Based on
        // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return ++n;
    }

    uint64_t* CoreCounterArena::counterPool = NULL;
    unsigned CoreCounterArena::ncores = 0;
    unsigned CoreCounterArena::allocSize = 0;
    AtomicUInt CoreCounterArena::nextCounterSlot;

}