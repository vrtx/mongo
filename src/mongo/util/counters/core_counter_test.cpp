#include <pthread.h>

#include "mongo/util/counters/core_counter.h"
#include "mongo/util/counters/core_counter_arena.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

    void* basicIncTestThread(void* a_cntr) {
        CoreCounter* cntr = reinterpret_cast<CoreCounter*>(a_cntr);
        (*cntr)++;
        (*cntr)++;
        (*cntr)++;
        (*cntr)++;
        return NULL;
    }

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


}
