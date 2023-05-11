#include "../include/ScanImageTiff.h"

namespace twophoton {
    RotaryEncoderLoader::RotaryEncoderLoader() {

    }

    RotaryEncoderLoader::RotaryEncoderLoader(const std::string & fname) {
        m_filename = fname;
    }

    RotaryEncoderLoader::~RotaryEncoderLoader() {

    }

    bool RotaryEncoderLoader::load() {
        std::ifstream ifs(m_filename, std::ifstream::in);
		ifs.unsetf(std::ios_base::skipws);
		std::string line, old_line, s1;
		std::chrono::system_clock::time_point pt;
		std::chrono::system_clock::time_point old_time;
        double rotation;
        std::cout << "Loading rotary encoder file: " << m_filename << std::endl;

        while(std::getline(ifs, line)) {
            // TBC
        }
    }

    std::vector<std::chrono::system_clock::time_point> RotaryEncoderLoader::getTimes() {
        return m_times;
    }

    std::vector<double> RotaryEncoderLoader::getRotations() {
        return m_rotations;
    }

} // namespace