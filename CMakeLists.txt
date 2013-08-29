#
#   Copyright (c) 2013 Insollo Entertainment, LLC.  All rights reserved.
#
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom
#   the Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#   IN THE SOFTWARE.
#

cmake_minimum_required (VERSION 2.6)
project (nanocat)
include (FindPkgConfig)

add_definitions (-D_POSIX_C_SOURCE=199309L)
add_executable (nanocat
    src/main.c
    src/options.c
    )
install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/nanocat DESTINATION bin)


pkg_search_module(NANOMSG REQUIRED nanomsg)
target_link_libraries(nanocat nanomsg)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${NANOMSG_CFLAGS} -std=c99 -Wpedantic -Wall")
