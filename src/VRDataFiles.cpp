#define _USE_MATH_DEFINES
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <date/date.h>
#include "../include/ScanImageTiff.h"

namespace twophoton
{
    // constrain angles to lie between 0 and 2*PI
    static double constrainAngleToPi(double x)
    {
        x = std::fmod(x, 2 * M_PI);
        if (x < 0)
            x += 2 * M_PI;
        return x;
    };

    // zeroRotations used in interpTiffData to subtract first element
    // from all others
    struct zeroRotations
    {
        double val;
        zeroRotations(double v) : val(v){};
        void operator()(double &elem) const
        {
            elem -= val;
        }
    };

    // normalizes each value to lie between 0 and 1 - used in processData() below
    struct normalize
    {
        double minval, maxval;
        normalize(double min, double max) : minval(min), maxval(max){};
        void operator()(double &elem) const
        {
            elem = (elem - minval) / (maxval - minval);
        }
    };

    static void zeroNormalize(std::vector<double> &vec)
    {
        // add the min val to zero and then divide by
        // the max result of this to get values between 0 and 1
        auto result = std::min_element(vec.begin(), vec.end());
        auto result2 = std::max_element(vec.begin(), vec.end());
        std::for_each(vec.begin(), vec.end(), normalize(*result, *result2));
    }

    // finds the first value in vector vec that does not compare less
    // than target
    int closest(std::vector<double> const &vec, double target)
    {
        auto const it = std::lower_bound(vec.begin(), vec.end(), target);
        if (it == vec.end())
        {
            return -1;
        }
        return std::distance(std::begin(vec), it);
    }

    bool VRDataFile::_calculateDurationsAndRotations(bool convertToRadians)
    {
        auto first_time = getTriggerTime();
        unsigned int count = 0;
        double raw_rotation = 0;
        for (std::vector<ptime>::iterator i = m_ptimes.begin(); i != m_ptimes.end(); ++i)
        {
            auto duration = *i - first_time;
            m_times.push_back(FpMilliseconds(duration).count() / 1000.0);
            if (convertToRadians)
            {
                raw_rotation = 2 * M_PI * (double(m_rotations[count]) / (double)rotary_encoder_units_per_turn);
                m_rotations_in_rads.push_back(constrainAngleToPi(raw_rotation));
            }
            else
            {
                m_rotations_in_rads.push_back(m_rotations[count]);
            }
            ++count;
        }
        return true;
    }

    bool RotaryEncoderLoader::load()
    {
        std::ifstream ifs{m_filename, std::ifstream::in};
        ifs.unsetf(std::ios_base::skipws);
        std::string line, old_line, s1;
        ptime pt;
        ptime old_time;
        ptime tmp_trigger_ptime;
        std::size_t pos;
        double rotation;
        std::cout << "Loading rotary encoder file: " << m_filename << std::endl;

        while (std::getline(ifs, line))
        {
            std::size_t found = line.find(rot_token);
            if (found != std::string::npos)
            {
                pos = line.find(X_token);
                s1 = line.substr(0, pos);
                std::istringstream is(s1);
                is >> date::parse(logfile_time_fmt, pt);
                if (old_time != pt)
                {
                    m_ptimes.push_back(pt);
                    auto pos_mx = line.find(rot_token);
                    auto rot_value = line.substr(pos_mx + 4, std::string::npos);
                    m_rotations.push_back(std::stof(rot_value));
                }
            }
            auto scope_triggered = line.find(rotary_trigger_token);
            if (scope_triggered != std::string::npos && foundTrigger == false)
            {
                m_hasAcquisition = true;
                tmp_trigger_ptime = pt;
                foundTrigger = true;
            }
            old_time = pt;
        }
        for (unsigned int i = 0; i < m_ptimes.size(); ++i)
        {
            if (tmp_trigger_ptime == m_ptimes[i])
            {
                m_trigger_time = m_ptimes[i];
                setTriggerIndex(i);
            }
        }
        if (calculateDurationsAndRotations())
        {
            isloaded = true;
        }
        return isloaded;
    }
    bool RotaryEncoderLoader::calculateDurationsAndRotations()
    {
        std::cout << "Calculating rotations and times from rotary encoder data..." << std::endl;
        auto result = _calculateDurationsAndRotations(false);
        std::cout << "Finished calculating rotations and times." << std::endl;
        std::cout << "The rotary encoder file has " << m_times.size() << " timestamps in it." << std::endl;
        return result;
    }

