// projection.cpp

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "pch.h"
#include "projection.h"
#include "matcher.h"
#include "util/mongoutils/str.h"

namespace mongo {

    void Projection::init( const BSONObj& o ) {
        massert( 10371 , "can only add to Projection once", _source.isEmpty());
        _source = o;

        BSONObjIterator i( o );
        int true_false = -1;
        bool hasPositional = false;
        while ( i.more() ) {
            BSONElement e = i.next();

            if ( ! e.isNumber() )
                _hasNonSimple = true;

            if (e.type() == Object) {
                BSONObj obj = e.embeddedObject();
                BSONElement e2 = obj.firstElement();
                if ( strcmp(e2.fieldName(), "$slice") == 0 ) {
                    if (e2.isNumber()) {
                        int i = e2.numberInt();
                        if (i < 0)
                            add(e.fieldName(), i, -i); // limit is now positive
                        else
                            add(e.fieldName(), 0, i);

                    }
                    else if (e2.type() == Array) {
                        BSONObj arr = e2.embeddedObject();
                        uassert(13099, "$slice array wrong size", arr.nFields() == 2 );

                        BSONObjIterator it(arr);
                        int skip = it.next().numberInt();
                        int limit = it.next().numberInt();
                        uassert(13100, "$slice limit must be positive", limit > 0 );
                        add(e.fieldName(), skip, limit);

                    }
                    else {
                        uassert(13098, "$slice only supports numbers and [skip, limit] arrays", false);
                    }
                }
                else if ( strcmp( e2.fieldName(), "$elemMatch" ) == 0 ) {
                    uassert( 16237, "elemMatch: invalid argument.  object required.",
                             e2.type() == Object );
                    add( e.fieldName(), true );
                }
                else {
                    uasserted(13097, string("Unsupported projection option: ") +
                                     obj.firstElementFieldName() );
                }

            }
            else if (!strcmp(e.fieldName(), "_id") && !e.trueValue()) {
                _includeID = false;
            }
            else {
                add( e.fieldName(), e.trueValue() );

                // validate input
                if (true_false == -1) {
                    true_false = e.trueValue();
                    _include = !e.trueValue();
                }
                else {
                    uassert( 10053 , "You cannot currently mix including and excluding fields. "
                                     "Contact us if this is an issue." ,
                             (bool)true_false == e.trueValue() );
                }
            }
            string fieldName( e.fieldName() );
            size_t opPos = fieldName.find(".$");
            if ( opPos != string::npos ) {
                // positional op found; add parent fields
                uassert( 16234, "Cannot exclude array elements with the positional operator"
                                " (currently unsupported).", e.trueValue() );
                uassert( 16235, "Cannot specify more than one positional array projection per query"
                                " (currently unsupported).", ! hasPositional );
                hasPositional = true;
                add( fieldName.substr( 0, opPos ) , e.trueValue() );
            }
        }
    }

    void Projection::add(const string& field, bool include) {
        if (field.empty()) { // this is the field the user referred to
            _include = include;
        }
        else {
            _include = !include;

            const size_t dot = field.find('.');
            const string subfield = field.substr(0,dot);
            const string rest = (dot == string::npos ? "" : field.substr(dot+1,string::npos));

            boost::shared_ptr<Projection>& fm = _fields[subfield];
            if (!fm)
                fm.reset(new Projection());

            fm->add(rest, include);
        }
    }

    void Projection::add(const string& field, int skip, int limit) {
        _special = true; // can't include or exclude whole object

        if (field.empty()) { // this is the field the user referred to
            _skip = skip;
            _limit = limit;
        }
        else {
            const size_t dot = field.find('.');
            const string subfield = field.substr(0,dot);
            const string rest = (dot == string::npos ? "" : field.substr(dot+1,string::npos));

            boost::shared_ptr<Projection>& fm = _fields[subfield];
            if (!fm)
                fm.reset(new Projection());

            fm->add(rest, skip, limit);
        }
    }

