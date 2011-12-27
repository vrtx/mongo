/** @file       engine_lua.h
 **             implements a luajit based scripting engine for mongodb.
 **
 ** @details    this engine should be implemented with the following design goals in mind (WIP):
 **                 - thread-safety
 **                 - consistency with existing javascript api
 **                 - support combined lua and javascript builds
 **                 - support lua-only mongo builds (e.g. optimized for embedded systems)?
 **                 - implement pool of lua_States; possibly tied to a thread pool?
 **                 - confer on language differences (e.g. js function scope vs. lua closures
 **                     and upvalues, globals, and possibly a js -> lua bridge)?
 **                 - optimize BSON bindings and make sure BSONType tables (from luamongo) are
 **                     JITable, or possibly build type tables at compile time?
 **
 ** @todo       copy luamongo bson bindings, check licensing, optimize if neccesary, etc.
 ** @todo       discuss some of the actual design goals and use cases w/ 10gen and community.
 ** @date       12/26/11
 ** @authors    ben becker
 *
 *
 *    Copyright 2011 10gen Inc.
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

#pragma once

#include "engine.h"
#include "lua.hpp"

// BB FIXME: symbols currently stolen from Lua driver (see git submodule '/src/third_party/luamongo').
extern void bson_to_lua( lua_State *L, const mongo::BSONObj &obj );
extern void lua_to_bson( lua_State *L, int stackpos, BSONObj &obj );
extern const char *bson_name( int type );

namespace mongo {

    // BB TODO: create static LUA equivilents
    extern JSClass bson_class;
    extern JSClass bson_ro_class;
    extern JSClass object_id_class;
    extern JSClass dbpointer_class;
    extern JSClass dbref_class;
    extern JSClass bindata_class;
    extern JSClass timestamp_class;
    extern JSClass numberlong_class;
    extern JSClass numberint_class;
    extern JSClass minkey_class;
    extern JSClass maxkey_class;

    // BB TODO: 

    // internal things
    // void dontDeleteScope( SMScope * s ) {}
    // void errorReporter( JSContext *cx, const char *message, JSErrorReport *report );
    // extern boost::thread_specific_ptr<SMScope> currentScope;

    // BB TODO: implement similar functions like a fast table <--> bson bridge, possibly with 
    //          additional type support (e.g. mongo date, minkey, etc.):

    // bson
    JSBool resolveBSONField( JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp );

    // mongo
    void initMongoLua( Scope * scope , lua_State * cx , JSObject * global , bool local );
    bool appendSpecialDBObject( Convertor * c , BSONObjBuilder& b , const string& name , jsval val , JSObject * o );
    bool isDate( lua_State * cx , JSObject * o );

}
