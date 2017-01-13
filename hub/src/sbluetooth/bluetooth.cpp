//STANDARD INCLUDES
#include <iostream>
#include <functional>
#include <cstdlib>
#include <fstream>
#include <pstream.h>

//BOOST INCLUDES
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

//SLUMBER INCLUDES
#include <util/log.hpp> //Slumber logging utilities
#include <util/config.hpp> //Slumber common defintions and configurations
#include <sbluetooth/sbluetooth.hpp> //Slumber bluetooth class connections
#include <sbluetooth/sbluetooth.h> //Include C library

extern "C" {
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/queue.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
}

//BLUETOOTH DEFINTIONS
#define SLUMBER_BLE_LOGTAG				"BLERFC" //Bluetooth log tag
#define SLUMBER_BLE_SCAN_PASSIVE		0x00 //Passive discovery flag (timed)
#define SLUMBER_BLE_SCAN_ACTIVE			0x01 //Active discovery flag (constant)
#define SLUMBER_BLE_SCAN_WIN			0x12 //Discovery flag
#define SLUMBER_BLE_SCAN_INT			0x12 //Discovery flag
#define SLUMBER_BLE_NAME_SHORT			0x08 //Short local name
#define SLUMBER_BLE_NAME_COMPLETE		0x09 //Full local name
#define SLUMBER_BLE_EVENT_TYPE			0x05 //BLE response type
#define SLUMBER_BLE_SCAN_RESPONSE		0x04 //Response hex for scan completion
#define SLUMBER_BLE_SCAN_TIMEOUT		4 //Seconds for the scan to timeout
#define SLUMBER_BLE_FOUND_TAG			"Adafruit" //Name of device to find
#define SLUMBER_BLE_DRIVER_DELAY		300 //Millis to wait for driver reload
#define SLUMBER_BLE_SCAN_REFRESH		2000 //Millis to wait for scan event
#define SLUMBER_BLE_SERVICE_UUID		"6e400001-b5a3-f393-e0a9-e50e24dcca9e" //The default Adafruit UART service id
#define SLUMBER_BLE_SERVICE_STR_UUID	"6e400001" //BLE UART short service uuid
#define SLUMBER_BLE_SERVICE_TX_ID		"6e400002" //BLE UART transmit characteristic
#define SLUMBER_BLE_SERVICE_RX_ID		"6e400003" //BLE UART recieve characteristic
#define SLUMBER_BLE_BUFFER_SIZE			100 //Max recieve buffer size
#define SLUMBER_BLE_MTU_SIZE			32 //Max ble send/recv limit
#define SLUMBER_BLE_RETRY_AMOUNT		3 //Max reconnect retries

SBluetooth::SBluetooth(int adapter_id) {
	_reloadDrivers(SW("hci0"));
	_reloadDrivers(SW("hci0"));
	this->_dev_id = hci_get_route(NULL);
	this->_local_adapter = NULL;
	
	if(this->_dev_id < 0) {
		_Logger(SW("Couldn't find HCI Bluetooth module!"), true);
	} else {
		_Logger(SW("Found Bluetooth module!"));
		this->_init(adapter_id);
	}
}

SBluetooth::SBluetooth(int adapter_id, const char *device) {
	_reloadDrivers(SW(device));
	_reloadDrivers(SW(device));
	this->_dev_id = hci_devid(device);
	this->_dev_id = 1;
	this->_local_adapter = device;
	if(this->_dev_id < 0) {
		_Logger(SW("Couldn't find HCI Bluetooth module!"), true);
	} else {
		_Logger(SW("Found Bluetooth module at: ") + device);
		this->_init(adapter_id);
	}
}

SBluetooth::~SBluetooth() {
	stopScan();
	
	this->_scanLoopThread->interrupt();
	delete this->_scanLoopThread;
}

void SBluetooth::_init(int adapter_id) {
	ble_set(adapter_id, STATE_DISCONNECTED);
	this->_dev_desc = hci_open_dev(this->_dev_id);
	this->_adapter_id = adapter_id;
	this->_auto_scanning = false;
	
	if(this->_dev_desc < 0) {
		_Logger(SW("Couldn't open the Bluetooth adapter!"), true);
	} else {
		_Logger(SW("Successfully opened the Bluetooth adapter!"));
	}
	
	ble_attach_logback(_adapter_id, [](const char *logmessage){
		Logger::Log(SLUMBER_BLE_LOGTAG, SW(logmessage)); //Bind the C bluetooth program to the logger
	});
	
	ble_attach_errorback(_adapter_id, [](int error, const char *errormessage){
		std::string code = "GENERAL";
		
		switch(error) {
			case SLUMBER_BLE_ERROR:
				code = "GENERAL";
				break;
			case SLUMBER_BLE_ALREADY_CONNECTED:
			case SLUMBER_BLE_CONNECTION_ERROR:
				code = "CONNECTION";
				break;
			case SLUMBER_BLE_ADAPTER_ERROR:
				code = "ADAPTER";
				break;
			case SLUMBER_BLE_DRIVER_ERROR:
				code = "DRIVER";
				break;
			case SLUMBER_BLE_DISCOVERY_ERROR:
				code = "DISCOVERY";
				break;
			case SLUMBER_BLE_UUID_ERROR:
				code = "UUID";
				break;
			case SLUMBER_BLE_VALUE_ERROR:
				code = "VALUE";
				break;
			case SLUMBER_BLE_READ_ERROR:
				code = "READ";
				break;
			case SLUMBER_BLE_WRITE_ERROR:
				code = "WRITE";
				break;
			case SLUMBER_BLE_CHAR_ERROR:
				code = "CHARACTERISTICS";
				break;
		}
		
		Logger::Log(SLUMBER_BLE_LOGTAG, 
			code + ": " + SW(errormessage), true); //Bind the C bluetooth program to the logger and error base
	});
	
	ble_attach_disconnected(_adapter_id, SBluetooth::_disconnectHandle);
}

