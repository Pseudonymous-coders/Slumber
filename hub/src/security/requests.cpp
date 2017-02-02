#include <iostream>
#include <string>
#include <algorithm>

#include <util/config.hpp> //Common configurations for Slumber
#include <security/requests.hpp>
#include <security/account.hpp>

//BOOST includes
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
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

//SLUMBER INCLUDES
#include <util/log.hpp> //Slumbers logging and print handling
#include <util/timestamps.hpp> //Time handling and common functions
#include <slumbererror.hpp> //Custom errorcode handler
#include <slumberui.h>

using namespace utility; //Common utilities like string conversions
using namespace web; //Common features like URIs.
using namespace web::http; //Common HTTP functionality
using namespace web::http::client; //HTTP client features
using namespace concurrency::streams; //Asynchronous streams
using namespace web::json; //JSON utils

namespace Requests {

unsigned int usedRequests = 0;
boost::mutex request_mutex;

//Return function async type
pplx::task<void> updateBandData(security::Account *account) {
	//Log the attempt of the token request
	_Logger(SW("Attempting to push new band values to the server!"));
	
	{
		boost::mutex::scoped_lock locks(request_mutex);
		if(usedRequests >= SLUMBER_BLE_DATA_FAIL_MAX) {
			_Logger(SW("Reached max request count, not sending current request!"), true);
			return pplx::task<void>(); //Do not continue
		}
		usedRequests++;
	}

	web::http::client::http_client_config http_config = http_client_config();
	
	#if defined(SLUMBER_BLE_SERVER_HTTPS_VALID) && SLUMBER_BLE_SERVER_HTTPS_VALID == false
		http_config.set_validate_certificates(false);
	#else
		http_config.set_validate_certificates(true);
	#endif
	
	//Create the client to access the server
	web::http::client::http_client http_client(SLUMBER_SERVER_DOMAIN, http_config);
	
	//Create the base builder for the authentication
	web::uri_builder h_builder = uri_builder(SLUMBER_BLE_SERVER_UPDATE_PATH);
	h_builder.append_query("token", account->getServerToken());
	
	_Logger(SW("Built the request path: ") 
		+ SLUMBER_BLE_SERVER_UPDATE_PATH);
	
	
	//Create the new request
	http_request hreq(methods::POST); //Server method (check config.hpp)
	
	//Set the request uri
	hreq.set_request_uri(h_builder.to_string());
	
	json::value body_json;
	body_json["accel"] = json::value::number(account->accelerometer);
	body_json["temp"] = json::value::number(account->temperature);
	body_json["hum"] = json::value::number(account->humidity);
	body_json["vbatt"] = json::value::number(account->voltage);
	
	//std::stringstream bodyss;
	//bodyss << "email=" << this->_user << "&password=" << this->_pass;
	
	hreq.set_body(body_json);
	
	return http_client.request(hreq)
			.then([=](http_response resp) {
		int status_code = resp.status_code(); //Get HTTP code response
	
		if(status_code == status_codes::OK) {
			_Logger(SW("Server succesfully responded! 200"));
			return resp.extract_json(); //Async json parsing
		} else {
			_Logger(SW("Server ERRORED status code: ") + SW(status_code), true);
		

			//Wait a predefined amount of time if the code responded with an invalid response (Add more time since the server doesn't contain the page)
			if(status_code > 400 && status_code < 410) {
				boost::this_thread::sleep_for(boost::chrono::seconds(SLUMBER_BLE_DATA_FAIL_SLEEP + 10));
			}

			return pplx::task_from_result(json::value()); //Send empty json
		}
	}).then([=](pplx::task<json::value> jsonParsed) {
		try {
			json::value respjson = jsonParsed.get();
			
			if(respjson.is_null()) throw http_exception("Invalid response");
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_CHECK)) {
				throw json_exception("Can't calculate the success of the server push");
			}

			//GET SMARTSCORE FROM USERDATA (@TODO REMOVE THIS WHEN COMPLETED THE TRANSFER)
			/*if(!respjson.has_field("smartScore")) {
				throw json_exception("CAN'T GET SMART SCORE!");
			}

			std::cout << "\n\n\n\n\n\nGOT RESPONSE: " << respjson.at("smartScore").as_number().to_int32() << std::endl;

			slumber::setProgress(respjson.at("smartScore").as_number().to_uint32());
			*/

			if(!respjson.at(SLUMBER_TOKEN_JSON_CHECK).as_bool()) {
				std::stringstream ss;
				ss << "Server message -> ";
				ss << respjson.at("error").as_string();
				throw http_exception(ss.str());
			}
			
		} catch (http_exception const & err) {
			_Logger(SW("http_request error: ") + err.what(), true);					
			if(strstr(err.what(), "invalid") != NULL || strstr(err.what(), "endpoint") != NULL || 
							strstr(err.what(), "resolving") != NULL) {
				boost::this_thread::sleep_for(
					boost::chrono::seconds(SLUMBER_BLE_DATA_FAIL_SLEEP));
			}

			throw Error::TokenError(err.error_code().value()); //Handle the error
			
		} catch (json_exception const & err) {
			_Logger(SW("Failed parsing push band data response from server! ERROR: ") 
									+ err.what(), true);
		} catch(...) {
			_Logger(SW("Slumber push values FATAL unknown error!"));
		}

		{
			boost::mutex::scoped_lock locks(request_mutex);
			if(usedRequests > 0) usedRequests--;
		}
	});
}


