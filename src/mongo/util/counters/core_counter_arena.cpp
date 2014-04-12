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

#include <stdlib.h>

#include "mongo/base/init.h"
#include "mongo/base/status.h"
#include "mongo/util/counters/core_counter_arena.h"
#include "mongo/util/counters/core_counter.h"
#include "mongo/util/processinfo.h"

namespace mongo {

    MONGO_INITIALIZER(CoreCounterArenaAllocator)(InitializerContext*) {
        CoreCounterArena::init();
        return Status::OK();
    }

    // initialize the 'arenas' for each core
    void CoreCounterArena::init() {
        ProcessInfo p;
        ncores = p.getNumCores();
        allocSize = ncores * slotsPerCore * sizeof(int64_t);
        int err = ::posix_memalign(reinterpret_cast<void**>(&counterArenas),
                                   roundUpToPow2(allocSize),
                                   allocSize);
        fassert(17431, err == 0);
        ::memset(counterArenas, 0, allocSize);
    }

    // Create a CoreCounter instance (factory style)
    CoreCounter CoreCounterArena::createCounter() {
        return CoreCounter(counterArenas + nextCounterSlot++);
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

    int64_t* CoreCounterArena::counterArenas = NULL;
    unsigned CoreCounterArena::ncores = 0;
    unsigned CoreCounterArena::allocSize = 0;
    AtomicUInt CoreCounterArena::nextCounterSlot;

} // namespace mongo
