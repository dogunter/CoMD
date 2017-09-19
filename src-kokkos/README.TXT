This is the Kokkos implementation of CoMD version 1.1
=====================================================

To contact the developers of CoMD send email to exmatex-comd@llnl.gov.

CoMD is a reference implementation of typical classical molecular
dynamics algorithms and workloads.  It is created and maintained by
The Exascale Co-Design Center for Materials in Extreme Environments
(ExMatEx).  http://codesign.lanl.gov/projects/exmatex.  The
code is intended to serve as a vehicle for co-design by allowing
others to extend and/or reimplement it as needed to test performance of 
new architectures, programming models, etc.

The current version of CoMD is available from:

https://github.com/exmatex/CoMD

Obtaining Documentation
=======================

CoMD documentation is produced by doxygen (www.doxygen.org).  See
../src-mpi/README.TXT for more information.

Information specific to the Kokkos version is in Kokkos.dox (human
readable).  If you run doxygen in src-kokkos this information will
automatically be included in the generated html files (html/index.html).


Building CoMD
=============

The Kokkos version of CoMD required compiling with modern C++ compilers.
To that end, several changes were made in my of the exiting source
code files to allow the C-code to compile under C++ compilers.
You will need to create a Makefile by copying the sample provided with 
the distribution (Makefile.vanilla).

$ cp Makefile.vanilla Makefile

and use the make command to build the code

$ make

The sample Makefile will compile the code on many platforms.  See
comments in Makefile.vanilla for information about specifying the name
of the C compiler and/or additional compiler switches that might be
necessary for your platform.
