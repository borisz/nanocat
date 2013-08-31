=======
nanocat
=======


The command-line interface for nanomsg. This utility is useful for debugging
applications and for scripting with nanomsg.

Building
========

Basic process is following. Feel free to adjust for your own needs::

    mkdir build
    cd build
    cmake --CMAKE_BUILD_PREFIX=/usr/local ..
    make
    sudo make install


Usage
=======

Basic usage is following::

    nanocat --SOCKTYPE {--connect ADDR|--bind ADDR}

Where ``ADDR`` is any nanomsg address, and ``SOCKTYPE`` is lowercased socket
type (without ``NN_`` preix. But there are much more arguments.
You should look::

    nanocat --help

