//STANDARD INCLUDES
#include <iostream>
#include <string>

//BOOST INCLUDES
#include <boost/lexical_cast.hpp>

//SLUMBER INCLUDES
#include <sbluetooth/sbluetooth.hpp>
#include <security/tokens.hpp>
#include <util/log.hpp>
#include <util/config.hpp>


#ifndef SLUMBER_ACCOUNT_H
#define SLUMBER_ACCOUNT_H

#define SLUMBER_ACCOUNT_LOGTAG "ACCNTS"

namespace security {

class Account;

namespace AutomaticGeneration {

class Accounts {
public:
	static std::vector<Account *> accounts;
	static int account_id;
};

}

class Account {
public:
	Account(const std::string &user = std::string(), 
			const std::string &pass = std::string(),
			int uId = 0) {
		this->_username = user;
		this->_password = pass;
		this->_tokenizer_init = false;
		this->_band_init = false;
		this->uniqueId = uId;

		bool locked = MUTEX_GLOBAL_TLOCK; //Lock onto the main thread before continuing
		AutomaticGeneration::Accounts::account_id += 1;
		AutomaticGeneration::Accounts::accounts.push_back(this); //Create the account
		if(locked) {
			MUTEX_GLOBAL_UNLOCK;
		}
	}

	~Account() {
		bool locked = MUTEX_GLOBAL_TLOCK; //Lock onto the main thread before continuing
		AutomaticGeneration::Accounts::accounts.erase(
			AutomaticGeneration::Accounts::accounts.begin() + 
			AutomaticGeneration::Accounts::account_id); //Destroy the account
		if(locked) {
			MUTEX_GLOBAL_UNLOCK;
		}
	}

	/*
	 *
	 * GETTERS
	 *
	 */

	std::string getUsername() {
		return this->_username;
	}
	
	std::string getPassword() {
		return this->_password;
	}
	
	BluetoothBand *getBand() {
		return this->_band;
	}

	/*
	 *
	 * SETTERS
	 *
	 */

	void setUsername(const std::string user) {
		this->_username = user;
	}
	
	void setPassword(const std::string pass) {
		this->_password = pass;
	}
	
	void setBand(BluetoothBand *band) {
		this->_band_init = true;
		this->_band = band;
	}
	
	void setBandDevice(const std::string mac) {
		this->macAddrSearch = mac;
		if(!this->_band_init) return;
		try {
			this->_band->setBandSearch(mac);
		} catch(...) {}
	}

	std::string getBandDevice() {
		return this->macAddrSearch;
	}
	
	void startTokenizer() {
		try {
			this->_tokenizer = 
				new Tokenizer(this->getUsername(), this->getPassword());
		
			this->_tokenizer_init = true;

			this->_tokenizer->start();
		} catch(...) {
			Logger::Log(SLUMBER_ACCOUNT_LOGTAG, SW("Failed to start tokenizer on account ") + this->getUsername(), true);
		}
	}

	bool isServerTokenValid() {
		if(!this->_tokenizer_init) {
			return false;
		}
		
		return this->_tokenizer->isValid();
	}
	
	std::string getServerToken() {
		if(!this->_tokenizer_init) {
			return std::string();
		}

		return this->_tokenizer->getToken();
	}

	void setBandId(const std::string &newId) {
		this->bandId = newId;
	}

	std::string getBandId() {
		return this->bandId;
	}
	
	static Account *getAccountByBand(BluetoothBand *band) {
		bool locked = MUTEX_GLOBAL_TLOCK; //Lock onto main thread before attempting to pull the new account
		for(Account *account : AutomaticGeneration::Accounts::accounts) {
			if(band->uniqueId == account->uniqueId) {
				if(locked) MUTEX_GLOBAL_UNLOCK;
				return account;
			}
		}
		if(locked) MUTEX_GLOBAL_UNLOCK;
		return nullptr;
	}
	
	int uniqueId;
	std::string macAddrSearch, bandId;
	
	int accelerometer;
	int temperature;
	int humidity;
	int voltage;
	
private:
	std::string _username;
	std::string _password;

	bool _tokenizer_init;
	bool _band_init;

	//boost::mutex account_lock;

	BluetoothBand *_band;
	Tokenizer *_tokenizer;
};

}

#endif
