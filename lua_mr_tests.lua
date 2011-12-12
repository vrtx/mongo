// cook some data

// use test_giant
// for (var i = 0; i < 1000000; i++) {
//     db.giant_test.insert({'int1':i, 
//                      'float1':i/2, 
//                      'data1':'a fairly small string; like a title', 
//                      'data2':'a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article.  a larger string representing a short web page or article. '
//                      });
// }
// 

// run a mr job

// lua mapper with js reducer
use test_giant
m = "print('testing iteration: '..iter)";
r = function( key , values )
{
    return { 'test': 'test' };
};
db.giant_test.mapReduce(m, r, {out:{replace:"test_merged"} } );
