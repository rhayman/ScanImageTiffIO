#include "../include/TransformContainer.hpp"

// namespace twophoton {
    void TransformContainer::write(cv::FileStorage & fs) const {
        fs << "{" << "Frame" << m_framenumber <<
        "Timestamp" << m_timestamp <<
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
        }

    }

// } // namespace twophoton