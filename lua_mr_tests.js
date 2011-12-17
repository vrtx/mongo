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
for (var i = 0; i < 1000000; i++) {
    db.complex_test.insert( { 'id': i, 
                              'wordcloud': ['some', 'words', 'that', 'may', 'or', 'may', 'not', 'repeat', 'themselves', 'although', 'this', 'is', 'a', 'somewhat', 'contrived', 'example', 'that', 'may', 'or', 'may', 'not', 'represent', 'a', 'real', 'world', 'test', 'its', 'a', 'start', 'hey', 'what', 'do', 'you', 'know', 'there', 'are', 'lots', 'and', 'lots', 'and', 'lots', 'of', 'lots', 'of', 'lots', 'lots', 'lots', 'lots', 'lots', 'lots', 'of', 'words', 'to', 'match', 'and', 'in', 'a', 'diverse', 'order', 'that', 'may', 'or', 'may', 'not', 'repeat', 'itself', 'some', 'words', 'that', 'may', 'or', 'may', 'not', 'repeat', 'themselves', '', 'this', 'is', 'a', 'somewhat', 'contrived', '', 'that', 'may', 'or', 'may', 'not', '', 'a', 'real', 'world', 'test', 'its', 'a', 'start', 'hey', 'what', 'do', 'you', 'know', 's', 'lots', 'lots', 'lots', 'of', 'words', 'to', 'match', 'and', 'in', 'a', 'diverse', 'order', 'that', 'may', 'or', 'may', 'not', 'repeat', 'itself']
                          } );
}



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




// lua test: concatenate strings
//////////////////////////////////////////
lconcat =  "local s_concat = doc.data1 .. doc.data2;";
lconcatr = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.small_test.mapReduce(lconcat, lconcatr, {out:{replace:"lua_test"}, useLua:true });


// js test: concatenate strings
//////////////////////////////////
jconcat = function() {
    var s_concat = this.data1 + this.data2;
}
jconcatr = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.small_test.mapReduce(jconcat, jconcatr, {out:{replace:"js_test"}, useLua:false });




// lua test: 3x4 test of math constnats (testing perf of BSON glue)
//////////////////////////////////////////
lstatmath =  "local isum = 1 + 1;";
lstatmath += "local msum = 1 + 1.01;";
lstatmath += "local fsum = 1.01 + 1.02;";
 
lstatmath =  "local idiv = 1 / 1;";
lstatmath += "local mdiv = 1 / 1.01;";
lstatmath += "local fdiv = 1.01 / 1.02;";

lstatmath =  "local imlt = 1 * 1;";
lstatmath += "local mmlt = 1 * 1.01;";
lstatmath += "local fmlt = 1.01 * 1.02;";

lstatmath =  "local isub = 1 - 1;";
lstatmath += "local msub = 1 - 1.01;";
lstatmath += "local fsub = 1.01 - 1.02;";

lstatmathr = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.giant_test.mapReduce(lstatmath, lstatmathr, {out:{replace:"lua_test"}, useLua:true });
