/**
 *    Copyright (C) 2013 10gen Inc.
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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include <boost/thread/thread.hpp>
#include <vector>

#include "mongo/unittest/scoped_probe.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/concurrency/mutex.h"
#include "mongo/util/concurrency/spin_lock.h"
#include "mongo/util/processinfo.h"
#include "mongo/util/timer.h"

using namespace mongo;

namespace {

  // Lock profling tests

  const int kNumSingleLocks = 1000000;  // Number of locks for single thread tests
  const int kNumMultiLocks = 100;       // Number of locks for multithreaded tests
  const int kMillisToWork = 1000;       // Milliseconds to work for throughput tests
  const int kNumWaitingLocks = 1000;    // Number of locks to wait on
  const int kNumThreadsPerLock = 5;     // Number of threads per lock

  ProcessInfo proc;

  // Fixture for locking/unlocking in a single thread
  class SingleThreadLockLatency : public unittest::Test {
  public:
    SpinLock spinLocks[kNumSingleLocks];
    vector<SimpleMutex*> mutexLocks;

  protected:
    void setUp() {
      for (int i = 0; i < kNumSingleLocks; ++i) {
        mutexLocks.push_back(new SimpleMutex("dummy"));
      }
    }

    void tearDown() {
      for (int i = 0; i < kNumSingleLocks; ++i) {
        delete mutexLocks[i];
      }
      mutexLocks.clear();
    }
  };

  // #1 and #2: Uncontended acquisition and relinquish latency: time how long it takes
  // to lock and unlock N locks.
  TEST_F(SingleThreadLockLatency, Unconteneded) {
    {
      ScopedProbe p("UncontendedSpinLockAcquisitionLatency");
      for (int i = 0; i < kNumSingleLocks; ++i) {
        spinLocks[i].lock();
      }
      p.done();
      p.setMetric("numThreads", 1);
    }

    {
      ScopedProbe p("UncontendedMutexAcquisitionLatency");
      for (int i = 0; i < kNumSingleLocks; ++i) {
        mutexLocks[i]->lock();
      }
      p.done();
      p.setMetric("numThreads", 1);
    }

    {
      ScopedProbe p("UncontendedSpinLockReleaseLatency");
      for (int i = 0; i < kNumSingleLocks; ++i) {
        spinLocks[i].unlock();
      }
      p.done();
      p.setMetric("numThreads", 1);
    }

    {
      ScopedProbe p("UncontendedMutexReleaseLatency");
      for (int i = 0; i < kNumSingleLocks; ++i) {
        mutexLocks[i]->unlock();
      }
      p.done();
      p.setMetric("numThreads", 1);
    }
  }

  // #3: Uncontended SimpleMutex throughput – one thread locks and unlocks the same mutex
  // repeatedly, and measures how many can be performed per unit time.
  TEST(SingleUncontendedLockThroughput, SimpleMutex) {
    int lockCount = 0;
    SimpleMutex m("");
    Timer t;
    {
      ScopedProbe p("UncontendedSimpleMutexThroughput");
      while (t.millis() <= kMillisToWork) {
        m.lock();
        ++lockCount;
        m.unlock();
      }
      p.done();
      p.setMetric("lockIters", lockCount);
      p.setMetric("numThreads", 1);
    }
  }

  // #3: Uncontended spin lock throughput – one thread locks and unlocks the same mutex
  // repeatedly, and measures how many can be performed per unit time.
  TEST(SingleUncontendedLockThroughput, SpinLock) {
    int lockCount = 0;
    Timer t;
    SpinLock l;
    {
      ScopedProbe p("UncontendedSpinLockThroughput");
      while (t.millis() <= kMillisToWork) {
        l.lock();
        ++lockCount;
        l.unlock();
      }
      p.done();
      p.setMetric("lockIters", lockCount);
      p.setMetric("numThreads", 1);
    }
  }

  // helpers
  class LockWrapper {
  public:
    virtual ~LockWrapper() { };
    virtual void lock() = 0;
    virtual void unlock() = 0;
  };

  class SpinLockWrapper : public LockWrapper {
  public:
    SpinLockWrapper() : l() { }
    void lock() { l.lock(); }
    void unlock() { l.unlock(); }
  private:
    SpinLock l;
  };

  class SimpleMutexWrapper : public LockWrapper {
  public:
    SimpleMutexWrapper() : l("") { }
    void lock() { l.lock(); }
    void unlock() { l.unlock(); }
  private:
    SimpleMutex l;
  };

  class SingleLockWorker {
  public:
    void doWork(LockWrapper* l, AtomicUInt* totalCount) {
      Timer t;
      int lockCount = 0;
      while (t.millis() <= kMillisToWork) {
        l->lock();
        ++lockCount;
        l->unlock();
      }
      totalCount->signedAdd(lockCount);
    }
  };

  // #4: Contended throughput: one thread per core fighting for a single spin lock
  TEST(SingleContendedLockThroughput, SpinLock) {
    SpinLockWrapper l;
    SingleLockWorker w;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    {
      ScopedProbe p("ContendedSpinLockThroughput");
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&SingleLockWorker::doWork,
                                                        &w,
                                                        &l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numThreads", proc.getNumCores());
    }
  }

  // #4: Contended throughput: one thread per core fighting for a single mutex
  TEST(SingleContendedLockThroughput, SimpleMutex) {
    SimpleMutexWrapper l;
    SingleLockWorker w;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    {
      ScopedProbe p("ContendedSimpleMutexThroughput");
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&SingleLockWorker::doWork,
                                                        &w,
                                                        &l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numThreads", proc.getNumCores());
    }
  }

  class MultiLockWorker {
  public:
    void doWork(LockWrapper* l[], AtomicUInt* totalCount) {
      Timer t;
      int lockCount = 0;
      while (t.millis() <= kMillisToWork) {
        for (int i = 0; i < kNumMultiLocks; ++i) {
          l[i]->lock();
          ++lockCount;
          l[i]->unlock();
        }
      }
      totalCount->signedAdd(lockCount);
    }
  };

  // #5: Contended throughput: one thread per core, fighting over several mutexes
  TEST(MultipleContendedLocksThroughput, SimpleMutex) {
    MultiLockWorker mw;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumMultiLocks];
    for (int i = 0; i < kNumMultiLocks; ++i)
      l[i] = new SimpleMutexWrapper();
    {
      ScopedProbe p("MultipleContendedSimpleMutexThroughput");
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&MultiLockWorker::doWork,
                                                        &mw,
                                                        l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numThreads", proc.getNumCores());
    }
    for (int i = 0; i < kNumMultiLocks; ++i)
      delete l[i];
  }

  // #5: Contended throughput: one thread per core, fighting over several spin locks
  TEST(MultipleContendedLocksThroughput, SpinLock) {
    MultiLockWorker mw;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumMultiLocks];
    for (int i = 0; i < kNumMultiLocks; ++i)
      l[i] = new SpinLockWrapper();
    {
      ScopedProbe p("MultipleContendedSpinLockThroughput");
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&MultiLockWorker::doWork,
                                                        &mw,
                                                        l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numThreads", proc.getNumCores());
    }
    for (int i = 0; i < kNumMultiLocks; ++i)
      delete l[i];
  }

  // #6: Contended mutex throughput: one thread per core + one thread per mutex, as above.
  TEST(MultipleContendedLocksPlusThreadPerLockThroughput, SimpleMutex) {
    MultiLockWorker mw;
    SingleLockWorker w;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumMultiLocks];
    for (int i = 0; i < kNumMultiLocks; ++i)
      l[i] = new SimpleMutexWrapper();
    {
      ScopedProbe p("MultipleContendedSimpleMutexesPlusThreadPerMutexThroughput");
      // create one thread per lock; constantly locking and unlocking
      for (unsigned i = 0; i < kNumMultiLocks; ++i) {
        threads.push_back(new boost::thread(boost::bind(&SingleLockWorker::doWork,
                                                        &w,
                                                        l[i],
                                                        &totalLockCount)));
      }
      // create a thread per core fighting over multiple locks
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&MultiLockWorker::doWork,
                                                        &mw,
                                                        l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numLocks", kNumMultiLocks);
      p.setMetric("numThreads", threads.size());
    }
    for (int i = 0; i < kNumMultiLocks; ++i)
      delete l[i];
  }

  // #6: Contended mutex throughput: one thread per core + one thread per mutex, as above.
  TEST(MultipleContendedLocksPlusThreadPerLockThroughput, SpinLock) {
    MultiLockWorker mw;
    SingleLockWorker w;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumMultiLocks];
    for (int i = 0; i < kNumMultiLocks; ++i)
      l[i] = new SpinLockWrapper();
    {
      ScopedProbe p("MultipleContendedSpinLocksPlusThreadPerLockThroughput");
      // create one thread per lock; constantly locking and unlocking
      for (unsigned i = 0; i < kNumMultiLocks; ++i) {
        threads.push_back(new boost::thread(boost::bind(&SingleLockWorker::doWork,
                                                        &w,
                                                        l[i],
                                                        &totalLockCount)));
      }
      // create a thread per core fighting over multiple locks
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&MultiLockWorker::doWork,
                                                        &mw,
                                                        l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numLocks", kNumMultiLocks);
      p.setMetric("numThreads", threads.size());
    }
    for (int i = 0; i < kNumMultiLocks; ++i)
      delete l[i];
  }

  // #7: Contended mutex throughput: one thread per core + multiple threads per mutex, as above.
  TEST(ThreadPerCorePlusMultiThreadsPerLockThroughput, SimpleMutex) {
    MultiLockWorker mw;
    SingleLockWorker w;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumMultiLocks];
    for (int i = 0; i < kNumMultiLocks; ++i)
      l[i] = new SimpleMutexWrapper();
    {
      ScopedProbe p("ThreadPerCorePlusMultiThreadsPerMutexThroughput");
      // create multiple threads per lock; constantly locking and unlocking
      for (int i = 0; i < kNumThreadsPerLock; ++i) {
        for (int j = 0; j < kNumMultiLocks; ++j) {
          threads.push_back(new boost::thread(boost::bind(&SingleLockWorker::doWork,
                                                          &w,
                                                          l[j],
                                                          &totalLockCount)));
        }
      }
      // create a thread per core fighting over multiple locks
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&MultiLockWorker::doWork,
                                                        &mw,
                                                        l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numLocks", kNumMultiLocks);
      p.setMetric("numThreads", threads.size());
    }
    for (int i = 0; i < kNumMultiLocks; ++i)
      delete l[i];
  }

  // #7: Contended lock throughput: one thread per core + multiple threads per spin lock, as above.
  TEST(ThreadPerCorePlusMultiThreadsPerLockThroughput, SpinLock) {
    MultiLockWorker mw;
    SingleLockWorker w;
    AtomicUInt totalLockCount;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumMultiLocks];
    for (int i = 0; i < kNumMultiLocks; ++i)
      l[i] = new SpinLockWrapper();
    {
      ScopedProbe p("ThreadPerSpinLockPlusMultiThreadsPerSpinLockThroughput");
      // create multiple threads per lock; constantly locking and unlocking
      for (int i = 0; i < kNumThreadsPerLock; ++i) {
        for (int j = 0; j < kNumMultiLocks; ++j) {
          threads.push_back(new boost::thread(boost::bind(&SingleLockWorker::doWork,
                                                          &w,
                                                          l[j],
                                                          &totalLockCount)));
        }
      }
      // create a thread per core fighting over multiple locks
      for (unsigned i = 0; i < proc.getNumCores(); ++i) {
        threads.push_back(new boost::thread(boost::bind(&MultiLockWorker::doWork,
                                                        &mw,
                                                        l,
                                                        &totalLockCount)));
      }
      for (vector<boost::thread*>::iterator it = threads.begin();
           it != threads.end();
           ++it) {
        (*it)->join();
        delete *it;
      }
      p.done();
      p.setMetric("lockIters", totalLockCount);
      p.setMetric("numCores", proc.getNumCores());
      p.setMetric("numLocks", kNumMultiLocks);
      p.setMetric("numThreads", threads.size());
    }
    for (int i = 0; i < kNumMultiLocks; ++i)
      delete l[i];
  }

  class BlockedWorker {
  public:
    void doWork(LockWrapper* l) {
      l->lock();
      l->unlock();
    }
  };

  class BlockingWorker {
  public:
    void doWork(LockWrapper* l[], string* lockType) {
      for (int i = 0; i < kNumWaitingLocks; ++i)
        l[i]->lock();

      // todo: is this enough to ensure all other threads have started?
      sleep(1);

      {
        ScopedProbe p(std::string("ContendedUnlockLatency").append(*lockType));
        for (int i = 0; i < kNumWaitingLocks; ++i)
          l[i]->unlock();
        p.done();
        p.setMetric("numCores", proc.getNumCores());
        p.setMetric("numLocks", kNumWaitingLocks);
        p.setMetric("numThreads", kNumWaitingLocks+1);
      }
    }
  };

  // #8: Contended unlock latency: time to unlock N mutexes that other threads are blocked on
  TEST(ContendedUnlockLatency, SimpleMutex) {
    BlockedWorker blockedWorker;
    BlockingWorker blockingWorker;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumWaitingLocks];
    std::string lockType = "SimpleMutex";
    for (int i = 0; i < kNumWaitingLocks; ++i)
      l[i] = new SimpleMutexWrapper();

    // start the thread which acquires all locks, waits, then unlocks each one
    threads.push_back(new boost::thread(boost::bind(&BlockingWorker::doWork,
                                                    &blockingWorker,
                                                    l,
                                                    &lockType)));

    // start one thread per lock
    for (int i = 0; i < kNumWaitingLocks; ++i) {
      threads.push_back(new boost::thread(boost::bind(&BlockedWorker::doWork,
                                                      &blockedWorker,
                                                      l[i])));
    }

    for (vector<boost::thread*>::iterator it = threads.begin();
         it != threads.end();
         ++it) {
      (*it)->join();
      delete *it;
    }

    for (int i = 0; i < kNumWaitingLocks; ++i)
      delete l[i];
  }

  // #8: Contended spinlock unlock latency: time to unlock N mutexes that other threads are blocked on
  TEST(ContendedUnlockLatency, SpinLock) {
    BlockedWorker blockedWorker;
    BlockingWorker blockingWorker;
    vector<boost::thread*> threads;
    LockWrapper* l[kNumWaitingLocks];
    std::string lockType = "SpinLock";
    for (int i = 0; i < kNumWaitingLocks; ++i)
      l[i] = new SpinLockWrapper();

    // start the thread which acquires all locks, waits, then unlocks each one
    threads.push_back(new boost::thread(boost::bind(&BlockingWorker::doWork,
                                                    &blockingWorker,
                                                    l,
                                                    &lockType)));

    // start one thread per lock
    for (int i = 0; i < kNumWaitingLocks; ++i) {
      threads.push_back(new boost::thread(boost::bind(&BlockedWorker::doWork,
                                                      &blockedWorker,
                                                      l[i])));
    }

    for (vector<boost::thread*>::iterator it = threads.begin();
         it != threads.end();
         ++it) {
      (*it)->join();
      delete *it;
    }

    for (int i = 0; i < kNumWaitingLocks; ++i)
      delete l[i];
  }

} // namespace
