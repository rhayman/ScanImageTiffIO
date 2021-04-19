#ifndef LOGFILELOADER_
#define LOGFILELOADER_

#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp> // for timestamps in the logfile
#include <boost/date_time/gregorian/gregorian.hpp>
#include <fstream> //for file io
#include <sstream> //for file io
#include <iostream> //for file io
#include <string> //for file i/o
#include <algorithm> //for line counting in file i/o
#include <math.h> //for M_PI

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace twophoton
{
    /* In newer iterations of the logfile there's a value near the top
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
        void saveRotationMats(cv::Point centre, std::string outPath);
        //...and this loads them and returns in the vector
        std::vector<cv::Mat> loadRotationMats(std::string filePath);
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
}; // namespace twophoton

#endif