//Return function async type
pplx::task<void> setBandDetails(security::Account *account, json::value band_updates) {
	//Log the attempt of the token request
	_Logger(SW("Attempting to push new band values to the server!"));
	
	web::http::client::http_client_config http_config = http_client_config();
	
	#if defined(SLUMBER_BLE_SERVER_HTTPS_VALID) && SLUMBER_BLE_SERVER_HTTPS_VALID == false
		http_config.set_validate_certificates(false);
	#else
		http_config.set_validate_certificates(true);
	#endif
	
	//Create the client to access the server
	web::http::client::http_client http_client(SLUMBER_SERVER_DOMAIN, http_config);
	
	//Create the base builder for the authentication
	web::uri_builder h_builder = uri_builder(SLUMBER_BLE_SERVER_DETAILS_PATH);
	h_builder.append_query("token", account->getServerToken());
	
	_Logger(SW("Built the request path: ") 
		+ SLUMBER_BLE_SERVER_DETAILS_PATH);
	
	
	//Create the new request
	http_request hreq(methods::POST); //Server method (check config.hpp)
	
	//Set the request uri
	hreq.set_request_uri(h_builder.to_string());
	hreq.set_body(band_updates);
	
	return http_client.request(hreq)
			.then([=](http_response resp) {
		int status_code = resp.status_code(); //Get HTTP code response
	
		if(status_code == status_codes::OK) {
			_Logger(SW("Server succesfully responded! 200"));
			try {
				return resp.extract_json(); //Async json parsing
			} catch (http_exception const & err) {
				_Logger(SW("Failed getting json!"), true);
			}

			return pplx::task_from_result(json::value());
		} else {
			_Logger(SW("Server ERRORED status code: ") + SW(status_code), true);
			return pplx::task_from_result(json::value()); //Send empty json
		}
	}).then([=](pplx::task<json::value> jsonParsed) {
		try {
			json::value respjson = jsonParsed.get();

			if(respjson.is_null()) throw http_exception("Invalid response");
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_CHECK)) {
				throw json_exception("Can't calculate the success of the server push");
			}
			
			if(!respjson.at(SLUMBER_TOKEN_JSON_CHECK).as_bool()) {
				std::stringstream ss;
				ss << "Server message -> ";
				ss << respjson.at(SLUMBER_TOKEN_JSON_ERROR).as_string();
				throw http_exception(ss.str());
			}
			
		} catch (http_exception const & err) {
			_Logger(SW("http_request error: ") + err.what(), true);					
			throw Error::TokenError(err.error_code().value()); //Handle the error
			
		} catch (json_exception const & err) {
			_Logger(SW("Failed parsing push band data response from server! ERROR: ") 
									+ err.what(), true);
		} catch(...) {
			_Logger(SW("Slumber push values FATAL unknown error!"));
		}
	});
}

