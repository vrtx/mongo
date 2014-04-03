/*
* Copyright 2013 10gen Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef MONGOC_COUNTERS_H
#define MONGOC_COUNTERS_H

#include <sys/types.h>

#ifndef SLOTS_PER_CACHELINE
#define SLOTS_PER_CACHELINE 8
#endif

/**
    Design notes.

    One counter consists of an array of NCPUS memory locations.
    Counter increment is done per-core to counter[CURCPU].
    Aggregate value for a counter is Sum_{i=0..NCPUS-1}( counter[i] ).

    We lay out counters in columns through blocks of 64B cache lines
    in order to avoid cache contentiton on updates by concurrent threads.

    In the following diagram c_ij labels the the j=0..NCPUS-1 components of counter i.
    Suppose NCPUS = 4 in this example, and each cache line holds 8 counters.
    (Each counter is a 64-bit unsigned int, and the cache lines are 64B.)

        slots[SLOTS_PER_CACHELINE][] :

        cache line 0: [c_00] [c_10] [c_20] [c_30] [c_40] [c_50] [c_60] [c_70]
        cache line 1: [c_01] [c_11] [c_21] [c_31] [c_41] [c_51] [c_61] [c_71]
        cache line 2: [c_02] [c_12] [c_22] [c_32] [c_42] [c_52] [c_62] [c_72]
        cache line 3: [c_03] [c_13] [c_23] [c_33] [c_43] [c_53] [c_63] [c_73]

        cache line 4: [c_80] [c_90] [c_a0] [c_b0] [c_c0] [c_d0] [c_e0] [c_f0]
        cache line 5: [c_81] [c_91] [c_a1] [c_b1] [c_c1] [c_d1] [c_e1] [c_f1]
        cache line 6: [c_82] [c_92] [c_a2] [c_b2] [c_c2] [c_d2] [c_e2] [c_f2]
        cache line 7: [c_83] [c_93] [c_a3] [c_b3] [c_c3] [c_d3] [c_e3] [c_f3]

        ...

    Typical access pattern:

        allocate:
            counter.cpus = (counter_slots_t*)(segment + off);
        increment:
            counter.cpus[CURCPU].slots[COUNTER_ENUM % SLOTS_PER_CACHELINE] += 1;

*/

namespace mongo {

    typedef struct {
        int64_t slots[SLOTS_PER_CACHELINE];
    } counter_slots_t;
    
    typedef struct {
        counter_slots_t *cpus;
    } counter_t;

}   // namespace mongo 
#endif
