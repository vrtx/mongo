#include "mongo/util/counters/core_counter.h"
#include "mongo/util/counters/core_counter_pool.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

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

}
