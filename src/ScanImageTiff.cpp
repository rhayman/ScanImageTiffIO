#include "../include/ScanImageTiff.h"
#include "../include/rapidxml.hpp"
#include <stdio.h>
#include <limits>
#include <tuple>
#include <memory>
#include <algorithm>

namespace twophoton
{
	void SITiffHeader::read(TIFF *m_tif, int dirnum)
	{
		if (m_tif)
		{
			if (dirnum == -1)
				TIFFSetDirectory(m_tif, dirnum);
			else
			{
				// scrape some big-ish strings out of the header
				char *imdesc;
				if (TIFFGetField(m_tif, TIFFTAG_IMAGEDESCRIPTION, &imdesc) == 1)
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

	void SITiffHeader::versionCheck(TIFF *m_tif)
	{
		if (m_tif)
		{
			m_imdesc = getImageDescTag(m_tif, 0);
			if (!m_imdesc.empty())
			{
				if (!grabStr(m_imdesc, "Frame Number =").empty()) // old
				{
					version = 0;
					channelSaved = "scanimage.SI5.channelsSave =";
					channelLUT = "scanimage.SI5.chan1LUT =";
					channelOffsets = "scanimage.SI5.channelOffsets =";
					frameString = "Frame Number =";
					frameTimeStamp = "Frame Timestamp(s) =";
				}
				else if (!grabStr(m_imdesc, "frameNumbers =").empty()) // new
				{
					version = 1;
					channelSaved = "SI.hChannels.channelSave =";
					channelLUT = "SI.hChannels.channelLUT =";
					channelOffsets = "SI.hChannels.channelOffset =";
					channelNames = "SI.hChannels.channelName =";
					frameString = "frameNumbers =";
					frameTimeStamp = "frameTimestamps_sec =";
				}

				uint32_t length;
				uint32_t width;
				TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &length);
				TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
				m_parent->setImageSize(length, width);
			}
			else
				m_parent->setImageSize(512, 512);
		}
	}

	std::string SITiffHeader::getSoftwareTag(TIFF *m_tif, unsigned int dirnum)
	{
		if (m_tif)
		{
			TIFFSetDirectory(m_tif, dirnum);
			if (version == 0)
			{
				// with older versions the information for channels live
				// in a different tag (ImageDescription) and some of that
				// information doesn't even exist so has to be inferred...
				std::string imdesc = getImageDescTag(m_tif, dirnum);
				if (!imdesc.empty())
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
			else if (version == 1)
			{
				char *swTag;
				if (TIFFGetField(m_tif, TIFFTAG_SOFTWARE, &swTag) == 1)
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

	std::string SITiffHeader::getImageDescTag(TIFF *m_tif, unsigned int dirnum)
	{
		if (m_tif)
		{
			TIFFSetDirectory(m_tif, dirnum);
			char *imdesc;
			if (TIFFGetField(m_tif, TIFFTAG_IMAGEDESCRIPTION, &imdesc) == 1)
			{
				m_imdesc = imdesc;
				return imdesc;
			}
			else
			{
				std::cout << "Image description tag empty\n";
				return std::string();
			}
		}
		std::cout << "m_tif not valid\n";
		return std::string();
	}

	unsigned int SITiffHeader::getSizePerDir(TIFF *m_tif, unsigned int dirnum)
	{
		if (m_tif)
		{
			TIFFSetDirectory(m_tif, dirnum);
			uint32_t length;
			uint32_t width;
			TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &length);
			TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
			// hard-code the sample format (signed int) as SI seems to only
			// use this which is of size 4
			return (length * width * 4);
		}
		else
			return 0;
	}

	void SITiffHeader::printHeader(TIFF *m_tif, int framenum)
	{
		if (m_tif)
		{
			TIFFSetDirectory(m_tif, framenum);
			TIFFPrintDirectory(m_tif, stdout, 0);
		}
	}

	int SITiffHeader::countDirectories(TIFF *m_tif)
	{
		int count = 0;
		if (m_tif)
		{
			TIFFSetDirectory(m_tif, count);
			do
			{
				++count;
			} while (TIFFReadDirectory(m_tif) == 1);
			return count;
		}
		return 0;
	}

	int SITiffHeader::scrapeHeaders(TIFF *m_tif, int &count)
	{
		if (m_tif)
		{
			if (TIFFReadDirectory(m_tif) == 1)
			{
				std::string imdescTag = getImageDescTag(m_tif, count);
				if (!imdescTag.empty())
				{
					std::string ts_str = grabStr(imdescTag, frameTimeStamp);
					if (!ts_str.empty()) // sometimes headers are corrupted esp. at EOF
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
		for (auto &x : LUT_split)
		{
			if (x.find('[') != std::string::npos)
				x = x.substr(1);
			if (x.find(']') != std::string::npos)
				x.pop_back();
		}
		int count = 1;
		for (unsigned int i = 0; i < LUT_split.size(); i += 2)
		{
			chanLUT[count] = std::make_pair<int, int>(std::stoi(LUT_split[i]), std::stoi(LUT_split[i + 1]));
			++count;
		}
	}

	void SITiffHeader::parseChannelOffsets(std::string offsets)
	{
		offsets.pop_back();
		offsets = offsets.substr(2);
		auto offsets_split = split(offsets, ' ');
		int count = 1;
		for (auto &x : offsets_split)
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
		if (savedchans.size() == 2)
		{
			chanSaved[0] = std::stoul(savedchans.substr(1));
		}
		else
		{
			savedchans.pop_back();
			savedchans = savedchans.substr(2);
			auto savedchans_split = split(savedchans, ';');
			unsigned int count = 0;
			for (auto &x : savedchans_split)
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
		if (headerdata)
			delete headerdata;
	}
	bool SITiffReader::open()
	{
		m_tif = TIFFOpen(m_filename.c_str(), "r");
		if (m_tif)
		{
			headerdata = new SITiffHeader{this};
			headerdata->versionCheck(m_tif);
			headerdata->getSoftwareTag(m_tif);
			isopened = true;
			return true;
		}
		return false;
	}

	bool SITiffReader::readheader()
	{
		if (m_tif)
		{
			std::string softwareTag = headerdata->getSoftwareTag(m_tif);
			return true;
		}
		return false;
	}

	std::vector<double> SITiffReader::getAllTimeStamps() const
	{
		if (m_tif)
		{
			std::cout << "Starting scraping timestamps..." << std::endl;
			TIFFSetDirectory(m_tif, 0);
			int count = 0;
			do
			{
			} while (headerdata->scrapeHeaders(m_tif, count) == 0);
			std::cout << "Finished scraping timestamps..." << std::endl;
			return headerdata->getTimeStamps();
		}
		return std::vector<double>();
	}

	void SITiffReader::getFrameNumAndTimeStamp(const unsigned int dirnum, unsigned int &framenum, double &timestamp) const
	{
		if (m_tif)
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

	arma::Mat<int16_t> SITiffReader::readframe(int framedir)
	{
		if (m_tif)
		{
			int framenum = framedir;
			/*
			From the man pages for TIFFSetDirectory:

			TIFFSetDirectory changes the current directory and reads its contents with TIFFReadDirectory.
			The parameter dirnum specifies the subfile/directory as an integer number, with the first directory numbered zero.

			NB This differs from the 1-based indexing for framenumbers that ScanImage uses (fucking Matlab)
			*/
			if (TIFFSetDirectory(m_tif, framenum) == 0)
				return arma::Mat<int16_t>();
			else
			{
				uint32_t w = 0, h = 0;
				uint16_t photometric = 0;
				auto buffer_size = TIFFStripSize(m_tif);
				if (TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &w) &&			// normally = 512
					TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &h) &&			// normally = 512
					TIFFGetField(m_tif, TIFFTAG_PHOTOMETRIC, &photometric)) // photometric = 1 (min-is-black)
				{
					m_imagewidth = w;
					m_imageheight = h;

					uint16_t bpp = 8, ncn = photometric > 1 ? 3 : 1;
					TIFFGetField(m_tif, TIFFTAG_BITSPERSAMPLE, &bpp);	// = 16
					TIFFGetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, &ncn); // = 1
					int is_tiled = TIFFIsTiled(m_tif);					// 0 ie false, which means the data is organised in strips
					uint32_t tile_height0 = 0, tile_width0 = m_imagewidth;
					TIFFGetField(m_tif, TIFFTAG_ROWSPERSTRIP, &tile_height0);

					if ((!is_tiled) ||
						(is_tiled &&
						 TIFFGetField(m_tif, TIFFTAG_TILEWIDTH, &tile_width0) &&
						 TIFFGetField(m_tif, TIFFTAG_TILELENGTH, &tile_height0)))
					{
						if (!is_tiled)
							TIFFGetField(m_tif, TIFFTAG_ROWSPERSTRIP, &tile_height0);

						if (tile_width0 <= 0)
							tile_width0 = m_imagewidth;

						if (tile_height0 <= 0 ||
							(!is_tiled && tile_height0 == std::numeric_limits<uint32_t>::max()))
							tile_height0 = m_imageheight;

						auto buffer = std::make_unique<int16_t[]>(buffer_size);
						int tileidx = 0;

						// ********* return frame created here ***********
						arma::Mat<int16_t> frame(h, w, arma::fill::zeros);
						auto *data = frame.memptr();
						tdata_t buf = _TIFFmalloc(TIFFScanlineSize(m_tif));
						uint16 s, nsamples;
						TIFFGetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
						uint32 row;
						auto slsz = TIFFScanlineSize(m_tif);
						for (row = 0; row < h; row++)
						{
							TIFFReadScanline(m_tif, buf, row);
							std::memcpy(frame.colptr(row), (int16_t *)buf, slsz);
						}
						_TIFFfree(buf);
						return frame;
					}
				}
			}
		}
		return arma::Mat<int16_t>();
	}

	bool SITiffReader::close()
	{
		if (m_tif)
		{
			TIFFClose(m_tif);
			m_tif = NULL;
			isopened = false;
			if (headerdata)
				delete headerdata;
			return true;
		}
		return false;
	}
	// ######################################################################
	// ################## Tiff encoder class ################################
	// ######################################################################

	static void readParam(const std::vector<int> &params, int key, int &value)
	{
		for (size_t i = 0; i + 1 < params.size(); i += 2)
			if (params[i] == key)
			{
				value = params[i + 1];
				break;
			}
	}

	SITiffWriter::~SITiffWriter()
	{
		if (opened)
			TIFFClose(m_tif);
	}

	bool SITiffWriter::writeLibTiff(arma::Mat<int16_t> &img, const std::vector<int> &params)
	{
		int channels = 1;
		int width = img.n_cols;
		int height = img.n_rows;
		int bitsPerChannel = 16;
		const int bitsPerByte = 16;
		size_t fileStep = (width * channels * bitsPerChannel) / bitsPerByte; // = image_width (with 1 channel)

		int rowsPerStrip = 8;
		readParam(params, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);
		rowsPerStrip = height;
		if (!(isOpened()))
			pTiffHandle = TIFFOpen(m_filename.c_str(), "w8");
		else
			pTiffHandle = m_tif;
		if (!pTiffHandle)
			return false;

		// defaults for now, maybe base them on params in the future
		int compression = COMPRESSION_NONE;
		int predictor = 1;
		int units = RESUNIT_INCH;
		double xres = 72.0;
		double yres = 72.0;
		int sampleformat = SAMPLEFORMAT_INT;
		int orientation = ORIENTATION_TOPLEFT;
		int planarConfig = 1;

		readParam(params, TIFFTAG_COMPRESSION, compression);
		readParam(params, TIFFTAG_PREDICTOR, predictor);
		TIFFSetField(pTiffHandle, TIFFTAG_ROWSPERSTRIP, 8);

		int colorspace = channels > 1 ? PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK;

		if (!TIFFSetField(pTiffHandle, TIFFTAG_IMAGEWIDTH, width) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_IMAGELENGTH, height) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_BITSPERSAMPLE, bitsPerChannel) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_COMPRESSION, compression) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_PHOTOMETRIC, colorspace) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_SAMPLESPERPIXEL, channels) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_PLANARCONFIG, planarConfig) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_ROWSPERSTRIP, 8) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_RESOLUTIONUNIT, units) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_XRESOLUTION, xres) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_YRESOLUTION, yres) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_SAMPLEFORMAT, sampleformat) ||
			!TIFFSetField(pTiffHandle, TIFFTAG_ORIENTATION, orientation))
		{
			TIFFClose(pTiffHandle);
			return false;
		}

		if (compression != COMPRESSION_NONE && !TIFFSetField(pTiffHandle, TIFFTAG_PREDICTOR, predictor))
		{
			TIFFClose(pTiffHandle);
			return false;
		}

		auto data = img.memptr();

		if (!data)
		{
			TIFFClose(pTiffHandle);
			return false;
		}
		size_t scanlineSize = TIFFScanlineSize(pTiffHandle);
		const size_t buffer_size = bitsPerChannel * channels * 16 * width;
		tdata_t buf = _TIFFmalloc(scanlineSize);

		for (int y = 0; y < height; ++y)
		{
			std::memcpy((int16_t *)buf, img.colptr(y), scanlineSize);
			int writeResult = TIFFWriteScanline(pTiffHandle, buf, y, 0);
			if (writeResult != 1)
			{
				TIFFClose(pTiffHandle);
				opened = false;
				return false;
			}
		}
		TIFFWriteDirectory(pTiffHandle); // write into the next directory
		return true;
	}

	bool SITiffWriter::writeHdr(const arma::Mat<int16_t> &img)
	{
		// IMPORTANT: Note the "w8" option here - this is what allows writing to the bigTIFF format
		// possible ('normal' tiff would be just "w")
		if (!(isOpened()))
			m_tif = TIFFOpen(m_filename.c_str(), "w8");
		if (!m_tif)
			return false;

		// TIFFWriteDirectory(pTiffHandle);
		TIFFSetField(m_tif, TIFFTAG_IMAGEWIDTH, img.n_cols);
		TIFFSetField(m_tif, TIFFTAG_IMAGELENGTH, img.n_rows);
		TIFFSetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(m_tif, TIFFTAG_COMPRESSION, 1);
		TIFFSetField(m_tif, TIFFTAG_PHOTOMETRIC, 1);
		TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, 8);
		return true;
	}

	bool SITiffWriter::writeSIHdr(const std::string swTag, const std::string imDescTag)
	{
		auto _swTag = swTag.c_str();
		auto _imDescTag = imDescTag.c_str();
		if ((TIFFSetField(m_tif, TIFFTAG_IMAGEDESCRIPTION, _imDescTag) > 0) &&
			(TIFFSetField(m_tif, TIFFTAG_SOFTWARE, _swTag) > 0))
			return true;
		return false;
	}

	std::string SITiffWriter::modifyChannel(std::string &src_str, const unsigned int chan2keep)
	{
		auto chan = std::to_string(chan2keep);
		std::string target = "SI.hChannels.channelSave = ";
		std::string replace_with = target + chan;
		std::string whole_target = grabStr(src_str, target);
		whole_target = target + whole_target;

		auto loc = src_str.find(target);
		if (loc != std::string::npos)
		{
			src_str.replace(loc, whole_target.size(), replace_with);
		}
		return whole_target;
	}

	bool SITiffWriter::write(arma::Mat<int16_t> &img, const std::vector<int> &params)
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

	bool SITiffWriter::close()
	{
		if (opened)
		{
			TIFFClose(m_tif);
			opened = false;
		}
		return opened;
	}

	void SITiffWriter::operator<<(arma::Mat<int16_t> &frame)
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

	SITiffIO::~SITiffIO()
	{
	}

	bool SITiffIO::openTiff(const std::string &fname, const std::string mode)
	{

		if (mode == "r")
		{
			TiffReader = std::make_shared<SITiffReader>(fname);
			if (TiffReader->open())
			{
				TiffReader->getSWTag(0); // ensures num channels are read
				auto chans = TiffReader->getSavedChans();
				if (chans.empty() || chans.size() == 0)
					m_nchans = 1;
				else
					m_nchans = chans.size();
				return true;
			}
		}
		else if (mode == "w")
		{
			TiffWriter = std::make_shared<SITiffWriter>();
			if (TiffWriter->open(fname))
				return true;
			return false;
		}
		return false;
	}

	bool SITiffIO::closeReaderTiff()
	{
		if (TiffReader == nullptr)
			return false;
		if (TiffReader->isOpen())
		{
			TiffReader->close();
			TiffReader = nullptr;
			return true;
		}
		return false;
	}

	bool SITiffIO::closeWriterTiff()
	{
		if (TiffWriter == nullptr)
			return false;
		if (TiffWriter->isOpened())
		{
			TiffWriter->close();
			return true;
		}
		return false;
	}

	bool SITiffIO::openLog(std::string fname)
	{
		log_fname = fname;
		LogLoader = std::make_shared<LogFileLoader>(fname);
		return LogLoader->load();
	}

	bool SITiffIO::openXML(std::string fname)
	{
		if (m_all_transforms == nullptr)
			m_all_transforms = std::make_shared<std::map<unsigned int, TransformContainer>>();
		else
			m_all_transforms->clear();

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<> *root_node;
		rapidxml::xml_node<> *summary_node;
		rapidxml::xml_node<> *transform_node;

		std::ifstream ifs(fname);
		std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(&buffer[0]);
		root_node = doc.first_node("opencv_storage");
		summary_node = root_node->first_node("Summary");
		transform_node = summary_node->next_sibling();

		for (rapidxml::xml_node<> *underscore_node = transform_node->first_node("_"); underscore_node != nullptr; underscore_node = underscore_node->next_sibling())
		{
			for (rapidxml::xml_node<> *frame_node = underscore_node->first_node("Frame"); frame_node != nullptr; frame_node = frame_node->next_sibling())
			{
				std::cout << frame_node->name() << ": " << frame_node->value() << std::endl;
				if (std::find(possible_transforms.begin(), possible_transforms.end(), frame_node->name()) != possible_transforms.end())
				{
					// found one of the possible transforms, calculate the size of the mat to fit the values in
					int n_rows = std::stoi(frame_node->first_node("rows")->value());
					int n_cols = std::stoi(frame_node->first_node("cols")->value());
					TransformContainer tc{std::stoi(frame_node->name()), std::stof(frame_node->value())};
					m_all_transforms->emplace(std::stoi(frame_node->name()), tc);
				}
			}
		}
		return true;
	}

	py::array_t<int16_t> SITiffIO::readFrame(int frame_num) const
	{
		if (TiffReader != nullptr)
		{
			int dir_to_read = (frame_num * m_nchans - (m_nchans - channel2display)) - 1;
			auto F = TiffReader->readframe(dir_to_read);
			return carma::mat_to_arr(F, true);
		}
		return py::array_t<int16_t>();
	}

	void SITiffIO::writeFrame(py::array_t<int16_t> frame, unsigned int frame_num) const
	{
		if (TiffWriter != nullptr)
		{
			int dir_to_read_write = (frame_num * m_nchans - (m_nchans - channel2display)) - 1;
			std::string swtag, imtag;
			if (TiffReader != nullptr)
			{
				swtag = TiffReader->getSWTag(dir_to_read_write);
				imtag = TiffReader->getImDescTag(dir_to_read_write);
				TiffWriter->modifyChannel(swtag, channel2display);
				TiffWriter->writeSIHdr(swtag, imtag);
			}
			arma::Mat<int16_t> write_frame = carma::arr_to_mat(frame, true);
			TiffWriter->writeHdr(write_frame);
			*TiffWriter << write_frame;
		}
	}

	std::tuple<double, double, double> SITiffIO::getPos(const unsigned int i) const
	{
		auto search = m_all_transforms->find(i);
		if (search != m_all_transforms->end())
		{
			auto transform_container = search->second;
			double x, z, r;
			transform_container.getPosData(x, z, r);
			return std::make_tuple(x, z, r);
		}
		return std::make_tuple(0, 0, 0);
	}

	std::tuple<double, double> SITiffIO::getTrackerTranslation(const unsigned int i) const
	{
		auto search = m_all_transforms->find(i);
		if (search != m_all_transforms->end())
		{
			auto transform_container = search->second;
			auto T = transform_container.getTransform(TransformType::kTrackerTranslation);
			double x_move = T.at(0, 0);
			double y_move = T.at(0, 1);
			return std::make_tuple(x_move, y_move);
		}
		return std::make_tuple(0, 0);
	}

	std::tuple<std::vector<double>, std::vector<double>> SITiffIO::getAllTrackerTranslation() const
	{
		std::vector<double> _x, _y;
		if (!m_all_transforms)
			return std::make_tuple(_x, _y);
		for (auto const &T : *m_all_transforms)
		{
			auto transform_container = T.second;
			if (transform_container.hasTransform(TransformType::kTrackerTranslation))
			{
				auto M = transform_container.getTransform(TransformType::kTrackerTranslation);
				_x.push_back(M.at(0, 0));
				_y.push_back(M.at(0, 1));
			}
		}
		return std::make_tuple(_x, _y);
	}

	unsigned int SITiffIO::countDirectories()
	{
		if (TiffReader != nullptr)
		{
			int endFrame = TiffReader->countDirectories();
			endFrame /= m_nchans;
			return endFrame;
		}
		return 0;
	}

	void SITiffIO::interpolateIndices()
	{
		if (TiffReader == nullptr)
		{
			// return some kind of error
			return;
		}
		if (LogLoader == nullptr)
		{
			// return some kind of error
			return;
		}

		int startFrame = 0;
		int endFrame = TiffReader->countDirectories();
		endFrame /= m_nchans;
		std::cout << "Counted " << endFrame << " frames" << std::endl;
		TiffReader->readheader(); // to get the number of channels...
		auto nchans = TiffReader->getSavedChans().size();

		double tiff_ts, x, z, r;
		unsigned int frame_num;
		int logfile_idx;
		TransformContainer tc{};

		if (m_all_transforms == nullptr)
			m_all_transforms = std::make_shared<std::map<unsigned int, TransformContainer>>();
		else
			m_all_transforms->clear();

		for (int i = startFrame; i < endFrame * nchans; i += nchans)
		{
			TiffReader->getFrameNumAndTimeStamp(i, frame_num, tiff_ts);
			logfile_idx = findNearestIdx(LogLoader->getTimes(), tiff_ts);
			r = LogLoader->getRadianRotation(logfile_idx);
			x = LogLoader->getXTranslation(logfile_idx);
			z = LogLoader->getZTranslation(logfile_idx);

			tc.m_framenumber = (int)(i / nchans);
			tc.m_timestamp = tiff_ts;
			tc.setPosData(x, z, r);
			arma::mat A(1, 1);
			A = {r};
			tc.addTransform(TransformType::kInitialRotation, A);
			auto transform_map = m_all_transforms.get();
			transform_map->emplace(frame_num, tc);
		}
	}

	std::vector<double> SITiffIO::getTimeStamps() const
	{
		std::vector<double> result;
		if (m_all_transforms != nullptr)
		{
			for (auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it)
			{
				auto tc = it->second;
				result.push_back(tc.m_timestamp);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getX() const
	{
		std::vector<double> result;
		if (m_all_transforms != nullptr)
		{
			double x, z, t;
			for (auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it)
			{
				auto tc = it->second;
				tc.getPosData(x, z, t);
				result.push_back(x);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getZ() const
	{
		std::vector<double> result;
		if (m_all_transforms != nullptr)
		{
			double x, z, t;
			for (auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it)
			{
				auto tc = it->second;
				tc.getPosData(x, z, t);
				result.push_back(z);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getTheta() const
	{
		std::vector<double> result;
		if (m_all_transforms != nullptr)
		{
			double x, z, t;
			for (auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it)
			{
				auto tc = it->second;
				tc.getPosData(x, z, t);
				result.push_back(t);
			}
		}
		return result;
	}

	std::vector<double> SITiffIO::getFrameNumbers() const
	{
		std::vector<double> result;
		if (m_all_transforms != nullptr)
		{
			for (auto it = m_all_transforms->cbegin(); it != m_all_transforms->cend(); ++it)
			{
				auto f = it->first;
				result.push_back(f);
			}
		}
		return result;
	}

	std::tuple<unsigned int> SITiffIO::getNChannels() const
	{
		return m_nchans;
	}

	std::pair<int, int> SITiffIO::getChannelLUT()
	{
		if (TiffReader == nullptr)
		{
			std::cout << "Tiff not opened/available" << std::endl;
			return std::make_pair(0, 0);
		}
		std::map<int, std::pair<int, int>> channelLUTs = TiffReader->getChanLut();
		auto search = channelLUTs.find(channel2display);
		if (search != channelLUTs.end())
		{
			return channelLUTs[channel2display];
		}
		else
		{
			std::cout << "Channel not available" << std::endl;
			return std::make_pair(0, 0);
		}
	}

}
