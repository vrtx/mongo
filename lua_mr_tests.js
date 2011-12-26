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

// for (var i = 0; i < 1000000; i++) {
//     db.complex_test.insert( { 'id': i, 
//                               'wordcloud': ['some', 'words', 'that', 'may', 'or', 'may', 'not', 'repeat', 'themselves', 'although', 'this', 'is', 'a', 'somewhat', 'contrived', 'example', 'that', 'may', 'or', 'may', 'not', 'represent', 'a', 'real', 'world', 'test', 'its', 'a', 'start', 'hey', 'what', 'do', 'you', 'know', 'there', 'are', 'lots', 'and', 'lots', 'and', 'lots', 'of', 'lots', 'of', 'lots', 'lots', 'lots', 'lots', 'lots', 'lots', 'of', 'words', 'to', 'match', 'and', 'in', 'a', 'diverse', 'order', 'that', 'may', 'or', 'may', 'not', 'repeat', 'itself', 'some', 'words', 'that', 'may', 'or', 'may', 'not', 'repeat', 'themselves', '', 'this', 'is', 'a', 'somewhat', 'contrived', '', 'that', 'may', 'or', 'may', 'not', '', 'a', 'real', 'world', 'test', 'its', 'a', 'start', 'hey', 'what', 'do', 'you', 'know', 's', 'lots', 'lots', 'lots', 'of', 'words', 'to', 'match', 'and', 'in', 'a', 'diverse', 'order', 'that', 'may', 'or', 'may', 'not', 'repeat', 'itself'],
//                               'num_array' : [ 1000, 2000, 3000, 4000, 5000, 6000, 7000, 9000, 10000],
//                           } );
// }




// Lua wordcount test (for loop)
////////////////////////////////
m_wc =  "words = {}; ";
m_wc += "for i,word in pairs(doc.wordcloud) do ";
m_wc += "    if (words[word]) then " ;
m_wc += "        words[word] = words[word] + 1; ";
m_wc += "    else ";
m_wc += "        words[word] = 1; ";
m_wc += "    end ";
m_wc += "end ";
m_wc += "for i,v in pairs(words) do print('word and count:', i, v) end";
// m_wc += "for word,v in pairs(words) do ";
// m_wc += "    print('word: ', word, 'count:', v); ";
// m_wc += "end ";
r_wc = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.complex_test.mapReduce(m_wc, r_wc, {out:{replace:"test_merged"}, useLua:true});
// Results: 28 seconds
// Mon Dec 19 10:50:04 [conn18] PERFORMANCE:  Cycles taken for Run Map Reduce: 79141316061
// Mon Dec 19 10:50:04 [conn18] PERFORMANCE: Microseconds taken to run MapReduce: 28264799




// JS wordcount test ('for-in' loop)
///////////////////////////////
m_jswc1 = function() {
    words = {};
    for (iter in this.wordcloud) {
        var word = this.wordcloud[iter];
        if (word) {
            words[word] += 1;
        } else {
            words[word] = 1;
        }
    }
}
r_wc = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.complex_test.mapReduce(m_jswc1, r_wc, {out:{replace:"test_merged"}, useLua:false});
// Results (using 'for-in' loop): 8400 seconds
// Mon Dec 19 13:19:09 [conn20] PERFORMANCE:  Cycles taken for Run Map Reduce: 23485058942805
// Mon Dec 19 13:19:09 [conn20] PERFORMANCE: Microseconds taken to run MapReduce: 8407429233




// JS wordcount test (forEach callback)
///////////////////////////////////////
m_jswc2 = function() {
    words = {};
    this.wordcloud.forEach(function(word) {
        if (words.hasOwnProperty(word)) {
            words[word] += 1;
        } else {
            words[word] = 1;
        }
    });
}
r_wc = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.complex_test.mapReduce(m_jswc2, r_wc, {out:{replace:"test_merged"}, useLua:false});
// Results (using foreach callback): 100 seconds
//    Mon Dec 19 10:55:06 [conn20] PERFORMANCE:  Cycles taken for Run Map Reduce: 280500443016
//    Mon Dec 19 10:55:06 [conn20] PERFORMANCE: Microseconds taken to run MapReduce: 100178810
// Results (with check for hasOwnProperty)
//    Mon Dec 19 21:43:58 [conn22] PERFORMANCE:  Cycles taken for Run Map Reduce: 410312344080
//    Mon Dec 19 21:43:58 [conn22] PERFORMANCE: Microseconds taken to run MapReduce: 146540190




// JS wordcount test (c-style for loop)
///////////////////////////////
m_jswc3 = function() {
    words = {};
    for (var iter = 0; iter < this.wordcloud.length; iter++) {
        var word = this.wordcloud[iter];
        if (word) {
            words[word] += 1;
        } else {
            words[word] = 1;
        }
    }
}
r_wc = function( key , values ) { return {'a':'a'} }; // dummy reducer
db.complex_test.mapReduce(m_jswc3, r_wc, {out:{replace:"test_merged"}, useLua:false});
// Results (using c-style for loop):  Well over 500 seconds (stopped due to lack of interest)
// See: http://jsperf.com/for-vs-foreach/9 for details of why .forEach is fastest



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
