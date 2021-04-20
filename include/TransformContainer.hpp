#ifndef TRANSFORMCONTAINER_H
#define TRANSFORMCONTAINER_H

#include <map>
#include <string>
#include <iostream>
#include <opencv2/core.hpp>

// This scoped enum needs to be given an int type so insertion order
// into a std::map follows the order below
enum class TransformType : int {
    kInitialRotation,
    kTrackerTranslation,
    kMultiTrackerTranslation,
    kLogPolarRotation,
    kHaimanFFTTranslation,
    kOpticalFlow,
    kHaimanPieceWiseMapping,
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
// namespace twophoton {
    class TransformContainer
    {
    private:
        using TransformMap = std::map<TransformType, cv::Mat>;
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
        void addTransform(const TransformType & T, cv::Mat M) { m_transforms[T] = M; }
        bool hasTransform(const TransformType & T) {
            auto search = m_transforms.find(T);
            if ( search != m_transforms.end() )
                return true;
            else
                return false;
        };
        // Grab a transform type and its contents
        cv::Mat getTransform(const TransformType & T) { 
            auto search = m_transforms.find(T);
            if ( search != m_transforms.end() )
                return m_transforms[T];
            else
                return cv::Mat();
        };
        void updateTransform(const TransformType & T, cv::Mat M) {
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
        void write(cv::FileStorage & fs) const;
	    void read(const cv::FileNode & node);
    };
// } // namespace twophoton

static void write(cv::FileStorage & fs, const std::string &, const TransformContainer & T) {
	T.write(fs);
}

static void read(cv::FileNode & node, TransformContainer & T, const TransformContainer & default_value = TransformContainer()) {
	if ( node.empty() )
		T = default_value;
	else
		T.read(node);
}

// "read" method
static void operator>>(const cv::FileNode & node, TransformContainer & T) {
	T.m_framenumber = (int)node["Frame"];
	T.m_timestamp = (double)node["Timestamp"];
	T.m_x = (double)node["X"];
	T.m_z = (double)node["Z"];
	T.m_r = (double)node["R"];
    // the transformations
    cv::FileNodeIterator iter = node.begin();
    for(; iter != node.end(); ++iter) {
        cv::FileNode n = *iter;
        std::string name = n.name();
        if ( name.compare("InitialRotation") == 0 )
            T.addTransform(TransformType::kInitialRotation, n.mat());
        if ( name.compare("TrackerTranslation") == 0 )
            T.addTransform(TransformType::kTrackerTranslation, n.mat());
        if ( name.compare("MultiTrackerTranslation") == 0 )
            T.addTransform(TransformType::kMultiTrackerTranslation, n.mat());
        if ( name.compare("HaimanFFTTranslation") == 0 )
            T.addTransform(TransformType::kHaimanFFTTranslation, n.mat());
        if ( name.compare("LogPolarRotation") == 0 )
            T.addTransform(TransformType::kLogPolarRotation, n.mat());
        if ( name.compare("OpticalFlow") == 0 )
            T.addTransform(TransformType::kOpticalFlow, n.mat());
        if ( name.compare("PieceWiseMapping") == 0 )
            T.addTransform(TransformType::kHaimanPieceWiseMapping, n.mat());
    }
}

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
#endif