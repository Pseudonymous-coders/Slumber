#include <iostream>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>

//BOOST INCLUDES
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>

#include <sbluetooth/sbluetooth.h>

#ifndef SLUMBER_BLE_H
#define SLUMBER_BLE_H

typedef std::vector<std::pair<int, std::string>> adapters_t;

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
	void reloadDrivers(const std::string);
	static adapters_t getLocalAdapters();
	bool isConnected();
	bool isConnecting();
	
	std::string macAddrSearch;
	bool _connected;
	
private:
	//Generic device setttings
	int _dev_id; //Device HCI file descriptor
	int _dev_desc; //Device BLE descriptor
	int _adapter_id; //The current adapter state
	bool _auto_scanning;

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
	static void _connectHandle(int);
	static void _disconnectHandle(int);
	
	char *_parseName(uint8_t *, size_t);
};

class BluetoothBand : public SBluetooth {
public:
	BluetoothBand(int, const char *);
	~BluetoothBand();
	
	void attachResponse(std::function< void(BluetoothBand*) >);
	void attachConnected(std::function< void(BluetoothBand*) >);
	void attachDisconnected(std::function< void(BluetoothBand*) >);
	void setBandSearch(const std::string);

	void startLoop();
	std::string getBody();

	bool acceptedResponse;
	bool overFlowDirection;
	std::string compiledResponse, futureResponse;
	std::function< void(BluetoothBand*) > __resp_func, __conn_func, __disconn_func;
	
	int uniqueId;
	
	static void __resp_handle(int, const char*);
private:

	boost::mutex *band_lock;

	template<typename T>
	void _Logger(const T &, const bool err=false) const;
};

typedef std::function< void(BluetoothBand*) > b_handler_t;

namespace AutomaticGeneration {

	void automaticBands(int, b_handler_t, b_handler_t, b_handler_t);
	void destructAutomaticBands();
	
	extern std::vector<BluetoothBand *> bands;
}

#endif //SLUMBER_BLE_H
