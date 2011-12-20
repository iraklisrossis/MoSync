#!/bin/sh
ld *.a -arch i386 -ios_version_min 4.2 -r -ObjC -o ../extensions.a
