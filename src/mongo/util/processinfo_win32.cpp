// processinfo_win32.cpp

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

#include "pch.h"
#include "processinfo.h"
#include <iostream>
#include <psapi.h>

using namespace std;

int getpid() {
    return GetCurrentProcessId();
}

namespace mongo {

    // dynamically link to psapi.dll (in case this version of Windows
    // does not support what we need)
    struct PsApiInit {
        bool supported;
        typedef BOOL (WINAPI *pQueryWorkingSetEx)(HANDLE hProcess, 
                                                  PVOID pv, 
                                                  DWORD cb);
        pQueryWorkingSetEx QueryWSEx;
        
        PsApiInit() {
            HINSTANCE psapiLib = LoadLibrary( TEXT("psapi.dll") );
            if (psapiLib) {
                QueryWSEx = reinterpret_cast<pQueryWorkingSetEx>
                    ( GetProcAddress( psapiLib, "QueryWorkingSetEx" ) );
                if (QueryWSEx) {
                    supported = true;
                    return;
                }
            }
            supported = false;
        }
    } psapiGlobal;
                
    int _wconvertmtos( SIZE_T s ) {
        return (int)( s / ( 1024 * 1024 ) );
    }

    ProcessInfo::ProcessInfo( pid_t pid ) {
    }

    ProcessInfo::~ProcessInfo() {
    }

    bool ProcessInfo::supported() {
        return true;
    }

    int ProcessInfo::getVirtualMemorySize() {
        MEMORYSTATUSEX mse;
        mse.dwLength = sizeof(mse);
        verify( 16050, GlobalMemoryStatusEx( &mse ) );
        DWORDLONG x = (mse.ullTotalVirtual - mse.ullAvailVirtual) / (1024 * 1024) ;
        assert( x <= 0x7fffffff );
        return (int) x;
    }

    int ProcessInfo::getResidentSize() {
        PROCESS_MEMORY_COUNTERS pmc;
        verify( 16051, GetProcessMemoryInfo( GetCurrentProcess() , &pmc, sizeof(pmc) ) );
        return _wconvertmtos( pmc.WorkingSetSize );
    }

    void ProcessInfo::getExtraInfo(BSONObjBuilder& info) {
        MEMORYSTATUSEX mse;
        mse.dwLength = sizeof(mse);
        PROCESS_MEMORY_COUNTERS pmc;
        if( GetProcessMemoryInfo( GetCurrentProcess() , &pmc, sizeof(pmc) ) ) {
            info.append("page_faults", static_cast<int>(pmc.PageFaultCount));
            info.append("usagePageFileMB", static_cast<int>(pmc.PagefileUsage / 1024 / 1024));
        }
        if( GlobalMemoryStatusEx( &mse ) ) {
            info.append("totalPageFileMB", static_cast<int>(mse.ullTotalPageFile / 1024 / 1024));
            info.append("availPageFileMB", static_cast<int>(mse.ullAvailPageFile / 1024 / 1024));
            info.append("ramMB", static_cast<int>(mse.ullTotalPhys / 1024 / 1024));
        }
    }

    void ProcessInfo::getSystemInfo( BSONObjBuilder& info ) {
        if (_serverStats.isEmpty())
            // lazy load sysinfo
            collectSystemInfo();
        info.append("os", _serverStats.getField("os").Obj());
        info.append("system", _serverStats.getField("system").Obj());
    }

    void ProcessInfo::collectSystemInfo() {
        BSONObjBuilder bSI, bSys, bOS;
        stringstream verstr;
        string osName;
        OSVERSIONINFOEX osvi;
        BOOL ignored;

        // populate the os version struct
        ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ));
        osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
        ignored = GetVersionEx( (OSVERSIONINFO*)&osvi );
        osName = "Microsoft ";

        switch ( osvi.dwMajorVersion ) {
        case 6:
            switch (osvi.dwMinorVersion) {
                case 2:
                    if (osvi.wProductType == VER_NT_WORKSTATION)
                        osName += "Windows 8";
                    else
                        osName += "Windows Server 8";
                    break;
                case 1:
                    if (osvi.wProductType == VER_NT_WORKSTATION)
                        osName += "Windows 7";
                    else
                        osName += "Windows Server 2008 R2";
                    break;
                case 0:
                    if (osvi.wProductType == VER_NT_WORKSTATION)
                        osName += "Windows Vista";
                    else
                        osName += "Windows Server 2008";
                    break;
                default:
                    osName += "Windows (Unkown Version)";
                    break;
            }
            break;
        case 5:
            switch (osvi.dwMinorVersion) {
                case 2:
                    osName += "Windows Server 2003";
                    break;
                case 1:
                    osName += "Windows XP";
                    break;
                case 0:
                    if (osvi.wProductType == VER_NT_WORKSTATION)
                        osName += "Windows 2000 Professional";
                    else
                        osName += "Windows 2000 Server";
                    break;
                default:
                    osName += "Windows (Unkown Version)";
                    break;
            }
            break;
        }

        verstr << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << " (build " << osvi.dwBuildNumber << ")";
        bOS.append("type", "Windows");
        bOS.append("name", osName);
        bOS.append("version", verstr.str());
        bSys.append("memBits",  ( sizeof(int*) == 4 ? 32 : 64 ) );
        bSI.append("os", bOS.obj().copy());
        bSI.append("system", bSys.obj().copy());
        _serverStats = bSI.obj().copy();
    }

    bool ProcessInfo::processHasNumaEnabled() {
        // TODO: detect/disable NUMA; maybe read SRATs?
        return false;
    }

    bool ProcessInfo::blockCheckSupported() {
        return psapiGlobal.supported;
    }

    bool ProcessInfo::blockInMemory( char * start ) {
#if 0
        // code for printing out page fault addresses and pc's --
        // this could be useful for targetting heavy pagefault locations in the code
        static BOOL bstat = InitializeProcessForWsWatch( GetCurrentProcess() );
        PSAPI_WS_WATCH_INFORMATION_EX wiex[30];
        DWORD bufsize =  sizeof(wiex);
        bstat = GetWsChangesEx( GetCurrentProcess(), &wiex[0], &bufsize );
        if (bstat) {
            for (int i=0; i<30; i++) {
                if (wiex[i].BasicInfo.FaultingPc == 0) break;
                cout << "faulting pc = " << wiex[i].BasicInfo.FaultingPc << " address = " << wiex[i].BasicInfo.FaultingVa << " thread id = " << wiex[i].FaultingThreadId << endl;
            }
        }
#endif
        PSAPI_WORKING_SET_EX_INFORMATION wsinfo;
        wsinfo.VirtualAddress = start;
        BOOL result = psapiGlobal.QueryWSEx( GetCurrentProcess(), &wsinfo, sizeof(wsinfo) );
        if ( result )
            if ( wsinfo.VirtualAttributes.Valid )
                return true;
        return false;
    }
}
