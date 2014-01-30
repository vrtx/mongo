#include <iostream>

#include "mongo/util/timer.h"

#include "third_party/pcm/cpucounters.h"

using namespace std;

namespace mongo {

  namespace unittest {

    /**
     * Profile code executed between the creation and destruction of a
     * ScopedProbe.  Usage is generally of the form:
     *
     *   {
     *     ScopedProbe probe;
     *     ... code to instrument ...
     *   }
     */
    class ScopedProbe {
    public:
        ScopedProbe() {
          init();
        }

        ScopedProbe(const std::string& name) {
          _name = name;
          init();
        }

        void init() {
          preState = getSystemCounterState();
          startTSC = rdtsc();
        }

        ~ScopedProbe() {
            endTSC = rdtsc();
            unsigned long long micros = timer.micros();
            postState = getSystemCounterState();
            cout << "\n Test [" << (_name.empty() ? "(unnamed)" : _name) << "]: \n"
                      << "  Wallclock:              " << micros << "us\n"
                      << "  Cycles (current core):  " << endTSC - startTSC << "\n"
                      << "  Cycles (all cores):     " << getInvariantTSC(preState, postState) << "\n"
                      << "  Instructions per cycle: " << getIPC(preState, postState) << "\n"
                      << "  Retired Instructions:   " << getInstructionsRetired(preState, postState) << "\n"
                      << "  L3 Hit Ratio:           " << getL3CacheHitRatio(preState, postState)*100 << "%\n"
                      << "  L2 Hit Ratio:           " << getL2CacheHitRatio(preState, postState)*100 << "%\n"
                      << "  MemCtrlr Read:          " << getBytesReadFromMC(preState, postState) << "b\n"
                      << "  MemCtrlr Written:       " << getBytesWrittenToMC(preState, postState) << "b\n"
                      << "  MC/QPI traffic ratio:   " << getQPItoMCTrafficRatio(preState, postState)*100 << "%\n";
                      // << "  L3 Hits:      " << getL3CacheHits(preState, postState) << "\n"
                      // << "  L3 Misses:    " << getL3CacheMisses(preState, postState) << "\n"
                      // << "  L2 Hits:      " << getL2CacheHits(preState, postState) << "\n"
                      // << "  L2 Misses:    " << getL2CacheMisses(preState, postState) << "\n"
        }

        static PCM* pcmInstance;

    private:
        std::string _name;
        mongo::Timer timer;
        SystemCounterState preState;
        SystemCounterState postState;
        int64_t startWallTime;
        int64_t endWallTime;
        int64_t startTSC;
        int64_t endTSC;

        // get timestamp counter.  todo: verify availability/stability at runtime.
        inline uint64_t rdtsc() {
#if defined(__i386__)
            uint32_t ret;
            __asm__ volatile ("rdtsc" : "=A" (ret) );
            return ret;
#elif defined(__x86_64__) || defined(__amd64__)
            uint32_t low, high;
            __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
            return ((uint64_t)high << 32) | low;
#else
#               warning Time stamp counter is not currently supported on this platform.
            return 0;
#endif
        }

    };


    /**
     * Performance, counter and timer stats collected by a probe
     */
    struct ProbeResult {
        int64_t cycleCount;
        int16_t instructionsPerCycle;
    };

  } // unittest

} // mongo

