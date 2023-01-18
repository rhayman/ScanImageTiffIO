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

- cmake (https://cmake.org/)

- libtiff (http://www.libtiff.org/)

Linux
=====

To install the armadillo branch:

```
git clone --recurse-submodules https://github.com/rhayman/ScanImageTiffIO.git
cd ScanImageTiffIO
git checkout armadillo
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
sudo make install
```

Windows 
=======

Clone the repo using git and update all the submodules as above

Use CMake GUI to configure and generate the solution for Visual Studio

Build the project in Visual Studio and then Install it.

Once installed it is necessary to copy the tiff.dll file into the same folder as the 
.pyd file that results from the install step.

On one of my windows machines the install location was:

C:\User\Robin\AppData\Local\Programs\Python\Python38\Lib\site-packages\scanimagetiffio

which contained

* scanimagetiffio.cp38-win_amd64.pyd
* scanimagetiffio.lib

You need to find and copy the tiff.dll file into the above folder.

Usage
=====

```python
from scanimagetiffio.scanimagetiffio import SITiffIO
S = SITiffIO()
S.open_tiff_file(<path_to_tif_file>)
S.open_log_file(<path_to_log_file>)
S.interp_times() # might take a while...
frame_0 = S.get_frame(1) # 1-indexed, returns a numpy int16 array
```

Here is a brief description of some of the functions exposed by the Python API:

* open_tiff_file(path_to_tifffile: str) - Open a tiff file. Returns True on success
* write_to_tiff(path_to_tifffile: str) - Open a tiff file for writing. NB Doesn't have to exist before this call. Returns True on success
* open_log_file(path_to_logfile: str) - Open a log file. Returns True on success
* open_xml_file(path_to_xmlfile: str) - Open an xml file - ignore
* get_n_frames() - Count the total number of frames in the tiff file. Returns int
* get_frame(n: int) - Gets the data/ image for the given frame. Retuens numpy array
* write_frame(f: np.array, n: int) - Writes a frame of data to the file supplied in the call to write_to_tiff(). The n argument refers to the frame in the source file (opened with the call to open_tiff_file()) that the headers should be copied from.
* set_channel(n: int) - Sets the channel to take frames from (see below)
* interp_times() - Interpolate the times in the tiff frames to events (position and time in the log file)

The following functions require the interp_times() function to have been called as this interpolates between the timestamps in the tiff and log files to calculate which positions from the log file relate to which directories in the tiff file:

* get_pos(n: int) - Gets a 3-tuple of X, Z and theta for the given frame. Returns 3-tuple
* get_tracker() - Get the x and y translation for a tracked bounding box. ignore
* get_all_tracker() - Get all the x and y translations for a tracked bounding box. ignore
* get_all_x() - Gets all the X values. Returns numpy array
* get_all_z() - Gets all the Z values. Returns numpy array
* get_all_theta() - Gets all the rotational values. Returns numpy array
* get_frame_numbers() - Gets all the frame numbers from the interpolated data. Returns numpy array
* get_all_timestamps() - Gets all the timestamps. Returns numpy array
* get_channel_LUT() - Gets the channel LUTs. Returns 2-tuple

NB A distinction should be made between "frames" and "directories". Frames can be thought of as slices in time whereas there can be >1 directory for a given slice of time. Less abstractly, you can think of a directory as an inidividual image in a multi-page tiff file and a frame as a single timestamps worth of acquisition data from the microscope. So, if 2 channels (red and green say) have been recorded from the microscope there will be 2 directories per frame.

The write function, write_frame(destination_file, iframe), should be called with the same instance as the file you opened with open_tiff_file(source_file). This is because there is potentially important header information in the source file that should be copied to the destination file. The call to write_frame() therefore also needs a frame number to know which header to copy from the src to the dst tiff file.

The indexing in the Python API takes account of this and uses frames as the index to retrieve directories, correctly taking account of the number of channels recorded from. If you want to examine the first frame of data for channel 1 you can call do:

```python
set_channel(1)
S.get_frame(1)
```

Similarly, for channel 2:

```python
set_channel(2)
S.get_frame(1)
```

In the first example this is retrieving tiff directory 0 (libtiff is 0-indexed) and in the second directory 1. If you ask for a frame > than the total number of frames then an empty array is returned.

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
