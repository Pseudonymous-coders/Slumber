#include <stdio.h>
//#include <iostream>
//#include <sstream>

//BOOST INCLUDES
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

//SLUMBER UTILITY INCLUDES
#include <util/log.hpp>


//SLUMBER SECURITY INCLUDES
#include <security/tokens.hpp>

//SLUMBER BLUETOOTH INCLUDES
#include <sbluetooth/sbluetooth.hpp>

int main() {
	printf(SLUMBER_STARTUP_MESSAGE); //Print the startup message

	Logger::Init(); //Start the logging interface
	
	SBluetooth::getLocalAdapters();
	
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
