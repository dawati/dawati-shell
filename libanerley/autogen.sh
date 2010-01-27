#!/bin/bash

intltoolize && autoreconf -v -i && ./configure $@
