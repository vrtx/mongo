// cook some data
// use test_giant
// for (var i = 0; i < 100; i++) {
//     db.small_test.insert({'int1':i, 
//                      'float1':i/2, 
//                      'data1':'a fairly small string; like a title', 
//                      'data2':'a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article. '
//                      });
// }
// for (var i = 0; i < 1000000; i++) {
//     db.large_test.insert({'int1':i, 
//                      'float1':i/2, 
//                      'data1':'a fairly small string; like a title', 
//                      'data2':'a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article. '
//                      });
// }



// lua test: print out the values from the bson object
//////////////////////////////////////////////////////
m =  " print('LUA MapReduce:  JIT Status: ', jit.status()); ";
m += " print('LUA MapReduce:  map function got document: '); ";
m += " for k,v in pairs(doc) do ";
m += "    print('mapping property: ', k, '=', v); ";
m += " end ";

r = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.small_test.mapReduce(m, r, {out:{replace:"test_merged"}, useLua:true});



// lua test: 3x4 test of basic math
//////////////////////////////////////////
lmath =  "local isum = doc.int1 + doc.int1;";
lmath += "local msum = doc.int1 + doc.float1;";
lmath += "local fsum = doc.float1 + doc.float1;";

lmath += "local idiv = doc.int1 / doc.int1;";
lmath += "local mdiv = doc.int1 / doc.float1;";
lmath += "local fdiv = doc.float1 / doc.float1;";

lmath += "local imlt = doc.int1 * doc.int1;";
lmath += "local mmlt = doc.int1 * doc.float1;";
lmath += "local fmlt = doc.float1 * doc.float1;";

lmath += "local isub = doc.int1 - doc.int1;";
lmath += "local msub = doc.int1 - doc.float1;";
lmath += "local fsub = doc.float1 - doc.float1;";

lmathr = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.giant_test.mapReduce(lmath, lmathr, {out:{replace:"lua_test"}, useLua:true });



// js test: 3x4 test of basic math
//////////////////////////////////
jmath = function() {
    var isum = this.int1 + this.int1;
    var msum = this.int1 + this.float1;
    var fsum = this.float1 + this.float1;

    var idiv = this.int1 / this.int1;
    var mdiv = this.int1 / this.float1;
    var fdiv = this.float1 / this.float1;

    var imlt = this.int1 * this.int1;
    var mmlt = this.int1 * this.float1;
    var fmlt = this.float1 * this.float1;

    var isub = this.int1 - this.int1;
    var msub = this.int1 - this.float1;
    var fsub = this.float1 - this.float1;
}
jmathr = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.giant_test.mapReduce(jmath, jmathr, {out:{replace:"js_test"}, useLua:false });




// lua test: 3x4 test of math constnats (testing perf of BSON glue)
//////////////////////////////////////////
lmath =  "local isum = 1 + 1;";
lmath += "local msum = 1 + 1.01;";
lmath += "local fsum = 1.01 + 1.02;";

lmath =  "local idiv = 1 / 1;";
lmath += "local mdiv = 1 / 1.01;";
lmath += "local fdiv = 1.01 / 1.02;";

lmath =  "local imlt = 1 * 1;";
lmath += "local mmlt = 1 * 1.01;";
lmath += "local fmlt = 1.01 * 1.02;";

lmath =  "local isub = 1 - 1;";
lmath += "local msub = 1 - 1.01;";
lmath += "local fsub = 1.01 - 1.02;";

lmathr = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.giant_test.mapReduce(lmath, lmathr, {out:{replace:"lua_test"}, useLua:true });





