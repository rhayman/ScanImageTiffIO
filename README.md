Synopsis
========

A shared library written in C/C++ to load data recorded using ScanImage (http://scanimage.vidriotechnologies.com/).

The cmake build downloads the most recent, stable version of libtiff. This means there is effectively no limit to the size of the tiff files that can be loaded. Also there is a very large number of "directories" (libtiff parlance) that can be read (think "frames" in ScanImage parlance). 

Also contains a Python interface. This works well under a normal system-wide install and also under in a virtual environment (I've only tested this with Miniconda so far). Note that the imports in Python might differ slightly if installed in a virtual environment due to differences in folder structure there; instead of 

```python
from scanimagetiffio.scanimagetiffio import SITiffIO
```

it might be


```python
from scanimagetiffio import SITiffIO
```

instead. Try both, see what works.


Installation
============

See below.

Dependencies
============

- gcc or similar compiler

- cmake (https://cmake.org/)

Other dependencies are automatically downloaded when building the project.

Linux
=====

To install:

```shell
git clone https://github.com/rhayman/ScanImageTiffIO.git
cd ScanImageTiffIO
mkdir build
cd build
cmake ..
make
sudo make install
```

If you want to build the documentation (auto-generated using Sphinx) then:

```shell
cd ScanImageTiffIO
cd docs
make html
```

The html documentation will now be in:

```shell
build/html/
```

You can open the index.html file in that folder with a web browser and the documentation should appear

Windows 
=======

Clone the repo using git

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
S.open_tiff_file(<path_to_tif_file>, 'r') # or 'w'
S.open_log_file(<path_to_log_file>) # optional
S.interp_times() # might take a while...but also optional
frame_0 = S.get_frame(1) # 1-indexed, returns a numpy int16 array
```

Here is a brief description of some of the functions exposed by the Python API:

* open_tiff_file(path_to_tifffile: str, mode: str) - Open a tiff file. mode is either "r" or "w" for read or write. Returns True on success

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

The write function, write_frame(destination_file, iframe), should be called with the same instance as the file you opened with open_tiff_file(source_file). This is because there is potentially important header information in the source file that should be copied to the destination file. The call to write_frame() therefore also needs a frame number to know which header to copy from the src to the dst tiff file. If no file is open for reading at the same time as data is written out then there will be only a basic header attached to that directory (i.e. missing all the extra info ScanImage adds).

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
