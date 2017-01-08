#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <stdio.h>
#include <cstring>

#ifndef SLUMBERERROR_H
#define SLUMBERERROR_H

#define SLUMBER_OK


#define SLUMBER_TOKEN_SERVER_NOT_FOUND_ERROR 113

namespace Error{

	class TokenError : public std::runtime_error {
	public:
		TokenError(int code) : std::runtime_error("TokenError"), 
				_error_code(code) {}
		
		virtual const char * what() const throw() {
			char * retMessage = new char[400];
			char buff[30], buffM[370];
			sprintf(buff, "TokenError: Code %d... ", _error_code);
			
			switch(_error_code) {
				case SLUMBER_TOKEN_SERVER_NOT_FOUND_ERROR:
					sprintf(buffM, "Server didn't respond, either no \
network is available or server isn't running");
					break;
				default:
					sprintf(buffM, "Unknown error code");
					break;
			}
			sprintf(retMessage, "%s%s", buff, buffM);
			return retMessage;
		}
		
	private:
		int _error_code;
	};

}

#endif //SLUMBERERROR_H
