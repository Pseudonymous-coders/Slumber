//STANDARD INCLUDES
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <ctime>

//BOOST TIME INCLUDES
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>

#ifndef SLUMBER_TIMESTAMPS_H
#define SLUMBER_TIMESTAMPS_H

namespace TimeStamp {

	template<typename T>
	inline boost::posix_time::ptime ToPOSIX(T timestamp) {
		return boost::posix_time::from_time_t(
			boost::lexical_cast<unsigned long>(timestamp)
		);
	}

	inline const std::string POSIXToString(
		boost::posix_time::ptime timestamp) {
			const boost::gregorian::date dd = timestamp.date();
			auto td = timestamp.time_of_day();
			
			std::stringstream dstr;
			dstr << dd.year() << "-" << 
				std::setw(2) << std::setfill('0') <<
				dd.month().as_number() << "-" << std::setw(2) << 
				std::setfill('0') << dd.day() << " " << td.hours() << ":" <<
				td.minutes() << ":" << td.seconds();
		return dstr.str();
	}
	
	template<typename T>
	inline boost::posix_time::ptime
		GregToPOSIX(T timestamp) {
			return boost::posix_time::time_from_string(
				boost::lexical_cast<std::string>(timestamp)
			);
	}
	
	inline boost::posix_time::ptime NowLocal() {
		return boost::posix_time::second_clock::local_time();
	}
	
	inline boost::posix_time::ptime NowUniversal() {
		return boost::posix_time::second_clock::universal_time();
	}
	
	inline long SecondDuration(
		boost::posix_time::ptime start, boost::posix_time::ptime end) {
		return (end - start).total_seconds();
	}

}

#endif
