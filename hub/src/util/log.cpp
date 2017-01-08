#include <util/log.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#define SLUMBER_LOGGER_PRINT "LOGGER"

#define NO_COLOR "\033[0m"
#define RED_COLOR "\033[0;31m"
#define BLUE_COLOR "\033[0;34m"


using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::filesystem;


/**
 * @brief Logger handler
 * Details located in header file. The reason why we use printf is because
 * it's lower than cout and takes less processing, so it's faster and just/almost as 
 * easy to use as cout
 */
namespace Logger {

	std::ofstream logFile;

	void Init() {
		#if SLUMBER_LOG_TEE == 2 || SLUMBER_LOG_TEE == 0
			if(logFile.is_open()) {
				Log(SLUMBER_LOGGER_PRINT, "Logger already opened");
				return;
			}
		
			//Check for the log folder so that the new file can be created
			path testLog(SLUMBER_LOG_LOCATION);
			
			try {
				if(!is_directory(status(testLog))) {
					Log(SLUMBER_LOGGER_PRINT, 
							"Slumber log folder doesn't exist! Creating...");
							
					if(create_directory(testLog)) {
						Log(SLUMBER_LOGGER_PRINT, "Done creating folder!");
					} else {
						LogError(SLUMBER_LOGGER_PRINT, "Failed creating log folder!");
					}
					
				}
			} catch (filesystem_error &e) {
				LogError(SLUMBER_LOGGER_PRINT, std::string("Log folder error: ") + e.what());
			}
		
			//Open the defined slumber log location
			logFile.open(SLUMBER_LOG_LATEST_LOCATION, std::ifstream::out);
		
			if(!logFile.is_open()) {
				LogError(SLUMBER_LOGGER_PRINT, "Failed loading Logger!"); 
				return;
			}
		#endif
		
		Log(SLUMBER_LOGGER_PRINT, "Started Slumber Logger!");
	}
	
	template<typename T>
	void Log(T toLog) {
		Log(SLUMBER_LOG_DEFAULT_NAMESPACE, toLog); //Use the standard namespace
	}
	
	template<typename T>
	void LogError(T toLog) {
		LoggError(SLUMBER_LOG_DEFAULT_NAMESPACE, toLog);	
	}
	
	template<typename T>
	void LogError(const char *space, const T &toPrint) {
		Log(space, toPrint, true);
	}
	
	std::string __formatTime(ptime now) {
	  time_facet* facet = new time_facet("%m/%d/%Y/ %H:%M:%S:%f");
	  std::stringstream wss;
	  wss.imbue(std::locale(wss.getloc(), facet));
	  wss << microsec_clock::universal_time();
	  return wss.str();
	}
	
	std::string __formatLog(const std::string &name_space, 
			const std::string &to_print, bool print_error=false, 
			bool color=false) {
		std::stringstream toret("");
		#if SLUMBER_LOG_DATE == true
			//Get current UTC date
			ptime now = second_clock::universal_time();
		
			toret << "[" << __formatTime(now) << "]";
		#endif
		
		#if SLUMBER_LOG_NAMESPACE == true
			toret << "|" << ((color) ? BLUE_COLOR : "") <<
					name_space << ((color) ? NO_COLOR : "") << "|:";
		#endif
		
		//Set the print color to red and show error inprint
		if(print_error) {
			toret << ((color) ? RED_COLOR : "") << 
					"(ERROR)-> ";
		}
		
		//Print the main message
		toret << to_print;
		
		//Reset the color back to normal
		if(print_error) {
			toret << ((color) ? NO_COLOR : "");
		}
		
		return toret.str();
	}
	
	template<typename T>
	void __printLog(const char *space, const T &toPrint, bool error=false) {
		std::cout << __formatLog(std::string(space), 
						std::string(toPrint), error, true) << std::endl;
	}
	
	template<typename T>
	void __saveLog(const char *space, const T &toPrint, bool error=false) {
		if(!logFile.is_open()) return;
		logFile << __formatLog(std::string(space), 
						std::string(toPrint), error) << std::endl;
	}
	
	template<typename T>
	void Log(const char * space, const T &toPrint, bool error) {
	#if SLUMBER_LOG_TEE == 0
		__saveLog<T>(space, toPrint, error);
	#else
		#if SLUMBER_LOG_TEE == 1
			__printLog<T>(space, toPrint, error);
		#else
			#if SLUMBER_LOG_TEE == 2
				__printLog<T>(space, toPrint, error);
				__saveLog<T>(space, toPrint, error);	
			#endif
		#endif
	#endif
	}
	
	void Free() {
		#if SLUMBER_LOG_TEE == 2 || SLUMBER_LOG_TEE == 0
			if(logFile.is_open()) {
				Log(SLUMBER_LOGGER_PRINT, "Closed the Logger!");
			} else {
				LogError(SLUMBER_LOGGER_PRINT, "Failed closing Logger!");
			}
		#endif
	}

}
