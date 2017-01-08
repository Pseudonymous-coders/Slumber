#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdarg>

#include <util/config.hpp>

#define SLOG_INIT() Logger::Init()
#define SLOG_PRINT(X) Logger::Log(X)
#define SLOG_FREE() Logger::Free()

///@brief Handle the slumber log tee (file and actual print output) operations
#ifndef SLUMBER_LOG_TEE

//Default operation is to print both to file and console
#define SLUMBER_LOG_TEE 2

#endif //SLUMBER_LOG_TEE


///@brief Handle the date settings of the logger
#ifndef SLUMBER_LOG_DATE

//Default operation is to print the current logged date
#define SLUMBER_LOG_DATE true

#endif

///@brief Handle the namespace settings of the logger
#if ! defined(SLUMBER_LOG_NAMESPACE) || ! defined (SLUMBER_LOG_DEFAULT_NAMESPACE)

//Default operations for namespacing of Slumber log
#define SLUMBER_LOG_NAMESPACE true
#define SLUMBER_LOG_DEFAULT_NAMESPACE "GENERAL"

#endif  

///@brief Handle the log message locations
#ifndef SLUMBER_LOG_LOCATION

//Default log location will be in the log home folder
#define SLUMBER_LOG_LOCATION SLUMBER_HOME_BASE"/logs"
#define SLUMBER_LOG_LATEST_LOCATION SLUMBER_LOG_LOCATION "/latest.txt"

#endif


#ifndef SLUMBER_LOG_H
#define SLUMBER_LOG_H

/** @class Logger log.hpp
 * @brief Class to handle all data logs and streams for the slumber hub
 *
 * This will update logs that can be live saved to files check time stamps
 * and upload files to another host if needed to. Mainly used for error reporting
 * for the developers of Slumber.
 *
 */
namespace Logger {
	/**
	 * @brief Initialize the logger by opening a new tee stream
	 *
	 */
	void Init();
	
	/**
	 * @brief
	 *
	 */
	template<typename T>
	void Log(T);
	
	template<typename T>
	void Log(const char *, const T &, bool error=false);
	
	template<typename T>
	void LogError(T);
	
	template<typename T>
	void LogError(const char *, const T &);
	
	void Free();
}

#endif //SLUMBER_LOG_H
