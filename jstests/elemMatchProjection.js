t = db.SERVER828Test;
t.drop();

// Insert various styles of arrays
t.insert({ group: 1, x: [ 1, 2, 3, 4, 5 ] });
t.insert({ group: 2, x: [ { a: 1, b: 2 }, { a: 2, c: 3 }, { a:1, d:5 } ] });
t.insert({ group: 3, x: [ { a: 1, b: 3 }, { a: -6, c: 3 } ] });
t.insert({ group: 4, x: [ { a: 1, b: 4 }, { a: -6, c: 3 } ] });
t.insert({ group: 5, x: [ new Date(), 5, 10, 'string', new ObjectId(), 123.456 ] });
t.insert({ group: 6, x: [ { a: 'string', b: new Date() }, { a: new ObjectId(), b: 1.2345 }, { a: 'string2', b: new Date() } ] });
t.insert({ group: 7, x: [ { y: [ 1, 2, 3, 4 ] } ] });
t.insert({ group: 8, x: [ { y: [ { a: 1, b: 2 }, {a: 3, b: 4} ] } ] });
t.insert({ group: 9, x: [ { y: [ { a: 1, b: 2 }, {a: 3, b: 4} ] }, { z: [ { a: 1, b: 2 }, {a: 3, b: 4} ] } ] });
t.insert({ group: 10, x: [ { a: 1, b: 2 }, {a: 3, b: 4} ], y: [ { c: 1, d: 2 }, {c: 3, d: 4} ] });

assert.eq( 1 , t.find( { group:4 }, { x: { $elemMatch: { a:-6 } } }             ).toArray()[0].x.length , "single object match" );
assert.eq( 2 , t.find( { group:4 }, { x: { $elemMatch: { a:{ $lte:2 } } } }     ).toArray()[0].x.length , "multi object match" );

assert.eq( 1 , t.find( { group:1 }, { x: { $elemMatch: 4 } }                    ).toArray()[0].x.length , "single number match" );
assert.eq( 4 , t.find( { group:1 }, { x: { $elemMatch: { x: { $gte:2 } } } }    ).toArray()[0].x.length , "multi number match" );

assert.eq( 1 , t.find( { group:5 }, { x: { $elemMatch: [ { $ne: 2 } ] } }       ).toArray()[0].x.length , "single mixed type match 1" ); // FIXME (array notation)
assert.eq( 1 , t.find( { group:5 }, { x: { $elemMatch: [ { $gte:123 } ] } }     ).toArray()[0].x.length , "single mixed type match 2" ); // FIXME (array notation)
assert.eq( 3 , t.find( { group:5 }, { x: { $elemMatch: [ { $gte:1 }  ] } }      ).toArray()[0].x.length , "multi mixed type match" ); // FIXME (array notation)

assert.eq( 1 , t.find( { group:6 }, { x: { $elemMatch: { a:'string' } } }       ).toArray()[0].x.length , "single mixed object match" );
assert.eq( 2 , t.find( { group:6 }, { x: { $elemMatch: { a:/string/ } } }       ).toArray()[0].x.length , "multi mixed object match 1" );
assert.eq( 2 , t.find( { group:6 }, { x: { $elemMatch: { a: { $type: 7 } } } }  ).toArray()[0].x.length , "multi mixed object match 2" );

assert.eq( 1 , t.find( { group:8 }, { x: { $elemMatch: { y: { $elemMatch: { a: 1 } } } } } ).toArray()[0].x.length , "single nested match" );  // FIXME (match within nested arrays)
assert.eq( 1 , t.find( { group:9 }, { 'x.y': { $elemMatch: { a: 1 } } } ).toArray()[0].x.length , "single dotted match" );  // FIXME (match within nested arrays)
assert.eq( 2 , t.find( { group:10 }, { x: { $elemMatch: { a: 1 } }, y: { $elemMatch: { c:3 } } } ).toArray()[0].x.length , "multi element match" );  // FIXME (multiple $elemMatch ops)

// limitations
///////////////

// Nested Arrays:  $elemMatch can only be used for one array.
// For arrays of values, use an exact match (e.g. x:{ $elemMatch: 1 } ).
//        -- use an array for more complex queries (e.g. x:{ $elemMatch:[ { $gte:1 }, { $lte: 500} ] } )
//        -- allow multiple $elemMatch's in one projection
