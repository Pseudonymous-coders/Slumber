#ifndef SERVERSTREAM_H
#define SERVERSTREAM_H

//WEBSCOKETPP Includes
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

//CPPREST INCLUDES
#include <cpprest/json.h>

//BOOST INCLUDES
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

//STANDARD INCLUDES
#include <iostream>
#include <memory>
#include <functional>

//SLUMBER INCLUDES
#include <security/account.hpp>
#include <util/log.hpp>
#include <util/config.hpp>

#define SLUMBER_SERVERSTREAM_LOGTAG			"SERVST" //The logger namespace tag
#define SLUMBER_SERVERSTREAM_RETRY			true //If client failed to connect to server attempt retries
#define SLUMBER_SERVERSTREAM_RETY_AMOUNT	-1 //If retry is enabled specify the retry limit (-1 is infinite)
#define SLUMBER_SERVERSTREAM_TOKEN_TRY		30 //Attempts to try and get a token before continuing
#define SLUMBER_SERVERSTREAM_TOKEN_DELAY	1000 //The millisecond count to delay between each failed token check


namespace slumber { namespace server {

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class ServerStream {
public:
		typedef boost::chrono::duration<int, boost::micro> dur_type;
		typedef std::function< void(slumber::server::ServerStream *) > b_handler_base_t;
		typedef std::function< void(slumber::server::ServerStream *, std::string) > b_base_t;


		ServerStream(const std::string &, unsigned int);
		~ServerStream();
		
		void attachAccount(security::Account *);
		void startLoop();

		void onInitHandler(websocketpp::connection_hdl);
		context_ptr onTLSInitHandler(websocketpp::connection_hdl);
		void onOpenHandler(websocketpp::connection_hdl);
		void onCloseHandler(websocketpp::connection_hdl);
		void onFailHanlder(websocketpp::connection_hdl);
		void onMessageHandler(websocketpp::connection_hdl, message_ptr);

		void attachOnOpen(b_handler_base_t);
		void attachOnMessage(b_base_t);
		void attachOnClose(b_handler_base_t); 

		void sendMessage(websocketpp::connection_hdl, std::string);
		client getClient();

		template<typename T>
		void _Logger(const T, bool err = false);
private:
		client _client;
		security::Account *_account;

		b_handler_base_t _open_handler, _close_handler;
		b_base_t _message_handler;

		bool _account_init;
		bool _is_authenticated;

		std::string _ip_addr;
		unsigned int _port;
};

}
}

typedef std::function< void(slumber::server::ServerStream *) > handler_base_t;
typedef std::function< void(slumber::server::ServerStream *, std::string) > message_base_t;

namespace AutomaticGeneration {
	
	void automaticStreams(int, handler_base_t, message_base_t, handler_base_t);
	void destructAutomaticStreams();

	extern std::vector<slumber::server::ServerStream *> server_streams;
}

#endif
