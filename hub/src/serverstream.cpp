#include <serverstream.hpp>



namespace slumber { namespace server {

ServerStream::ServerStream(const std::string &ip, unsigned int port) {
	//Set full logging levels on the client websocket
	this->_client.set_access_channels(websocketpp::log::alevel::all);
	this->_client.set_error_channels(websocketpp::log::alevel::all);

	//Start boost async
	this->_client.init_asio();


	this->_ip_addr = ip + ((port != 0) ? (SW(":") + SW(port)) : "");
	this->_port = port;
	this->_account_init = false;
	this->_is_authenticated = false;

	//Register the handlers
	this->_client.set_socket_init_handler(bind(&ServerStream::onInitHandler, this, ::_1));
	this->_client.set_tls_init_handler(bind(&ServerStream::onTLSInitHandler, this, ::_1));
	this->_client.set_message_handler(bind(&ServerStream::onMessageHandler, this, ::_1, ::_2));
	this->_client.set_open_handler(bind(&ServerStream::onOpenHandler, this, ::_1));
	this->_client.set_close_handler(bind(&ServerStream::onCloseHandler, this, ::_1));
}

ServerStream::~ServerStream() {

}

void ServerStream::attachAccount(security::Account *account) {
	this->_account = account;
	this->_account_init = true;
}

void ServerStream::startLoop() {
	_Logger(SW("Attempting to connect to ") + this->_ip_addr);
	websocketpp::lib::error_code error_code;
	client::connection_ptr con = this->_client.get_connection(this->_ip_addr, error_code);

	//If an initialization of the code failed then print out the error
	if(error_code) {
		_Logger(SW("Failed to initialize the websocket connection! ERROR: ") + error_code.message(), true);
		return;
	}

	_Logger(SW("Websocket connecting to client"));

	//Connect to the client
	this->_client.connect(con);

	//Start the asynchronous response loop
	this->_client.run();

	_Logger(SW("Websocket connection loop started"));
}

void ServerStream::onInitHandler(websocketpp::connection_hdl hdl) {
	//Log the initialization of the websocket handler
	_Logger(SW("Websocket initialized on ") + this->_ip_addr);
}

bool verify_callback(
	bool preverified, // True if the certificate passed pre-verification.
	boost::asio::ssl::verify_context& ctx // The peer certificate and other context.
)
{
  return true;
}


context_ptr ServerStream::onTLSInitHandler(websocketpp::connection_hdl hdl) {
	context_ptr context = websocketpp::lib::make_shared<boost::asio::ssl::context>
		(boost::asio::ssl::context::tlsv12_client);

	try {
		context->set_verify_callback(verify_callback); //Always validate certificates 
		context->set_options(boost::asio::ssl::context::sslv3_client);
	} catch(const std::exception &err) {
		_Logger(SW("Failed to create the secure Websocket connection! ERROR: ") + err.what());
	}

	return context;
}

void ServerStream::onOpenHandler(websocketpp::connection_hdl hdl) {
	//Log the opening connection of the websocket connection
	_Logger(SW("Websocket connection initiated on ") + this->_ip_addr);

	std::string tokenSend;

	try {
		MUTEX_GLOBAL_SLOCK; //Lock onto main thread before pulling new Token

		if(!this->_account_init) {
			_Logger(SW("There isn't any account attached to websocket!"), true);
			return;
		}
		
		//Attempt to check the validity of a token
		for(int ind = 0; ind < SLUMBER_SERVERSTREAM_TOKEN_TRY; ind++) {
			if(this->_account->isServerTokenValid()) {
				break;
			} else {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(SLUMBER_SERVERSTREAM_TOKEN_DELAY));
			}
		}

		if(!this->_account->isServerTokenValid()) {
			_Logger(SW("Failed getting a valid token from the server in time!"), true);
		}
		
		tokenSend = this->_account->getServerToken(); //Get the server Token from the tokenizer client 

	} catch(...) {
		_Logger(SW("Failed to get the token from the server for the Websocket connection!"), true);
	}

	web::json::value to_send;

	try {
		to_send["token"] = web::json::value(tokenSend);
	} catch(...) {
		_Logger(SW("Failed to generate the Websocket token check!"), true);
	}

	_Logger(SW("Sending the initial token check via the Websocket"));

	websocketpp::lib::error_code error_code;   
 
	this->_client.send(hdl, to_send.serialize(), websocketpp::frame::opcode::BINARY, error_code);
	
