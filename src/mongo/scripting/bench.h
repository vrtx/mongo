/*
 *    Copyright (C) 2010 10gen Inc.
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

#pragma once

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <pcrecpp.h>

#include "mongo/bson/util/atomic_int.h"
#include "mongo/client/dbclientinterface.h"
#include "mongo/db/jsobj.h"
#include "mongo/util/timer.h"

namespace mongo {

    /**
     * Configuration object describing a bench run activity.
     */
    class BenchRunConfig : private boost::noncopyable {
    public:

        /**
         * Create a new BenchRunConfig object, and initialize it from the BSON
         * document, "args".
         *
         * Caller owns the returned object, and is responsible for its deletion.
         */
        static BenchRunConfig *createFromBson( const BSONObj &args );

        BenchRunConfig();

        void initializeFromBson( const BSONObj &args );

        // Create a new connection to the mongo instance specified by this configuration.
        DBClientBase *createConnection() const;

        /**
         * Connection std::string describing the host to which to connect.
         */
        std::string host;

        /**
         * Name of the database on which to operate.
         */
        std::string db;

        /**
         * Optional username for authenticating to the database.
         */
        std::string username;

        /**
         * Optional password for authenticating to the database.
         *
         * Only useful if username is non-empty.
         */
        std::string password;

        /**
         * Number of parallel threads to perform the bench run activity.
         */
        unsigned parallel;

        /**
         * Desired duration of the bench run activity, in seconds.
         *
         * NOTE: Only used by the javascript benchRun() and benchRunSync() functions.
         */
        double seconds;

        bool hideResults;
        bool handleErrors;
        bool hideErrors;

        boost::shared_ptr< pcrecpp::RE > trapPattern;
        boost::shared_ptr< pcrecpp::RE > noTrapPattern;
        boost::shared_ptr< pcrecpp::RE > watchPattern;
        boost::shared_ptr< pcrecpp::RE > noWatchPattern;

        /**
         * Operation description.  A BSON array of objects, each describing a single
         * operation.
         *
         * Every thread in a benchRun job will perform these operations in sequence, restarting at
         * the beginning when the end is reached, until the job is stopped.
         *
         * TODO: Document the operation objects.
         *
         * TODO: Introduce support for performing each operation exactly N times.
         */
        BSONObj ops;

        bool throwGLE;
        bool breakOnTrap;

    private:
        /// Initialize a config object to its default values.
        void initializeToDefaults();
    };

    /**
     * Statistics object representing the result of a bench run activity.
     */
    class BenchRunStats : private boost::noncopyable {
    public:
        BenchRunStats();
        ~BenchRunStats();

        void reset();

        void updateFrom( const BenchRunStats &other );

        bool error;
        unsigned long long errCount;

        uint64_t findOneCounter;
        uint64_t updateCounter;
        uint64_t insertCounter;
        uint64_t deleteCounter;
        uint64_t queryCounter;
        uint64_t commandCounter;
        uint64_t gleCounter;

        std::map<std::string, long long> opcounters;
        std::vector<BSONObj> trappedErrors;
        int64_t threadTimeMicros;
    };

    /**
     * State of a BenchRun activity.
     *
     * Logically, the states are "starting up", "running" and "finished."
     */
    class BenchRunState : private boost::noncopyable {
    public:
        enum State { BRS_STARTING_UP, BRS_RUNNING, BRS_FINISHED };

        explicit BenchRunState(unsigned numWorkers);
        ~BenchRunState();

        //
        // Functions called by the job-controlling thread, through an instance of BenchRunner.
        //

        /**
         * Block until the current state is "awaitedState."
         *
         * massert() (uassert()?) if "awaitedState" is unreachable from
         * the current state.
         */
        void waitForState(State awaitedState);

        /**
         * Notify the worker threads to wrap up.  Does not block.
         */
        void tellWorkersToFinish();

        /// Check that the current state is BRS_FINISHED.
        void assertFinished();

        //
        // Functions called by the worker threads, through instances of BenchRunWorker.
        //

        /**
         * Predicate that workers call to see if they should finish (as a result of a call
         * to tellWorkersToFinish()).
         */
        bool shouldWorkerFinish();

        /**
         * Called by each BenchRunWorker from within its thread context, immediately before it
         * starts sending requests to the configured mongo instance.
         */
        void onWorkerStarted();

        /**
         * Called by each BenchRunWorker from within its thread context, shortly after it finishes
         * sending requests to the configured mongo instance.
         */
        void onWorkerFinished();

    private:
        boost::mutex _mutex;
        boost::condition _stateChangeCondition;
        unsigned _numUnstartedWorkers;
        unsigned _numActiveWorkers;
        AtomicUInt _isShuttingDown;
    };

    /**
     * A single worker in the bench run activity.
     *
     * Represents the behavior of one thread working in a bench run activity.
     */
    class BenchRunWorker : private boost::noncopyable {
    public:

        /**
         * Create a new worker, performing one thread's worth of the activity described in
         * "config", and part of the larger activity with state "brState".  Both "config"
         * and "brState" must exist for the life of this object.
         *
         * "id" is a positive integer which should uniquely identify the worker.
         */
        BenchRunWorker(size_t id, const BenchRunConfig *config, BenchRunState *brState);
        ~BenchRunWorker();

        /**
         * Start performing the "work" behavior in a new thread.
         */
        void start();

        /**
         * Get the run statistics for a worker.
         *
         * Should only be observed _after_ the worker has signaled its completion by calling
         * onWorkerFinished() on the BenchRunState passed into its constructor.
         */
        const BenchRunStats &stats() const { return _stats; }

    private:
        /// The main method of the worker, executed inside the thread launched by start().
        void run();

        /// The function that actually sets about generating the load described in "_config".
        void generateLoadOnConnection( DBClientBase *conn );

        /// Predicate, used to decide whether or not it's time to terminate the worker.
        bool shouldStop() const;

        size_t _id;
        const BenchRunConfig *_config;
        BenchRunState *_brState;
        BenchRunStats _stats;
    };

    /**
     * Object representing a "bench run" activity.
     */
    class BenchRunner : private boost::noncopyable {
    public:
        /**
         * Utility method to create a new bench runner from a BSONObj representation
         * of a configuration.
         *
         * TODO: This is only really for the use of the javascript benchRun() methods,
         * and should probably move out of the BenchRunner class.
         */
        static BenchRunner* createWithConfig( const BSONObj &configArgs );

        /**
         * Look up a bench runner object by OID.
         *
         * TODO: Same todo as for "createWithConfig".
         */
        static BenchRunner* get( OID oid );

        /**
         * Stop a running "runner", and return a BSON representation of its resultant
         * BenchRunStats.
         *
         * TODO: Same as for "createWithConfig".
         */
        static BSONObj finish( BenchRunner* runner );

        /**
         * Create a new bench runner, to perform the activity described by "*config."
         *
         * Takes ownership of "config", and will delete it.
         */
        explicit BenchRunner( BenchRunConfig *config );
        ~BenchRunner();

        /**
         * Start the activity.  Only call once per instance of BenchRunner.
         */
        void start();

        /**
         * Stop the activity.  Block until the activitiy has stopped.
         */
        void stop();

        /**
         * Store the collected event data from a completed bench run activity into "stats."
         *
         * Illegal to call until after stop() returns.
         */
        void populateStats(BenchRunStats *stats);

        OID oid() const { return _oid; }

        const BenchRunConfig &config() const { return *_config; } // TODO: Remove this function.

        // JS bindings
        static BSONObj benchFinish(const BSONObj& argsFake, void* data);
        static BSONObj benchStart(const BSONObj& argsFake, void* data);
        static BSONObj benchRunSync(const BSONObj& argsFake, void* data);

    private:
        // TODO: Same as for createWithConfig.
        static boost::mutex _staticMutex;
        static std::map< OID, BenchRunner* > _activeRuns;

        OID _oid;
        BenchRunState _brState;
        boost::scoped_ptr<BenchRunConfig> _config;
        std::vector<BenchRunWorker *> _workers;

        BSONObj before;
        BSONObj after;
    };

}  // namespace mongo
