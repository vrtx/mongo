#include <stdint.h>

namespace mongo {

    // Design Notes:
    // Memory layout is:
    //
    //    pool[core1]: [cntr1][cntr2][cntr3][cntr4][cntr5][cntr6][cntr7][cntr8]
    //                 [cntr9][cntrA][cntrB][cntrC][cntrD][cntrE] ...   [cntrN]
    //    ...
    //    pool[coreN]: [cntr1][cntr2][cntr3][cntr4][cntr5][cntr6][cntr7][cntr8]
    //                 [cntr9][cntrA][cntrB][cntrC][cntrD][cntrE] ...   [cntrN]
    //
    // pool[0] is aligned to the ^2 >= ncores * slotsPerCore.
    //

#pragma pack(1)
    class CoreCounter {
    public:
        inline CoreCounter(uint64_t slot) : value(0) { }
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
        uint16_t getCurrentCore() const;    // todo: inline
        int64_t value;
    };
#pragma pack()


    CoreCounter& CoreCounter::operator++() {
        ++value;
        // printf("[++] pool: %p, counter value: %llu, this: %p\n", CoreCounterPool::getBase(), value, this);
        return *this;
    }

    CoreCounter& CoreCounter::operator++(int) {
        return operator++();
    }

    int64_t CoreCounter::get() const {
        // todo:
        return value;
    }

}
