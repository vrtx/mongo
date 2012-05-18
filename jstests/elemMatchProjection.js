// Insert various styles of arrays
db.SERVER828Test.insert({ group: 1, b: [ 1, 2, 3, 4, 5 ] });
db.SERVER828Test.insert({ group: 2, x: [ { a: 1, b: 2 }, { a: 2, c: 3 }, { a:1, d:5 } ] });
db.SERVER828Test.insert({ group: 3, x: [ { a: 1, b: 3 }, { a: -6, c: 3 } ] });
db.SERVER828Test.insert({ group: 4, x: [ { a: 1, b: 4 }, { a: -6, c: 3 } ] });
db.SERVER828Test.insert({ group: 5, b: [ new Date(), 5, 10, 'string', new ObjectId(), 123.456 ] });
db.SERVER828Test.insert({ group: 6, b: [ { a: 'string', b: new Date() }, { a: new ObjectId(), b: 1.2345 }, { a: 'string2', b: new Date() } ] });
db.SERVER828Test.insert({ group: 7, x: [ { y: [ 1, 2, 3, 4 ] } ] });
db.SERVER828Test.insert({ group: 8, x: [ { y: [ { a: 1, b: 2 }, {a: 3, b: 4} ] } ] });
db.SERVER828Test.insert({ group: 9, x: [ { y: [ { a: 1, b: 2 }, {a: 3, b: 4} ] }, { z: [ { a: 1, b: 2 }, {a: 3, b: 4} ] } ] });


var singleObjMatch =    db.SERVER828Test.find({ group:4 }, { x: { $elemMatch: { a:-6 } } });
var multiObjMatch =     db.SERVER828Test.find({ group:4 }, { x: { $elemMatch: { a:{ $lte:2 } } } });

var singleNumMatch =    db.SERVER828Test.find({ group:1 }, { b: { $elemMatch: 4 } });
var multiNumMatch =     db.SERVER828Test.find({ }, { b: { $elemMatch: [ { $gte:2 } ] } });   // FIXME

var singleMixMatch1 =   db.SERVER828Test.find({ group:5 }, { b: { $elemMatch: [ { $ne: 2 } ] } }); // FIXME
var singleMixMatch2 =   db.SERVER828Test.find({ group:5 }, { b: { $elemMatch: [ { $gte:123 } ] } }); // FIXME
var multiMixMatch =     db.SERVER828Test.find({ group:5 }, { b: { $elemMatch: [ { $gte:1 }  ] } });   // FIXME

var singleMixObjMatch = db.SERVER828Test.find({ group:6 }, { b: { $elemMatch: { a:'string' } } });
var multiMixObjMatch =  db.SERVER828Test.find({ group:6 }, { b: { $elemMatch: { a:/string/ } } });
var multiMixObjMatch =  db.SERVER828Test.find({ group:6 }, { b: { $elemMatch: { a: { $type: 7 } } } });

var singleNestedMatch = db.SERVER828Test.find({ group:8 }, { x: { $elemMatch: { y: { $elemMatch: { a: 1 } } } } });  // FIXME
var singleDottedMatch = db.SERVER828Test.find({ group:9 }, { 'x.y': { $elemMatch: { a: 1 } } }); // FIXME

// limitations
///////////////

// Nested Arrays:  $elemMatch can only be used for one array at a time.  (give example)
// For arrays of values, use an exact match (e.g. x:{ $elemMatch: 1 } ) or an array (e.g. x:{ $elemMatch:[ { $gte:1 } ] } )
//  * Could use array to specify multiple values