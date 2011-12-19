#!/bin/sh
ls | grep ".a" | xargs -I FILE ar -x FILE
ld *.o -arch i386 -r -o ../ExtensionSystemTest/extensions.o
