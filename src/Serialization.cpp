#include "../include/ScanImageTiff.h"

namespace twophoton {
    void TransformContainer::write(cv::FileStorage & fs) const {
        fs << "{" << "Frame" << m_framenumber;
        fs << "Timestamp" << m_timestamp <<
        "X" << m_x <<
        "Z" << m_z <<
        "R" << m_r;
        auto transforms = getTransforms();
        for ( const auto & transform : transforms ) {
            auto transform_type = transform.first;
            auto transform_val = transform.second;
            if ( transform_type == TransformType::kInitialRotation ) {
                fs << "InitialRotation";
            }
            if ( transform_type == TransformType::kTrackerTranslation ) {
                fs << "TrackerTranslation";
            }
            if ( transform_type == TransformType::kMultiTrackerTranslation ) {
                fs << "MultiTrackerTranslation";
            }
            if ( transform_type == TransformType::kHaimanFFTTranslation ) {
                fs << "HaimanFFTTranslation";
            }
            if ( transform_type == TransformType::kLogPolarRotation ) {
                fs << "LogPolarRotation";
            }
            if ( transform_type == TransformType::kOpticalFlow ) {
                fs << "OpticalFlow";
            }
            if ( transform_type == TransformType::kHaimanPieceWiseMapping ) {
                fs << "PieceWiseMapping";
            }
            if ( transform_type == TransformType::kLogPolarPieceWiseMapping ) {
                fs << "LogPolarPieceWiseMapping";
            }
            if ( transform_type == TransformType::kFFTTranslation ) {
                fs << "FFTTranslation";
            }
            if ( transform_type == TransformType::kFFTTranslationPieceWise ) {
                fs << "FFTTranslationPieceWise";
            }
            fs << transform_val;
        }
        fs << "}";
    }

    void TransformContainer::read(const cv::FileNode & node) {
        m_framenumber = (int)node["Frame"];
        m_timestamp = (double)node["Timestamp"];
        m_x = (double)node["X"];
        m_z = (double)node["Z"];
        m_r = (double)node["R"];
        cv::FileNodeIterator iter = node.begin();
        for(; iter != node.end(); ++iter) {
            cv::FileNode n = *iter;
            std::string name = n.name();
            cv::Mat M;
            if ( name.compare("InitialRotation") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kInitialRotation, M);
            }
            if ( name.compare("TrackerTranslation") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kTrackerTranslation, M);
            }
            if ( name.compare("MultiTrackerTranslation") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kMultiTrackerTranslation, M);
            }
            if ( name.compare("HaimanFFTTranslation") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kHaimanFFTTranslation, M);
            }
            if ( name.compare("LogPolarRotation") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kLogPolarRotation, M);
            }
            if ( name.compare("OpticalFlow") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kOpticalFlow, M);
            }
            if ( name.compare("PieceWiseMapping") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kHaimanPieceWiseMapping, M);
            }
            if ( name.compare("LogPolarPieceWiseMapping") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kLogPolarPieceWiseMapping, M);
            }
            if ( name.compare("FFTTranslation") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kFFTTranslation, M);
            }
            if ( name.compare("FFTTranslationPieceWise") == 0 ) {
                M = n.mat();
                addTransform(TransformType::kFFTTranslationPieceWise, M);
            }
        }

    }

    void ChanInfo::write(cv::FileStorage & fs) const {
	fs << "{" << "name" << name <<
	"lut_lower" << lut_lower <<
    "lut_upper" << lut_upper <<
	"offset" << offset << "}";
    }

    void ChanInfo::read(const cv::FileNode & node) {
        name = (int)node["name"];
        lut_lower = (int)node["lut_lower"];
        lut_upper = (int)node["lut_upper"];
        offset = (int)node["offset"];
    }

    // ------------------- FileStorageHeaderData ------------------

    void FileStorageHeaderData::write(cv::FileStorage & fs) const {
        fs << "{" << "tiff_file" << tiffname <<
        "log_file" << logname;
        fs << "Channels" << "{";
        for ( const auto & channel : channels) {
            fs << "chan" << channel;
        }
        fs << "}";
        fs << "Image_height" << imageheight <<
        "Image_width" << imagewidth <<
        "Output_image_height" << outputimageheight <<
        "Output_image_width" << outputimagewidth << 
        "bounding_box_x_centre" << bounding_box_x_centre <<
        "bounding_box_y_centre" << bounding_box_y_centre;
        fs << "multiple_bounding_boxes" << "{";
        unsigned int count = 0;
        for ( const auto & bbox : multi_bbox_centres) {
            fs << "bounding_box_centre_" + std::to_string(count) << bbox;
            ++count;
        }
        fs << "}";
        fs << "}";
    }

    void FileStorageHeaderData::read(const cv::FileNode & node) {
        tiffname = (std::string)node["tiff_file"];
        logname = (std::string)node["log_file"];

        cv::FileNode node1 = node["Channels"];
        cv::FileNodeIterator iter = node1.begin();
        for(; iter != node1.end(); ++iter) {
            cv::FileNode n = *iter;
            ChanInfo c;
            n >> c;
            channels.push_back(c);
        }
        
        imageheight = (int)node["Image_height"];
        imagewidth = (int)node["Image_width"];
        outputimageheight = (int)node["Output_image_height"];
        outputimagewidth = (int)node["Output_image_width"];
        bounding_box_x_centre = (int)node["bounding_box_x_centre"];
        bounding_box_y_centre = (int)node["bounding_box_y_centre"];
        node1 = node["multiple_bounding_boxes"];
        iter = node1.begin();
        for(; iter != node1.end(); ++iter) {
            cv::FileNode n = *iter;
            auto M = n.mat();
            multi_bbox_centres.push_back(M);
        }
    }
} // namespace twophoton