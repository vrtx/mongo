#!/bin/bash
./build.sh
./mongod -f /etc/mongodb.conf -vvvv
./mongo test_giant < lua_mr_tests.lua
