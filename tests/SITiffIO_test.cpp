#include <gtest/gtest.h>
#include "../include/ScanImageTiff.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

const static fs::path tiff_name{fs::current_path().string() + "/../tests/data/test_file1.tif"};
const static fs::path log_name(fs::current_path().string() + "/../tests/data/test_logfile.txt");

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