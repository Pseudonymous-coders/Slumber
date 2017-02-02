#include <stdio.h>
//#include <iostream>
//#include <sstream>

//BOOST INCLUDES
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

//SLUMBER UTILITY INCLUDES
#include <util/log.hpp>
#include <responsehandler.hpp>


//SLUMBER SECURITY INCLUDES
#include <security/tokens.hpp>
#include <security/account.hpp>
#include <security/requests.hpp>

//SLUMBER BLUETOOTH INCLUDES
#include <sbluetooth/sbluetooth.hpp>

//SLUMBER USER INTERFACE INCLUDES
#include <slumberui.h>

#define MAX_ACCOUNTS 			A_MAX //Maximum accounts is equal to the maximum adapters provided
#define SLUMBER_MAIN_LOG_TAG	"SLUMBR"

//Account global declarations
std::vector<security::Account *> security::AutomaticGeneration::Accounts::accounts;
int security::AutomaticGeneration::Accounts::account_id = -1;

template<typename T>
void _Logger(const T toLog, bool err = false) {
	Logger::Log(SLUMBER_MAIN_LOG_TAG, toLog, err);
}


void smartScoreLoop() {
	_Logger(SW("Starting smartscore main pull loop"));
	while(1) {
		for(security::Account *account : security::AutomaticGeneration::Accounts::accounts) { //Load all global accounts
			_Logger(SW("Calculating smartscore for ") + account->getUsername());
			try {
				Requests::getSmartScore(account).wait(); //Update the global smartscore status
			} catch(const std::exception &err) {
				_Logger(SW("Failed getting calculated smartscore from server! ERROR: ") + err.what(), true);
				boost::this_thread::sleep_for(boost::chrono::seconds(SLUMBER_SMARTSCORE_FAIL_DELAY));
				continue; //Attempt to pull the smartscore again
			}
		}
		_Logger(SW("Getting new smartscore in ") + SW(SLUMBER_SMARTSCORE_PULL_LOOP) + SW(" seconds"));

		boost::this_thread::sleep_for(boost::chrono::seconds(SLUMBER_SMARTSCORE_PULL_LOOP)); //Wait the config second count before pulling a new smartscore
	}
}


int main() {
	printf(SLUMBER_STARTUP_MESSAGE); //Print the startup message

	Logger::Init(); //Start the logging interface
	
	//TEMPORARY ACCOUNT INFORMATION
	security::Account tempaccount("sam@sam.com", "password", 0);
	tempaccount.setBandDevice(SW("D0:41:31:31:40:BB")); //Set the preffered band device to look for
	tempaccount.setBandId(SW("fkH4dtx6"));
	tempaccount.startTokenizer();
	//Already loaded in global space, this account is now accesible by the AutomaticGenerators
	
	//Start the user interface
	slumber::runUI();

	//Start the smartscore pull loop
	//Start when new token should be retrieved
	boost::thread([]() {
		_Logger(SW("Starting smartscore pull loop in ") + SW(SLUMBER_SMARTSCORE_START_DELAY) + SW(" seconds"));
		boost::this_thread::sleep_for(boost::chrono::seconds(SLUMBER_SMARTSCORE_START_DELAY));
		boost::thread smartScoreThread(smartScoreLoop);
	});

	AutomaticGeneration::automaticBands(MAX_ACCOUNTS,
		Handler::onBluetoothResponse,
		Handler::onBluetoothConnected,
		Handler::onBluetoothDisconnected);

	while(1);

	//slumber::setProgress(80);

	return 0;
	
	/*SBluetooth band(0);
	
	band.onRecieve([](int adapter, const char *recv) {
		Logger::Log("PACKET", "BLE response: " + SW(recv));
	});
	
	band.autoScan();
	
	Tokenizer user("test@example.com", "password");
	
	try {
        //HTTPStreamingAsync().wait();
        //partTwo().wait();
        	user.start();
        	Logger::Log("const", SW("YAY"));
    } catch (const std::exception &e) {
        printf("Error exception:%s\n", e.what());
    }
    
    boost::this_thread::sleep_for(boost::chrono::seconds(70)); */
    
    Logger::Free();

	return 0;
}