    bool LogFileLoader::calculateDurationsAndRotations()
    {
        if (!containsAcquisition())
        {
            std::cout << "Warning: The file " << m_filename << " has no microscope trigger associated" << std::endl;
            return false;
        }
        else
        {
            std::cout << "Calculating rotations and times from log file data..." << std::endl;
        }
        auto result = _calculateDurationsAndRotations();
        zeroNormalize(m_x_translation);
        zeroNormalize(m_z_translation);
        std::cout << "Finished calculating rotations and times." << std::endl;
        std::cout << "The log file file has " << m_times.size() << " timestamps in it." << std::endl;
        return result;
    };

    bool LogFileLoader::load()
    {
        std::ifstream ifs(m_filename, std::ifstream::in);
        ifs.unsetf(std::ios_base::skipws);
        std::string line, old_line, s1;
        ptime pt;
        ptime old_time; //(boost::gregorian::date(2002,1,10),boost::posix_time::time_duration(1,2,3));// old line is our "memory" - see comment in while loop below and header file
                        // see comment before loop that sets trigger index below (after the next while statement)
        // to understand why this temporary is used
        ptime tmp_trigger_ptime;
        std::size_t pos, posZ;
        double x_trans, z_trans;
        unsigned int trig_index = 0;
        std::cout << "\nLoading log file: " << m_filename << std::endl;
        while (std::getline(ifs, line))
        {
            /*grab the angular reference from the top of the file
            // in newer versions of the logfile; in older versions
            // this is just before the line 'MicroscopeTriggered'
            // in newer versions it appears on the same line
            // see the LogFileLoader.h tokens description(s)
            We need to keep a 1 line "memory" of the times
            as there are sometimes repeats from the rotary encoder
            or its attached bit of kit, that means sometimes we get
            the same sample twice
            */
            pos = line.find(init_rot_token);
            if (pos != std::string::npos)
            {
                auto s2 = line.substr(pos);
                pos = s2.find_last_of(space_token);
                auto tmp = s2.substr(pos);
                m_init_rotation = std::stoi(tmp);
            }
            std::size_t found = line.find(rot_token);
            if (found != std::string::npos)
            {
                // get the date...
                pos = line.find(X_token);
                s1 = line.substr(0, pos);
                std::istringstream is(s1);
                is >> date::parse(logfile_time_fmt, pt);
                if (old_time != pt) // ie skip repeats of the same time (logging error?)
                {
                    // grab the posix time
                    m_ptimes.push_back(pt);

                    // get the translation...
                    auto pos_MX = line.find(X_token);
                    auto pos_MY = line.find(Z_token);
                    auto pos_Rot = line.find(rot_token);

                    s1 = line.substr(pos_MX, pos_MY - pos_MX);
                    auto tmp_pos = s1.find(equal_token);
                    auto s2 = s1.substr(tmp_pos + 1);
                    x_trans = std::stof(s2);
                    m_x_translation.push_back(x_trans);

                    s1 = line.substr(pos_MY, pos_Rot - pos_MY);
                    tmp_pos = s1.find(equal_token);
                    s2 = s1.substr(tmp_pos + 1);
                    z_trans = std::stof(s2);
                    m_z_translation.push_back(z_trans);

                    // grab the amount of rotation...
                    pos = line.find(rot_token);
                    s1 = line.substr(pos);
                    pos = s1.find(equal_token);
                    s1 = s1.substr(pos);
                    pos = s1.find(mouse_move_token);
                    s1 = s1.substr(1, pos - 2);
                    m_rotations.push_back(std::stoi(s1));
                }
            }

            std::size_t scope_triggered = line.find(scope_token);
            if (scope_triggered != std::string::npos)
            {
                m_hasAcquisition = true;
                tmp_trigger_ptime = pt;
            }
            // deal with newer versions of the logfile
            scope_triggered = line.find(spacebar_token);
            if (scope_triggered != std::string::npos)
            {
                m_hasAcquisition = true;
                tmp_trigger_ptime = pt;
            }
            old_time = pt;
        }
        /* I think we should set the start of logfile -> tifffile registration
        at the time point immediately following the string that says acquisition
        was started, so we modify the trigger_ptime and the trigger index to be 1
        item further ahead than the time detected in the logfile
        */
        for (unsigned int i = 0; i < m_ptimes.size(); ++i)
        {
            if (tmp_trigger_ptime == m_ptimes[i])
            {
                m_trigger_time = m_ptimes[i];
                setTriggerIndex(i);
            }
        }
        if (calculateDurationsAndRotations())
        {
            isloaded = true;
        }
        return isloaded;
    }

} // namespace
