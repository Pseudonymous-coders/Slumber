#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <map>

//BOOST INCLUDES
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <sbluetooth/sbluetooth.h>

#ifndef SLUMBER_BLE_H
#define SLUMBER_BLE_H

typedef std::vector<std::pair<std::string, std::string>> adapters_t;


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
	void onRecieve(callback_ptr_t);
	
	static adapters_t getLocalAdapters();
	
	bool isConnected();
	bool isConnecting();
	
private:
	//Generic device setttings
	int _dev_id; //Device HCI file descriptor
	int _dev_desc; //Device BLE descriptor
	int _adapter_id; //The current adapter state
	bool _connected, _auto_scanning;

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

	bool _discovered(const std::string, const std::string);
	void _connect_device(const std::string);
	static void _disconnectHandle(int);
	
	void _reloadDrivers(const std::string);
	
	char *_parseName(uint8_t *, size_t);
};

class BluetoothBand {
public:
	BluetoothBand();
	~BluetoothBand();
};

#endif //SLUMBER_BLE_H
