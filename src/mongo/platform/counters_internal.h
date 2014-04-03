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

#ifndef MONGO_COUNTERS_INTERNAL_H
#define MONGO_COUNTERS_INTERNAL_H

/**
    Design notes.

    The pattern here is to create a set of static declarations for each counter.
    We redefine the macro COUNTER(), and then #include "counters.def" to get the
    desired result.

*/

#include "mongo/platform/counters.h"

#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
#include <sys/sysinfo.h>
#define CURCPU sched_getcpu()
#define NCPU   get_nprocs()

#elif __APPLE__
#include <sched.h>
#include <sys/sysctl.h>
#define CURCPU sched_getcpu()
#define NCPU   get_nprocs()

#elif _WIN32
#define CURCPU 0
#define NCPU   4
#else
#error
#endif


namespace mongo {

static inline uint32_t get_n_cpu() { return NCPU; }
static inline uint32_t get_cpu() { return CURCPU; }

/**
  Extern decl for each counter, one per line from .def file.
*/
#define COUNTER( ident, Category, Name, Description ) extern counter_t _counter_##ident;
#include "mongo/platform/counters.def"
#undef COUNTER

/**
  Declare counter enum codes, one per line from .def file.
*/
enum {
#define COUNTER( ident, Category, Name, Description ) COUNTER_##ident,
#include "mongo/platform/counters.def"
#undef COUNTER
    LAST_COUNTER
};

/**
  Declare counter methods, one per line from .def file.
*/
#define COUNTER( ident, Category, Name, Description ) \
	static inline void counter_##ident##_inc( ) { \
        volatile unsigned cpu; \
        __asm__ __volatile__("rdtscp": "=c"(cpu)); \
	_counter_##ident.cpus[cpu].slots[COUNTER_##ident % SLOTS_PER_CACHELINE] += 1; } \
	static inline void counter_##ident##_dec( ) { \
        volatile unsigned cpu; \
        __asm__ __volatile__("rdtscp": "=c"(cpu)); \
	_counter_##ident.cpus[cpu].slots[COUNTER_##ident % SLOTS_PER_CACHELINE] -= 1; } \
	static inline void counter_##ident##_add( int64_t val ) { \
        volatile unsigned cpu; \
        __asm__ __volatile__("rdtscp": "=c"(cpu)); \
	_counter_##ident.cpus[cpu].slots[COUNTER_##ident % SLOTS_PER_CACHELINE] += val; } \
	static inline int64_t counter_##ident##_get( ) { \
        volatile unsigned cpu; \
        __asm__ __volatile__("rdtscp": "=c"(cpu)); \
	return _counter_##ident.cpus[cpu].slots[COUNTER_##ident % SLOTS_PER_CACHELINE]; }
#include "mongo/platform/counters.def"
#undef COUNTER

#undef CURCPU
#undef NCPU

}   /* namespace mongo */
#endif /* !MONGO_COUNTERS_INTERNAL_H */