pplx::task<void> getSmartScore(security::Account *account) {
	//Log the attempt of getting the smart score
	_Logger(SW("Attempting to get a calculated smart score from the server!"));
	
	web::http::client::http_client_config http_config = http_client_config();
	
	#if defined(SLUMBER_BLE_SERVER_HTTPS_VALID) && SLUMBER_BLE_SERVER_HTTPS_VALID == false
		http_config.set_validate_certificates(false);
	#else
		http_config.set_validate_certificates(true);
	#endif
	
	//Create the client to access the server
	web::http::client::http_client http_client(SLUMBER_SERVER_DOMAIN, http_config);
	
	//Create the base builder for the authentication
	web::uri_builder h_builder = uri_builder(SLUMBER_SMARTSCORE_PATH);
	h_builder.append_query("token", account->getServerToken());
	
	_Logger(SW("Built the request path: ") 
		+ SLUMBER_SMARTSCORE_PATH);
	
	//Create the new request
	http_request hreq(methods::GET);
	
	//Set the request uri
	hreq.set_request_uri(h_builder.to_string());
	
	return http_client.request(hreq)
			.then([=](http_response resp) {
		int status_code = resp.status_code(); //Get HTTP code response
	
		if(status_code == status_codes::OK) {
			_Logger(SW("Server succesfully responded! 200"));
			try {
				return resp.extract_json(); //Async json parsing
			} catch (http_exception const & err) {
				_Logger(SW("Failed getting json!"), true);
			}

			return pplx::task_from_result(json::value());
		} else {
			_Logger(SW("Server ERRORED status code: ") + SW(status_code), true);
			return pplx::task_from_result(json::value()); //Send empty json
		}
	}).then([=](pplx::task<json::value> jsonParsed) {
		try {
			json::value respjson = jsonParsed.get();

			if(respjson.is_null()) throw http_exception("Invalid response");
			
			if(!respjson.has_field(SLUMBER_TOKEN_JSON_CHECK)) {
				throw json_exception("Can't calculate the success of the SmartScore");
			}

			if(!respjson.at(SLUMBER_TOKEN_JSON_CHECK).as_bool()) {
				std::stringstream ss;
				ss << "Server message -> ";
				ss << respjson.at(SLUMBER_TOKEN_JSON_ERROR).as_string();
				throw http_exception(ss.str());
			}

			if(!respjson.has_field(SLUMBER_SMARTSCORE_TAG)) {
				throw json_exception("Can't get the smartscore value from the server!");
			}

			unsigned int smart_score = respjson.at(SLUMBER_SMARTSCORE_TAG).as_number().to_uint32();
			std::cout << "GOT NEW SMARTSCORE: " << smart_score << std::endl;
			
			//Update the ui
			slumber::setProgress(smart_score);

		} catch (http_exception const & err) {
			_Logger(SW("http_request error: ") + err.what(), true);					
			throw Error::TokenError(err.error_code().value()); //Handle the error
			
		} catch (json_exception const & err) {
			_Logger(SW("Failed parsing smartscore response from server! ERROR: ") 
									+ err.what(), true);
		} catch(...) {
			_Logger(SW("Slumber get smartscore FATAL unknown error!"));
		}
	});
}

template<typename T>
void _Logger(const T &toLog, const bool err) {
	//Log the new Token namespace sample
	Logger::Log<T>(SLUMBER_TOKEN_LOGTAG, toLog, err);
}

}

