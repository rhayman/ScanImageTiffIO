#include <gtest/gtest.h>
#include "../include/ScanImageTiff.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

const static fs::path tiff_name{"test_file1.tif"};
const static fs::path log_name("test_logfile.txt");

class SITiffIOTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        S.openTiff(tiff_name.string(), "r");
    }
    void FrameTest()
    {
        S.openTiff(tiff_name.string(), "r");
        auto f = S.readFrame(1);
        EXPECT_TRUE(f.size() > 0);
    }
    twophoton::SITiffIO S{};
};

// TEST_F(SITiffIOTest, ReadFrame)
// {
//     FrameTest();
// }

TEST_F(SITiffIOTest, CountDirectories)
{
    EXPECT_GT(S.countDirectories(), 0);
}

TEST_F(SITiffIOTest, CloseWriter)
{
    EXPECT_FALSE(S.closeWriterTiff());
}

TEST_F(SITiffIOTest, OpenLogFile)
{
    EXPECT_TRUE(S.openLog(log_name.string()));
}

TEST_F(SITiffIOTest, PositionProcessing)
{
    S.openLog(log_name.string());
    S.interpolateIndices();
    EXPECT_GT(S.getTimeStamps().size(), 0);
    EXPECT_GT(S.getX().size(), 0);
    EXPECT_GT(S.getZ().size(), 0);
    EXPECT_GT(S.getTheta().size(), 0);
    EXPECT_GT(S.getFrameNumbers().size(), 0);
    EXPECT_GT(std::get<0>(S.getPos(1)), 0);
}
TEST_F(SITiffIOTest, ChannelProcessing)
{
    EXPECT_GT(std::get<0>(S.getNChannels()), 0);
    EXPECT_NE(std::get<0>(S.getChannelLUT()), 0);
}