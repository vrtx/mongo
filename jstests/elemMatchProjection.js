// Tests for $elemMatch projections and $ positional operator projection.
t = db.SERVER828Test;
t.drop();

// Insert various styles of arrays
for ( i = 0; i < 100; i++ ) {
    t.insert({ group: 1, x: [ 1, 2, 3, 4, 5 ] });
    t.insert({ group: 2, x: [ { a: 1, b: 2 }, { a: 2, c: 3 }, { a:1, d:5 } ] });
    t.insert({ group: 3, x: [ { a: 1, b: 2 }, { a: 2, c: 3 }, { a:1, d:5 } ],
                         y: [ { aa: 1, bb: 2 }, { aa: 2, cc: 3 }, { aa:1, dd:5 } ] });
    t.insert({ group: 3, x: [ { a: 1, b: 3 }, { a: -6, c: 3 } ] });
    t.insert({ group: 4, x: [ { a: 1, b: 4 }, { a: -6, c: 3 } ] });
    t.insert({ group: 5, x: [ new Date(), 5, 10, 'string', new ObjectId(), 123.456 ] });
    t.insert({ group: 6, x: [ { a: 'string', b: new Date() },
                              { a: new ObjectId(), b: 1.2345 },
                              { a: 'string2', b: new Date() } ] });
    t.insert({ group: 7, x: [ { y: [ 1, 2, 3, 4 ] } ] });
    t.insert({ group: 8, x: [ { y: [ { a: 1, b: 2 }, {a: 3, b: 4} ] } ] });
    t.insert({ group: 9, x: [ { y: [ { a: 1, b: 2 }, {a: 3, b: 4} ] },
                              { z: [ { a: 1, b: 2 }, {a: 3, b: 4} ] } ] });
    t.insert({ group: 10, x: [ { a: 1, b: 2 }, {a: 3, b: 4} ],
                          y: [ { c: 1, d: 2 }, {c: 3, d: 4} ] });
}

//
// SERVER-828:  Positional operator ($) projection tests
//
assert.eq( 2,
           t.find( { group:3, 'x.a':2 }, { 'x.$':1 } ).toArray()[0].x[0].a,
           "single object match 1" );

assert.eq( { aa:1, dd:5 },
           t.find( { group:3, 'y.dd':5 }, { 'y.$':1 } ).toArray()[0].y[0],
           "single object match 3" );

// FIXME: multiple array matches with one projection
assert.eq( 2,
           t.find( { group:3, 'y.bb':2, 'x.d':5 }, { 'y.$':1 } ).toArray()[0].y[0].bb,
           "multi match, single proj 1" );

// FIXME: multiple array matches with one projection
assert.eq( 2,
           t.find( { group:3, 'y.cc':3, 'x.b':2 }, { 'x.$':1 } ).toArray()[0].x[0].b,
           "multi match, single proj 2" );

if (false) {

    assert.eq( { dd:5 }, // FUTURE: positional projection should fields after the $
               t.find( { group:3, 'y.dd':5 }, { 'y.$.dd':1 } ).toArray()[0].y[0],
               "single object match 3" );

    assert.eq( 2,       // FUTURE: allow multiple positional operators
               t.find( { group:3, 'y.bb':2, 'x.d':5 }, { 'y.$':1, 'x.$':1 } ).toArray()[0].y[0].bb,
               "multi match, multi proj 1" );

    assert.eq( 5,       // FUTURE: allow multiple positional operators
               t.find( { group:3, 'y.bb':2, 'x.d':5 }, { 'y.$':1, 'x.$':1 } ).toArray()[0].x[0].d,
               "multi match, multi proj 2" );

    assert.eq( 2,        // FUTURE: allow multiple results from same matcher
               t.find( { group:2, x: { $elemMatch: { a:1 } } }, { 'x.$':1 } ).toArray()[0].x.length,
               "multi element match, single proj" );
}

//
// SERVER-2238:  $elemMatch projections
//
assert.eq( -6,
            t.find( { group:4 }, { x: { $elemMatch: { a:-6 } } } ).toArray()[0].x[0].a,
            "single object match" );

assert.eq(  3,
            t.find( { group:4 }, { x: { $elemMatch: { a:{ $lt:1 } } } } ).toArray()[0].x[0].c,
            "object operator match" );

assert.eq( 4,
           t.find( { group:1 }, { x: { $elemMatch: { $in:[100, 4, -123] } } } ).toArray()[0].x[0],
           "$in number match" );

assert.eq( 'string',
           t.find( { group:6 }, { x: { $elemMatch: { a:'string' } } } ).toArray()[0].x[0].a,
           "mixed object match on string eq" );

assert.eq( 'string2',
           t.find( { group:6 }, { x: { $elemMatch: { a:/ring2/ } } } ).toArray()[0].x[0].a ,
           "mixed object match on regexp" );

assert.eq( 'string',
           t.find( { group:6 }, { x: { $elemMatch: { a: { $type: 2 } } } } ).toArray()[0].x[0].a,
           "mixed object match on type" );

assert.eq( 1,
           t.find( { group:10 }, { x: { $elemMatch: { a: 1 } },
                                   y: { $elemMatch: { c: 3 } } } ).toArray()[0].x[0].a,
           "multiple $elementMatch on unique fields 1" );

assert.eq( 3,
          t.find( { group:10 }, { x: { $elemMatch: { a: 1 } },
                                  y: { $elemMatch: { c: 3 } } } ).toArray()[0].y[0].c,
          "multiple $elementMatch on unique fields 2" );

// FIXME ($elemMatch within nested arrays)
assert.eq( 1 ,
           t.find( { group:9 }, { 'x.y': { $elemMatch: { a: 1 } } } ).toArray()[0].x.length,
           "single dotted match" );


if (false) {

    assert.eq(  2 ,         // FUTURE: handle multiple $elemMatch results
                t.find( { group:4 }, { x: { $elemMatch: { a:{ $lte:2 } } } } ).toArray()[0].x.length,
                "multi object match" );

    assert.eq( 3 ,          // FUTURE: handle multiple $elemMatch results
               t.find( { group:1 }, { x: { $elemMatch: { $in:[1, 2, 3] } } } ).toArray()[0].x.length,
               "$in number match" );

    assert.eq( 1 ,          // FUTURE: handle multiple $elemMatch results
               t.find( { group:5 }, { x: { $elemMatch: { $ne: 5 } } } ).toArray()[0].x.length,
               "single mixed type match 1" );

}

//
// Batch/getMore tests
//
// test positional operator across multiple batches
a = t.find( { group:3, 'x.b':2 }, { 'x.$':1 } ).batchSize(1)
while ( a.hasNext() ) {
    assert.eq( 2, a.next().x[0].b, "positional getMore test");
}

// test $elemMatch operator across multiple batches
a = t.find( { group:3 }, { x:{$elemMatch:{a:1}} } ).batchSize(1)
while ( a.hasNext() ) {
    assert.eq( 1, a.next().x[0].a, "positional getMore test");
}

// TODO: covered index projection tests in Projection::checkKey()
