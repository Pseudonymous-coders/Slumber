#ifndef SERVERSTREAM_H
#define SERVERSTREAM_H

//WEBSCOKETPP Includes
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

//STANDARD INCLUDES
#include <iostream>
#include <memory>
#include <functional>

//SLUMBER INCLUDES
#include <security/account.hpp>

#define SLUMBER_SERVERSTREAM_RETRY			true //If client failed to connect to server attempt retries
#define SLUMBER_SERVERSTREAM_RETY_AMOUNT	-1 //If retry is enabled specify the retry limit (-1 is infinite)


namespace slumber { namespace server {

class ServerStream {
public:
		ServerStream(const std::string &, const std::string &, unsigned int);
		~ServerStream();
		
		void attachAccount(security::account &);

		template<typename T>
		void _Logger(const T);
};

}
}

#endif
