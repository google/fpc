#!/bin/bash

fpc() {
    echo
    echo ___[ fpc $@ ]___
    ./fpc $@
}

fpc -256 -l-p 0.01
fpc -512 -l-p 1
fpc 2^70 l+256 1
fpc 1 2 3
fpc 2 1 1
fpc 1 2 0
fpc 1+2*3^4
fpc '(1+2)*3'
fpc -2^7
fpc -h-p 2^8-p 0.01
fpc -2^63 -l-p 1
fpc -2^63 -l 1

exit 0
