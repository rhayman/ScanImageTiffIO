#include <numeric>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <date/date.h>
#include "../include/ScanImageTiff.h"

namespace twophoton
{

// specify the format of the time data in the logfile so it's parsed
// correctly later on
static constexpr char time_fmt[] = "%Y-%m-%d %T";
using FpMilliseconds = std::chrono::duration<float, std::chrono::milliseconds::period>;

//Convert radians to degrees...
static inline double rad2deg(double rad){return rad*(180.0/M_PI);}
// ... and degrees to radians
static inline double deg2rad(double deg){return deg*(M_PI/180.0);}

// constrain angles to lie between 0 and 2*PI
static double constrainAngleToPi(double x) {
	x = std::fmod(x, 2*M_PI);
	if ( x < 0 )
		x += 2*M_PI;
	return x;
};

// zeroRotations used in interpTiffData to subtract first element
// from all others
struct zeroRotations {
	double val;
	zeroRotations(double v) : val(v) {};
	void operator()(double &elem) const {
		elem -= val;
	}
};

// normalizes each value to lie between 0 and 1 - used in processData() below
struct normalize {
	double minval, maxval;
	normalize(double min, double max) : minval(min), maxval(max) {};
	void operator()(double &elem) const
	{
		elem = (elem - minval) / (maxval - minval);
	}
};

static void zeroNormalize(std::vector<double> & vec) {
	// add the min val to zero and then divide by
	// the max result of this to get values between 0 and 1
	auto result = std::min_element(vec.begin(), vec.end());
	auto result2 = std::max_element(vec.begin(), vec.end());
	std::for_each(vec.begin(), vec.end(), normalize(*result, *result2));
}

// finds the first value in vector vec that does not compare less
// than target
int closest(std::vector<double> const & vec, double target) {
	auto const it = std::lower_bound(vec.begin(), vec.end(), target);
	if ( it == vec.end()) { return -1; }
	return std::distance(std::begin(vec), it);
}

LogFileLoader::LogFileLoader(std::string fname) {
	filename = fname;
}

void LogFileLoader::setFilename(std::string f) {
	filename = f;
}

int LogFileLoader::getRotation(int idx) {
	return rotation[idx];
}

double LogFileLoader::getXTranslation(int idx) {
	return x_translation[idx];
}

double LogFileLoader::getZTranslation(int idx) {
	return z_translation[idx];
}

std::vector<double> LogFileLoader::getX() {
	return x_translation;
}

std::vector<double> LogFileLoader::getZ() {
	return z_translation;
}

std::vector<double> LogFileLoader::getTheta() {
	return rotation_in_rads;
}

std::vector<int> LogFileLoader::getLineNums() {
	return logfile_line_numbers;
}

double LogFileLoader::getRadianRotation(int idx) {
	return rotation_in_rads[idx];
}

double LogFileLoader::getTime(int idx) {
	return times[idx];
}

int LogFileLoader::getTriggerIndex() {
	return trigger_index;
}

bool LogFileLoader::containsAcquisition() {
	return hasAcquisition;
}

std::chrono::system_clock::time_point LogFileLoader::getTriggerTime() {
	return trigger_ptime;
}

std::vector<std::chrono::system_clock::time_point> LogFileLoader::getPTimes() {
	return ptimes;
}

std::vector<double> LogFileLoader::getTimes() {
	return times;
}

int LogFileLoader::findIndexOfNearestDuration(double this_duration) {
	return 1 + closest(times, this_duration); //a vector of double - see calculateDurationsAndRotations() below
}

void LogFileLoader::interpolatePositionData(std::vector<double> tiffTimestamps) {
	if ( ! m_interpolationDone ) { // only interpolate once!
		std::cout << "\nExtracting position data from logfile based on tiff file timestamps..." << std::endl;
		zeroNormalize(x_translation);
		zeroNormalize(z_translation);
		if (!(times.empty())) {
			std::vector<double>::iterator low;
			std::vector<double> interpRotations;
			std::vector<double> interpX;
			std::vector<double> interpZ;
			std::vector<double> interpTimes;
			std::vector<int> interpLineNumbers;
			std::vector<std::chrono::system_clock::time_point> interpPTimes;
			for (std::vector<double>::iterator i = tiffTimestamps.begin(); i != tiffTimestamps.end(); ++i) {
				low = std::lower_bound(times.begin(), times.end(), *i);
				auto interpIdx = low - times.begin();
				interpRotations.push_back(rotation_in_rads[interpIdx]);
				interpX.push_back(x_translation[interpIdx]);
				interpZ.push_back(z_translation[interpIdx]);
				interpTimes.push_back(times[interpIdx]);
				interpPTimes.push_back(ptimes[interpIdx]);
				interpLineNumbers.push_back(logfile_line_numbers[interpIdx]);
			}
			auto sz = interpRotations.size();

			x_translation.resize(sz);
			x_translation.shrink_to_fit();
			x_translation = interpX;

			z_translation.resize(sz);
			z_translation.shrink_to_fit();
			z_translation = interpZ;

			rotation_in_rads.resize(sz);
			rotation_in_rads.shrink_to_fit();
			rotation_in_rads = interpRotations;

			times.resize(sz);
			times.shrink_to_fit();
			times = interpTimes;

			ptimes.resize(sz);
			ptimes.shrink_to_fit();
			ptimes = interpPTimes;

			logfile_line_numbers.resize(sz);
			logfile_line_numbers.shrink_to_fit();
			logfile_line_numbers = interpLineNumbers;

		}
		m_interpolationDone = true;
		std::cout << "\nFinished extracting position data." << std::endl;
		std::cout << "Final number of samples extracted from log file: " << x_translation.size() << std::endl;
	}
}

bool LogFileLoader::calculateDurationsAndRotations() {
	if ( ! hasAcquisition ) {
		std::cout << "Warning: The file " << filename << " has no microscope trigger associated" << std::endl;
		return false;
	}
	else {
		std::cout << "Calculating rotations and times from log file data..." << std::endl;
		auto first_time = getTriggerTime();
		unsigned int count = 0;
		for (std::vector<std::chrono::system_clock::time_point>::iterator i = ptimes.begin(); i != ptimes.end(); ++i) {
			auto duration = *i - first_time;
			times.push_back(FpMilliseconds(duration).count()/1000.0);
			auto raw_rotation = 2 * M_PI * (double(rotation[count]) / (double)rotary_encoder_units_per_turn);
			rotation_in_rads.push_back(constrainAngleToPi(raw_rotation));
			++count;
		}
		std::cout << "Finished calculating rotations and times." << std::endl;
		std::cout << "The raw log file has " << times.size() << " timestamps in it." << std::endl;
		zeroNormalize(x_translation);
		zeroNormalize(z_translation);
		return true;
	}
};

bool LogFileLoader::interpTiffData(std::vector<double> tiffTimestamps) {
	if (!(times.empty()))
	{
		std::cout << "\nInterpolating logfile data from .tiff file timestamps..." << std::endl;
		std::vector<double>::iterator low;
		std::vector<double> interpRotations;
		for (std::vector<double>::iterator i = tiffTimestamps.begin(); i != tiffTimestamps.end(); ++i)
		{
			low = std::lower_bound(times.begin(), times.end(), *i);
			interpRotations.push_back(rotation_in_rads[low - times.begin()]);
		}
		rotation_in_rads.resize(interpRotations.size());
		rotation_in_rads.shrink_to_fit();
		rotation_in_rads = interpRotations;
		std::cout << "\nFinished interpolating logfile data." << std::endl;
		return true;
	}
	else
		return false;
}

int LogFileLoader::findNearestIdx(double tiffTimestamp) {
	if (!(times.empty())) {
		std::vector<double>::iterator low;
		low = std::lower_bound(times.begin(), times.end(), tiffTimestamp);
		auto time_idx = low - times.begin();
		return time_idx;
	}
	return -1;
}

bool LogFileLoader::load() {
	std::ifstream ifs(filename, std::ifstream::in);
	ifs.unsetf(std::ios_base::skipws);
	std::string line, old_line, s1;
	std::chrono::system_clock::time_point pt;
	std::chrono::system_clock::time_point old_time;//(boost::gregorian::date(2002,1,10),boost::posix_time::time_duration(1,2,3));// old line is our "memory" - see comment in while loop below and header file
	 //see comment before loop that sets trigger index below (after the next while statement)
	// to understand why this temporary is used
	std::chrono::system_clock::time_point tmp_trigger_ptime;
	std::size_t pos, posZ;
	double x_trans, z_trans;
	unsigned int trig_index = 0;
	int line_index = 0;
	std::cout << "\nLoading log file: " << filename << std::endl;
	while (std::getline(ifs, line)) {
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
    	if (pos != std::string::npos) {
    		auto s2 = line.substr(pos);
        	pos = s2.find_last_of(space_token);
        	auto tmp = s2.substr(pos);
        	init_rotation = std::stoi(tmp);
    	}
		std::size_t found = line.find(rot_token);
		if (found != std::string::npos) {
        	//get the date...
        	pos = line.find(X_token);
        	s1 = line.substr(0, pos);
        	std::istringstream is(s1);
        	is >> date::parse(time_fmt, pt);
        	if ( old_time != pt ) {
        		// get the line number first
        		logfile_line_numbers.push_back(line_index);
        		// grab the posix time
	        	ptimes.push_back(pt);

	        	//get the translation...
				auto pos_MX = line.find(X_token);
				auto pos_MY = line.find(Z_token);
				auto pos_Rot = line.find(rot_token);
				
				s1 = line.substr(pos_MX, pos_MY-pos_MX);
				auto tmp_pos = s1.find(equal_token);
				auto s2 = s1.substr(tmp_pos+1);
				x_trans = std::stof(s2);
	        	x_translation.push_back(x_trans);

				s1 = line.substr(pos_MY, pos_Rot-pos_MY);
				tmp_pos = s1.find(equal_token);
				s2 = s1.substr(tmp_pos+1);
	        	z_trans = std::stof(s2);
	        	z_translation.push_back(z_trans);

	        	//grab the amount of rotation...
	        	pos = line.find(rot_token);
	        	s1 = line.substr(pos);
	        	pos = s1.find(equal_token);
	        	s1 = s1.substr(pos);
	        	pos = s1.find(mouse_move_token);
	        	s1 = s1.substr(1,pos-2);
	        	rotation.push_back(std::stoi(s1));
	        }
	    }

        std::size_t scope_triggered = line.find(scope_token);
        if (scope_triggered != std::string::npos) {
        	hasAcquisition = true;
        	tmp_trigger_ptime = pt;
        }
        // deal with newer versions of the logfile
        scope_triggered = line.find(spacebar_token);
        if (scope_triggered != std::string::npos) {
        	hasAcquisition = true;
        	tmp_trigger_ptime = pt;
        }
        old_time = pt;
		++line_index;
	}
	/* I think we should set the start of logfile -> tifffile registration
	at the time point immediately following the string that says acquisition
	was started, so we modify the trigger_ptime and the trigger index to be 1
	item further ahead than the time detected in the logfile
	*/
	for (unsigned int i = 0; i < ptimes.size(); ++i) {
		if ( tmp_trigger_ptime == ptimes[i] ) {
			trigger_ptime = ptimes[i];
			setTriggerIndex(i);
		}
	}
	if ( calculateDurationsAndRotations() ) {
		isloaded = true;
	}
    return isloaded;
}
void LogFileLoader::setTriggerIndex(int n) {
	trigger_index = n;
}

std::vector<std::pair<int, int>> LogFileLoader::findStableFrames(const unsigned int minFrames, const double minAngle) {
	auto theta = getTheta();

	std::vector<double> theta_df;
	theta_df.resize(theta.size());

	// get the 2nd derivative so zeros are where there is no rotation
	std::adjacent_difference(theta.begin(), theta.end(), theta_df.begin());
	std::adjacent_difference(theta_df.begin(), theta_df.end(), theta_df.begin());
	// set a small tolerance for rounding error and return zeros where there's no rotation, 1 otherwise
	std::transform(theta_df.begin(), theta_df.end(), theta_df.begin(), [minAngle](double &n){ if (n > -minAngle && n < minAngle) return 0; else return 1; });

	std::vector<int> th(theta_df.begin(), theta_df.end());

	int start, end;
	std::vector<std::pair<int, int>> stableFrames;

	std::vector<int>::iterator iter = th.begin();
	while ( iter != th.end() )
	{
		iter = std::find(th.begin() + std::distance(th.begin(), iter), th.end(), 0);
		start = std::distance(th.begin(), iter);
		iter = std::find(th.begin() + std::distance(th.begin(), iter), th.end(), 1);
		end = std::distance(th.begin(), iter);
		auto res = end-start;
		if (res > minFrames)
		{
			stableFrames.push_back(std::make_pair(start, end));
		}
	}
	return stableFrames;
}

} // namespace twophoton