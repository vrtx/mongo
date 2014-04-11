#include <pthread.h>

#include "mongo/util/counters/core_counter.h"
#include "mongo/util/counters/core_counter_pool.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

    void* basicIncTest(void* a_cntr) {
        CoreCounter* cntr = reinterpret_cast<CoreCounter*>(a_cntr);
        (*cntr)++;
        ++(*cntr);
        return NULL;
    }

    TEST(CoreCounterTests, BasicOperatorInc) {
        CoreCounterPool::init();
        CoreCounter& cntr1 = CoreCounterPool::createCounter();
        CoreCounter& cntr2 = CoreCounterPool::createCounter();

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

    TEST(CoreCounterTests, MultiThreadedBasicInc) {
        CoreCounterPool::init();
        CoreCounter& cntr = CoreCounterPool::createCounter();
        pthread_t threads[100];
        for (int i = 0; i < 100; ++i) {
            pthread_create(&threads[i], NULL, basicIncTest, &cntr);
        }
        for (int i = 0; i < 100; ++i) {
            pthread_join(threads[i], NULL);
        }
        ASSERT_EQUALS(200, cntr.get());
    }


}
