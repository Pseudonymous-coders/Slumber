//SLUMBER BLUETOOTH INCLUDE
#include <sbluetooth/sbluetooth.hpp>

//SLUMBER UTILITY INCLUDE
#include <util/log.hpp>
#include <util/config.hpp>

//SLUMBER SECURITY INCLUDES
#include <security/account.hpp>

struct bandUpdate {
	int accelerometer;
	int temperature;
	int humidity;
	float voltage;
};

typedef struct bandUpdate bandUpdate_t;


namespace Handler {
	
std::vector<std::string> checkBluetoothResponse(const std::string &);
void bandResponseParseThread(const std::string, security::Account *); 

void onBluetoothConnected(BluetoothBand *);
void onBluetoothDisconnected(BluetoothBand *);
void onBluetoothResponse(BluetoothBand *);



template<typename T>
void _Logger(const T &toLog, const bool err=false);

}
