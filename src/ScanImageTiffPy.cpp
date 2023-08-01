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
      .def("open_tiff_file", &twophoton::SITiffIO::openTiff,
           "Open a TIFF file for reading or writing.",
           py::arg("fname"), py::arg("mode"))
      .def("close_reader_tif", &twophoton::SITiffIO::closeReaderTiff,
           "Close the TIFF reader file.")
      .def("close_writer_tif", &twophoton::SITiffIO::closeWriterTiff,
           "Close the TIFF writer file.")
      .def("open_log_file", &twophoton::SITiffIO::openLog,
           "Open a log file that contains position and time data.",
           py::arg("fname"))
      .def("open_rotary_file", &twophoton::SITiffIO::openRotary,
           "Open a rotary encoder log file that contains rotational angle data.")
      .def("count_directories", &twophoton::SITiffIO::countDirectories,
           "Count the number of frames in the TIFF file.")
      .def("set_channel", &twophoton::SITiffIO::setChannel,
           "Set the channel to take frames from.",
           py::arg("channel"))
      .def("get_n_frames", &twophoton::SITiffIO::countDirectories,
           "Count the number of frames in the TIFF file.")
      .def("get_n_channels", &twophoton::SITiffIO::getNChannels,
           "Get the number of channels available in this file.")
      .def("get_display_channel", &twophoton::SITiffIO::getDisplayChannel,
           "Get the channel that frames are currently being taken from.")
      .def("interp_times", &twophoton::SITiffIO::interpolateIndices,
           "Interpolate the indices of the TIFF frames to events (position and time) in the log file.",
           py::arg("n") = 0)
      .def("get_pos", &twophoton::SITiffIO::getPos,
           "Get the position data for the current frame.",
           py::arg("frame"))
      .def("get_frame", &twophoton::SITiffIO::readFrame,
           "Get the image data for the current frame.",
           py::arg("frame"))
      .def("write_frame", &twophoton::SITiffIO::writeFrame,
           "Write image data to the TIFF file.",
           py::arg("frame"), py::arg("i_frame"))
      .def("get_all_x", &twophoton::SITiffIO::getX,
           "Get all the x position data from the log file.",
           py::return_value_policy::reference_internal)
      .def("get_all_z", &twophoton::SITiffIO::getZ,
           "Get all the x position data from the log file.",
           py::return_value_policy::reference_internal)
      .def("get_all_theta", &twophoton::SITiffIO::getTheta,
           "Get all the x position data from the log file.",
           py::return_value_policy::reference_internal)
      .def("get_all_raw_x", &twophoton::SITiffIO::getRawX,
           "Get all the x position data from the log file.",
           py::return_value_policy::reference_internal)
      .def("get_all_raw_z", &twophoton::SITiffIO::getRawZ,
           "Get all the x position data from the log file.",
           py::return_value_policy::reference_internal)  
      .def("get_tiff_times", &twophoton::SITiffIO::getTiffTimeStamps,
           "Get the times from the TIFF file.",
           py::return_value_policy::reference_internal)
      .def("get_frame_numbers", &twophoton::SITiffIO::getFrameNumbers,
           "Get the frame numbers from the TIFF file.",
           py::return_value_policy::reference_internal)
      .def("get_channel_LUT", &twophoton::SITiffIO::getChannelLUT,
           "Get the channel LUT from the TIFF file.",
           py::return_value_policy::reference_internal)
      .def("get_log_times", &twophoton::SITiffIO::getLogFileTimes,
           "Get the times from the log file.",
           py::return_value_policy::reference_internal)
      .def("get_rotary_times", &twophoton::SITiffIO::getRotaryTimes,
           "Get the times from the log file.",
           py::return_value_policy::reference_internal)
      .def("get_logfile_trigger_time",
           &twophoton::SITiffIO::getLogFileTriggerTime,
           "Get the time the logfile registered microscope acquisition.")
      .def("get_rotary_encoder_trigger_time",
           &twophoton::SITiffIO::getRotaryEncoderTriggerTime,
           "Get the time the rotary encoder registered acquisition.")
      .def("get_epoch_time", &twophoton::SITiffIO::getEpochTime,
           "Get the epoch time from the TIFF header.")
      .def("get_sw_tag", &twophoton::SITiffIO::getSWTag,
           "Get the software tag part of the header for frame n.",
           py::arg("frame"))
      .def("get_image_description_tag", &twophoton::SITiffIO::getImageDescTag,
           "Get the image description part of the header for frame n.",
           py::arg("frame"))
      .def("tail", &twophoton::SITiffIO::tail,
           "Get the last n frames from the file currently open for reading.",
           py::arg("n") = 1000,
           py::return_value_policy::reference_internal,
           R"pbdoc(
           Grabs the last n frames of the file currently open for reading as an ndarray.

           :param n: The number of frames to get.
           :type n: int
           :return: A tuple containing an ndarray of the last n frames and a list of the rotation angles for each frame.
           :rtype: tuple(numpy.ndarray, list[float])
           )pbdoc")
      .def("save_tail", &twophoton::SITiffIO::saveTiffTail,
           "Save the last n frames of the TIFF file currently open for reading.",
           py::arg("n") = 1000, py::arg("fname") = "",
           R"pbdoc(
           Save the last n frames of the TIFF file currently open for reading.

           :param n: The number of frames at the tail of the file currently open for reading to save.
           :type n: int
           :param fname: The name of the file to save the last n_frame images to. This will default to the currently open file name with _tail appended just before the file type extension.
           :type fname: str
           )pbdoc");
}