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

#include <sched.h>
#include <stdio.h>

#include "mongo/util/counters/core_counter.h"

namespace mongo {

    uint32_t CoreCounter::getCurrentCore() const {
#if defined(_WIN32)
        // WARNING: UNTESTED!
        return ::GetCurrentProcessorNumberEx();

#elif defined(__linux__) || defined(__sunos__)
        return ::sched_getcpu();

#elif defined(__APPLE__)
        // WARNING: The mac os implementation is currently slower than using
        //          an AtomicUInt as a counter due to the cost of the CPUID
        //          instruction (which is serializing).  RDTSCP would be ideal,
        //          however IA32_TSC_AUX is not set.
        uint32_t core = 0;
        asm volatile ("cpuid"
                      : "=b"(core)
                      : "a"(1)
                      : "%eax", "%ebx", "%ecx", "%edx");
        core >>= 24;
        return core;
#endif

    }

}
