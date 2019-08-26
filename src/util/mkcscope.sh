#!/bin/sh

find . -name *.[chsCHS] -print > cscope.files
cscope -b -i cscope.files
ctags -R .
