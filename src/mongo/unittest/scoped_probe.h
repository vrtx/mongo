#include <iostream>
#include <fstream>
#include <map>

#include "mongo/db/jsobj.h"
#include "mongo/client/dbclientinterface.h"
#include "mongo/util/timer.h"
#include "mongo/util/time_support.h"

using namespace std;

namespace mongo {

  /**
   * Profile code executed between the creation and destruction of a
   * ScopedProbe.  Usage is generally of the form:
   *
   *   {
   *     ScopedProbe probe;
   *     ... code to instrument ...
   *     p.done();  // signals that profiled code did not end due to an exception
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

    void done() {
      success = true;
      recordEndTime();
    }

    void setMetric(string name, int64_t value) {
      metrics[name] = value;
    }

    ~ScopedProbe() {
      if (!success) recordEndTime();

      std::cout << std::endl
                << "{" << std::endl
                << "  TestName: \t\t'" << (_name.empty() ? "(unnamed)" : _name) << "'," << std::endl
                << "  Success:  \t\t"  << (success ? "true" : "false") << "," << std::endl;
      for (MetricMap::const_iterator it = metrics.begin();
           it != metrics.end();
           ++it) {
        std::cout << "  " << it->first << ": \t\t" << it->second << "," << std::endl;
      }
      std::cout << "  WallClockUs: \t\t"  << endMicros << "," << std::endl
                << "  CoreCycles:  \t\t"  << endTSC - startTSC << std::endl
                << "}," << std::endl;
                // << "  AllCycles:           " << getInvariantTSC(preState, postState) << ",\n"
                // << "  IPS:                 " << getIPC(preState, postState) << ",\n"
                // << "  RetiredIns:          " << getInstructionsRetired(preState, postState) << ",\n"
                // << "  L3HitRatio:          " << getL3CacheHitRatio(preState, postState)*100 << ",\n"
                // << "  L2HitRatio:          " << getL2CacheHitRatio(preState, postState)*100 << ",\n"
                // << "  MemCtrlrReadBytes:   " << getBytesReadFromMC(preState, postState) << ",\n"
                // << "  MemCtrlrWriteBytes:  " << getBytesWrittenToMC(preState, postState) << ",\n"
                // << "  MCQPITrafficRatio:   " << getQPItoMCTrafficRatio(preState, postState)*100 << "\n},\n";
                // << "  L3 Hits:      " << getL3CacheHits(preState, postState) << "\n"
                // << "  L3 Misses:    " << getL3CacheMisses(preState, postState) << "\n"
                // << "  L2 Hits:      " << getL2CacheHits(preState, postState) << "\n"
                // << "  L2 Misses:    " << getL2CacheMisses(preState, postState) << "\n"
    }

  private:
    bool success;
    std::string _name;
    mongo::Timer timer;
    int64_t startTSC;
    int64_t endTSC;
    uint64_t endMicros;
    typedef std::map<string, int> MetricMap;
    MetricMap metrics;

    void init() {
      startTSC = rdtsc();
      success = false;
    }

    void recordEndTime() {
      endTSC = rdtsc();
      endMicros = timer.micros();
    }

    // get timestamp counter
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
#       warning Time stamp counter is not currently supported on this platform.
      return 0;
#endif
    }

  };  // ScopedProbe

} // mongo


