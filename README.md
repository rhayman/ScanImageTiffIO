Synopsis
========

A shared library written in C/C++ to load data recorded using ScanImage (http://scanimage.vidriotechnologies.com/). Also contains a Python interface.

Installation
============

There are two branches of the repo, master and armadillo, the main difference being the libraries used to present the tiff data to the user. The master branch uses openCV whereas armadillo uses armadillo. The other big difference is that the armadillo branch installs a shared library primarily meant as a Python API so the data can be inspected etc from Python. The shared library is installed into a platform dependent installation directory so that you can import the library into Python from anywhere.

The armadillo branch has the following dependencies:

Dependencies
============

Armadillo branch
-----------------

- cmake (https://cmake.org/)

- libtiff (http://www.libtiff.org/)

- boost (https://www.boost.org/)

- Python 3 (and NumPy)


To install the armadillo branch:

```
git clone https://github.com/rhayman/ScanImageTiffIO.git --recurse-submodules
cd ScanImageTiffIO
git checkout armadillo
mkdir build
cd build
cmake ..
make
sudo make install
```

The last install step will put a shared library called scanimagetiffio into the correct platform dependent installation directory so you should now be able to do this from any directory:

```python
from scanimagetiffio.scanimagetiffio import SITiffIO
S = SITiffIO()
S.open_tiff_file(<path_to_tif_file>)
S.open_log_file(<path_to_log_file>)
S.interp_times() # might take a while...
frame_0 = S.get_frame(0) # returns a numpy int16 array
```

Master branch
-------------

There are a bunch of extra things you can install to speed up openCV before actually installing it including
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
cmake -DOPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules -D CMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ../opencv/
make -j8
sudo make install
```

To install the master branch:

```
git clone https://github.com/rhayman/ScanImageTiffIO.git
cd ScanImageTiffIO
git checkout master
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

You should now be able to include the library in a C++ project:

```c++
#include <ScanImageTiff.h>
```
