#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <date/date.h>
#include "../include/ScanImageTiff.h"

namespace twophoton
{
    RotaryEncoderLoader::RotaryEncoderLoader()
    {
    }

    RotaryEncoderLoader::RotaryEncoderLoader(const std::string &fname)
    {
        m_filename = fname;
    }

    RotaryEncoderLoader::~RotaryEncoderLoader()
    {
    }

    bool RotaryEncoderLoader::load()
    {
        std::ifstream ifs{m_filename, std::ifstream::in};
        ifs.unsetf(std::ios_base::skipws);
        std::string line, old_line, s1;
        std::chrono::system_clock::time_point pt;
        std::chrono::system_clock::time_point old_time;
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
                    m_times.push_back(pt);
                    auto pos_mx = line.find(rot_token);
                    auto rot_value = line.substr(pos_mx+4, std::string::npos);
                    m_rotations.push_back(std::stof(rot_value));
                }
            }
        }
        return true;
    }

    std::vector<std::chrono::system_clock::time_point> RotaryEncoderLoader::getTimes()
    {
        return m_times;
    }

    std::vector<double> RotaryEncoderLoader::getRotations()
    {
        return m_rotations;
    }

} // namespace
