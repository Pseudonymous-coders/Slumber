#include <iostream> //General std functions such as cout
#include <fstream> //File input and output
#include <string> //String functions

//BOOST includes
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <cpprest/http_client.h> //Http/Rest client library
#include <cpprest/json.h> //JSON library
#include <cpprest/uri.h> //URI library
#include <cpprest/containerstream.h> //Async streams backed by STL containers
#include <cpprest/interopstream.h> //Async stream bridges
#include <cpprest/rawptrstream.h> //Async streams backed by raw pointer to memory
#include <cpprest/producerconsumerstream.h>  //Async streams for producer consumer scenarios

#include <util/config.hpp> //Global slumber definitions

#ifndef SLUMBER_TOKENS_H
#define SLUMBER_TOKENS_H

/** @class Tokenizer tokens.hpp
 * @brief Class to handle the users tokens to the Slumber Server
 *
 * This will handle the server request for the tokens and authentication of the hub
 * making sure the connection is secured and no data is leaked. Side note: this also
 * handles the saving and loading of the profile to be persistent between boots
 *
 */
class Tokenizer {
public:
	//Constructor for defualt args username and password
	Tokenizer(const std::string &, const std::string &);
	
	//Constructor for defuault test sample
	Tokenizer();
	
	//Deconstructor of the tokenizer, release current user token
	~Tokenizer();
	
	//Set the credentials to load the token from the server
	void setCreds(const std::string &, const std::string &, bool save=false);
	
	//Get the loaded file credentials or current user space credentials
	void getCreds(const std::string &, const std::string &, bool load=false);

	pplx::task<void> pullToken(const std::function< void() > &func = {});
	pplx::task<void> pullCheckToken();

	//Start the token thread loop
	void start();
private:
	std::string _user, _pass, _token;
	
	long _delayAmount;
	
	bool _validToken;
	
	boost::optional<web::http::client::http_client_config> _hconfig;
	boost::optional<web::http::client::http_client> _httpclient;
	boost::optional<web::uri_builder> _hbuilder;
	
	boost::thread *_loopThread;

	void _constructHTTP();
	
	template<typename T>
	void _Logger(const T &, const bool err=false) const;
	
	void _loopTokens();
};

#endif //SLUMBER_TOKENS_H
