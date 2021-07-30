#ifndef SCANIMAGETIFF_H_
#define SCANIMAGETIFF_H_

#include <tiffio.h>
#include <iostream>
#include <string>
#include <map>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
#include <boost/regex.hpp>

#include <armadillo>

#include <vector>
#include <sstream>


// Some string utilities
// Split a string given a delimiter and either return in a
// pre-constructed vector (#1) or returns a new one (#2)
inline void split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim))
		elems.push_back(item);
};

inline std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

static std::string grabStr(const std::string & source, const std::string & target) {
	std::size_t start = source.find(target);
	if ( start != std::string::npos ) {
		std::string fn = source.substr(start + target.length());
		std::size_t newline = fn.find('\n');
		if ( newline != std::string::npos ) {
			fn = fn.substr(0, newline);
			return fn;
		}
		else
			return std::string();
	}
	else
		return std::string();
}

namespace twophoton {
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
		arma::Mat<int16_t> readframe(int framedir=0);
		bool close();
		bool release();
		int getVersion() const { return headerdata->getVersion(); }

		std::string getfilename() const { return m_filename; }
		int getNumFrames(int idx, int & count) const { return headerdata->getNumFrames(m_tif, idx, count); }
		std::vector<double> getAllTimeStamps() const;
		/*
		quickGetNumFrames - called when a tif file is opened and counts the number of
		directories in a tif file. should scrape the tiff headers for all pertinent information
		so this operation is done only once at load time
		TODO: rename sensibly and fill out all vectors etc
		*/
		int scrapeHeaders(int & count) const { return headerdata->scrapeHeaders(m_tif, count); }
		int countDirectories(int & count) const { return headerdata->countDirectories(m_tif, count); }
		unsigned int getSizePerDir(int dirnum=0) const { return headerdata->getSizePerDir(m_tif, dirnum); }
		std::map<int, std::pair<int, int>> getChanLut() const { return headerdata->getChanLut(); }
		std::map<unsigned int, unsigned int> getSavedChans() const { return headerdata->getChanSaved(); }
		std::map<int, int> getChanOffsets() const { return headerdata->getChanOffsets(); }

		/*
		The first argument is the directory in the tiff file you want the frame number & timestamp for
		which are the last two args
		*/
		void getFrameNumAndTimeStamp(const unsigned int, unsigned int &, double &) const;

		void printHeader(int framenum) const { headerdata->printHeader(m_tif, framenum); }
		void printImageDescriptionTag() const { std::string tag; headerdata->getImageDescTag(m_tif); }
		std::string getSWTag(int n) const { return headerdata->getSoftwareTag(m_tif, n); }
		std::string getImDescTag(int n) const { return headerdata->getImageDescTag(m_tif, n); }