void SBluetooth::_disconnectHandle(int adapter_id) {
	Logger::Log(SLUMBER_BLE_LOGTAG, SW("The Device disconnected!"));
	ble_set(adapter_id, STATE_DISCONNECTED); //Set the diconnected flag
}

void SBluetooth::_reloadDrivers(const std::string device) {
	ble_reset_drivers(device.c_str());
}

void SBluetooth::onRecieve(callback_ptr_t func) {
	ble_attach_callback(this->_adapter_id, func);
}

void SBluetooth::_enableScan() {
	_Logger(SW("Enabling the BLE scanner"));
	//Get address of the BLE init register
	uint16_t interval = htobs(SLUMBER_BLE_SCAN_INT);
	uint16_t window = htobs(SLUMBER_BLE_SCAN_WIN);

	//Filter policy flags
	uint8_t own_address_type = 0x00;
	uint8_t filter_policy = 0x00;

	//Send the default filter policy flags to the BLE device
	int ret = hci_le_set_scan_parameters(this->_dev_desc, SLUMBER_BLE_SCAN_ACTIVE, interval, window, own_address_type, filter_policy, 10000);
	if (ret < 0) { //Check if the paramaters were set
		_Logger(SW("Couldn't set scan paramters!"), true);
		return;
	}

	//Enable HCI BLE scan
	ret = hci_le_set_scan_enable(this->_dev_desc, 0x01, 1, 10000);
	if (ret < 0) {
		_Logger(SW("Couldn't enable the Bluetooth scan!"), true);
	}
}

char *SBluetooth::_parseName(uint8_t *data, size_t size) {
	size_t offset = 0;

	while (offset < size) {
		uint8_t field_len = data[0];
		size_t name_len;

		if (field_len == 0 || offset + field_len > size)
			return NULL;

		switch (data[1]) {
		case SLUMBER_BLE_NAME_SHORT:
		case SLUMBER_BLE_NAME_COMPLETE:
			name_len = field_len - 1;
			if (name_len > size)
				return NULL;

			return strndup((const char*)(data + 2), name_len);
		}

		offset += field_len + 1;
		data += field_len + 1;
	}

	return NULL;
}

void SBluetooth::_disableScan() {
	_Logger(SW("Disabling the BLE scanner"));
	if (this->_dev_desc == -1) {
		_Logger(SW("The Bluetooth device isn't initialized"), true);
		return;
	}

	//Disable HCI BLE scan
	int result = hci_le_set_scan_enable(this->_dev_desc, 0x00, 1, 10000);
	if (result < 0) {
		_Logger(SW("Couldn't disable the Bluetooth scan"), true);
	}
}

