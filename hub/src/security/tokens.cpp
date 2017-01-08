//STD INCLUDES
#include <functional>

#include <security/tokens.hpp> //Main header file

//BOOST includes
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

//CPPREST includes
#include <cpprest/http_client.h> //Http/Rest client library
#include <cpprest/json.h> //JSON library
#include <cpprest/uri.h> //URI library
#include <cpprest/containerstream.h> //Async streams backed by STL containers
#include <cpprest/interopstream.h> //Async stream bridges
#include <cpprest/rawptrstream.h> //Async streams backed by raw pointer to memory
#include <cpprest/producerconsumerstream.h>  //Async streams for producer consumer scenarios

#include <util/log.hpp> //Slumbers logging and print handling
#include <util/timestamps.hpp> //Time handling and common functions
#include <slumbererror.hpp> //Custom errorcode handler

using namespace utility; //Common utilities like string conversions
using namespace web; //Common features like URIs.
using namespace web::http; //Common HTTP functionality
using namespace web::http::client; //HTTP client features
using namespace concurrency::streams; //Asynchronous streams
using namespace web::json; //JSON utils


/**
 * @brief Initialize a tokenizer on a user account
 *
 * @param STRING - the username to set the account to
 * @param STRING - the password associated with the username
 * @return TOKENIZER - the tokenizer constructor
 */
Tokenizer::Tokenizer(const std::string &user, const std::string &pass) {
	this->setCreds(user, pass); //Set user credentials
	this->_validToken = false;
	this->_token = "";
	_constructHTTP();
}

/**
 * @brief Initialize a tokenizer on a user account
 *
 * @note the default username and password will be used
 * @return TOKENIZER - the tokenizer constructor
 */
Tokenizer::Tokenizer() {
	this->setCreds("test@example.com", "password"); //Set as default view
	this->_validToken = false;
	this->_token = "";
	_constructHTTP();
}

Tokenizer::~Tokenizer() {
	_loopThread->interrupt(); //Stop the looping thread
	delete this->_loopThread;
}


/**
 * @brief Private method to construct the cpprest objects
 *
 */
void Tokenizer::_constructHTTP() {
	_Logger(SW("Attaching to Slumber server: ") + SLUMBER_SERVER_DOMAIN);
	Tokenizer::_hconfig = http_client_config();
	
	#if defined(SLUMBER_TOKEN_HTTPS_VALID) && SLUMBER_TOKEN_HTTPS_VALID == false
		Tokenizer::_hconfig->set_validate_certificates(false);
		_Logger(SW("Setting to not validate certificates"));
	#else
		Tokenizer::_hconfig->set_validate_certificates(true);
		_Logger(SW("Setting to validate certificates"));
	#endif
	
	//Create the client to access the server
	Tokenizer::_httpclient = 
				http_client(SLUMBER_SERVER_DOMAIN, Tokenizer::_hconfig.get());
	
	//Create the base builder for the authentication
	Tokenizer::_hbuilder = uri_builder(SLUMBER_TOKEN_DOMAIN_PATH);
	
	_Logger(SW("Built the authentication path: ") 
		+ SLUMBER_TOKEN_DOMAIN_PATH);
}

void Tokenizer::setCreds(const std::string &user, 
								const std::string &pass, bool save) {
	//Set the 
	this->_user = user;
	this->_pass = pass;
	
	#if defined(SLUMBER_TOKEN_LOG_USERPASS) && SLUMBER_TOKEN_LOG_USERPASS == true
		_Logger(SW("Set the username: ") + user + SW(" and password: ") + pass);
	#endif
	
	if(save) {
		_Logger(SW("Saving the new credentials"));
		//std::of
	}
	
}

template<typename T>
void Tokenizer::_Logger(const T &toLog, const bool err) const {
	//Log the new Token namespace sample
	Logger::Log<T>(SLUMBER_TOKEN_LOGTAG, toLog, err);
}

void Tokenizer::start() {
	_loopThread = new boost::thread(boost::bind(
		&Tokenizer::_loopTokens, this));
	_Logger(SW("Attempted to start token mainloop!"));
}

void Tokenizer::_loopTokens() {
	_Logger(SW("Starting Tokenizer mainloop..."));
	
	//Run until the object goes out of scope or is deleted
	while(1) {
		//We don't want to quit our loop unless something really fatal happens
		try {
			_Logger(SW("Pulling new token from server! Try 1..."));
			
			//Try pulling multiple times for safety
			for(int i = 1; i <= SLUMBER_TOKEN_RETRY_AMOUNT; i++) {
				pullToken().wait(); //Pull the new token and update vars
				pullCheckToken().wait(); //Check to see if the token is valid
			
				if(this->_validToken) {
					_Logger(SW("Got token on try: ") + SW(i));
					break;
				}
				boost::this_thread::sleep_for(
					boost::chrono::seconds(SLUMBER_TOKEN_RETRY_SLEEP));
				_Logger(SW("Failed on try: ") + SW(i));
			}
			
			//Calculate new pull token value based on defined fraction
			long sleepAmount = static_cast<long>(
				(static_cast<float>(SLUMBER_TOKEN_REMOVE_FRACTION) * 
				static_cast<float>(this->_delayAmount))
			);
			
			_Logger(SW("Waiting for the token to expire! Seconds: ") 
				+ SW(sleepAmount));
			
			boost::this_thread::sleep_for(boost::chrono::seconds(sleepAmount));
		} catch (const Error::TokenError &terr) {
			_Logger(SW("Error recieving token!") + terr.what()); 
		} catch (const  boost::thread_interrupted &err) {
			_Logger(SW("Token thread interrupted, killing!"));
			break;
		} catch (const std::exception &err) {
			_Logger(SW("General loop error: ") + err.what());
		}
	}
}

