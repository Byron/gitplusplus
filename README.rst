What is Git++ ?
===============
In its current form, git++ models the concept behind git in general, and specializes is to allow reading and writing of git repositories, in a platform independent manner.

The model is, as the name suggests, heavily based on templates, which model the git-concept. 
The specialization of these models makes them usable in the real world, and allows third partys to link them statically.

As a major difference to the existing implementation, git++ operates exclusively on streams, which makes the abstraction of different sources and destinations easy, i.e. to implement new protocols or streaming of objects across a network. Additionally, the stream implementation attempts to stream packed objects as well, to reduce memory bandwidth requirements.

Why Git++ ?
===========
Primarily it is used to exercise advanced features of c++0x and to get used to the programming of usable models which integrate well with the stl and boost.

This should allow git-like behaviour even for non-standard applications which require an own repository format, but still want to benefit from the advantages of a git-like data structure.

As such, git++ is a pet project, which at first aims to satisfy my own curiosity, and which should later become fully usable and thus part of my own software.

Prerequesites
=============
 * A c++0x compatible compiler, gcc4.6 is currently used
 * cmake to direct the toolchain on the respective OS
 * Boost 1.38 or higher ( headers + static libraries )
 * Doxygen ( only required for building of documentation )

