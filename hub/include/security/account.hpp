//STANDARD INCLUDES
#include <iostream>
#include <string>

//SLUMBER INCLUDES
#include <sbluetooth/sbluetooth.hpp>
#include <security/tokens.hpp>
#include <util/log.hpp>


#ifndef SLUMBER_ACCOUNT_H
#define SLUMBER_ACCOUNT_H

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
		this->uniqueId = uId;

		AutomaticGeneration::Accounts::account_id += 1;
		AutomaticGeneration::Accounts::accounts.push_back(this); //Create the account
	}

	~Account() {
		AutomaticGeneration::Accounts::accounts.erase(
			AutomaticGeneration::Accounts::accounts.begin() + 
			AutomaticGeneration::Accounts::account_id); //Destroy the account
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
		this->_band = band;
	}
	
	void setBandDevice(const std::string mac) {
		this->account_lock.lock();
		this->macAddrSearch = mac;
		this->account_lock.unlock();
		if(this->_band == nullptr) return;
		this->_band->setBandSearch(mac);
	}

	std::string getBandDevice() {
		this->account_lock.lock();
		std::string getMac = this->macAddrSearch;
		this->account_lock.unlock();

		return getMac;
	}
	
	void startTokenizer() {
		this->_tokenizer = 
			new Tokenizer(this->getUsername(), this->getPassword());
		this->_tokenizer->start();
	}
	
	std::string getServerToken() {
		if(this->_tokenizer == nullptr) {
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
		for(Account *account : AutomaticGeneration::Accounts::accounts) {
			if(band->uniqueId == account->uniqueId) {
				return account;
			}
		}
		
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
	
	boost::mutex account_lock;

	BluetoothBand *_band;
	Tokenizer *_tokenizer;
};

}

#endif
