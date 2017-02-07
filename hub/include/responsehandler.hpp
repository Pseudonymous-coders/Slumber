//SLUMBER BLUETOOTH INCLUDE
#include <sbluetooth/sbluetooth.hpp>

//SLUMBER UTILITY INCLUDE
#include <util/log.hpp>
#include <util/config.hpp>

//SLUMBER SECURITY INCLUDES
#include <security/account.hpp>

//SLUMBER SERVER STREAMS INCLUDES
#include <serverstream.hpp>

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

void onServerStreamConnect(slumber::server::ServerStream *);
void onServerStreamMessage(slumber::server::ServerStream *, std::string);
void onServerStreamDisconnect(slumber::server::ServerStream *);


template<typename T>
void _Logger(const T &toLog, const bool err=false);

}
