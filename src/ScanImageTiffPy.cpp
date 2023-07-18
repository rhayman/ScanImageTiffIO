#include "../include/ScanImageTiff.h"
#include <pybind11/chrono.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

PYBIND11_MODULE(scanimagetiffio, m) {

  py::class_<twophoton::SITiffIO>(m, "SITiffIO")
      .def(py::init<>())
      .def("open_tiff_file", &twophoton::SITiffIO::openTiff, "Open a tiff file",
           py::arg("fname"), py::arg("'r' or 'w'"), R"mydelimiter(
Open a tiff file for reading

	Parameters
	----------
	fname: str - the name of the file to open
)mydelimiter")
      .def("close_reader_tif", &twophoton::SITiffIO::closeReaderTiff,
           "Close the tiff reader file")
      .def("close_writer_tif", &twophoton::SITiffIO::closeWriterTiff,
           "Close the writer tiff file")
      .def("open_log_file", &twophoton::SITiffIO::openLog, "Open a log file",
           py::arg("fname"), R"mydelimiter(

	Parameters
	----------
	fname: str - the name of the log file to open. Must match the tiff file or position data will be wrong
)mydelimiter")
      .def("open_rotary_file", &twophoton::SITiffIO::openRotary,
           "Open a rotary encoder log file")
      .def("open_xml_file", &twophoton::SITiffIO::openXML, "Open an xml file",
           py::arg("fname"), R"mydelimiter(
	open_xml_file

	Parameters
	----------
	fname: str - the name of the xml file to open (DEPRECATED). These files were generated to summarise file processing
)mydelimiter")
      .def("get_n_frames", &twophoton::SITiffIO::countDirectories,
           "Count the number of frames")
      .def("set_channel", &twophoton::SITiffIO::setChannel,
           "Sets the channel to take frames from", py::arg("channel"),
           R"mydelimiter(

  Parameters
  ----------
  channel : int
    The channel to take frames from

  See also
  --------
  get_n_channels : get the number of channels available in this file
           )mydelimiter")
      .def_readonly("get_n_channels", &twophoton::SITiffIO::m_nchans,
                    "Get the number of channels")
      .def_readonly("get_display_channel",
                    &twophoton::SITiffIO::channel2display,
                    "Get the channel to display", R"mydelimiter(

  Returns
  -------
  channel : int
    The channel that frames are currently being taken from

  See also
  --------
  get_n_channels : The number of channels available in this file
           )mydelimiter")
      .def("interp_times", &twophoton::SITiffIO::interpolateIndices,
           "Interpolate the times in the tiff frames to events (position and "
           "time) in the log file",
           R"mydelimiter(

  Calculates the positions and rotations that correspond to the timestamps
  present in the tiff file.

  Notes
  -----
  Position (x and y) is taken from the log file and rotational angle from the
  rotary encoder file. If no rotary encoder file is loaded then rotation is 
  taken from the log file, but in recent versions of this software that will 
  always contain a value of 0.
  Because this function has to extract the timestamp for each frame of the 
  tiff file it has to do an exhaustive (and linear) search which can take a 
  long time with large files.
           )mydelimiter")
      .def("get_pos", &twophoton::SITiffIO::getPos,
           "Gets a 3-tuple of X, Z and theta for the given frame",
           py::arg("i_frame"))
      .def("get_tracker", &twophoton::SITiffIO::getTrackerTranslation,
           "Get the x and y translation for a tracked bounding box",
           py::arg("i_frame"))
      .def("get_all_tracker", &twophoton::SITiffIO::getAllTrackerTranslation,
           "Get all the x and y translations for a tracked bounding box")
      .def("get_frame", &twophoton::SITiffIO::readFrame,
           "Gets the data/ image for the given frame", py::arg("i_frame"),
           R"mydelimiter(

	Parameters
	----------
	i_frame: int - the number of the frame to read (1-indexed)
)mydelimiter")
      .def("write_frame", &twophoton::SITiffIO::writeFrame,
           "Write frame to file", py::arg("frame"), py::arg("i_frame"),
           R"mydelimiter(

	Parameters
	----------
	i_frame: int - the number of the frame to write out. 
	
	Note that an instance of scanimagetiffio must have both a file open for reading and
	another open for writing for this function to copy the ScanImage headers as they need to be copied 
	from the former to the latter.
)mydelimiter")
      .def("get_all_x", &twophoton::SITiffIO::getX, "Gets all the X values")
      .def("get_all_z", &twophoton::SITiffIO::getZ, "Gets all the Z values")
      .def("get_all_theta", &twophoton::SITiffIO::getTheta,
           "Gets all the rotational values")
      .def("get_frame_numbers", &twophoton::SITiffIO::getFrameNumbers,
           "Gets all the frame numbers from the interpolated data")
      .def("get_channel_LUT", &twophoton::SITiffIO::getChannelLUT,
           "Gets the channel LUTs", R"mydelimiter(
  Returns
  -------
  2-tuples of the channel look-up-table (LUT) values
           )mydelimiter")
      .def("get_log_times", &twophoton::SITiffIO::getLogFileTimes,
           "Gets the times from the log file", R"mydelimiter(
  Returns
  -------
  A list of datetimes extracted from the logfile
           )mydelimiter")
      .def("get_rotary_times", &twophoton::SITiffIO::getRotaryTimes,
           "Gets the times from the rotary file", R"mydelimiter(
  Returns
  -------
  A list of datetimes extracted from the rotary file
           )mydelimiter")
      .def("get_tiff_times", &twophoton::SITiffIO::getTiffTimeStamps,
           "Gets the times from the tiff file", R"mydelimiter(
  Returns
  -------
  A list of datetimes from the tiff file
           )mydelimiter")
      .def("get_logfile_trigger_time",
           &twophoton::SITiffIO::getLogFileTriggerTime,
           "Gets the time the logfile registered microscope acquisition")
      .def("get_rotary_encoder_trigger_time",
           &twophoton::SITiffIO::getRotaryEncoderTriggerTime,
           "Gets the time the rotary encoder registered acquisition")
      .def("get_epoch_time", &twophoton::SITiffIO::getEpochTime,
           "Gets the epoch time from the tiff header")
      .def("get_sw_tag", &twophoton::SITiffIO::getSWTag,
           "Get the software tag part of the header for frame n")
      .def("get_image_description_tag", &twophoton::SITiffIO::getImageDescTag,
           "Get the image description part of the header for frame n")
      .def("save_tail", &twophoton::SITiffIO::saveTiffTail,
           "Save the last n frames of the tiff file currently open for reading",
           py::arg("n") = 1000, py::arg("fname") = "",
           R"mydelimiter(
    Parameters
    ----------
    n_frames: int - the number of frames at the tail of the file currently open for reading to save
    fname: str - the name of the file to save the last n_frame images to. This will 
    default to the currently open file name with _tail appended just before the file
    type extension)mydelimiter");
}
