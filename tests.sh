#!/bin/bash

# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
fpc '(1*1)+99'
fpc -2^7
fpc -h-p 2^8-p 0.01
fpc -2^63 -l-p 1
fpc -2^63 -l 1
fpc 2^-7
fpc '-(1)'
fpc '-2^-(2)'

exit 0