    void Projection::transform( const BSONObj& in , BSONObjBuilder& b, const MatchDetails* details ) const {
        const ArrayOpType& arrayOpType = getArrayOpType();
        typedef map<string, shared_ptr<Matcher> > Matchers;
        Matchers matchers;

        if ( arrayOpType == ARRAY_OP_ELEM_MATCH ) {
            // $elemMatch projection operator(s) specified.  create new matcher(s).
            BSONObjIterator specIt( getSpec() );
            while ( specIt.more() ) {
                BSONElement specElem = specIt.next();
                BSONObjBuilder ob;
                ob.append( specElem );
                BSONObj fieldObj = ob.done();
                log() << "checking for $elemMatch in specifier field: " << fieldObj << endl;
                if ( specElem.type() == Object &&
                     mongoutils::str::equals( specElem.Obj().firstElement().fieldName(),
                                              "$elemMatch" ) ) {
                    matchers.insert( make_pair( specElem.fieldName(),
                                                new Matcher(fieldObj, true ) ) );
                }
            }
        }

        BSONObjIterator i(in);
        while ( i.more() ) {
            BSONElement e = i.next();
            if ( mongoutils::str::equals( "_id" , e.fieldName() ) ) {
                if ( _includeID )
                    b.append( e );
            }
            else {
                if ( matchers.find( e.fieldName() ) == matchers.end() ) {
                    // no array matchers for this field
                    log() << "appending non $elemMatch field: " << e.fieldName() << endl;
                    append( b, e, details, arrayOpType );
                }
            }
        }

        if ( arrayOpType == ARRAY_OP_ELEM_MATCH ) {
            // $elemMatch projection operator(s) specified

            for ( Matchers::const_iterator matcher = matchers.begin();
                  matcher != matchers.end(); ++matcher ) {

                MatchDetails arrayDetails;
                arrayDetails.requestElemMatchKey();
                log() << "checking $elemMatch match on: " << matcher->first  << endl;
                if ( matcher->second->matches( in, &arrayDetails ) ) {
                    log() << "Matched array on field: " << matcher->first  << endl
                          << " from array: " << in.getField( matcher->first ) << endl
                          << " in object: " << in << endl
                          << " at position: " << arrayDetails.elemMatchKey() << endl;
                    FieldMap::const_iterator field = _fields.find( matcher->first );
                    if ( field != _fields.end() ) {
                        log() << "    found " << matcher->first << " in field map. " << endl;
                        BSONArrayBuilder a;
                        BSONObjBuilder o;
                        a.append( in.getField( matcher->first )
                                    .Obj().getField( arrayDetails.elemMatchKey() ) );
                        o.appendArray( matcher->first, a.arr() );
                        field->second->append( b, o.done().firstElement(), details, arrayOpType );
                    }
                }
            }
        }
    }

    BSONObj Projection::transform( const BSONObj& in, const MatchDetails* details ) const {
        BSONObjBuilder b;
        transform( in , b, details );
        return b.obj();
    }


    //b will be the value part of an array-typed BSONElement
    void Projection::appendArray( BSONObjBuilder& b , const BSONObj& a , bool nested) const {
        int skip  = nested ?  0 : _skip;
        int limit = nested ? -1 : _limit;

        if (skip < 0) {
            skip = max(0, skip + a.nFields());
        }

        int i=0;
        BSONObjIterator it(a);
        while (it.more()) {
            BSONElement e = it.next();

            if (skip) {
                skip--;
                continue;
            }

            if (limit != -1 && (limit-- == 0)) {
                break;
            }

            switch(e.type()) {
            case Array: {
                BSONObjBuilder subb;
                appendArray(subb , e.embeddedObject(), true);
                b.appendArray(b.numStr(i++), subb.obj());
                break;
            }
            case Object: {
                BSONObjBuilder subb;
                BSONObjIterator jt(e.embeddedObject());
                while (jt.more()) {
                    append(subb , jt.next());
                }
                b.append(b.numStr(i++), subb.obj());
                break;
            }
            default:
                if (_include)
                    b.appendAs(e, b.numStr(i++));
            }
        }
    }

