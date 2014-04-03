var conn = new ShardingTest({
    shards: 2,
    mongos: 1,
	nopreallocj : true
});
var db = conn.getDB('counters_sharded');
var res = db.runCommand({getCounter:1});

// TODO: 
assert.eq(res.type, "mongos");

