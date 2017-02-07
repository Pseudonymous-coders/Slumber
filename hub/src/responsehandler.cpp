#include <responsehandler.hpp>

//STANDARD INCLUDES
#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>

//SLUMBER UTILITY INCLUDES
#include <util/log.hpp>
#include <util/config.hpp>
#include <util/stringparse.hpp>
#include <security/account.hpp>

//SLUMBER INCLUDES
#include <sbluetooth/sbluetooth.hpp>
#include <security/requests.hpp>
#include <slumberui.h>

//BOOST INCLUDES
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

//CPPREST INCLUDES
#include <cpprest/json.h>

using namespace web::json;

#define SLUMBER_HANDLER_LOGTAG "RESPNS"

namespace Handler {

std::vector<std::string> checkBluetoothResponse(const std::string &body) {
	size_t start_c = std::count(body.begin(), body.end(), SLUMBER_BLE_START_CHAR);
	size_t end_c = std::count(body.begin(), body.end(), SLUMBER_BLE_END_CHAR);

	std::vector<std::string> responses;
	
	if(start_c > 0 && end_c > 0) {
		if(start_c == 1 && end_c == 1) {
			responses.push_back(body);
			return responses;
		} else {
			int start_t = start_c % 2;
			int end_t = end_c % 2;
			
			if(start_t == end_t) {
				_Logger(SW("Got multiple responses from band"));
				
				std::string newMessage = body;
				for(int ind = 0; ind < start_t; ind++) {
					std::size_t find_pos = newMessage.find(SLUMBER_BLE_END_TAG);
					if(find_pos == std::string::npos) break;
					std::string currentResponse = newMessage.substr(0, find_pos);
					responses.push_back(currentResponse);
					_Logger(SW("\t") + SW(ind) + SW(": ") + currentResponse); 
					newMessage = newMessage.substr(find_pos); //Create new string without the new duplicate response
				}
			} else {
				std::size_t find_pos = body.find(SLUMBER_BLE_END_TAG);
				if(find_pos != std::string::npos) {
					std::string currentResponse = body.substr(0, find_pos);
					responses.push_back(currentResponse);
				}
			}
		}
	} else {
		_Logger(SW("Band invalid response: ") + body);
	}
	return responses;
}

template<typename T>
T limitFix(T input, T lower_limit, T upper_limit) {
	return (input < lower_limit) ? lower_limit : (input > upper_limit) ? upper_limit : input;
}

void bandResponseParseThread(const std::string body, security::Account *account) {
	std::vector<std::string> responses = checkBluetoothResponse(body);
	
	for(std::string response : responses) {
		int accelerometer;
		int temperature;
		int humidity;
		int voltage;
		
		_Logger(SW("Parsed band response:"));
		
		int resp = sscanf(body.c_str(), 
			SLUMBER_BLE_START_TAG "%d;%d;%d;%d" SLUMBER_BLE_END_TAG,
			&accelerometer,
			&temperature,
			&humidity,
			&voltage);
		
		if(resp != 4) {
			_Logger(SW("Couldn't parse band response!"), true);
			continue;
		}
		
		float newVoltage = boost::lexical_cast<float>(voltage) / 100.0f;
		
		_Logger(SW("    Accel: ") + SW(accelerometer) + SW(" Temp: ") + 
			SW(temperature) + SW(" Humidity: ") + SW(humidity) + 
			SW(" Voltage: ") + SW(newVoltage));
		
		
		//@TODO CHANGE THIS TO ACTUALLY CALIBRATED VALUE
		accelerometer = (accelerometer < 2800) ? 0 : accelerometer;
		accelerometer = limitFix((limitFix(accelerometer, 0, 13500) * 100) / 13500, 0, 100);
		
		humidity = limitFix(humidity, 0, 100);
		temperature = limitFix(temperature, 0, 100);

		int battery_level = static_cast<int>(
				static_cast<float>(limitFix(voltage, 
						SLUMBER_BLE_MIN_VOLTAGE, 
						SLUMBER_BLE_MAX_VOLTAGE) - 
						static_cast<float>(SLUMBER_BLE_MIN_VOLTAGE)) * 
						SLUMBER_BLE_VOLTAGE_CONST);

		//Update the UI components
		slumber::setTemperature(temperature);
		slumber::setHumidity(humidity);
		slumber::setMovement(accelerometer);

		//Update the account information
		account->accelerometer = accelerometer;
		account->temperature = temperature;
		account->humidity = humidity;
		//voltage = static_cast<int>((newVoltage * 100.0f) / 4.23f);
		
		//if(voltage > 100) voltage = 100;
		//else if(voltage < 0) voltage = 0;
		
		account->voltage = voltage;
		
		try {
			Requests::updateBandData(account).wait();
		} catch(const std::exception &err) {
			if(strstr(err.what(), "wait()") != NULL) return;
			_Logger(SW("Failed pushing band response to the server! ERROR: ") + err.what(), true); 
		}

	}
}

void onBluetoothResponse(BluetoothBand *band) {
	std::string body = stringparse::trim(band->getBody()); //Fix trimming on both sides
	security::Account *account = security::Account::getAccountByBand(band);
	
	boost::thread parserThread(bandResponseParseThread, body, account);
}

void onBluetoothConnected(BluetoothBand *band) {
	security::Account *account = security::Account::getAccountByBand(band);

	_Logger(SW("Band device connected! ID: ") + account->getBandId());

	//Update UI
	slumber::setStatus(L"Band connected!");

	try {
		web::json::value to_send;
		to_send["id"] = web::json::value(account->getBandId());
		to_send["status"] = web::json::value("1"); //Set that the band is connected

		Requests::setBandDetails(account, to_send).wait();
	} catch(const std::exception &err) {
		_Logger(SW("Failed pushing band details to server! ERROR: ") + err.what(), true);
	}
}

void onBluetoothDisconnected(BluetoothBand *band) {
	security::Account *account = security::Account::getAccountByBand(band);

	_Logger(SW("Band device disconnected! ID: ") + account->getBandId());

	//Update UI
	slumber::setStatus(L"Band disconnected!");

	try {
		web::json::value to_send;
		to_send["id"] = web::json::value(account->getBandId());
		to_send["status"] = web::json::value("0"); //Set that the band is disconnected

		Requests::setBandDetails(account, to_send).wait();
	} catch(const std::exception &err) {
		_Logger(SW("Failed pushing band details to server! ERROR: ") + err.what(), true);
	}
}

void onServerStreamConnect(slumber::server::ServerStream *) {
	
}

void onServerStreamMessage(slumber::server::ServerStream *, std::string message) {
	
}

void onServerStreamDisconnect(slumber::server::ServerStream *) {

}

template<typename T>
void _Logger(const T &toLog, const bool err) {
	Logger::Log<T>(SLUMBER_HANDLER_LOGTAG, toLog, err);
}

}
