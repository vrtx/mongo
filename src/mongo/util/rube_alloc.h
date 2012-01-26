// allocator.h

/*    Copyright 2009 10gen Inc.
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

#pragma once

#if !defined(_WIN32)
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#endif

#define DO_TRACK_ALLOC

namespace mongo {

    // slow, but functional.  write allocation data to stderr
    // allocType: 'A', 'R', 'F', 'X' // alloc, realloc, free, fault
    inline void track_alloc( const void *addr, const size_t len, const char allocType = 'A') {
#if !defined(_WIN32)
            const int MAX_DEPTH = 7;

            // generate backtrace addrs
            void* stackFrames[MAX_DEPTH];
            int numFrames = backtrace( stackFrames , MAX_DEPTH - 1);
            char ** bt = backtrace_symbols (stackFrames, numFrames );

            // get the timestamp
            struct timeval tv;
            gettimeofday( &tv, NULL );
 
            // write the json
            fprintf( stderr, "db.allocs.insert({ " );
            fprintf( stderr, " startAddr:%p, ",    addr );
            fprintf( stderr, " endAddr:%llu, ",   (unsigned long long)addr + len );
            fprintf( stderr, " size:%lu, ",        len );
            fprintf( stderr, " allocType:'%c', ", allocType );
            for ( int i = 1; i < numFrames; i++ ) { // fist frame uninteresting
                fprintf( stderr , " func%d: {addr:%p, sym:'%s'}, " , i, stackFrames[i], bt[i] );
            }
            fprintf( stderr, " ts: %llu ",        (((unsigned long long) tv.tv_sec) * 1000*1000) + tv.tv_usec);
            fprintf( stderr, "});\n");
#endif
    }

} // namespace mongo
