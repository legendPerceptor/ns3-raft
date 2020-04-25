
The Raft Implementation on NS-3
===============================

## Table of Contents:

1) [An overview](#an-open-source-project)
2) [Run Raft on NS-3](#running-raft-on-ns-3)
3) [Getting access to the ns-3 documentation](#getting-access-to-the-ns-3-documentation)
4) [Working with the development version of ns-3](#working-with-the-development-version-of-ns-3)


## An Open Source project

This project implements the Raft consensus algorithm on the ns-3 network simulator.
It is based on the [Cornerstone](https://github.com/datatechnology/cornerstone) project, which
serves as the core implementation of the Raft consensus algorithm. The original project
builds on the real networks with multi-threads using a third-party library [Asio](https://github.com/chriskohlhoff/asio).
In order to test and evaluate the algorithm with scalable number of cluster nodes, we modify the Raft kernel and build a ns-3 network topology
to run Raft algorithm on the ns-3 environment.

In our implementation, we still preserve the asio-related version and make it easy to change between
real networks and ns-3 environment. To run the ns-3 version of Raft implementation, you don't need
to install any third-party libs except for necessary system libs. All logics of Raft and ns-3 are
written in C++ and you have access to all those codes in this repository.


ns-3 is a free open source project aiming to build a discrete-event
network simulator targeted for simulation research and education. We use the CMake-supported
version of ns-3 and our work lies in the examples directory named **raftcore**. To some extent, we consider the project
an application of the ns-3 simulator.

This project is part of the graduation design for bachelor's degree by Yuanjian Liu, Zhejiang University.
We anticipate the project to be finished by the end of May, 2020. Currently, the project is under active development.


## Running Raft on NS-3

The project is configured with CMakeLists.txt; We recommend you to open this folder with CLion, and
you can directly view the codes and run any targets you want.

However, you can always use cmake to
build it from the command line. The easiest way to do that is as follows

```sh
$ git clone git@github.com:legendPerceptor/ns3-raft.git
$ cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" $(pwd)
$ make
```

The above commands will generate lots of cmake-related files that are not useful for us,
so we recommend letting CLion do the intricate configuration of cmake.

Our codes locate in examples/raftcore.

## Getting access to the ns-3 documentation

Once you have verified that your build of ns-3 works by running
the simple-point-to-point example as outlined in 3) above, it is
quite likely that you will want to get started on reading
some ns-3 documentation.

All of that documentation should always be available from
the ns-3 website: http:://www.nsnam.org/documentation/.

This documentation includes:

  - a tutorial

  - a reference manual

  - models in the ns-3 model library

  - a wiki for user-contributed tips: http://www.nsnam.org/wiki/

  - API documentation generated using doxygen: this is
    a reference manual, most likely not very well suited
    as introductory text:
    http://www.nsnam.org/doxygen/index.html

## Working with the development version of ns-3

If you want to download and use the development version of ns-3, you
need to use the tool `git`. A quick and dirty cheat sheet is included
in the manual, but reading through the git
tutorials found in the Internet is usually a good idea if you are not
familiar with it.

If you have successfully installed git, you can get
a copy of the development version with the following command:
```shell
git clone https://gitlab.com/nsnam/ns-3-dev.git
```

However, we recommend to follow the Gitlab guidelines for starters,
that includes creating a Gitlab account, forking the ns-3-dev project
under the new account's name, and then cloning the forked repository.
You can find more information in the manual [link].
