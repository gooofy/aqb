#!/bin/bash

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt \
     target/x86_64-linux/bin/aqb foo.bas

