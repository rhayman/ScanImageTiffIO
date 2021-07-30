#include "../include/ScanImageTiff.h"
#include "../include/rapidxml.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/stl_bind.h>

#include "../include/ndarray_converter.h"

#include <stdio.h>
#include <stdio_ext.h>
#include <limits>
#include <tuple>
#include <memory>
#include <algorithm>

namespace twophoton {

	void SITiffHeader::read(TIFF * m_tif, int dirnum)
	{
		if ( m_tif )
		{
			if ( dirnum == -1 )
				TIFFSetDirectory(m_tif, dirnum);
			else
			{
				// scrape some big-ish strings out of the header
				char * imdesc;
				if ( TIFFGetField(m_tif, TIFFTAG_IMAGEDESCRIPTION, &imdesc) == 1)
				{
					m_imdesc = imdesc;
				}
				else
					return;
				// get / set frame number and timestamp

				// get / set image size
				int length, width;
				TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &length);
				TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
				//
			}
		}
	}

	void SITiffHeader::versionCheck(TIFF * m_tif)
	{
		if ( m_tif )
		{
			m_imdesc = getImageDescTag(m_tif, 0);
			if ( ! grabStr(m_imdesc, "Frame Number =").empty() ) // old
			{
				version = 0;
				channelSaved = "scanimage.SI5.channelsSave =";
				channelLUT = "scanimage.SI5.chan1LUT =";
				channelOffsets = "scanimage.SI5.channelOffsets =";
				frameString = "Frame Number =";
				frameTimeStamp = "Frame Timestamp(s) =";
			}
			else if ( ! grabStr(m_imdesc, "frameNumbers =").empty() ) // new
			{
				version = 1;
				channelSaved = "SI.hChannels.channelSave =";
				channelLUT = "SI.hChannels.channelLUT =";
				channelOffsets = "SI.hChannels.channelOffset =";
				channelNames = "SI.hChannels.channelName =";
				frameString = "frameNumbers =";
				frameTimeStamp = "frameTimestamps_sec =";
			}

			uint32 length;
			uint32 width;
			TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &length);
			TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
			m_parent->setImageSize(length, width);
		}
	}

	std::string SITiffHeader::getSoftwareTag(TIFF * m_tif, unsigned int dirnum)
	{
		if ( m_tif )
		{
			TIFFSetDirectory(m_tif, dirnum);
			if ( version == 0 )
			{
				// with older versions the information for channels live
				// in a different tag (ImageDescription) and some of that
				// information doesn't even exist so has to be inferred...
				std::string imdesc = getImageDescTag(m_tif, dirnum);
				if ( ! imdesc.empty() )
				{
					std::string chanSave = grabStr(imdesc, channelSaved);
					chanSaved[0] = 1;

					std::string chanLUTs = grabStr(imdesc, channelLUT);
					parseChannelLUT(chanLUTs);

					std::string chanOffsets = grabStr(imdesc, channelOffsets);
					parseChannelOffsets(chanOffsets);
					return imdesc;
				}
				else
					return std::string();

			}
			else if ( version == 1 )
			{
				char * swTag;
				if ( TIFFGetField(m_tif, TIFFTAG_SOFTWARE, & swTag) == 1 )
				{
					m_swTag = swTag;
					std::string chanNames = grabStr(m_swTag, channelNames);
					std::string chanLUTs = grabStr(m_swTag, channelLUT);
					std::string chanSave = grabStr(m_swTag, channelSaved);
					std::string chanOffsets = grabStr(m_swTag, channelOffsets);
					parseChannelLUT(chanLUTs);
					parseChannelOffsets(chanOffsets);
					parseSavedChannels(chanSave);
					return m_swTag;
				}
				else
					return std::string();
			}
			else
				return std::string();
		}
		return std::string();
	}

	std::string SITiffHeader::getImageDescTag(TIFF * m_tif, unsigned int dirnum)
	{
		if ( m_tif )
		{
			TIFFSetDirectory(m_tif, dirnum);
			char * imdesc;
			if ( TIFFGetField(m_tif, TIFFTAG_IMAGEDESCRIPTION, &imdesc) == 1)
			{
				m_imdesc = imdesc;
				return imdesc;
			}
			else {
				std::cout << "Image description tag empty\n";
				return std::string();
			}
		}
		std::cout << "m_tif not valid\n";
		return std::string();
	}

	unsigned int SITiffHeader::getSizePerDir(TIFF * m_tif, unsigned int dirnum)
	{
		if ( m_tif )
		{
			TIFFSetDirectory(m_tif, dirnum);
			uint32 length;
			uint32 width;
			TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &length);
			TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
			// hard-code the sample format (signed int) as SI seems to only
			// use this which is of size 4
			return (length * width * 4);
		}
		else
			return 0;
	}

	void SITiffHeader::printHeader(TIFF * m_tif, int framenum)
	{
		if ( m_tif )
		{
			TIFFSetDirectory(m_tif, framenum);
			TIFFPrintDirectory(m_tif, stdout, 0);
		}
	}

	int SITiffHeader::getNumFrames(TIFF * m_tif, int idx, int & count)
	{
		if ( m_tif )
		{
			TIFFSetDirectory(m_tif, idx);
			if ( TIFFReadDirectory(m_tif) == 1 )
				++count;
			else
				TIFFSetDirectory(m_tif, 0);
			return 0;
		}
		return 1;
	}

	int SITiffHeader::countDirectories(TIFF * m_tif, int & count)
	{
		if ( m_tif )
		{
			do {
				++count;
			}
			while ( TIFFReadDirectory(m_tif) == 1 );
		}
		return 1;
	}

	int SITiffHeader::scrapeHeaders(TIFF * m_tif, int & count)
	{
		if ( m_tif )
		{
			if ( TIFFReadDirectory(m_tif) == 1 )
			{
				std::string imdescTag = getImageDescTag(m_tif, count);
				if ( ! imdescTag.empty() )
				{
					std::string ts_str = grabStr(imdescTag, frameTimeStamp);
					if ( ! ts_str.empty() )// sometimes headers are corrupted esp. at EOF
					{
						double ts = std::stof(ts_str);
						m_timestamps.emplace_back(ts);
						++count;
						// std::cout << count << "\t";
						return 0;
					}
					else
						return 1;
				}
				else
					return 1;
			}
			else
				return 1;
		}
		return 1;
	}

	void SITiffHeader::parseChannelLUT(std::string LUT)
	{
		LUT.pop_back();
		LUT = LUT.substr(2);
		auto LUT_split = split(LUT, ' ');
		for (auto & x : LUT_split)
		{
			if (x.find('[') != std::string::npos)
				x = x.substr(1);
			if (x.find(']') != std::string::npos)
				x.pop_back();
		}
		int count = 1;
		for (unsigned int i = 0; i < LUT_split.size(); i+=2)
		{
			chanLUT[count] = std::make_pair<int,int>(std::stoi(LUT_split[i]), std::stoi(LUT_split[i+1]));
			++count;
		}
	}

	void SITiffHeader::parseChannelOffsets(std::string offsets)
	{
		offsets.pop_back();
		offsets = offsets.substr(2);
		auto offsets_split = split(offsets, ' ');
		int count = 1;
		for ( auto & x : offsets_split)
		{
			if (x.find('[') != std::string::npos)
				x = x.substr(1);
			if (x.find(']') != std::string::npos)
				x.pop_back();
			chanOffs[count] = std::stoi(x);
			++count;
		}
	}

	void SITiffHeader::parseSavedChannels(std::string savedchans)
	{
		// in case only a single channel has been saved
		if ( savedchans.size() == 2 ) {
			chanSaved[0] = std::stoul(savedchans.substr(1));
		}
		else {
			savedchans.pop_back();
			savedchans = savedchans.substr(2);
			auto savedchans_split = split(savedchans, ';');
			unsigned int count = 0;
			for (auto & x: savedchans_split)
			{
				chanSaved[count] = std::stoul(x);
				++count;
			}
		}
	}

	/* -----------------------------------------------------------
	class SITiffReader
	------------------------------------------------------------*/
	SITiffReader::~SITiffReader()
	{
		if ( headerdata )
			delete headerdata;
	}
	bool SITiffReader::open()
	{
		m_tif = TIFFOpen(m_filename.c_str(), "r");
		if ( m_tif )
		{
			headerdata = new SITiffHeader{this};
			headerdata->versionCheck(m_tif);
			isopened = true;
			return true;
		}
		return false;
	}

	bool SITiffReader::readheader()
	{
		if ( m_tif )
		{
			std::string softwareTag = headerdata->getSoftwareTag(m_tif);
			return true;
		}
		return false;
	}

	std::vector<double> SITiffReader::getAllTimeStamps() const {
		if ( m_tif ) {
			std::cout << "Starting scraping timestamps..." << std::endl;
			TIFFSetDirectory(m_tif, 0);
			int count = 0;
			do {}
			while ( headerdata->scrapeHeaders(m_tif, count) == 0 );
			std::cout << "Finished scraping timestamps..." << std::endl;
			return headerdata->getTimeStamps();
		}
		return std::vector<double>();
	}

	void SITiffReader::getFrameNumAndTimeStamp(const unsigned int dirnum, unsigned int & framenum, double & timestamp) const
	{
		if ( m_tif )
		{
			// strings to grab from the header
			const std::string frameNumberString = headerdata->getFrameNumberString();
			const std::string frameTimeStampString = headerdata->getFrameTimeStampString();
			std::string imdescTag = headerdata->getImageDescTag(m_tif, dirnum);
			std::string frameN;
			std::string ts;
			frameN = grabStr(imdescTag, frameNumberString);
			ts = grabStr(imdescTag, frameTimeStampString);
			framenum = std::stoi(frameN);
			timestamp = std::stof(ts);
		}
	}

	bool SITiffReader::release()
	{
		if ( m_tif )
		{
			TIFFClose(m_tif);
			return true;
		}
		else
			return false;
	}

	arma::Mat<int16_t> SITiffReader::readframe(int framedir) {
		if ( m_tif ) {
			arma::Mat<int16_t> frame;
			int framenum = framedir;
			/*
			From the man pages for TIFFSetDirectory:

			TIFFSetDirectory changes the current directory and reads its contents with TIFFReadDirectory.
			The parameter dirnum specifies the subfile/directory as an integer number, with the first directory numbered zero.

			NB This differs from the 1-based indexing for framenumbers that ScanImage uses (fucking Matlab)
			*/
			TIFFSetDirectory(m_tif, framenum);
			uint32 w = 0, h = 0;
			uint16 photometric = 0;
			if( TIFFGetField( m_tif, TIFFTAG_IMAGEWIDTH, &w ) && // normally = 512
				TIFFGetField( m_tif, TIFFTAG_IMAGELENGTH, &h ) && // normally = 512
				TIFFGetField( m_tif, TIFFTAG_PHOTOMETRIC, &photometric )) // photometric = 1 (min-is-black)
			{
				m_imagewidth = w;
				m_imageheight = h;

				uint16 bpp=8, ncn = photometric > 1 ? 3 : 1;
				TIFFGetField( m_tif, TIFFTAG_BITSPERSAMPLE, &bpp ); // = 16
				TIFFGetField( m_tif, TIFFTAG_SAMPLESPERPIXEL, &ncn ); // = 1
				int is_tiled = TIFFIsTiled(m_tif); // 0 ie false, which means the data is organised in strips
				uint32 tile_height0 = 0, tile_width0 = m_imagewidth;
				TIFFGetField(m_tif, TIFFTAG_ROWSPERSTRIP, &tile_height0);

				if( (!is_tiled) ||
					(is_tiled &&
					TIFFGetField( m_tif, TIFFTAG_TILEWIDTH, &tile_width0) &&
					TIFFGetField( m_tif, TIFFTAG_TILELENGTH, &tile_height0 )))
				{
					if(!is_tiled)
						TIFFGetField( m_tif, TIFFTAG_ROWSPERSTRIP, &tile_height0 );

					if( tile_width0 <= 0 )
						tile_width0 = m_imagewidth;

					if( tile_height0 <= 0 ||
					(!is_tiled && tile_height0 == std::numeric_limits<uint32>::max()) )
						tile_height0 = m_imageheight;

					const size_t buffer_size = bpp * ncn * tile_height0 * tile_width0;

					std::unique_ptr<int16_t[]> buffer = std::make_unique<int16_t[]>(buffer_size);
					int tileidx = 0;

					// ********* return frame created here ***********

					frame = arma::Mat<int16_t>(h, w);
					int16_t* data = frame.memptr();

					for (unsigned int y = 0; y < m_imageheight; y+=tile_height0)
					{
						unsigned int tile_height = tile_height0;


						if( y + tile_height > m_imageheight )
							tile_height = m_imageheight - y;
						// tile_height is always equal to 8

						for(unsigned int x = 0; x < m_imagewidth; x += tile_width0, tileidx++)
						{
							unsigned int tile_width = tile_width0, ok;

							if( x + tile_width > m_imagewidth )
								tile_width = m_imagewidth - x;
							// I've cut out lots of bpp testing etc here
							// tileidx goes from 0 to 63
							// buffer has 4096 int16's in
							ok = (int)TIFFReadEncodedStrip(m_tif, tileidx, buffer.get(), buffer_size ) >= 0;
							if ( !ok )
							{
								close();
								return arma::Mat<int16_t>();
							}
							std::memcpy(data + tileidx*(tile_height*tile_width),
										buffer.get(),
										(tile_height*tile_width)*sizeof(buffer.get()));
						}
					}

				}
			}
			return frame;
		}
		return arma::Mat<int16_t>();
	}

	bool SITiffReader::close() {
		TIFFClose(m_tif);
		isopened = false;
		if ( headerdata )
			delete headerdata;
		return true;
	}
	// ######################################################################
	// ################## Tiff encoder class ################################
	// ######################################################################

	static void readParam(const std::vector<int>& params, int key, int& value) {
		for(size_t i = 0; i + 1 < params.size(); i += 2)
			if(params[i] == key)
			{
				value = params[i+1];
				break;
			}
	}

	SITiffWriter::~SITiffWriter() {
		if ( opened )
			TIFFClose(m_tif);
	}

	bool SITiffWriter::writeLibTiff(const arama::Mat<int16_t> & img, const std::vector<int> & params) {
		int channels = 1;
		int width = img.n_cols, height = img.n_rows;
		int bitsPerChannel = 16 ;
		const int bitsPerByte = 16;
		size_t fileStep = (width * channels * bitsPerChannel) / bitsPerByte;// = image_width (with 1 channel)

		int rowsPerStrip = (int)((1 << 13)/fileStep);
		readParam(params, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);
		rowsPerStrip = height;
		if (!(isOpened()))
			pTiffHandle = TIFFOpen(m_filename.c_str(), "w8");
		else
			pTiffHandle = m_tif;
		if (!pTiffHandle)
			return false;

		// defaults for now, maybe base them on params in the future
		int    compression  = COMPRESSION_NONE;
		int    predictor    = PREDICTOR_HORIZONTAL;
		int    units        = RESUNIT_INCH;
		double xres         = 72.0;
		double yres         = 72.0;
		int    sampleformat = SAMPLEFORMAT_INT;
		int    orientation  = ORIENTATION_TOPLEFT;
		int planarConfig = 1;

		readParam(params, TIFFTAG_COMPRESSION, compression);
		readParam(params, TIFFTAG_PREDICTOR, predictor);

		int   colorspace = channels > 1 ? PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK;

		if ( !TIFFSetField(pTiffHandle, TIFFTAG_IMAGEWIDTH, width)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_IMAGELENGTH, height)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_BITSPERSAMPLE, bitsPerChannel)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_COMPRESSION, compression)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_PHOTOMETRIC, colorspace)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_SAMPLESPERPIXEL, channels)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_PLANARCONFIG, planarConfig)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_ROWSPERSTRIP, rowsPerStrip)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_RESOLUTIONUNIT, units)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_XRESOLUTION, xres)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_YRESOLUTION, yres)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_SAMPLEFORMAT, sampleformat)
		|| !TIFFSetField(pTiffHandle, TIFFTAG_ORIENTATION, orientation)
		)
		{
			TIFFClose(pTiffHandle);
			return false;
		}

		if (compression != COMPRESSION_NONE && !TIFFSetField(pTiffHandle, TIFFTAG_PREDICTOR, predictor) )
		{
			TIFFClose(pTiffHandle);
			return false;
		}

		const size_t buffer_size = bitsPerChannel * channels * rowsPerStrip * width;
		std::unique_ptr<int16_t[]> buffer = std::make_unique<int16_t[]>(buffer_size);
		int tileidx = 0;
		int16_t * data = img.memptr();
		
		if (!data)
		{
			TIFFClose(pTiffHandle);
			return false;
		}

		for (unsigned int y = 0; y < height; y += rowsPerStrip) {
			unsigned int tile_height0 = rowsPerStrip;
			if ( y + tile_height0 > height )
				tile_height0 = height - y;
			for (unsigned int x = 0; x < width; x+= width, tileidx++) {
				unsigned int tile_width0 = width, ok;
				if ( x + tile_width0 > width )
					tile_width0 - width - x;
				std::memcpy(buffer.get(),
					data + tileidx*(tile_height0*tile_width0),
					buffer_size);
				ok = (int)TIFFWritedEncodedStrip(pTiffHandle, tileidx, buffer.get(), buffer_size) >=0;
				if ( !ok ) {
					TIFFClose(pTiffHandle);
					return false;
				}
			}
		}
		TIFFWriteDirectory(pTiffHandle); // write into the next directory
		return true;
	}

	bool SITiffWriter::writeHdr(const arma::Mat<int16_t> & img) {
		// IMPORTANT: Note the "w8" option here - this is what allows writing to the bigTIFF format
		// possible ('normal' tiff would be just "w")
		if (!(isOpened()))
			m_tif = TIFFOpen(m_filename.c_str(), "w8");
		if (!m_tif)
			return false;

		TIFFWriteDirectory(pTiffHandle);
		TIFFSetField(m_tif, TIFFTAG_IMAGEWIDTH, img.n_cols);
		TIFFSetField(m_tif, TIFFTAG_IMAGELENGTH, img.n_rows);
		TIFFSetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(m_tif, TIFFTAG_COMPRESSION, 1);
		TIFFSetField(m_tif, TIFFTAG_PHOTOMETRIC, 1);
		TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, 8);
		return true;
	}

	bool SITiffWriter::writeSIHdr(const std::string swTag, const std::string imDescTag) {
		auto _swTag = swTag.c_str();
		auto _imDescTag = imDescTag.c_str();
		if ( ( TIFFSetField(m_tif, TIFFTAG_IMAGEDESCRIPTION, _imDescTag) > 0 ) &&
			( TIFFSetField(m_tif, TIFFTAG_SOFTWARE, _swTag) > 0 ) )
			return true;
		return false;
	}

	std::string SITiffWriter::modifyChannel(std::string & src_str, const unsigned int chan2keep) {
		auto chan = std::to_string(chan2keep);
		std::string target = "SI.hChannels.channelSave = ";
		std::string replace_with = target + chan;
		std::string whole_target = grabStr(src_str, target);
		whole_target = target + whole_target;

		auto loc = src_str.find(target);
		if ( loc != std::string::npos ) {
			src_str.replace(loc, whole_target.size(), replace_with);
		}
		return whole_target;
	}

	bool SITiffWriter::write( const arma::Mat<int16_t>& img, const std::vector<int>& params)
	{
		return writeLibTiff(img, params);
	}

	bool SITiffWriter::isOpened()
	{
		if (opened)
			return true;
		else
			return false;
	}

	bool SITiffWriter::open(std::string outputPath)
	{
		if (!(opened))
		{
			// IMPORTANT: Note the "w8" option here - this is what allows writing to the bigTIFF format
			// possible ('normal' tiff would be just "w")
			m_tif = TIFFOpen(outputPath.c_str(), "w8");
			m_filename = outputPath;
			opened = true;
		}
		return opened;
	}

	bool SITiffWriter::close() {
		if ( opened ) {
			TIFFClose(m_tif);
			opened = false;
		}
		return opened;
	}

	void SITiffWriter::operator << (arma::Mat<int16_t>& frame)
	{
		if (opened)
		{
			std::vector<int> params;
			write(frame, params);
		}
	}
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/  +++++++++++++++++++++++  SITiffIO  +++++++++++++++++++++++++++++++++++
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

	bool SITiffIO::openTiff(std::string fname) {
		tiff_fname = fname;
		TiffReader = std::make_shared<SITiffReader>(fname);
		if ( TiffReader->open() ) {
			auto chans = TiffReader->getSavedChans();
			m_nchans = chans.size();
			return true;
		}
		return false;
	}

	bool SITiffIO::openLog(std::string fname) {
		log_fname = fname;
		LogLoader = std::make_shared<LogFileLoader>(fname);
		return LogLoader->load();
	}

	bool SITiffIO::openXML(std::string fname) {
		if ( m_all_transforms == nullptr )
			m_all_transforms = std::make_shared<std::map<unsigned int, TransformContainer>>();
		else
			m_all_transforms->clear();

		xml_document<> doc;
		xml_node<> * root_node;
		xml_node<> * summary_node;
		xml_node<> * transform_node;

		std::ifstream ifs(fname);
		std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(&buffer[0]);
		root_node = doc.first_node("opencv_storage");
		summary_node = root_node->first_node("Summary");
		transform_node = summary_node->next_sibling();

		for (xml_node<> * underscore_node = transform_node->first_node("_"); underscore_node != nullptr; underscore_node = underscore_node->next_sibling()) {
			for( xml_node<> * frame_node = underscore_node->first_node("Frame"); frame_node != nullptr; frame_node = frame_node->next_sibling()) {
				std::cout << frame_node->name() << ": " << frame_node->value() << std::endl;
				if ( std::find(possible_transforms.begin(), possible_transforms.end(), frame_node->name()) != possible_transforms.end() ) {
					// found one of the possible transforms, calculate the size of the mat to fit the values in
					int n_rows = std::stoi(frame_node->first_node("rows")->value());
					int n_cols = std::stoi(frame_node->first_node("cols")->value());
					

				}
			}
		}
		cv::FileStorage storage(fname, cv::FileStorage::READ);
		cv::FileNode node = storage["Transformations"];
		cv::FileNodeIterator iter = node.begin(), iter_end = node.end();
		
		for(; iter != iter_end; ++iter) {
			TransformContainer tc_node;
			*iter >> tc_node;
			m_all_transforms->emplace(tc_node.m_framenumber, tc_node);
		}
		return true;
	}

	cv::Mat SITiffIO::readFrame(int frame_num) const {
		auto dir_to_read = frame_num * m_nchans - (m_nchans - channel2display);
		return TiffReader->readframe(dir_to_read);
	}

	std::tuple<double, double, double> SITiffIO::getPos(const unsigned int i) const {
		auto search = m_all_transforms->find(i);
		if ( search != m_all_transforms->end() ) {
			auto transform_container = search->second;
			double x, z, r;
			transform_container.getPosData(x, z, r);
			return std::make_tuple(x, z, r);
		}
	}

	std::tuple<double, double> SITiffIO::getTrackerTranslation(const unsigned int i) const {
		auto search = m_all_transforms->find(i);
		if ( search != m_all_transforms->end() ) {
			auto transform_container = search->second;
			cv::Mat T = transform_container.getTransform(TransformType::kTrackerTranslation);
			double x_move = T.at<double>(0, 0);
			double y_move = T.at<double>(0, 1);
			return std::make_tuple(x_move, y_move);
		}
	}

	std::tuple<std::vector<double>, std::vector<double>> SITiffIO::getAllTrackerTranslation() const {
		std::vector<double> _x, _y;
		if ( ! m_all_transforms )
			return std::make_tuple(_x, _y);
		for ( auto const & T : *m_all_transforms ) {
			auto transform_container = T.second;
			if ( transform_container.hasTransform(TransformType::kTrackerTranslation) ) {
				auto M = transform_container.getTransform(TransformType::kTrackerTranslation);
				_x.push_back(M.at<double>(0, 0));
				_y.push_back(M.at<double>(0, 1));
			}
		}
		return std::make_tuple(_x, _y);
	}

	void SITiffIO::interpolateIndices() {
		if ( TiffReader == nullptr ) {
			// return some kind of error
			return;
		}
		if ( LogLoader == nullptr ) {
			// return some kind of error
			return;
		}

		int startFrame = 0;
		int endFrame = 0;
		TiffReader->countDirectories(endFrame);
		std::cout << "Counted " << endFrame << " directories" << std::endl;

		cv::Mat T = cv::Mat::eye(2, 3, CV_64F);
		double tiff_ts, x, z, r;
		unsigned int frame_num;
		int logfile_idx;
		TransformContainer tc{};

		if ( m_all_transforms == nullptr )
			m_all_transforms = std::make_shared<std::map<unsigned int, TransformContainer>>();
		else
			m_all_transforms->clear();

		for (int i = startFrame; i < endFrame; ++i) {
			TiffReader->getFrameNumAndTimeStamp(i, frame_num, tiff_ts);
			logfile_idx = LogLoader->findNearestIdx(tiff_ts);
			r = LogLoader->getRadianRotation(logfile_idx);
			x = LogLoader->getXTranslation(logfile_idx);
			z = LogLoader->getZTranslation(logfile_idx);

			tc.m_framenumber = i;
			tc.m_timestamp = tiff_ts;
			tc.setPosData(x,z,r);
			cv::Mat A = (cv::Mat_<double>(1,1) << r);
			tc.addTransform(TransformType::kInitialRotation, A);
			auto transform_map = m_all_transforms.get();
			transform_map->emplace(frame_num, tc);
		}
	}

	std::vector<double> SITiffIO::getTimeStamps() const {
		std::vector<double> result;
		if ( m_all_transforms != nullptr ) {
			for( auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it) {
				auto tc = it->second;
				result.push_back(tc.m_timestamp);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getX() const {
		std::vector<double> result;
		if ( m_all_transforms != nullptr ) {
			double x, z, t;
			for( auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it) {
				auto tc = it->second;
				tc.getPosData(x, z, t);
				result.push_back(x);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getZ() const {
		std::vector<double> result;
		if ( m_all_transforms != nullptr ) {
			double x, z, t;
			for( auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it) {
				auto tc = it->second;
				tc.getPosData(x, z, t);
				result.push_back(z);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getTheta() const {
		std::vector<double> result;
		if ( m_all_transforms != nullptr ) {
			double x, z, t;
			for( auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it) {
				auto tc = it->second;
				tc.getPosData(x, z, t);
				result.push_back(t);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getFrameNumbers() const {
		std::vector<double> result;
		if ( m_all_transforms != nullptr ) {
			for( auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it) {
				auto f = it->first;
				result.push_back(f);
			}
		}
		return result;
	}
}
// ----------------- Python binding ----------------

namespace py = pybind11;

PYBIND11_MODULE(scanimagetiffio, m) {
	NDArrayConverter::init_numpy();

	py::class_<twophoton::SITiffIO>(m, "SITiffIO")
		.def(py::init<>())
		.def("open_tiff_file", &twophoton::SITiffIO::openTiff, "Open a tiff file")
		.def("open_log_file", &twophoton::SITiffIO::openLog, "Open a log file")
		.def("open_xml_file", &twophoton::SITiffIO::openXML, "Open an xml file")
		.def("set_channel", &twophoton::SITiffIO::setChannel, "Sets the channel to take frames from")
		.def("interp_times", &twophoton::SITiffIO::interpolateIndices, "Interpolate the times in the tiff frames to events (position and time) in the log file")
		.def("get_pos", &twophoton::SITiffIO::getPos, "Gets a 3-tuple of X, Z and theta for the given frame")
		.def("get_tracker", &twophoton::SITiffIO::getTrackerTranslation, "Get the x and y translation for a tracked bounding box")
		.def("get_all_tracker", &twophoton::SITiffIO::getAllTrackerTranslation, "Get all the x and y translations for a tracked bounding box")
		.def("get_frame", &twophoton::SITiffIO::readFrame, "Gets the data/ image for the given frame")
		.def("get_all_x", &twophoton::SITiffIO::getX, "Gets all the X values")
		.def("get_all_z", &twophoton::SITiffIO::getZ, "Gets all the Z values")
		.def("get_all_theta", &twophoton::SITiffIO::getTheta, "Gets all the rotational values")
		.def("get_frame_numbers", &twophoton::SITiffIO::getFrameNumbers, "Gets all the rotational values")
		.def("get_all_timestamps", &twophoton::SITiffIO::getTimeStamps, "Gets all the timestamps");
}