    void Projection::append( BSONObjBuilder& b , const BSONElement& e, const MatchDetails* details,
                             const ArrayOpType arrayOpType ) const {

        FieldMap::const_iterator field = _fields.find( e.fieldName() );
        if (field == _fields.end()) {
            if (_include)
                b.append(e);
        }
        else {
            Projection& subfm = *field->second;
            if ( ( subfm._fields.empty() && !subfm._special && arrayOpType == ARRAY_OP_NORMAL) ||
                 !(e.type()==Object || e.type()==Array) ) {
                // field map empty, or element is not an array/object
                if (subfm._include)
                    b.append(e);
            }
            else if (e.type() == Object) {
                BSONObjBuilder subb;
                BSONObjIterator it(e.embeddedObject());
                while (it.more()) {
                    subfm.append(subb, it.next(), details, arrayOpType);
                }
                b.append(e.fieldName(), subb.obj());
            }
            else { //Array
                BSONObjBuilder matchedBuilder;
                if ( details && arrayOpType == ARRAY_OP_POSITIONAL ) {
                    // $ positional operator specified

                    log() << "checking if element " << e << " is in spec: " << getSpec()
                          << " match details: " << *details << endl;
                    uassert(16233, mongoutils::str::stream() << "positional operator ("
                                        << e.fieldName()
                                        << ".$) requires corresponding field in query specifier",
                                   details && details->hasElemMatchKey() );

                    // append as the first and only element in the projected array
                    subfm.append( matchedBuilder, e.embeddedObject()[details->elemMatchKey()]
                                                  .wrap( "0" ).firstElement(),
                                  details, arrayOpType );
                }
                else {
                    // append exact array; no subarray matcher specified
                    subfm.appendArray( matchedBuilder, e.embeddedObject(), details );
                }
                b.appendArray( e.fieldName(), matchedBuilder.obj() );
            }
        }
    }

    Projection::ArrayOpType Projection::getArrayOpType( ) const {
        return getArrayOpType( getSpec() );
    }

    Projection::ArrayOpType Projection::getArrayOpType( const BSONObj spec ) const {
        BSONObjIterator iq( spec );
        while ( iq.more() ) {
            // iterate through each element
            const BSONElement& elem = iq.next();
            const char* const& fieldName = elem.fieldName();
            if ( mongoutils::str::contains( fieldName, ".$" ) ) {
                // projection contains positional or $elemMatch operator
                return ARRAY_OP_POSITIONAL;
            }
            if ( mongoutils::str::contains( fieldName, "$elemMatch" ) ) {
                // projection contains positional or $elemMatch operator
                return ARRAY_OP_ELEM_MATCH;
            }

            // check nested elements
            if ( elem.type() == Object )
                return getArrayOpType( elem.embeddedObject() );
        }
        return ARRAY_OP_NORMAL;
    }

    Projection::KeyOnly* Projection::checkKey( const BSONObj& keyPattern ) const {
        if ( _include ) {
            // if we default to including then we can't
            // use an index because we don't know what we're missing
            return 0;
        }

        if ( _hasNonSimple )
            return 0;

        if ( _includeID && keyPattern["_id"].eoo() )
            return 0;

        // at this point we know its all { x : 1 } style

        auto_ptr<KeyOnly> p( new KeyOnly() );

        int got = 0;
        BSONObjIterator i( keyPattern );
        while ( i.more() ) {
            BSONElement k = i.next();

            if ( _source[k.fieldName()].type() ) {

                if ( strchr( k.fieldName() , '.' ) ) {
                    // TODO we currently don't support dotted fields
                    //      SERVER-2104
                    return 0;
                }

                if ( ! _includeID && mongoutils::str::equals( k.fieldName() , "_id" ) ) {
                    p->addNo();
                }
                else {
                    p->addYes( k.fieldName() );
                    got++;
                }
            }
            else if ( mongoutils::str::equals( "_id" , k.fieldName() ) && _includeID ) {
                p->addYes( "_id" );
            }
            else {
                p->addNo();
            }

        }

        int need = _source.nFields();
        if ( ! _includeID )
            need--;

        if ( got == need )
            return p.release();

        return 0;
    }

    BSONObj Projection::KeyOnly::hydrate( const BSONObj& key ) const {
        verify( _include.size() == _names.size() );

        BSONObjBuilder b( key.objsize() + _stringSize + 16 );

        BSONObjIterator i(key);
        unsigned n=0;
        while ( i.more() ) {
            verify( n < _include.size() );
            BSONElement e = i.next();
            if ( _include[n] ) {
                b.appendAs( e , _names[n] );
            }
            n++;
        }

        return b.obj();
    }
}
