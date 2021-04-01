#ifndef SCANIMAGETIFF_H_
#define SCANIMAGETIFF_H_

#include <tiffio.h>
#include <iostream>
#include <string>
#include <map>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>
#include <sstream>
// Some string utilities

// Split a string given a delimiter and either return in a
// pre-constructed vector (#1) or returns a new one (#2)
inline void split(const std::string &s, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim))
		elems.push_back(item);
};

inline std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

class SITiffReader;

class SITiffHeader
{
public:
	SITiffHeader(SITiffReader * parent) : m_parent(parent) {}; // created on SITiffReader::open()
	~SITiffHeader() {};
	/*
	The main function that scrapes the header for all the relevant information
	The goal here is to be reduce the number of times the current directory is accessed
	when reading - this was done at least a couple of times per "frame" read which I think
	was slowing the display of each image down reasonably significantly
	*/
	void read(TIFF * m_tif, int dirnum=-1);
	/*
	Only read in the first frames header as most of the data is the
	same for each frame - data that changes includes stuff like
	timestamps
	*/
	std::string getSoftwareTag(TIFF * m_tif, unsigned int dirnum=0);
	/* Different versions of scanimage have different formats for the
	headers - the strings used for the keys of the various parameters
	differ. The first place this is detectable is the ImageDescription
	tag of the tiff headers (to date anyway). Older versions of scanimage
	has a key for numbering frames called "Frame Timestamp(s)"; newer
	versions use "frameNumbers". Also newer versions more correctly fill
	out the Software tag (older versions dont use it at all).
	There are also differences in how the LUTs for each channel are stored.
	Older versions store them as separate entries i.e. chan1LUT = [0 32767],
	chan2LUT = [32767] etc, whereas newer versions store them as a single
	header key/value pair i.e. channelLUT = {[-50 224], [43 124],...}
	Also newer versions have a lot more key/value pairs saved in a header
	including things like channelNames etc that aren't present in older
	versions.
	versionCheck() looks at the first directories header and specifically
	the ImageDescription tag and sets the 'version' variable as well as
	the string keys used in the header (which are version dependent)

	Also gets the image width and height
	*/
	void versionCheck(TIFF * m_tif); // called on SITiffReader::open()
	int getVersion() { return version; }
	/* Gets the imagedescription tag for the directory dirnum
	Note that the directories are zero-indexed but that scanimage one-indexes
	its frame-numbers
	*/
	std::string getImageDescTag(TIFF * m_tif, unsigned int dirnum=0);
	int getNumFrames(TIFF * m_tif, int idx, int & count);
	int scrapeHeaders(TIFF * m_tif, int & count);
	std::map<int, std::pair<int, int>> getChanLut() { return chanLUT; }
	std::map<int, int> getChanOffsets() { return chanOffs; }
	std::map<unsigned int, unsigned int> getChanSaved() { return chanSaved; }
	friend std::ostream & operator << (std::ostream & stream, const SITiffHeader & header)
	{
		stream << header.m_swTag;
		return stream;
	}
	void printHeader(TIFF * m_tif, int framenum);
	std::string grabStr(const std::string & source, const std::string & target);
	unsigned int getSizePerDir(TIFF * m_tif, unsigned int dirnum=0);
	std::vector<double> getTimeStamps() { return m_timestamps; }
	int countDirectories(TIFF *, int &);
	const std::string getFrameNumberString() { return frameString; }
	const std::string getFrameTimeStampString() { return frameTimeStamp; }

private:
	SITiffReader * m_parent;
	// int holding a 'version' value
	int version = -1;
	// string holding the Software tag
	std::string m_swTag;
	// string holding the Image Description tag
	std::string m_imdesc;
	// Utility methods to grab and parse some key/value pairs in the tiff file header
	void parseChannelLUT(std::string); // fills out chanLUT map (see below)
	void parseChannelOffsets(std::string); // fills out chanOffs map (see below)
	void parseSavedChannels(std::string savedchans); // fills out chanSaved map (see below)

	// Member variables
	// target key strings to grab from the tiff header (using grabStr)
	// these are set in versionCheck()
	std::string channelSaved;
	std::string channelLUT;
	std::string channelOffsets;
	std::string channelNames;
	std::string frameString;
	std::string frameTimeStamp;

	// Filled out in scrapeHeaders
	std::vector<double> m_timestamps;

