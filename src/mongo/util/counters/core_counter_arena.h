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

#pragma once

#include <stdint.h>

#include "mongo/bson/util/atomic_int.h"

namespace mongo {

    class CoreCounter;

    // Design Notes:
    //
    // Creating a counter allocates a slot in each core's arena.  The counter
    // instance stores the reference to the slot in arena 0.  When a counter is
    // modified, the appropriate arena is selected by applying a bitwise or
    // of the executing CPU shifted left by the arena size.
    //
    // Memory layout is:
    //
    //    arena[core1]: [cntr1][cntr2][cntr3][cntr4][cntr5][cntr6][cntr7][cntr8]
    //                  [cntr9][cntrA][cntrB][cntrC][cntrD][cntrE] ...   [cntrN]
    //    ... 
    //    arena[coreN]: [cntr1][cntr2][cntr3][cntr4][cntr5][cntr6][cntr7][cntr8]
    //                  [cntr9][cntrA][cntrB][cntrC][cntrD][cntrE] ...   [cntrN]
    //
    // arena[0] is aligned to the ^2 >= ncores * slotsPerCore * sizeof(uint64_t).
    // arena[n] is aligned to the ^2 >= slotsPerCore * sizeof(uint64_t).

    // TODO: The current implementation has a fixed limit on the number of counters,
    //       and does not reuse slots when a counter is destroyed.  To make the
    //       CoreCounter class a drop-in replacement for AtomicInt, we need to:
    //          1) reuse slots from destroyed counters
    //          2) allocate new arenas when full
    //
    //       Once a proper allocator has been implemented, the factory-esque nature
    //       of this class should probably be replaced.

    class CoreCounterArena {
    public:
        static void init();
        static CoreCounter createCounter();
        static inline unsigned getNumCores() { return ncores; }
        static inline unsigned getCoreOffsetBits() { return coreOffsetBits; }
    private:
        static inline uint64_t roundUpToPow2(uint64_t n);

        static int64_t* counterArenas;
        static AtomicUInt nextCounterSlot;
        static const uint32_t slotsPerCore = 8192;
        static const uint8_t coreOffsetBits = 16;   // (8192 * sizeof(uint64_t) = 2^16)
        static unsigned ncores;
        static unsigned allocSize;
    };

} // namespace mongo
