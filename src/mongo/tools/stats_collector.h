#pragma once

#include <stdio.h>

#include "mongo/base/initializer.h"
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
    mongo::string message = m.toString();
    int32_t msgid = 0;
    switch (m.operation()) {
      case mongo::opReply:
        {
          mongo::DbMessage dbm(m);
          msgid = m.header()->responseTo.get();
          uint64_t queryTime = mongo::curTimeMicros() - opTimeMap[msgid];
          logQuery(collections[msgid].rawData(), queryTime);
          opTimeMap.erase(msgid);
          collections.erase(msgid);
        }
        break;
      case mongo::dbQuery:
        {
          mongo::DbMessage dbm(m);
          msgid = m.header()->id.get();
          mongo::StringData ns = dbm.getns();
          opTimeMap[msgid] = mongo::curTimeMicros();
          collections[msgid] = ns;
          // todo: will leak if cursor is not exhausted.  handle killcursor and socket disconnect.
        }
        break;
      case mongo::dbGetMore:
        break;
      case mongo::dbKillCursors:
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

private:
  typedef mongo::unordered_map<int32_t, uint64_t> OpTimeMap;
  typedef mongo::unordered_map<int32_t, mongo::StringData> CollectionMap;
  OpTimeMap opTimeMap;
  CollectionMap collections;
  FILE* reportFile;
  static StatsCollector* singleton; // not thread safe

  void logQuery(const char* ns, uint64_t time_micros) {
    // log format is:
    // <namespace> <optime>\n
    if (fprintf(reportFile, "%s %llu\n", ns, time_micros) < 0)
      printf("error writing to file: %s\n", strerror(errno));
    fflush(reportFile); // todo: remove once signal handlers are installed
  }

};

StatsCollector* StatsCollector::singleton = NULL;
