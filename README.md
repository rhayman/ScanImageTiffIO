Synopsis
========

A shared library written in C/C++ to load data recorded using ScanImage (link).

Some details
============
Initially this was for use with r2p2 (link), a GUI I wrote that is basically an interface into openCV (link) and many of its algorithms.

There is now a python branch that I'm writing that allows the library to be accessed from Python using pybind11 (link). I'm still figuring the Python interface out...

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

