#include <gtest/gtest.h>
#include "../include/ScanImageTiff.h"
#include <filesystem>

namespace fs = std::filesystem;

fs::path tiff_pname = "data";
tiff_pname /= "test_file1.tif";
fs::path log_pname = "data";
log_pname /= "test_logfile.txt";

std::string tiff_path = tiff_pname.string();
std::string log_path = log_pname.string();

// Test the ctor of TiffReader
TEST(TiffReader ConstructorFromString)
{
    const twophoton::TiffReader reader{tiff_path};
    EXPECT_EQ(0, std::strcmp(reader.getfilename(), tiff_path));
}

// Basic i/o stuff
TEST(TiffReader OpenRelease)
{
    const twophoton::TiffReader reader{tiff_path};
    EXPECT_TRUE(reader.open());
    EXPECT_TRUE(reader.release());
}

TEST(TiffReader OpenClose)
{
    const twophoton::TiffReader reader{tiff_path};
    EXPECT_TRUE(reader.open());
    EXPECT_TRUE(reader.close());
}

TEST(TiffReader ReadHeader)
{
    const twophoton::TiffReader reader{tiff_path};
    EXPECT_TRUE(reader.open());
    EXPECT_TRUE(reader.readheader());
}

// Version check
TEST(TiffReader VersionCheck)
{
    const twophoton::TiffReader reader{tiff_path};
    EXPECT_TRUE(reader.open());
    EXPECT_TRUE(reader.getVersion() > 0);
}

// Tiff file image contents