		void getImageSize(unsigned int & h, unsigned int & w) const { h = m_imageheight; w = m_imagewidth; }
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
		bool isopened = false;
	};

	class SITiffWriter {
	public:
		SITiffWriter() {};
		virtual ~SITiffWriter();
		bool  write( const arma::Mat<int16_t>& img, const std::vector<int>& params );

		virtual bool isOpened();
		virtual bool open(std::string outputPath);
		virtual bool close();
		virtual void operator << (arma::Mat<int16_t>& frame);
		bool writeSIHdr(const std::string swTag, const std::string imDescTag);
		std::string modifyChannel(std::string &, const unsigned int);

	protected:
		bool writeLibTiff( const arma::Mat<int16_t>& img, const std::vector<int>& params );
		bool writeHdr( const arma::Mat<int16_t>& img );
		std::string m_filename;
		TIFF* m_tif;
		TIFF* pTiffHandle;
		bool opened = false;
	};

	 /* 
	************************* LOG FILE LOADING STUFF *************************
	 
	 In newer iterations of the logfile there's a value near the top
    which gives the value for "North" - this is the token / key 
    defining that value. In older versions this was immediately before
    'MicroscopeTriggered' string - that string now reads 'Started with SpaceBar'

    Another issue is that apparently the rotary encoder or the controller
    that's taking data from it can grab the same sample more than once
    */
    static constexpr char init_rot_token[] = "Angular reference";
    // The token for the amount of rotation
    static constexpr char rot_token[] = "Rot";
    // Token(s) for microscope recording initiation
    static constexpr char scope_token[] = "MicroscopeTriggered";
    static constexpr char spacebar_token[] = "Started with SpaceBar";
    static constexpr char equal_token[] = "=";
    // Mouse movement token(s)
    static constexpr char mouse_move_token[] = "MouseMove";
    static constexpr char X_token[] = "X";
    static constexpr char Z_token[] = " Z=";
    static constexpr char space_token[] = " ";

    // The rotary encoder might (and has) change so the number of units
    // per full rotation might change too
    // static constexpr unsigned int rotary_encoder_units_per_turn = 8845; // the old value
    static constexpr unsigned int rotary_encoder_units_per_turn = 36800; // the new value

    class LogFileLoader
    {
    public:
        LogFileLoader() {};
        LogFileLoader(std::string);
        ~LogFileLoader();
        void setFilename(std::string);
        std::string getFilename() { return filename; }
        bool load();
        void save(std::string);
        int getRotation(int);
        double getRadianRotation(int);
        double getXTranslation(int);
        double getZTranslation(int);
        double getTime(int);
        std::vector<double> getX();
        std::vector<double> getZ();
        std::vector<double> getTheta();
        std::vector<int> getLineNums();
        std::vector<double> getTimes();
        int findIndexOfNearestDuration(double /* frame acquisition time in fractional seconds - a key in the tiff header*/);
        int getTriggerIndex();
        boost::posix_time::ptime getTriggerTime();
        std::vector<boost::posix_time::ptime> getPTimes();
        bool containsAcquisition();
        bool interpTiffData(std::vector<double> /*timestamps from tiff headers*/);
        /*
        Interpolate x, z, and theta based on tiff header
        timestamps (taken from acquisition time of frame from scanimage)
        */
        void interpolatePositionData(std::vector<double>);
        /* given a tiff file timestamp returns the nearest index in the logfile
        that matches that timestamp
        */
        int findNearestIdx(double tiffTimestamp);
        //saves rotation matrices to file with centre to file in outPath...
        // void saveRotationMats(cv::Point centre, std::string outPath);
        // //...and this loads them and returns in the vector
        // std::vector<cv::Mat> loadRotationMats(std::string filePath);
        void saveRaw(std::string fname);
        bool isloaded = false;
        // findStableFrames: pairs of start and end frames for frames with no head rotation
        std::vector<std::pair<int, int>> findStableFrames(const unsigned int minFrames, const double minAngle=1e-3);
    private:
        // processData converts log file times to fractional seconds
        // and converts rotations from log file units (rotary encoder units)
        // into radians
        bool calculateDurationsAndRotations();
        bool m_interpolationDone = false;
        std::string filename;
        std::vector<int> logfile_line_numbers;
        std::vector<int> rotation;
        std::vector<double> x_translation;
        std::vector<double> z_translation;
        std::vector<double> rotation_in_rads;
        std::vector<boost::posix_time::ptime> ptimes;
        std::vector<double> times;
        int trigger_index = 0;
        std::ofstream out_file;
        std::ifstream in_file;
        int idx = 0;//index for location into various vectors
        int init_rotation = 0;
        bool hasAcquisition = false;
        boost::posix_time::ptime trigger_ptime;
        void setTriggerIndex(int);
    };

	/*
	This class holds the transformations that are to be applied to the images in a 2-photon (2P)
	video file (a .tiff file) recorded from a rig that allows the animal to rotate using a bearing
	that allows full 2D exploration of virtual reality (VR) environments. The transformations included are
	described in the scoped enum above and are usually applied in that order. The objective is to
	remove all rotation/ translation from the image so that what is left is a stabilised video file
	that can be subsequently passed through other tools to extract the ROIs, fluorescent traces, do the spike
	deconvolution steps etc.

	This class has serialization methods associated with it - the objective being to store the transformations
	on disk in an .xml file. Each frame of the video file has an x, z and rotation value attached. These are 
	the positional information extracted from the movement of the animal through the VR (x,z) and the rotation
	of the bearing as detected by a rotary encoder. 
	*/

	enum class TransformType : int {
    kInitialRotation,
    kTrackerTranslation,
    kMultiTrackerTranslation,
    kLogPolarRotation,
    kHaimanFFTTranslation,
    kOpticalFlow,
    kHaimanPieceWiseMapping,
	};

	std::vector<std::string> possible_transforms{
		"InitialRotation",
		"TrackerTranslation",
		"MultiTrackerTranslation",
		"HaimanFFTTranslation",
		"LogPolarRotation",
		"OpticalFlow",
		"PieceWiseMapping"
	};

	class TransformContainer
    {
	private:
		using TransformMap = std::map<TransformType, arma::mat>;
		TransformMap m_transforms;
	public:
		double m_timestamp = 0;
		int m_framenumber = 0;
		double m_x = 0;
		double m_z = 0;
		double m_r = 0;
		TransformContainer() {};
		TransformContainer(const int & frame,
			const double & ts) :
			m_framenumber(frame), m_timestamp(ts) {};
	
		~TransformContainer() {};
		void setPosData(double x, double z, double r) {
			m_x = x;
			m_z = z;
			m_r = r;
		};
		void getPosData(double & x, double & z, double & r) {
			x = m_x;
			z = m_z;
			r = m_r;
		};
		// Push a transform type and its contents onto the top of the container
		const TransformMap getTransforms() const { return m_transforms; }
		void addTransform(const TransformType & T, arma::mat M) { m_transforms[T] = M; }
		bool hasTransform(const TransformType & T) {
			auto search = m_transforms.find(T);
			if ( search != m_transforms.end() )
				return true;
			else
				return false;
		};
		// Grab a transform type and its contents
		arma::mat getTransform(const TransformType & T) { 
			auto search = m_transforms.find(T);
			if ( search != m_transforms.end() )
				return m_transforms[T];
			else
				return arma::mat();
		};
		void updateTransform(const TransformType & T, arma::mat M) {
			auto search = m_transforms.find(T);
			if ( search != m_transforms.end() ) {
				if ( T == TransformType::kTrackerTranslation )
					m_transforms[T] = M + m_transforms[T];
				else
					m_transforms[T] = M;
			}
			else {
				addTransform(T, M);
			}
		};
		// I/O methods for when serializing using cv::FileStorage
		// void write(cv::FileStorage & fs) const;
		// void read(const cv::FileNode & node);
	};

	// static void write(cv::FileStorage & fs, const std::string &, const TransformContainer & T) {
	// 	T.write(fs);
	// }

	// static void read(cv::FileNode & node, TransformContainer & T, const TransformContainer & default_value = TransformContainer()) {
	// 	if ( node.empty() )
	// 		T = default_value;
	// 	else
	// 		T.read(node);
	// }

	// "read" method
	// static void operator>>(const cv::FileNode & node, TransformContainer & T) {
	// 	T.m_framenumber = (int)node["Frame"];
	// 	T.m_timestamp = (double)node["Timestamp"];
	// 	T.m_x = (double)node["X"];
	// 	T.m_z = (double)node["Z"];
	// 	T.m_r = (double)node["R"];
	// 	// the transformations
	// 	cv::FileNodeIterator iter = node.begin();
	// 	for(; iter != node.end(); ++iter) {
	// 		cv::FileNode n = *iter;
	// 		std::string name = n.name();
	// 		if ( name.compare("InitialRotation") == 0 )
	// 			T.addTransform(TransformType::kInitialRotation, n.mat());
	// 		if ( name.compare("TrackerTranslation") == 0 )
	// 			T.addTransform(TransformType::kTrackerTranslation, n.mat());
	// 		if ( name.compare("MultiTrackerTranslation") == 0 )
	// 			T.addTransform(TransformType::kMultiTrackerTranslation, n.mat());
	// 		if ( name.compare("HaimanFFTTranslation") == 0 )
	// 			T.addTransform(TransformType::kHaimanFFTTranslation, n.mat());
	// 		if ( name.compare("LogPolarRotation") == 0 )
	// 			T.addTransform(TransformType::kLogPolarRotation, n.mat());
	// 		if ( name.compare("OpticalFlow") == 0 )
	// 			T.addTransform(TransformType::kOpticalFlow, n.mat());
	// 		if ( name.compare("PieceWiseMapping") == 0 )
	// 			T.addTransform(TransformType::kHaimanPieceWiseMapping, n.mat());
	// 	}
	// }

	// "write" method
	static std::ostream& operator<<(std::ostream & out, const TransformContainer & T) {
		out << "{ Frame " << T.m_framenumber << ", ";
		out << "Timestamp " << T.m_timestamp << ", ";
		out << "X" << T.m_x << ", ";
		out << "Z" << T.m_z << ", ";
		out << "R" << T.m_r << ", ";
		// the transformations
		auto transforms = T.getTransforms();
		for ( const auto & transform : transforms ) {
			auto transform_type = transform.first;
			auto transform_val = transform.second;
			if ( transform_type == TransformType::kInitialRotation )
				out << "InitialRotation";
			if ( transform_type == TransformType::kTrackerTranslation )
				out << "TrackerTranslation";
			if ( transform_type == TransformType::kMultiTrackerTranslation )
				out << "MultiTrackerTranslation";
			if ( transform_type == TransformType::kHaimanFFTTranslation )
				out << "HaimanFFTTranslation";
			if ( transform_type == TransformType::kLogPolarRotation )
				out << "LogPolarRotation";
			if ( transform_type == TransformType::kOpticalFlow )
				out << "OpticalFlow";
			if ( transform_type == TransformType::kHaimanPieceWiseMapping )
				out << "PieceWiseMapping";
			out << "{:" << transform_val << "}";
		}
		out << "}";
		return out;
	}

	class SITiffIO {
		public:
			bool openTiff (std::string fname);
			bool openLog (std::string fname);
			bool openXML(std::string fname);
			void interpolateIndices();
			unsigned int getNChannels() { return m_nchans; }
			void setChannel(unsigned int i) { channel2display = i; }
			cv::Mat readFrame(int frame_num=0) const;
			std::vector<double> getTimeStamps() const;
			std::vector<double> getX() const;
			std::vector<double> getZ() const;
			std::vector<double> getTheta() const;
			std::vector<double> getFrameNumbers() const;
			// double getTimeStamp(const int) const;
			// double getX(const unsigned int) const;
			// double getZ(const unsigned int) const;
			// double getTheta(const int) const;
			std::tuple<double, double, double> getPos(const unsigned int) const;
			std::tuple<double, double> getTrackerTranslation(const unsigned int) const;
			std::tuple<std::vector<double>, std::vector<double>> getAllTrackerTranslation() const;
		private:
			std::string tiff_fname;
			std::string log_fname;
			unsigned int m_nchans = 1;
			unsigned int channel2display = 1;
			std::shared_ptr<SITiffReader> TiffReader = nullptr;
			std::shared_ptr<LogFileLoader> LogLoader = nullptr;
			std::shared_ptr<std::map<unsigned int, TransformContainer>> m_all_transforms = nullptr;

	};
}; // namespace twophoton
#endif