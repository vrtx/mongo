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

#include "mongo/platform/counters_internal.h"

#include <assert.h>
#include <boost/utility.hpp>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace mongo {

#pragma pack(1)
typedef struct {
    uint32_t offset;
    uint32_t slot;
    char category[24];
    char name[32];
    char description[64];
} counter_info_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint32_t size;
    uint32_t n_cpu;
    uint32_t n_counters;
    uint32_t infos_offset;
    uint32_t values_offset;
    uint8_t padding[44];
} counters_t;
#pragma pack()

/*
     Declare the counters using 'ident', one per line from .def file
*/
#define COUNTER(ident, Category, Name, Description) counter_t _counter_##ident;
#include "mongo/platform/counters.def"
#undef COUNTER

/**
* counters_use_shm:
*
* Checks to see if counters should be exported over a shared memory segment.
*
* Returns: TRUE if SHM is to be used.
*/
static bool counters_use_shm(void) {
    return !getenv("MONGOC_DISABLE_SHM");
}

/**
* counters_calc_size:
*
* Returns the number of bytes required for the shared memory segment of
* the process. This segment contains the various statistical counters for
* the process.
*
* Returns: The number of bytes required.
*/
static size_t counters_calc_size(void) {
     size_t n_cpu;
     size_t n_groups;
     size_t size;

     n_cpu = get_n_cpu();
     n_groups = (LAST_COUNTER / SLOTS_PER_CACHELINE) + 1;
     size = (sizeof(counters_t) +
             (LAST_COUNTER * sizeof(counter_info_t)) +
             (n_cpu * n_groups * sizeof(counter_slots_t)));
     size_t pagesize = (size_t)getpagesize();
     return (pagesize < size ? size : pagesize);
}

/**
* counters_register:
* @counters: A counter_t.
* @num: The counter number.
* @category: The counter category.
* @name: The counter name.
* @description: The counter description.
*
* Registers a new counter in the memory segment for counters. If the counters
* are exported over shared memory, it will be made available.
*
* Returns: The offset to the data for the counters values.
*/
static size_t counters_register( counters_t *counters,
                                        uint32_t num,
                                        const char *category,
                                        const char *name,
                                        const char *description) {
    counter_info_t *infos;
    char *segment;
    int n_cpu;

    n_cpu = get_n_cpu();
    segment = (char *)counters;

    infos = (counter_info_t *)(segment + counters->infos_offset);
    infos = &infos[counters->n_counters++];
    infos->slot = num % SLOTS_PER_CACHELINE;
    infos->offset = (counters->values_offset +
                     ((num / SLOTS_PER_CACHELINE) *
                      n_cpu * sizeof(counter_slots_t)));

    strncpy(infos->category, category, sizeof infos->category);
    strncpy(infos->name, name, sizeof infos->name);
    strncpy(infos->description, description, sizeof infos->description);
    infos->category[sizeof infos->category-1] = '\0';
    infos->name[sizeof infos->name-1] = '\0';
    infos->description[sizeof infos->description-1] = '\0';

    return infos->offset;
}

static void counters_init(void) __attribute__((constructor));

/**
* counters_init:
*
* Initializes the counters system. This should be run on library
* initialization using the GCC constructor attribute.
*/
static void counters_init() {
    counter_info_t *info;
    counters_t *counters;
    size_t infos_size;
    size_t off;
    size_t size;
    char *segment;

    printf("counters_init\n");

    size = counters_calc_size();
    segment = (char *)malloc(size);
    infos_size = LAST_COUNTER * sizeof *info;

    counters = (counters_t *)segment;
    counters->size = size;
    counters->n_cpu = get_n_cpu();
    counters->n_counters = 0;
    counters->infos_offset = sizeof *counters;
    counters->values_offset = counters->infos_offset + infos_size;


/**
     Register each of the counters, one per line from the .def file.
*/
#define COUNTER(ident, Category, Name, Desc) \
off = counters_register(counters, COUNTER_##ident, Category, Name, Desc); \
_counter_##ident.cpus = (counter_slots_t*)(segment + off);
#include "mongo/platform/counters.def"
#undef COUNTER

}

} /* namespace mongo */
