#pragma once

#include <stdint.h>

#include "core_counter_arena.h"

namespace mongo {

    // Design Notes:
    //
    // Each counter allocates space in an arena for each core.  The counter
    // is actually a reference to the arena for core 0.  When a counter is
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
    // arena[n] is aligned to the ^2 >= slotsPerCore * sizeof(uint64_t)

#pragma pack(1)
    class CoreCounter {
    public:
        inline CoreCounter(int64_t* slot) : value(slot) { }
        inline CoreCounter& operator++();    // ++prefix
        inline CoreCounter& operator++(int); // postfix++
        inline CoreCounter& operator--();    // --prefix
        inline CoreCounter& operator--(int); // postfix--
        inline CoreCounter& operator+=(int);
        inline CoreCounter& operator-=(int);
        inline CoreCounter& operator+=(CoreCounter&);
        inline CoreCounter& operator-=(CoreCounter&);
        inline int64_t get() const;
    private:
        uint32_t getCurrentCore() const;    // todo: inline
        int64_t* value;
    };
#pragma pack()

    CoreCounter& CoreCounter::operator++() {
        ++(*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) | (getCurrentCore() << CoreCounterArena::getCoreOffsetBits())));
        return *this;
    }

    CoreCounter& CoreCounter::operator++(int) {
        return operator++();
    }

    int64_t CoreCounter::get() const {
        int64_t total = 0;
        for (unsigned i = 0; i < CoreCounterArena::getNumCores(); ++i)
            total += (*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) | (i << CoreCounterArena::getCoreOffsetBits())));
        return total;
    }

}
