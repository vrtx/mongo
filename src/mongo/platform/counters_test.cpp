/*    Copyright 2013 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "mongo/platform/basic.h"
#include "mongo/platform/counters_internal.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {

    TEST(Counters, Test1) {
        for (uint32_t i=0; i<1000; ++i) {
            mongo::counter_op_test1_inc();
            if (i%2) mongo::counter_op_test2_inc();
            if (i%3) mongo::counter_op_test3_inc();
            if (i%4) mongo::counter_op_test4_inc();
        }
        ASSERT_TRUE(mongo::counter_op_test1_get() == 1000);
        ASSERT_TRUE(mongo::counter_op_test2_get() == 500);
        ASSERT_TRUE(mongo::counter_op_test3_get() == 666);
        ASSERT_TRUE(mongo::counter_op_test4_get() == 750);
    }

}  // namespace
}  // namespace mongo

