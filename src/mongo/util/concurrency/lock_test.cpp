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
#include "mongo/util/timer.h"

using namespace mongo;

namespace {

	const int NLocks = 1000000;

	// Uncontended acquisition and relinquish latency: time how long it takes
	// to lock and unlock N locks.
	TEST(MutexLocks, UncontenededSingleThreadLatency) {
		SpinLock spinLocks[NLocks];
		vector<SimpleMutex*> mutexLocks;
		for (int i = 0; i < NLocks; ++i) {
			mutexLocks.push_back(new SimpleMutex("dummy"));
		}
		{
			ScopedProbe p("UncontendedSpinLockAcquisitionLatency");
			for (int i = 0; i < NLocks; ++i) {
				spinLocks[i].lock();
			}
		}

		{
			ScopedProbe p("UncontendedMutexAcquisitionLatency");
			for (int i = 0; i < NLocks; ++i) {
				mutexLocks[i]->lock();
			}
		}

		{
			ScopedProbe p("UncontendedSpinLockReleaseLatency");
			for (int i = 0; i < NLocks; ++i) {
				spinLocks[i].unlock();
			}
		}

		{
			ScopedProbe p("UncontendedMutexReleaseLatency");
			for (int i = 0; i < NLocks; ++i) {
				mutexLocks[i]->unlock();
			}
		}

		for (int i = 0; i < NLocks; ++i) {
			delete mutexLocks.back();
		}
		mutexLocks.clear();
	}

}