	/*
	Note quite sure what these LUT's are for but they might be so you
	can restrict the range of values (once scaled using the offsets?)
	to look at before doing the remapping of values to uint8's (e.g RGB)
	*/
	std::map<int, std::pair<int, int>> chanLUT;
	/* Channel offsets I think can be subtracted from each channel
	to 'normalize' the distribution of values such that the mean of
	the distribution sits at 0 and positive values are the signal
	*/
	std::map<int, int> chanOffs;
	/*
	The channels saved. Note that if two or more channels are saved then
	channel 1 is saved first then the next frame (TIFF terminology) is
	the second frame and so on, i.e. channels are saved sequentially, and,
	in this example, channel 1 and channel 2 would have been acqured AT THE
	SAME TIME
	the map is 1-indexed for both members
	*/
	std::map<unsigned int, unsigned int> chanSaved;
};

class SITiffReader
{
public:
	SITiffReader(std::string filename) : m_filename(filename) {};
	~SITiffReader();
	bool open();
	bool isOpen() { return isopened; }
	bool readheader();
	cv::Mat readframe(int framedir=0);
	// bool readframe(cv::OutputArray);
	bool close();
	bool release();
	int getVersion() { return headerdata->getVersion(); }

	std::string getfilename() { return m_filename; }
	int getNumFrames(int idx, int & count) { return headerdata->getNumFrames(m_tif, idx, count); }
	std::vector<double> getAllTimeStamps();
	/*
	quickGetNumFrames - called when a tif file is opened and counts the number of
	directories in a tif file. should scrape the tiff headers for all pertinent information
	so this operation is done only once at load time
	TODO: rename sensibly and fill out all vectors etc
	*/
	int scrapeHeaders(int & count) { return headerdata->scrapeHeaders(m_tif, count); }
	int countDirectories(int & count) { return headerdata->countDirectories(m_tif, count); }
	unsigned int getSizePerDir(int dirnum=0) { return headerdata->getSizePerDir(m_tif, dirnum); }
	std::map<int, std::pair<int, int>> getChanLut() { return headerdata->getChanLut(); }
	std::map<unsigned int, unsigned int> getSavedChans() { return headerdata->getChanSaved(); }
	std::map<int, int> getChanOffsets() { return headerdata->getChanOffsets(); }

	/*
	The first argument is the directory in the tiff file you want the frame number & timestamp for
	which are the last two args
	*/
	void getFrameNumAndTimeStamp(const unsigned int, unsigned int &, double &);

	void printHeader(int framenum) { headerdata->printHeader(m_tif, framenum); }
	void printImageDescriptionTag() { std::string tag; headerdata->getImageDescTag(m_tif); }
	std::string getSWTag(int n) { return headerdata->getSoftwareTag(m_tif, n); }
	std::string getImDescTag(int n) { return headerdata->getImageDescTag(m_tif, n); }

	void getImageSize(unsigned int & h, unsigned int & w) { h = m_imageheight; w = m_imagewidth; }
	// called only by SITiffHeader
	void setImageSize(int h, int w) { m_imageheight = h; m_imagewidth = w; }

private:
	SITiffHeader * headerdata = nullptr;
	std::string m_filename;
	TIFF * m_tif = NULL;
	// some values to do with frame size, byte values etc
	unsigned int m_imagewidth;
	unsigned int m_imageheight;
	/* created based on bpp (bytes per pixel), ncn (samples per pixel)
	and photometric (min-is-black etc). Basically this is fixed as
	scanimage uses 16 bpp, 1 sample per pixel and the check for
	photometric only would change this if it's greater than 1 which
	corresponds to things not used in scanimage.
	Photometric interpretation as follows:
	0 = WhiteIsZero. For bilevel and grayscale images: 0 is imaged as white.
	1 = BlackIsZero. For bilevel and grayscale images: 0 is imaged as black.
	More values listed here:
	https://www.awaresystems.be/imaging/tiff/tifftags/photometricinterpretation.html
	*/
	int cv_matrix_type = CV_16SC1;

	bool isopened = false;
};

class SITiffWriter {
public:
	SITiffWriter() {};
	virtual ~SITiffWriter();
	bool isFormatSupported( int depth ) const;
	bool  write( const cv::Mat& img, const std::vector<int>& params );

	virtual bool isOpened();
	virtual bool open(std::string outputPath);
	virtual bool close();
	virtual void operator << (cv::Mat& frame);
    bool writeSIHdr(const std::string swTag, const std::string imDescTag);

protected:
    // void  writeTag( cv::WLByteStream& strm, TiffTag tag,
    //                 TiffFieldType fieldType,
    //                 int count, int value );
    bool writeLibTiff( const cv::Mat& img, const std::vector<int>& params );
    bool writeHdr( const cv::Mat& img );
    std::string type2str(int type);
    std::string m_filename;
	TIFF* m_tif;
	TIFF* pTiffHandle;
	bool opened = false;
};

#endif