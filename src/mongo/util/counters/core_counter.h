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

#include "core_counter_arena.h"

namespace mongo {

#pragma pack(1)
    class CoreCounter {
    public:
        inline CoreCounter(int64_t* slot) : value(slot) { }
        inline CoreCounter() : value(NULL) { }
        inline ~CoreCounter() { }
        inline CoreCounter(const CoreCounter& rhs) { value = rhs.value; }
        inline CoreCounter& operator=(const CoreCounter& rhs) { value = rhs.value; return *this; }
        inline CoreCounter& operator++();    // ++prefix
        inline CoreCounter& operator++(int); // postfix++
        inline CoreCounter& operator--();    // --prefix
        inline CoreCounter& operator--(int); // postfix--
        inline CoreCounter& operator+=(int);
        inline CoreCounter& operator-=(int);
        inline int64_t get() const;
    private:
        uint32_t getCurrentCore() const;    // todo: inline
        int64_t* value;
    };
#pragma pack()

    CoreCounter& CoreCounter::operator++() {
        ++(*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) |
                                       (getCurrentCore() <<
                                        CoreCounterArena::getCoreOffsetBits())));
        return *this;
    }

    CoreCounter& CoreCounter::operator++(int) {
        return operator++();
    }

    CoreCounter& CoreCounter::operator--() {
        --(*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) |
                                       (getCurrentCore() <<
                                        CoreCounterArena::getCoreOffsetBits())));
        return *this;
    }

    CoreCounter& CoreCounter::operator--(int) {
        return operator--();
    }

    CoreCounter& CoreCounter::operator-=(int n) {
        (*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) |
                                     (getCurrentCore() << CoreCounterArena::getCoreOffsetBits())))
            -=n;
        return *this;
    }

    CoreCounter& CoreCounter::operator+=(int n) {
        (*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) |
                                     (getCurrentCore() << CoreCounterArena::getCoreOffsetBits())))
            +=n;
        return *this;
    }

    int64_t CoreCounter::get() const {
        int64_t total = 0;
        for (unsigned i = 0; i < CoreCounterArena::getNumCores(); ++i)
            total += (*reinterpret_cast<int64_t*>(reinterpret_cast<intptr_t>(value) |
                                                  (i << CoreCounterArena::getCoreOffsetBits())));
        return total;
    }

} // namespace mongo
