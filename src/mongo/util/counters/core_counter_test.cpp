#include <pthread.h>

#include "mongo/util/counters/core_counter.h"
#include "mongo/util/counters/core_counter_arena.h"
#include "mongo/unittest/unittest.h"
#include "mongo/bson/util/atomic_int.h"

namespace mongo {

    void* basicIncTestThread(void* a_cntr) {
        CoreCounter* cntr = reinterpret_cast<CoreCounter*>(a_cntr);
        (*cntr)++;
        (*cntr)++;
        (*cntr)++;
        (*cntr)++;
        return NULL;
    }

    class BasicPerfTestHelper {
    public:
        static CoreCounter coreCounters[1024];
        static AtomicUInt atomicCounters[1024];

        static void* init() {
            for (int i = 0; i < 1024; ++i) {
                coreCounters[i] = CoreCounterArena::createCounter();
            }
            return NULL;
        }
        static void* incCoreCounters(void*) {
            for (int j = 0; j < 1024 * 40; j++) {
                for (int i = 0; i < 1024; ++i) {
                    coreCounters[i]++;
                }
            }
            return NULL;
        }
        static void* incAtomicCounters(void*) {
            for (int j = 0; j < 1024 * 40; j++) {
                for (int i = 0; i < 1024; ++i) {
                    atomicCounters[i]++;
                }
            }
            return NULL;
        }
    };
    CoreCounter BasicPerfTestHelper::coreCounters[1024];
    AtomicUInt BasicPerfTestHelper::atomicCounters[1024];
    const int nThreads =  10;

    TEST(CoreCounterTests, BasicOperatorInc) {
        CoreCounterArena::init();
        CoreCounter cntr1 = CoreCounterArena::createCounter();
        CoreCounter cntr2 = CoreCounterArena::createCounter();

        ASSERT_EQUALS(cntr1.get(), 0LL);
        cntr1++;
        ASSERT_EQUALS(cntr1.get(), 1LL);
        ++cntr1;
        ASSERT_EQUALS(cntr1.get(), 2LL);

        ASSERT_EQUALS(cntr2.get(), 0LL);
        cntr2++;
        ASSERT_EQUALS(cntr2.get(), 1LL);
        ++cntr2;
        ASSERT_EQUALS(cntr2.get(), 2LL);
    }

    // create 256 counters and increment from 100 different threads
    TEST(CoreCounterTests, MultiThreadedBasicInc) {
        CoreCounterArena::init();
        for (int j = 0; j < 256; ++j) {
            CoreCounter cntr = CoreCounterArena::createCounter();
            pthread_t threads[100];
            for (int i = 0; i < 100; ++i) {
                pthread_create(&threads[i], NULL, basicIncTestThread, &cntr);
            }
            for (int i = 0; i < 100; ++i) {
                pthread_join(threads[i], NULL);
            }
            ASSERT_EQUALS(400, cntr.get());
        }
    }

    TEST(CoreCounterPerfTests, CoreCounterPerfTest) {
        CoreCounterArena::init();
        BasicPerfTestHelper::init();
        pthread_t threads[nThreads];
        for (int i = 0; i < nThreads; ++i) {
            pthread_create(&threads[i], NULL, BasicPerfTestHelper::incCoreCounters, NULL);
        }
        for (int i = 0; i < nThreads; ++i) {
            pthread_join(threads[i], NULL);
        }
        ASSERT_EQUALS(BasicPerfTestHelper::coreCounters[0].get(), 1024 * 40 * nThreads);
    }

    TEST(AtomicCounterPerfTests, AtomicCounterPerfTest) {
        CoreCounterArena::init();
        BasicPerfTestHelper::init();
        pthread_t threads[nThreads];
        for (int i = 0; i < nThreads; ++i) {
            pthread_create(&threads[i], NULL, BasicPerfTestHelper::incAtomicCounters, NULL);
        }
        for (int i = 0; i < nThreads; ++i) {
            pthread_join(threads[i], NULL);
        }
        ASSERT_EQUALS(BasicPerfTestHelper::atomicCounters[0], 1024 * 40 * nThreads);
    }

}
