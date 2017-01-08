#include <iostream>
#include <string>
#include <functional>

//BOOST INCLUDES
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <sbluetooth/sbluetooth.h>

#ifndef SLUMBER_BLE_H
#define SLUMBER_BLE_H

class SBluetooth {
public:

	//Use the default device no binding
	SBluetooth(int);
	
	//Use a bluetooth hci device address
	SBluetooth(int, const char *);

	//Deconstructor to disconnect the bluetooth device
	~SBluetooth();
	
	void scan(const bool over_ride_scan = false);
	void autoScan();
	void stopScan(const bool over_ride_scan = false);
	
	void connect(const std::string);
	
	template<typename T>
	void onRecieve(T);
	
	bool isConnected();
	
private:
	//Generic device setttings
	int _dev_id; //Device HCI file descriptor
	int _dev_desc; //Device BLE descriptor
	int _adapter_id; //The current adapter state
	bool _connected;

	const char *_local_adapter = NULL;
	const std::string _sec_type = "low";

	//Methods and private functions
	boost::thread *_scanLoopThread;

	void _init(int);
	
	template<typename T>
	void _Logger(const T &, const bool err=false) const;
	
	void _scanner(int);
	void _enableScan();
	void _disableScan();
	void _scanLoop();

	void _discovered(const std::string, const std::string);
	void _connect_device(const std::string);
	
	void _reloadDrivers(const std::string);
	
	char *_parseName(uint8_t *, size_t);
};

#endif //SLUMBER_BLE_H
