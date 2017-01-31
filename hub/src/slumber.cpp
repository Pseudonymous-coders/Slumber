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

//SLUMBER BLUETOOTH INCLUDES
#include <sbluetooth/sbluetooth.hpp>

//SLUMBER USER INTERFACE INCLUDES
#include <slumberui.h>

#define MAX_ACCOUNTS A_MAX //Maximum accounts is equal to the maximum adapters provided

//Account global declarations
std::vector<security::Account *> security::AutomaticGeneration::Accounts::accounts;
int security::AutomaticGeneration::Accounts::account_id = -1;

int main() {
	printf(SLUMBER_STARTUP_MESSAGE); //Print the startup message

	Logger::Init(); //Start the logging interface
	
	//TEMPORARY ACCOUNT INFORMATION
	security::Account tempaccount("sam@sam.com", "password", 0);
	tempaccount.setBandDevice("D0:41:31:31:40:BB"); //Set the preffered band device to look for
	tempaccount.startTokenizer();
	//Already loaded in global space, this account is now accesible by the AutomaticGenerators
	
	slumber::runUI();

	AutomaticGeneration::automaticBands(MAX_ACCOUNTS,
		Handler::onBluetoothResponse,
		Handler::onBluetoothConnected,
		Handler::onBluetoothDisconnected);
	

	//Start the ui mainloop
	//slumber::setProgress(80);
	//slumber::setHumidity(20);

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