//Return function async type
pplx::task<void> Tokenizer::pullToken(const std::function< void() > &func) {
	//Log the attempt of the token request
	_Logger(SW("Attempting to retrieve a new Auth Token from the server!"));
	
	//Create the new request
	http_request hreq(SLUMBER_TOKEN_GET_METHOD_TYPE); //Server method (check config.hpp)
	
	//Set the request uri
	hreq.set_request_uri(this->_hbuilder.get().to_string());
	hreq.headers().set_content_type("application/x-www-form-urlencoded");
	std::stringstream bodyss;
	bodyss << "email=" << this->_user << "&password=" << this->_pass;
	
	hreq.set_body(bodyss.str());
	
	return (this->_httpclient.get()).request(hreq)
			.then([=](http_response resp) {
		int status_code = resp.status_code(); //Get HTTP code response
	
		if(status_code == status_codes::OK) {
			_Logger(SW("Server succesfully responded! 200"));
			return resp.extract_json(); //Async json parsing
		} else {
			_Logger(SW("Server ERRORED status code: ") + SW(status_code), true);
			return pplx::task_from_result(json::value()); //Send empty json
		}
	}).then([=](pplx::task<json::value> jsonParsed) {
		try {
			json::value respjson = jsonParsed.get();
			
			if(respjson.is_null()) throw http_exception("Invalid response");
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_SUCCESS)) {
				throw json_exception("Can't calculate success of token");
			}
			
			if(respjson.at(SLUMBER_TOKEN_JSON_SUCCESS).as_bool() != true) {
				std::stringstream ss;
				ss << "Server message -> ";
				ss << respjson.at("message").as_string();
				throw http_exception(ss.str());
			}
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_EXPIRE)) {
				throw json_exception("Can't calculate the expire of token");
			}
			
			boost::posix_time::ptime posixTemp = TimeStamp::ToPOSIX(
				respjson.at(SLUMBER_TOKEN_JSON_EXPIRE).as_number().to_uint32()
			);
			
			//Set the loop delay amount
			this->_delayAmount = TimeStamp::SecondDuration(
				TimeStamp::NowUniversal(), posixTemp); //Get differencial 
			
			//Print expire time of token
			_Logger(SW("Token expires at: ") + TimeStamp
				::POSIXToString(posixTemp) + 
				" Seconds: " + SW(this->_delayAmount));
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_TOKEN)) {
				throw json_exception("Can't grab the token from the response");
			}
			
			this->_token = respjson.at(SLUMBER_TOKEN_JSON_TOKEN).as_string();
			
			#if defined(SLUMBER_TOKEN_LOG_USERPASS) && SLUMBER_TOKEN_LOG_USERPASS == true
				_Logger(SW("New token from server: ") + _token); 
			#endif
			
			if(func) func(); //Call asynchronous function
			
		} catch (http_exception const & err) {
			_Logger(SW("http_request error: ") + err.what(), true);
			this->_validToken = false;						
			throw Error::TokenError(err.error_code().value()); //Handle the error
		} catch (json_exception const & err) {
			_Logger(SW("Failed parsing token response from server! ERROR: ") 
									+ err.what(), true);
			this->_validToken = false;
		} catch(...) {
			_Logger(SW("Slumber token FATAL unknown error!"));
			this->_validToken = false;
		}
	});
}

pplx::task<void> Tokenizer::pullCheckToken() {
	//Log the attempt of the token request
	_Logger(SW("Checking if the token is a valid token"));
	
	//Create the new request
	http_request hreq(SLUMBER_TOKEN_CHECK_METHOD_TYPE); //Server method (check config.hpp)
	
	//Set the request uri
	uri_builder uri_b = this->_hbuilder.get(); //Make a copy of the uri_builder
	uri_b.append_query("token", this->_token); //Check if this token is valid
	hreq.set_request_uri(uri_b.to_string());
	
	return (this->_httpclient.get()).request(hreq)
			.then([=](http_response resp) {
		int status_code = resp.status_code(); //Get HTTP code response
	
		if(status_code == status_codes::OK) {
			_Logger(SW("Server succesfully responded! 200"));
			return resp.extract_json(); //Async json parsing
		} else {
			_Logger(SW("Server ERRORED status code: ") + SW(status_code), true);
			return pplx::task_from_result(json::value()); //Send empty json
		}
	}).then([=](pplx::task<json::value> jsonParsed) {
		try {
			json::value respjson = jsonParsed.get();
			
			if(respjson.is_null()) throw http_exception("Invalid response");
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_CHECK)) {
				throw json_exception("Can't calculate success of token");
			}
			
			if(respjson.at(SLUMBER_TOKEN_JSON_CHECK).as_string()
					.compare(SW("granted")) != 0) { //Check if the token success is true
				std::stringstream ss;
				ss << "Server message -> ";
				ss << respjson.at("message").as_string();
				throw http_exception(ss.str());
			}
			
			this->_validToken = true;
		} catch (http_exception const & err) {
			_Logger(SW("http_request error: ") + err.what(), true);
			this->_validToken = false;						
			throw Error::TokenError(err.error_code().value()); //Handle the error
		} catch (json_exception const & err) {
			_Logger(SW(
				"Failed parsing check token response from server! ERROR: ") 
					+ err.what(), true);
			this->_validToken = false;
		} catch(...) {
			_Logger(SW("Slumber token FATAL unknown error!"));
			this->_validToken = false;
		}
	});
}

