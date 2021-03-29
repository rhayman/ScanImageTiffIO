Synopsis
========

A shared library written in C/C++ to load data recorded using ScanImage (link).

Some details
============
Initially this was for use with r2p2 (link), a GUI I wrote that is basically an interface into openCV (link) and many of its algorithms.

There is now a python branch that I'm writing that allows the library to be accessed from Python using pybind11 (link). I'm still figuring the Python interface out...

Dependencies
============
You'll need to install:

- opencv (https://opencv.org/)

- libtiff (http://www.libtiff.org/)

Installation
============

So far I've done

`
python3 -m pip install pybind11
`

Presumably you need to clone this git repo etc:

`
git clone https://github.com/rhayman/ScanImageTiffIO.git
cd ScanImageTiffIO
mkdir build
cd build
cmake ..
sudo make install
`

That will install the shared lib as:

`
/usr/local/lib/libScanImageTiff.so
`

The header file will be:

`
/usr/local/include/ScanImageTiff.h
`

and am reading the pybind11 documentation...

Based on which I've now done:

`
python3 -m pip install pytest
`

and,

`
git clone https://github.com/pybind/pybind11.git
cd pybind11
mkdir build
cd build
cmake ..
make check -j 8
sudo make install
`

The last step dumped a load of .h files in /usr/local/include/pybind11 and a bunch of files in the cmake folders (/usr/local/share/cmake/pybind11)
