Synopsis
========

A shared library written in C/C++ to load data recorded using ScanImage (http://scanimage.vidriotechnologies.com/). Also contains a Python interface.


Dependencies
============

- opencv (https://opencv.org/)

- libtiff (http://www.libtiff.org/)

- Python 3 (and NumPy)

- pybind11 (see below)

Installation
============

For opencv and pybind11 you can clone from their respective github repos.

To install pybind11 do:

```
git clone https://github.com/pybind/pybind11.git
cd pybind11
mkdir build
cd build
cmake ..
make check -j 8
sudo make install
```

There are a bunch of extra things you can install to speed up openCV including
things like Eigen, Lapack, openBLAS and so on. If you want to see more details,
take a look at the output from the cmake step show below, i.e. before typing
make.

This next is a bit more work but here is what I usually do (this takes a while):

```
mkdir opencv_repos
cd opencv_repos
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
mkdir build
cd build
cmake -DOPENCV_EXTRA_MODULES_PATH=/home/robin/opencv/opencv_contrib/modules -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local ../opencv/
make -j8
sudo make install
```

I'm assuming Python3 is already installed and working (as is NumPy).

Finally, to get this library working do the following:

```
git clone https://github.com/rhayman/ScanImageTiffIO.git
cd ScanImageTiffIO
mkdir build
cd build
cmake ..
make
sudo make install
```

That will install the shared lib as:

`
/usr/local/lib/libScanImageTiff.so
`

The header file will be:

`
/usr/local/include/ScanImageTiff.h
`

In the directory you built the library in ('build' if you followed the above instructions), 
there should now be a file called scanimagetiffio.so. This is the library that you will 
import into Python.

Whilst in the build directory you can start an iPython session and import and use like so:

```python
import scanimagetiffio
data = scanimagetiffio.SITiffIO()
data.open_tiff_file("/path/to/file.tif")
data.open_log_file("/path/to/log.txt")
data.interp_times()
frame_zero = data.get_frame(0)
x,y,theta = data.get_pos(0)
```