void SBluetooth::_scanner(int timeout) {	
	//HCI filter options
	struct hci_filter old_options;
	socklen_t slen = sizeof(old_options); //Socket connection
	struct hci_filter new_options; //HCI filter options
	
	int len;
	
	//Characteristics response buffer and meta info
	unsigned char buffer[HCI_MAX_EVENT_SIZE];
	evt_le_meta_event* meta = (evt_le_meta_event*)(buffer + HCI_EVENT_HDR_SIZE + 1);
	le_advertising_info* info;
	
	//Struct for the timeouts
	struct timeval wait;
	fd_set read_set;
	
	char addr[18];

	//Load the already made socket options
	if (getsockopt(this->_dev_desc, SOL_HCI, HCI_FILTER, 
			&old_options, &slen) < 0) {
		_Logger(SW("Couldn't get bluetooth options! (ROOT REQUIRED)"), true);
		return;
	}

	//Clean the preset options
	hci_filter_clear(&new_options);
	hci_filter_set_ptype(HCI_EVENT_PKT, &new_options);
	hci_filter_set_event(EVT_LE_META_EVENT, &new_options);

	//Set the new option values
	if (setsockopt(this->_dev_desc, SOL_HCI, HCI_FILTER,
			&new_options, sizeof(new_options)) < 0) {
		_Logger(SW("Couldn't set bluetooth options (ROOT REQUIRED)"), true);
		return;
	}
	
	if(timeout > 0) {
		//Timeouts
		wait.tv_sec = timeout;
	}
	
	int ts = time(NULL);

	//Loop connection tests
	while(1) {
		FD_ZERO(&read_set);
		FD_SET(this->_dev_desc, &read_set);

		int err = select(FD_SETSIZE, &read_set, NULL, NULL, &wait);
		if (err <= 0)
			break;

		len = read(this->_dev_desc, buffer, sizeof(buffer));

		if (meta->subevent != 0x02 || (uint8_t)buffer[SLUMBER_BLE_EVENT_TYPE] != SLUMBER_BLE_SCAN_RESPONSE)
			continue;

		//Get socket characteristics video
		info = (le_advertising_info*) (meta->data + 1);
		ba2str(&info->bdaddr, addr);

		//Parse the name into a readable format
		char* name_raw = this->_parseName(info->data, info->length);
		
		//String wrap the found name
		std::string name = (name_raw == NULL) ? 
			std::string("") : SW(name_raw);
		
		//Run discovered function
		if(this->_discovered(SW(addr), name)) {
			boost::this_thread::sleep_for(
				boost::chrono::milliseconds(SLUMBER_BLE_SCAN_REFRESH + 500));
			break;
		}

		//Remove all dynamic allocations
		if (name_raw) free(name_raw);

		//If the current time passed the timeout quit then
		if(timeout > 0) {
			int elapsed = time(NULL) - ts;
			if (elapsed >= timeout)
				break;
			
			//Update the timeout elapsed method
			wait.tv_sec = timeout - elapsed;
		}
	}

	//Set the new socket options
	setsockopt(this->_dev_desc, SOL_HCI, HCI_FILTER, 
		&old_options, sizeof(old_options));
}

void SBluetooth::_connect_device(const std::string addr_s) {
	const char *addr = addr_s.c_str();

	this->stopScan(); //Disable the scanning process

	_Logger(SW("Connecting to device: ") + addr_s);

	ble_start(_adapter_id, _local_adapter, addr, _sec_type.c_str());
}

void SBluetooth::connect(const std::string addr) {
	if(addr.length() == 0) {
		_Logger(SW("Address cannot be NULL or 0"));
		return;
	}
	
	_connect_device(addr);
}

bool SBluetooth::_discovered(const std::string addr, const std::string name) {
	_Logger(SW("Bluetooth device discovered, NAME: ") 
		+ name + SW(" ADDR: ") + addr);

	if(name.find(SW(SLUMBER_BLE_FOUND_TAG)) != std::string::npos) {
		_Logger(SW("Testing a possible band device!"));
		
		this->_connect_device(addr);
		return true;
	}
	return false;
}

void SBluetooth::scan(const bool over_ride_scan) {
	if(this->isConnecting()) {
		_Logger(SW("Not scanning, already connecting to device"));
		return;
	}
	
	if(this->isConnected() && !over_ride_scan && !this->_auto_scanning) {
		_Logger(SW("Already connected!"));
		return; //No need to scan if already connected to device
	}
	_enableScan();
	
	int timeout = (this->_auto_scanning) ? 0 : SLUMBER_BLE_SCAN_TIMEOUT;
	_scanner(timeout);
}

void SBluetooth::stopScan(const bool over_ride_scan) {
	_Logger(SW("Stopping the BLE scanner"));
	_disableScan();
}

bool SBluetooth::isConnected() {
	return ((char) ble_connected(_adapter_id) == (char) 1);
}

bool SBluetooth::isConnecting() {
	return ((char) ble_connecting(_adapter_id) == (char) 1);
}

void SBluetooth::_scanLoop() {
	_Logger(SW("Started autoscan threaded loop"));	

	while(1) {
		this->scan(false); //Don't over_ride the default scan
		//Wait for the scan delay to check the scanner connection
		boost::this_thread::sleep_for(
			boost::chrono::milliseconds(SLUMBER_BLE_SCAN_REFRESH));
	}
}

void SBluetooth::autoScan() {
	this->_auto_scanning = true;
	//this->_scanLoopThread = new boost::thread(
	//	boost::bind(&SBluetooth::_scanLoop, this));
	this->_scanLoop();
}

adapters_t SBluetooth:getLocalAdapters() {
	adapters_t adapters;
	redi::ipstream in("ls -l");
	std::stringstream ss;
	ss << in.rdbuf();
	std::string s = ss.str();
	return adapters;	
}


template<typename T>
void SBluetooth::_Logger(const T &toLog, const bool err) const {
	//Log the new Token namespace sample
	Logger::Log<T>(SLUMBER_BLE_LOGTAG, toLog, err);
}

BluetoothBand::BluetoothBand(int adapter) {
	this->_ble_interface = new SBluetooth(adapter);
}

BluetoothBand::~BluetoothBand() {
	delete this->_ble_interface;
}



/*int main() {
	Logger::Init();

	SBluetooth bluetooth(0);
	bluetooth.onRecieve([](int adapter, const char *recv) {
		Logger::Log("PACKET", "BLE response: " + SW(recv));
	});
	bluetooth.autoScan();
	
	boost::this_thread::sleep_for(boost::chrono::seconds(300));
}*/

