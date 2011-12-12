#!/bin/bash
./build.sh
sudo ./mongod -f /etc/mongodb.conf -vvvv
./mongo test_giant < lua_mr_tests.lua
