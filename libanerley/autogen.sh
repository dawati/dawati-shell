#!/bin/bash
# Run this to generate all the initial makefiles, etc.

autoreconf -v -i && ./configure $@
