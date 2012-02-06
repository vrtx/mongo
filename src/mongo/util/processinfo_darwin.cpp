// processinfo_darwin.cpp

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

#include "../pch.h"
#include "processinfo.h"
#include "log.h"

#include <mach/vm_statistics.h>
#include <mach/task_info.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach_traps.h>
#include <mach/task.h>
#include <mach/vm_map.h>
#include <mach/shared_region.h>
#include <iostream>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/sysctl.h>

using namespace std;

namespace mongo {

    ProcessInfo::ProcessInfo( pid_t pid ) : _pid( pid ) {
    }

    ProcessInfo::~ProcessInfo() {
    }

    bool ProcessInfo::supported() {
        return true;
    }

    int ProcessInfo::getVirtualMemorySize() {
        task_t result;

        mach_port_t task;

        if ( ( result = task_for_pid( mach_task_self() , _pid , &task) ) != KERN_SUCCESS ) {
            cout << "error getting task\n";
            return 0;
        }

#if !defined(__LP64__)
        task_basic_info_32 ti;
#else
        task_basic_info_64 ti;
#endif
        mach_msg_type_number_t  count = TASK_BASIC_INFO_COUNT;
        if ( ( result = task_info( task , TASK_BASIC_INFO , (task_info_t)&ti, &count ) )  != KERN_SUCCESS ) {
            cout << "error getting task_info: " << result << endl;
            return 0;
        }
        return (int)((double)ti.virtual_size / (1024.0 * 1024 ) );
    }

    int ProcessInfo::getResidentSize() {
        task_t result;

        mach_port_t task;

        if ( ( result = task_for_pid( mach_task_self() , _pid , &task) ) != KERN_SUCCESS ) {
            cout << "error getting task\n";
            return 0;
        }


#if !defined(__LP64__)
        task_basic_info_32 ti;
#else
        task_basic_info_64 ti;
#endif
        mach_msg_type_number_t  count = TASK_BASIC_INFO_COUNT;
        if ( ( result = task_info( task , TASK_BASIC_INFO , (task_info_t)&ti, &count ) )  != KERN_SUCCESS ) {
            cout << "error getting task_info: " << result << endl;
            return 0;
        }
        return (int)( ti.resident_size / (1024 * 1024 ) );
    }

    void ProcessInfo::getExtraInfo(BSONObjBuilder& info) {
        struct task_events_info taskInfo;
        mach_msg_type_number_t taskInfoCount = TASK_EVENTS_INFO_COUNT;

        if ( KERN_SUCCESS != task_info(mach_task_self(), TASK_EVENTS_INFO, 
                                       (integer_t*)&taskInfo, &taskInfoCount) ) {
            cout << "error getting extra task_info" << endl;
            return;
        }

        info.append("page_faults", taskInfo.pageins);
        getSystemInfo(info);
    }

    void ProcessInfo::getSystemInfo( BSONObjBuilder& info ) {
        if (_serverStats.isEmpty())
            // lazy load sysinfo
            collectSystemInfo();
    }

    enum sysctlValueType {
        sysctlValue_Int = 0,
        sysctlValue_String =1
    };

    /**
     * Get a sysctl value by name
     */
    string getSysctlByName( const string& sysctlName, int type ) {
        char value[256];
        long long intValue = 0;
        size_t len = sizeof(value);
        size_t intLen = sizeof(value);
        if ( type == sysctlValue_String) {
            if ( sysctlbyname(sysctlName.c_str(), &value, &len, NULL, 0) < 0 ) {
                log() << "Unable to resolve sysctl " << sysctlName << " (string) " << endl;
            }
            value[len - 1] = '\0';
        }
        else {
            if ( sysctlbyname(sysctlName.c_str(), &intValue, &intLen, NULL, 0) < 0 ) {
                log() << "Unable to resolve sysctl " << sysctlName << " (number) " << endl;
            }
            if (intLen > 8) {
                log() << "Unable to resolve sysctl " << sysctlName << " as integer.  System returned " << intLen << " bytes." << endl;
            }
            return boost::lexical_cast <string>(intValue);
        }
        return string(value, len - 1);
    }

    string getSysctlByName( const string& sysctlName) {
        return getSysctlByName( sysctlName, sysctlValue_String );
    }

    void ProcessInfo::collectSystemInfo() {
        BSONObjBuilder bSI, bSys, bOS;
        bOS.append("Type", "Darwin");
        bOS.append("Distro", "Mac OS X");
        bOS.append("Version", getSysctlByName("kern.osrelease"));
        bOS.append("VersionString", getSysctlByName("kern.version"));
        bOS.append("BootTime", getSysctlByName("kern.boottime", sysctlValue_Int));
        bOS.append("AlwaysFullSync", getSysctlByName("vfs.generic.always_do_fullfsync", sysctlValue_Int));
        bOS.append("NFSAsync", getSysctlByName("vfs.generic.nfs.client.allow_async", sysctlValue_Int));

        bSys.append("Architecture",  getSysctlByName("hw.machine"));
        bSys.append("Model", getSysctlByName("hw.model"));
        bSys.append("MemSize", getSysctlByName("hw.memsize", sysctlValue_Int));
        bSys.append("NumCores", getSysctlByName("hw.ncpu", sysctlValue_Int));    // include hyperthreading
        bSys.append("PhysicalCores", getSysctlByName("machdep.cpu.core_count", sysctlValue_Int));
        bSys.append("CPUFrequency", getSysctlByName("hw.cpufrequency", sysctlValue_Int));
        bSys.append("CPUString", getSysctlByName("machdep.cpu.brand_string"));
        bSys.append("CPUFeatures", getSysctlByName("machdep.cpu.features"));
        bSys.append("CPUExtraFeatures", getSysctlByName("machdep.cpu.extfeatures"));
        bSys.append("PageSize", getSysctlByName("hw.pagesize", sysctlValue_Int));
        bSys.append("Scheduler", getSysctlByName("kern.sched"));
        bSI.append(StringData("System"), bSys.obj());
        bSI.append(StringData("OS"), bOS.obj());
        _serverStats = bSI.obj();
        log() << "Performance: " << _serverStats.jsonString(Strict, true) << endl;
    }

    bool ProcessInfo::processHasNumaEnabled() {
        return false;
    }

    bool ProcessInfo::blockCheckSupported() {
        return true;
    }

    bool ProcessInfo::blockInMemory( char * start ) {
        static long pageSize = 0;
        if ( pageSize == 0 ) {
            pageSize = sysconf( _SC_PAGESIZE );
        }
        start = start - ( (unsigned long long)start % pageSize );
        char x = 0;
        if ( mincore( start , 128 , &x ) ) {
            log() << "mincore failed: " << errnoWithDescription() << endl;
            return 1;
        }
        return x & 0x1;
    }

}
