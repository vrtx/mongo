#pragma once

#include <stdio.h>

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
          logResponse(m.header()->responseTo.get());
        break;
      case mongo::dbQuery:
        // log start time
        logQuery(m, host, port);
        break;
      case mongo::dbGetMore:
        break;
      case mongo::dbKillCursors:
        // mark the query as finished
        logResponse(m.header()->responseTo.get());
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
    fflush(reportFile);
  }

  void reset() {
    // queryStats.clear(); // don't clear; required for aggregated result key
    aggResults.clear(); // never clear the aggregated results
  }

private:
  struct QueryStat {
    mongo::string signature;
    uint64_t startTime;
    uint64_t totalTime;
  };
  struct AggResults {
    uint32_t totalTime;
    uint32_t count;
  };
  typedef mongo::unordered_map<int32_t, QueryStat> QueryMap;
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
    s << dbm.nextJsObj().toString(false, true) << " (" << host << ":" << port << ")";
    stat.signature = s.str();
    while (dbm.moreJSObjs())
      stat.signature += "\t" + dbm.nextJsObj().toString(false, true);

    queryStats[m.header()->id.get()] = stat;
  }

  void logResponse(int32_t msgid) {
    uint64_t now = mongo::curTimeMicros64();
    QueryStat &stat = queryStats[msgid];
    stat.totalTime = now - stat.startTime;
  }

};

StatsCollector* StatsCollector::singleton = NULL;
