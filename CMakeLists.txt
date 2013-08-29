cmake_minimum_required (VERSION 2.6)
project (nanocat)
include (FindPkgConfig)

add_definitions (-D_POSIX_C_SOURCE=199309L)
add_executable (nanocat
    src/main.c
    src/options.c
    )


pkg_search_module(NANOMSG REQUIRED nanomsg)
target_link_libraries(nanocat nanomsg)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${NANOMSG_CFLAGS} -std=c99 -Wpedantic -Wall")
