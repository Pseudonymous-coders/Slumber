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
		accelerometer = (accelerometer * 100) / 13500;
		
		if(accelerometer > 100) accelerometer = 100;
		else if(accelerometer < 0) accelerometer = 0;	

		slumber::setTemperature(temperature);
		slumber::setHumidity(humidity);
		slumber::setMovement(accelerometer);
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
			_Logger(SW("Failed pushing band response to the server! ERROR: ") + err.what(), true); 
		}
	}
}

void onBluetoothResponse(BluetoothBand *band) {
	std::string body = stringparse::trim(band->getBody()); //Fix trimming on both sides
	security::Account *account = security::Account::getAccountByBand(band);
	
	//std::cout << "GOT ACCOUNT: " << account->getUsername() << std::endl;
	
	boost::thread parserThread(bandResponseParseThread, body, account);
}

void onBluetoothConnected(BluetoothBand *band) {
	std::cout << "CONNECTED!" << std::endl;
}

void onBluetoothDisconnected(BluetoothBand *band) {
	std::cout << "DISCONNECTED!" << std::endl;
}

template<typename T>
void _Logger(const T &toLog, const bool err) {
	Logger::Log<T>(SLUMBER_HANDLER_LOGTAG, toLog, err);
}

}
