/**
 *    Copyright (C) 2014 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include <pthread.h>

#include "mongo/util/counters/core_counter.h"
#include "mongo/util/counters/core_counter_arena.h"
#include "mongo/unittest/unittest.h"
#include "mongo/bson/util/atomic_int.h"

namespace mongo {

    void* basicIncTestThreadHelper(void* a_cntr) {
        CoreCounter* cntr = reinterpret_cast<CoreCounter*>(a_cntr);
        (*cntr)++;
        (*cntr)++;
        (*cntr)++;
        (*cntr)++;
        return NULL;
    }

    class BasicPerfTestFixture : public mongo::unittest::Test {
    protected:
        CoreCounter coreCounters[1024];
        AtomicUInt atomicCounters[1024];
        static const int nThreads = 32;

        void setUp() {
            for (int i = 0; i < 1024; ++i) {
                coreCounters[i] = CoreCounterArena::createCounter();
            }
        }

        static void* incCoreCounters(void* instance) {
            for (int j = 0; j < 1024; j++) {
                for (int i = 0; i < 1024; ++i) {
                    reinterpret_cast<BasicPerfTestFixture*>(instance)->coreCounters[i]++;
                }
            }
            return NULL;
        }

        static void* incAtomicCounters(void* instance) {
            for (int j = 0; j < 1024; j++) {
                for (int i = 0; i < 1024; ++i) {
                    reinterpret_cast<BasicPerfTestFixture*>(instance)->atomicCounters[i]++;
                }
            }
            return NULL;
        }
    };

    TEST(CoreCounterTests, BasicOperatorIncAndDec) {
        CoreCounter cntr1 = CoreCounterArena::createCounter();
        CoreCounter cntr2 = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr1.get(), 0LL);
        ASSERT_EQUALS(cntr2.get(), 0LL);
        cntr1++;
        ++cntr2;
        ASSERT_EQUALS(cntr1.get(), 1LL);
        ASSERT_EQUALS(cntr2.get(), 1LL);
        --cntr1;
        cntr2--;
        ASSERT_EQUALS(cntr1.get(), 0LL);
        ASSERT_EQUALS(cntr2.get(), 0LL);
    }

    TEST(CoreCounterTests, BasicOperatorPrefixDec) {
        CoreCounter cntr = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr.get(), 0LL);
        --cntr;
        ASSERT_EQUALS(cntr.get(), -1LL);
    }

    TEST(CoreCounterTests, BasicOperatorPostfixDec) {
        CoreCounter cntr = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr.get(), 0LL);
        cntr--;
        ASSERT_EQUALS(cntr.get(), -1LL);
    }

    TEST(CoreCounterTests, BasicOperatorPrefixInc) {
        CoreCounter cntr = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr.get(), 0LL);
        ++cntr;
        ASSERT_EQUALS(cntr.get(), 1LL);
    }

    TEST(CoreCounterTests, BasicOperatorPostfixInc) {
        CoreCounter cntr = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr.get(), 0LL);
        cntr++;
        ASSERT_EQUALS(cntr.get(), 1LL);
    }

    TEST(CoreCounterTests, BasicOperatorDecValue) {
        CoreCounter cntr = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr.get(), 0LL);
        cntr -= 10;
        ASSERT_EQUALS(cntr.get(), -10LL);
        cntr -= 20;
        ASSERT_EQUALS(cntr.get(), -30LL);
    }

    TEST(CoreCounterTests, BasicOperatorIncValue) {
        CoreCounter cntr = CoreCounterArena::createCounter();
        ASSERT_EQUALS(cntr.get(), 0LL);
        cntr += 10;
        ASSERT_EQUALS(cntr.get(), 10LL);
        cntr += 20;
        ASSERT_EQUALS(cntr.get(), 30LL);
    }

    TEST(CoreCounterTests, MultiThreadedBasicInc) {
        // create a counter 256 times and increment from 100 different threads
        for (int j = 0; j < 256; ++j) {
            CoreCounter cntr = CoreCounterArena::createCounter();
            pthread_t threads[100];
            for (int i = 0; i < 100; ++i) {
                pthread_create(&threads[i], NULL, basicIncTestThreadHelper, &cntr);
            }
            for (int i = 0; i < 100; ++i) {
                pthread_join(threads[i], NULL);
            }
            ASSERT_EQUALS(400, cntr.get());
        }
    }

    TEST_F(BasicPerfTestFixture, CoreCounterPerfTest) {
        pthread_t threads[nThreads];
        for (int i = 0; i < nThreads; ++i) {
            pthread_create(&threads[i], NULL, incCoreCounters, this);
        }
        for (int i = 0; i < nThreads; ++i) {
            pthread_join(threads[i], NULL);
        }
        ASSERT_EQUALS(coreCounters[0].get(), 1024L * nThreads);
    }

    TEST_F(BasicPerfTestFixture, AtomicCounterPerfTest) {
        pthread_t threads[nThreads];
        for (int i = 0; i < nThreads; ++i) {
            pthread_create(&threads[i], NULL, incAtomicCounters, this);
        }
        for (int i = 0; i < nThreads; ++i) {
            pthread_join(threads[i], NULL);
        }
        ASSERT_EQUALS(atomicCounters[0], 1024UL * nThreads);
    }

}
