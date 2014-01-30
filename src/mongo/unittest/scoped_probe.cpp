#include "mongo/unittest/scoped_probe.h"

namespace mongo {

    namespace unittest {

        PCM* ScopedProbe::pcmInstance = PCM::getInstance();

        struct StaticPCMInit {
            static StaticPCMInit* initOrDie() {
                PCM::ErrorCode status = ScopedProbe::pcmInstance->program();

                switch (status)
                {
                    case PCM::Success:
                        break;
                    case PCM::MSRAccessDenied:
                        cout << "Access to performance counter monitor denied." << endl;
                        exit(0);
                    case PCM::PMUBusy:
                        cout << "Performance counter monitor wasy busy.  Rerun the unit test." << endl;
                        ScopedProbe::pcmInstance->resetPMU();
                        exit(0);
                    default:
                        cout << "Unknown error while trying to initialize PCM module." << endl;
                        exit(0);
                }
                return new StaticPCMInit();
            }
        };

        StaticPCMInit* staticInit = StaticPCMInit::initOrDie();

    }
}