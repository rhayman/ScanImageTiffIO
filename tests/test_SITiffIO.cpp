#include "../include/ScanImageTiff.h"
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;

const static fs::path tiff_name{"test_file1.tif"};
const static fs::path log_name("test_logfile.txt");
const static fs::path rotary_name("test_rotaryfile.txt");

class TiFFReaderTest : public ::testing::Test {
protected:
  void SetUp() override { assert(fs::exists(tiff_name)); }
  twophoton::SITiffReader R{tiff_name.string()};
};

class SITiffIOTest : public ::testing::Test {
protected:
  void SetUp() override {
    assert(fs::exists(tiff_name));
    S.openTiff(tiff_name.string(), "r");
  }
  void FrameTest() {
    S.openTiff(tiff_name.string(), "r");
    auto f = S.readFrame(1);
    EXPECT_TRUE(f.size() > 0);
  }
  twophoton::SITiffIO S{};
};

TEST_F(TiFFReaderTest, ReadFrame) { auto f = R.readframe(); }

TEST_F(SITiffIOTest, CountDirectories) { EXPECT_GT(S.countDirectories(), 0); }

TEST_F(SITiffIOTest, CloseWriter) { EXPECT_FALSE(S.closeWriterTiff()); }

TEST_F(SITiffIOTest, OpenLogFile) { EXPECT_TRUE(S.openLog(log_name.string())); }

TEST_F(SITiffIOTest, OpenRotaryFile) {
  EXPECT_TRUE(S.openRotary(rotary_name.string()));
}

TEST_F(SITiffIOTest, PositionProcessing) {
  S.openLog(log_name.string());
  S.interpolateIndices(0);
  EXPECT_GT(S.getTiffTimeStamps().size(), 0);
  EXPECT_GT(S.getX().size(), 0);
  EXPECT_GT(S.getZ().size(), 0);
  EXPECT_GT(S.getTheta().size(), 0);
  EXPECT_GT(S.getFrameNumbers().size(), 0);
  EXPECT_GT(std::get<0>(S.getPos(1)), 0);
}
TEST_F(SITiffIOTest, ChannelProcessing) {
  EXPECT_GT(std::get<0>(S.getNChannels()), 0);
  EXPECT_NE(std::get<0>(S.getChannelLUT()), 0);
}

TEST_F(SITiffIOTest, TimeProcessing) {
  std::chrono::system_clock::time_point tp{};
  S.openLog(log_name.string());
  S.openRotary(rotary_name.string());
  EXPECT_GT(S.getEpochTime(), tp);
  EXPECT_GT(S.getRotaryEncoderTriggerTime(), tp);
  EXPECT_GT(S.getLogFileTriggerTime(), tp);

  EXPECT_GT(S.getLogFileTimes().size(), 0);
  EXPECT_GT(S.getRotaryTimes().size(), 0);
}

TEST_F(SITiffIOTest, Tags) {
  EXPECT_STRNE(S.getSWTag(1).c_str(), "blah");
  EXPECT_STRNE(S.getImageDescTag(1).c_str(), "blah");
  EXPECT_NE(S.getChannelLUT().first, -1000000);
}