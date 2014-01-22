#pragma once

#include <stdio.h>
#include <cmath>
#include <algorithm>

#include "mongo/base/initializer.h"
#include "mongo/base/string_data.h"
#include "mongo/bson/util/builder.h"
#include "mongo/client/dbclientinterface.h"
#include "mongo/db/dbmessage.h"
#include "mongo/util/net/message.h"
#include "mongo/util/mmap.h"
#include "mongo/util/text.h"
#include "mongo/util/time_support.h"
#include "mongo/platform/unordered_map.h"

class StatsCollector {
public:

  static StatsCollector* initSingleton(mongo::StringData reportFilePath) {
    if (singleton == NULL) {
      singleton = new StatsCollector();
      singleton->reportFile = fopen(reportFilePath.rawData(), "w");
    }
    return singleton;
  }

  ~StatsCollector() {
    fclose(reportFile);
  }

  void recordMessage(char* host, int port, mongo::Message& m) {
    switch (m.operation()) {
      case mongo::opReply:
        if (m.header()->responseTo.get() > 0 && m.header()->getCursor() == 0)
          // log when reply indicates results have been exhausted
          logResponse(m.header()->responseTo.get(), host, port);
        break;
      case mongo::dbQuery:
        // log start time
        logQuery(m, host, port);
        break;
      case mongo::dbGetMore:
        break;
      case mongo::dbKillCursors:
        // mark the query as finished
        logResponse(m.header()->responseTo.get(), host, port);
        break;
      default:
        // unhandled operation
        break;
    }
  }

  // todo: hook up to signal handlers (and check file state)
  void done() {
    fflush(reportFile);
  }

  void aggregateResults() {
    printf("aggregating results.\n");
    for (QueryMap::const_iterator i = queryStats.begin(); i != queryStats.end(); ++i) {
      mongo::StringData key(i->second.signature);
      AggResults &queryAgg = aggResults[key];
      ++queryAgg.count;
      queryAgg.totalTime += i->second.totalTime;
    }
  }

  void writeResults() {
    // format is:
    // count \t total \t querySignature
    // where count is the number of repeated queries, and totalTime is the sum of all response times
    printf("writing results.\n");
    fseek(reportFile, 0, SEEK_SET); // overwrite any previously written results
    for (ResultsMap::const_iterator i = aggResults.begin(); i != aggResults.end(); ++i) {
      if (fprintf(reportFile, "%u\t%u\t%s\n", i->second.count, i->second.totalTime, i->first.rawData()) < 0) {
        printf("error writing to file: %s\n", strerror(errno));
        break;
      }
    }

    // generate a summary of queries outside of the accepted range
    if (fprintf(reportFile, "==== Queries with delta exceeding threshold: ====\n") < 0) {
      printf("error writing to file: %s\n", strerror(errno));
    }
    mongo::string lastHost;
    mongo::StringData lastHash;
    int lastPort = 0;
    int32_t lastTotalTime = 0;
    uint32_t lastCount = 0;
    int32_t totalOverThreshold = 0;
    for (ResultsMap::const_iterator i = aggResults.begin(); i != aggResults.end(); ++i) {
      mongo::StringData prefix = i->first.substr(0, i->first.find(" || ("));
      if (lastHash.startsWith(prefix)) {
        // last query matches current query
        if (std::abs(lastTotalTime - i->second.totalTime) > std::min(lastTotalTime, i->second.totalTime) * 0.03 &&
            std::abs(lastTotalTime - i->second.totalTime) >= 2) {
          // delta is > 3% (and at least 2 microseconds)
          ++totalOverThreshold;
          if (fprintf(reportFile, "q1 time/count: %d/%d\t q2 time/count: %d/%d\t query: %s\n",
                                  lastTotalTime, lastCount,
                                  i->second.totalTime, i->second.count,
                                  i->first.rawData()) < 0) {
            printf("error writing to file: %s\n", strerror(errno));
            break;
          }
        }
      }
      lastHash = prefix;
      lastCount = i->second.count;
      lastTotalTime = i->second.totalTime;
      lastHost = i->second.host;
      lastPort = i->second.port;
    }

    if (fprintf(reportFile, "\n ==\n Number of queries over threshold: %d / %lu\n", totalOverThreshold, aggResults.size() / 2)) {
      printf("error writing to file: %s\n", strerror(errno));
    }
    fflush(reportFile);
  }

  void reset() {
    // queryStats.clear(); // don't clear; required for aggregated result key
    aggResults.clear(); // never clear the aggregated results
  }

private:
  struct QueryStat {
    mongo::string signature;
    mongo::string host;
    int port;
    uint64_t startTime;
    uint64_t totalTime;
  };
  struct AggResults {
    int32_t totalTime;
    uint32_t count;
    mongo::string host;
    int port;
  };
  typedef mongo::unordered_map<mongo::string, QueryStat> QueryMap;
  // typedef mongo::unordered_map<mongo::StringData, AggResults, mongo::StringData::Hasher> ResultsMap;
  typedef mongo::map<mongo::StringData, AggResults> ResultsMap;
  QueryMap queryStats;
  ResultsMap aggResults;
  FILE* reportFile;
  static StatsCollector* singleton; // not thread safe

  void logQuery(mongo::Message &m, char* host, int port) {
    QueryStat stat;
    stat.startTime = mongo::curTimeMicros64();
    stat.totalTime = 0;

    // get the query and projection(s?) from the message
    mongo::DbMessage dbm(m);
    dbm.pullInt();  // skip nToSkip
    dbm.pullInt();  // skip nToRetrun
    mongo::stringstream s;
    s << dbm.nextJsObj().toString(false, true);
    while (dbm.moreJSObjs())
      s << "\t" + dbm.nextJsObj().toString(false, true);
    s << " || (" << host << ":" << port << ")";
    stat.host = host;
    stat.port = port;
    stat.signature = s.str();
    queryStats[generateUniqueMessageId(m.header()->id.get(), host, port)] = stat;
  }
  mongo::string generateUniqueMessageId(int32_t msgid, char* host, int port) {
    mongo::stringstream s;
    s << host << port << msgid;
    return s.str();
  }

  void logResponse(int32_t msgid, char* host, int port) {
    uint64_t now = mongo::curTimeMicros64();
    QueryStat &stat = queryStats[generateUniqueMessageId(msgid, host, port)];
    stat.totalTime = now - stat.startTime;
  }

};

StatsCollector* StatsCollector::singleton = NULL;
