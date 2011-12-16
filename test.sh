#!/bin/bash
sudo ./mongod -f /etc/mongodb.conf -vvvv &
sleep 2
./mongo test_giant < lua_mr_tests.js
