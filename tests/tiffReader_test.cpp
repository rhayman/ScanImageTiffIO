#include <gtest/gtest.h>
#include "../include/ScanImageTiff.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

const static fs::path tiff_name{fs::current_path().string() + "/../tests/data/test_file1.tif"};
const static fs::path log_name(fs::current_path().string() + "/../tests/data/test_logfile.txt");

class TiffReaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        R.open();
    }
    void FrameTest()
    {
        R.open();
        auto f = R.readframe(1);
    }
    twophoton::SITiffReader R{tiff_name.string()};
};

TEST_F(TiffReaderTest, FileNameValid)
{
    EXPECT_EQ(0, std::strcmp(R.getfilename().c_str(), tiff_name.string().c_str()));
}

TEST_F(TiffReaderTest, Opened)
{
    EXPECT_TRUE(R.isOpen());
}

TEST_F(TiffReaderTest, ReadHeader)
{
    EXPECT_TRUE(R.readheader());
}

// Version check
TEST_F(TiffReaderTest, VersionCheck)
{
    EXPECT_TRUE(R.getVersion() > 0);
}

// Tiff file image contents
TEST_F(TiffReaderTest, CountDirectories)
{
    EXPECT_TRUE(R.countDirectories() > 0);
}

TEST_F(TiffReaderTest, ScrapeHeaders)
{
    int count = 0;
    EXPECT_EQ(R.scrapeHeaders(count), 0);
}

TEST_F(TiffReaderTest, GetTimeStamps)
{
    EXPECT_TRUE(R.getAllTimeStamps().size() > 0);
}

TEST_F(TiffReaderTest, GetSizePerDirectory)
{
    EXPECT_TRUE(R.getSizePerDir(1) > 0);
}

TEST_F(TiffReaderTest, CheckChannelInfo)
{
    EXPECT_TRUE(R.getChanLut().size() > 0);
    EXPECT_TRUE(R.getSavedChans().size() > 0);
    EXPECT_TRUE(R.getChanOffsets().size() > 0);
}

TEST_F(TiffReaderTest, FrameNumberAndTimestamp)
{
    unsigned int frame_num = 0;
    double timestamp = -1;
    R.getFrameNumAndTimeStamp(2, frame_num, timestamp);
    EXPECT_NE(frame_num, 0);
    EXPECT_NE(timestamp, -1);
}

TEST_F(TiffReaderTest, SWTagNotEmpty)
{
    std::string empty = "";
    EXPECT_STRNE(R.getSWTag(1).c_str(), empty.c_str());
    EXPECT_STRNE(R.getImDescTag(1).c_str(), empty.c_str());
}

TEST_F(TiffReaderTest, ImageSizeNotZero)
{
    unsigned int h, w = 0;
    R.getImageSize(h, w);
    EXPECT_GT(h, 0) << "Image width = " << std::to_string(w);
    EXPECT_GT(w, 0) << "Image height = " << std::to_string(h);
}

// TEST_F(TiffReaderTest, ReadFrame) {
//     FrameTest();
// }