	if(error_code) {
		_Logger(SW("Failed to send initial token to server! ERROR: ") + error_code.message(), true);
	}

	_Logger(SW("Sent the authentication request token!"));

	this->_open_handler(this);
}

void ServerStream::onCloseHandler(websocketpp::connection_hdl hdl) {
	_Logger(SW("The websocket closed!"));
	this->_close_handler(this);
}

void ServerStream::onMessageHandler(websocketpp::connection_hdl hdl, message_ptr message) {
	std::string server_response = message->get_payload();	
	_Logger(SW("Recieved response from the websocket server"));

	try {
		if(this->_is_authenticated) {
			this->_message_handler(this, server_response);
		} else {
			_Logger(SW("Websocket stream currently not authenticated, attempting to check if the request was successful!"));

			web::json::value auth_json = web::json::value::parse(server_response);
			if(!auth_json.has_field(SLUMBER_TOKEN_JSON_SUCCESS)) {
				throw web::json::json_exception("The success field doesn't exist!");
			}

			if(auth_json.at(SLUMBER_TOKEN_JSON_SUCCESS).as_bool()) {
				_Logger(SW("Successfully captured websocket stream!"));	
				this->_is_authenticated = true;
			} else {
				_Logger(SW("Failed to authenticate the token with the server!"), true);
				this->_is_authenticated = false;
			}
		}
	} catch(const std::exception &err) {
		_Logger(SW("Failed getting authentication or message from server! ERROR: ") + err.what());
	}
}

void ServerStream::attachOnOpen(b_handler_base_t open_handle) {
	this->_open_handler = open_handle;
}

void ServerStream::attachOnMessage(b_base_t message_handle) {
	this->_message_handler = message_handle;
}

void ServerStream::attachOnClose(b_handler_base_t close_handle) {
	this->_close_handler = close_handle;
}

template<typename T>
void ServerStream::_Logger(T toLog, bool err) {
	Logger::Log(SLUMBER_SERVERSTREAM_LOGTAG, toLog, err);
}

}
}

namespace AutomaticGeneration {
	
	void automaticStreams(int amount, handler_base_t on_open, 
									  message_base_t on_message, 
									  handler_base_t on_close) {
		Logger::Log(SLUMBER_SERVERSTREAM_LOGTAG, SW("Automatically attaching to ") + 
			SW(amount) + " servers!");
		
		MUTEX_GLOBAL_SLOCK; //Lock onto main thread before getting a generator
		std::size_t account_size = security::AutomaticGeneration::Accounts::accounts.size();

		if(amount > A_MAX || amount > account_size) {
			amount = std::min(A_MAX, boost::lexical_cast<int>(account_size)); //Get the smallest out of the two

			Logger::Log(SLUMBER_SERVERSTREAM_LOGTAG, SW("Currently not enough accounts to attach servers to, Account size: ") + SW(account_size));

		}

		MUTEX_GLOBAL_UNLOCK; //Unlock from the main thread since the generator has already been accessed

		for(int ind = 0; ind < amount; ind++) {
			try {
				//@TODO MAKE SURE TO ADD THE GLOBAL EXTERN FOR THE SERVERS
				
				MUTEX_GLOBAL_SLOCK; //Make sure to lock onto the main thread before accessing the account information
				security::Account *account = security::AutomaticGeneration::Accounts::accounts.at(ind); //Get current account
				MUTEX_GLOBAL_UNLOCK; //We can now unlock from the main thread since the account data vector pointer is accessed

				//Create a new websocket client connection with the server
				slumber::server::ServerStream *server = new slumber::server::ServerStream(SLUMBER_SERVER_STREAM, SLUMBER_SERVER_STREAM_PORT);
				server->attachAccount(account); //Attach the server to a global account
				server->attachOnOpen(on_open);
				server->attachOnMessage(on_message);
				server->attachOnClose(on_close);
				server->startLoop(); //Start the band main loop
				//@TODO ADD THE SERVER DESTRUCTOR TO CLEAN UP THE DYNAMIC POINTERS
				Logger::Log(SLUMBER_SERVERSTREAM_LOGTAG, SW("Started server on account user ") + account->getUsername());
			} catch(const std::exception &err) {
				Logger::Log(SLUMBER_SERVERSTREAM_LOGTAG, SW("Failed to automatically generate a server on the account! ERROR: ") + err.what(), true);
			}
		}
	}

	void destructAutomaticStreams() {

	}

	//std::vector<slumber::server::ServerStream *> server_streams;
}
