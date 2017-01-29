#include <sbluetooth/sbluetooth.h>
#include <sbluetooth/bluetooth.h>
#include <sbluetooth/gatt.h>
#include <sbluetooth/gattrib.h>
#include <sbluetooth/hci.h>
#include <sbluetooth/hci_lib.h>
#include <sbluetooth/btio.h>
#include <sbluetooth/uuid.h>
#include <sbluetooth/rfcomm.h>

#include <pthread.h> //Timeout threading
#include <time.h> //Time hanlder

#define SLUMBER_BLE_SERVICE_STR_UUID	"6e400001" //BLE UART short service uuid
#define SLUMBER_BLE_SERVICE_TX_ID		"6e400002" //BLE UART transmit characteristic
#define SLUMBER_BLE_SERVICE_RX_ID		"6e400003" //BLE UART recieve characteristic
#define SLUMBER_BLE_SERVICE_RX_FLAG		"0100" //Flag to get requests from RX characteristic
#define SLUMBER_BLE_SERVICE_FLAGS		"2902" //Flag to get all service handlers
#define SLUMBER_BLE_THREAD_TIMEOUT		4 //Seconds to timeout on a function call

#define ERRORBACK(X, Y, Z) if(X->_errorback_func != NULL) X->_errorback_func(Y, Z)
#define LOGBACK(X, Y) if(X->_logback_func != NULL) X->_logback_func(Y)
#define PROPS _t_props

#define MUTEXLOCK(X) pthread_mutex_lock(&X->mutex) //printf("locking LINE: %d\n", __LINE__);
#define MUTEXUNLOCK(X) pthread_mutex_unlock(&X->mutex) //printf("unlocking LINE: %d\n", __LINE__);

//DEFAULT STRUCT SETTINGS
int _start_t = 0x0001;
int _end_t = 0xffff;
int _handle_t = -1;
int _mtu = 0;
int _psm = 0;
const char *_address_type = "random"; //Connect to the device with a random address
char *_src = NULL; //The local adapter to use
char *_dst = NULL; //The BLE device to connect to (MAC address) 
char *_sec_level = NULL; //The security level to use with the device
bt_uuid_t *_uuid_t = NULL; //The uuid to look for
GIOChannel *chan = NULL; //Channels to attach BLE devices to
GAttrib *attrib = NULL; //Attributes for the devices
GError *gerr = NULL; //Error reporting
char char_passed = 0; //Characteristic passing flag
char rx_handler[50]; //The RX characteristic handler flag

pthread_mutex_t mutex_locks[A_MAX] = {PTHREAD_MUTEX_INITIALIZER};

struct params {
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t done;
	
	enum state conn_state;
	
	int _a_num;
	int _start_t;
	int _end_t;
	int _handle_t;
	int _mtu;
	int _psm;
	const char *_address_type; //Connect to the device with a random address
	
	//Device and adapter definitions
	char *_src; //The local adapter to use
	char *_dst; //The BLE device to connect to (MAC address) 
	char *_sec_level; //The security level to use with the device
	bt_uuid_t *_uuid_t; //The uuid to look for
	bt_uuid_t _uuid_temp; //Temporary uuid holder
	GMainLoop *event_loop; //Mainloops per BLE device
	GIOChannel *chan; //Channels to attach BLE devices to
	GAttrib *attrib; //Attributes for the devices
	GError *gerr; //Error reporting

	int start; //Temporary adapter beginnning address
	int end; //Same as above except the final address
	char char_passed; //Characteristic passing flag
	char rx_handler[50]; //The RX characteristic handler flag

	callback_ptr_t _callback_func; //Callback functions to call when a notification happens
	errorback_ptr_t _errorback_func; //Callback functions when a error occurs
	logback_ptr_t _logback_func; //Callback functions to log data on main logger
	disconnected_ptr_t _disconnected_func; //Callback function when the BLE device disconnects
	connected_ptr_t _connected_func; //Callback function when the BLE device connects
};

//Creeate the same structured list to easily access the managers (The index
//Doesn't matter)
params_t _t_props[A_MAX + 1]; //Add one for safety

/*pthread_mutex_t start_func = PTHREAD_MUTEX_INITIALIZER; //On the start of a function
pthread_cond_t end_func = PTHREAD_COND_INITIALIZER; //On the end of a function

//The struct to send to the thread
struct thread_args {
	GAttrib *att;

	int *a_num, *ret_data, start_t, end_t;
	
	void (*char_discovery)(guint8, const guint8 *, guint16, gpointer);
};

//Define the struct
typedef struct thread_args thread_args_t;
*/


//Revised gatt_connection
GIOChannel *ble_gatt_connect(const char *src, const char *dst,
				const char *dst_type, const char *sec_level,
				int psm, int mtu, BtIOConnect connect_cb,
				gpointer user_data) { //Added custom variable for user data
	GIOChannel *chan;
	bdaddr_t sba, dba;
	uint8_t dest_type;
	GError *tmp_err = NULL;
	BtIOSecLevel sec;
	GError **gerr = NULL; //Null pointer

	str2ba(dst, &dba);

	/* Local adapter */
	if (src != NULL) {
		if (!strncmp(src, "hci", 3))
			hci_devba(atoi(src + 3), &sba);
		else
			str2ba(src, &sba);
	} else
		bacpy(&sba, BDADDR_ANY);

	/* Not used for BR/EDR */
	if (strcmp(dst_type, "random") == 0)
		dest_type = BDADDR_LE_RANDOM;
	else
		dest_type = BDADDR_LE_PUBLIC;

	if (strcmp(sec_level, "medium") == 0)
		sec = BT_IO_SEC_MEDIUM;
	else if (strcmp(sec_level, "high") == 0)
		sec = BT_IO_SEC_HIGH;
	else
		sec = BT_IO_SEC_LOW;

	//I don't know why the BLUEZ team didn't add a custom user data variable
	//But I found this function and decided to modify it
	if (psm == 0)
		chan = bt_io_connect(connect_cb, user_data, NULL, &tmp_err,
				BT_IO_OPT_SOURCE_BDADDR, &sba,
				BT_IO_OPT_SOURCE_TYPE, BDADDR_LE_PUBLIC,
				BT_IO_OPT_DEST_BDADDR, &dba,
				BT_IO_OPT_DEST_TYPE, dest_type,
				BT_IO_OPT_CID, ATT_CID,
				BT_IO_OPT_SEC_LEVEL, sec,
				BT_IO_OPT_INVALID);
	else
		chan = bt_io_connect(connect_cb, user_data, NULL, &tmp_err,
				BT_IO_OPT_SOURCE_BDADDR, &sba,
				BT_IO_OPT_DEST_BDADDR, &dba,
				BT_IO_OPT_PSM, psm,
				BT_IO_OPT_IMTU, mtu,
				BT_IO_OPT_SEC_LEVEL, sec,
				BT_IO_OPT_INVALID);

	if (tmp_err) {
		g_propagate_error(gerr, tmp_err);
		return NULL;
	}

	return chan;
}
//END OF CUSTOM FUNCTION MODIFICATIONS



//Function to turn a string into an attribute
size_t ble_string_to_attr(const char *str, uint8_t **data)
{
	char tmp[3];
	size_t size, i;

	size = strlen(str) / 2;
	*data = g_try_malloc0(size);
	if (*data == NULL)
		return 0;

	tmp[2] = '\0';
	for (i = 0; i < size; i++) {
		memcpy(tmp, str + (i * 2), 2);
		(*data)[i] = (uint8_t) strtol(tmp, NULL, 16);
	}

	return size;
}

//Function to turn a character into a hex format for handling
char ble_string_to_handle(const char *src)
{
	char *e;
	int dst;

	errno = 0;
	dst = strtoll(src, &e, 16);
	if (errno != 0 || *e != '\0')
		return -EINVAL;

	return dst;
}

char ble_connected(int a_num) {
	if(a_num > A_MAX) return 0;
	params_t *_param = &PROPS[a_num];
	MUTEXLOCK(_param);
	char ret = (char) (_param->conn_state == STATE_CONNECTED) ? 1 : 0;
	MUTEXUNLOCK(_param);
	return ret;
}

char ble_connecting(int a_num) {
	if(a_num > A_MAX) return 0;
	params_t *_param = &PROPS[a_num];
	MUTEXLOCK(_param);
	char ret = (char) (_param->conn_state == STATE_CONNECTING) ? 1 : 0;
	MUTEXUNLOCK(_param);
	return ret;
}

char ble_disconnected(int a_num) {
	if(a_num > A_MAX) return 0;
	params_t *_param = &PROPS[a_num];
	MUTEXLOCK(_param);
	char ret = (char) (_param->conn_state == STATE_DISCONNECTED) ? 1 : 0;
	MUTEXUNLOCK(_param);
	return ret;
}

void ble_set(int a_num, enum state st) {
	if(a_num > A_MAX) return;
	params_t *_param = &PROPS[a_num];
	MUTEXLOCK(_param);
	_param->conn_state = st;
	MUTEXUNLOCK(_param);
}

//Reset the device drivers
char ble_reset_drivers(const char *adapter) {
	const char *execute_buff = "service bluetooth restart && hciconfig ";
	char adapter_buff[150];
	int ret;
	
	params_t *_param = &PROPS[0];
	
	MUTEXLOCK(_param);
	
	if(adapter == NULL) {
		sprintf(adapter_buff, "%s%s reset", execute_buff, "hci0");
	} else {
		sprintf(adapter_buff, "%s%s reset", execute_buff, adapter);
	}

	LOGBACK(_param, "Unblocking all rfkill options");
	ret = system("rfkill unblock all");
	if(ret != 0) {
		ERRORBACK(_param, SLUMBER_BLE_DRIVER_ERROR, "Failed to unblock bluetooth from rfkill");
	} else {
		LOGBACK(_param, "Unblocked everything from rfkill");
	}

	LOGBACK(_param, "Resetting the bluetooth drivers");
	ret = system(adapter_buff);
	sleep(1); //Wait one second to finish
	
	if(ret != 0) {
		ERRORBACK(_param, SLUMBER_BLE_DRIVER_ERROR, "Failed to reset bluetooth drivers!");
		MUTEXUNLOCK(_param);
		return SLUMBER_BLE_DRIVER_ERROR;
	} else {
		LOGBACK(_param, "Bluetooth driver reset. Complete.");
	}
	
	MUTEXUNLOCK(_param);
	return SLUMBER_BLE_OK;
}

char ble_attach_callback(int a_num, callback_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	PROPS[a_num]._callback_func = func;
	
	return SLUMBER_BLE_OK;
}

char ble_attach_errorback(int a_num, errorback_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	PROPS[a_num]._errorback_func = func;
	
	return SLUMBER_BLE_OK;
}

char ble_attach_logback(int a_num, logback_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	PROPS[a_num]._logback_func = func;
	
	return SLUMBER_BLE_OK;
}

char ble_attach_disconnected(int a_num, disconnected_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	PROPS[a_num]._disconnected_func = func;
	
	return SLUMBER_BLE_OK;
}

char ble_attach_connected(int a_num, connected_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	PROPS[a_num]._connected_func = func;
	
	return SLUMBER_BLE_OK;
}

//Handle glib events such as when the library does get a response
void __ble_events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data) {
	
	//for(int ind = 0; ind < A_MAX; ind++) {
	//	if(chan
	//}
	
	//int a_num = *((int*)user_data); //The adapter number
	
	params_t *_param = (params_t*) user_data;
	
	int a_num = _param->_a_num;
	
	uint8_t *opdu;
	uint16_t handle, i, olen;
	size_t plen;
	GString *s;

	//Get data from event handler
	handle = att_get_u16(&pdu[1]);

	//Check if the notification return matches our data
	switch (pdu[0]) {
	case ATT_OP_HANDLE_NOTIFY: //If glib returns a notification
		s = g_string_new(NULL);
		//g_string_printf(s, "Notification handle = 0x%04x value: ",
		//							handle);
		break;
	/*case ATT_OP_HANDLE_IND: //If glib returns an indication
		s = g_string_new(NULL);
		g_string_printf(s, "Indication   handle = 0x%04x value: ",
									handle);
		break;*/
	default:
		ERRORBACK(_param, SLUMBER_BLE_VALUE_ERROR, "Invalid opcode");
		return;
	}

	//Complete the returned string
	for (i = 3; i < len; i++) {
		g_string_append_printf(s, "%c", (char) pdu[i]);
	}
	
	if(_param->_callback_func == NULL) {
		LOGBACK(_param, "No callback function provided response: ");
		LOGBACK(_param, s->str);
	} else {
		_param->_callback_func(a_num, s->str); //Call the callback function
	}
	
	g_string_free(s, TRUE); //Free the gstring

	//Only respond on attrib if it's an indication
	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
		return;

	opdu = g_attrib_get_buffer(_param->attrib, &plen);
	olen = enc_confirmation(opdu, plen);

	if (olen > 0) {
		g_attrib_send(_param->attrib, 0, opdu, olen, NULL, NULL, NULL);
	}
}

//Disconnect the specified channel adapter
void __ble_disconnect_channel(params_t *_param) {
	if (_param->conn_state == STATE_DISCONNECTED)
		return;

	int a_num = _param->_a_num;

	g_attrib_unref(_param->attrib);
	_param->attrib = NULL;

	g_io_channel_shutdown(_param->chan, FALSE, NULL);
	g_io_channel_unref(_param->chan);
	_param->chan = NULL;

	//ble_set(a_num, STATE_DISCONNECTED);

	g_main_loop_quit(_param->event_loop); //Destroy the loop that hangs the program
	g_main_loop_unref(_param->event_loop); //Cleanup the mainloop

	if(_param->_disconnected_func != NULL) {
		_param->_disconnected_func(a_num); //Call the callback function
	} else {
		LOGBACK(_param, "The BLE disconnected!");
	}
}




/*
 *
 *
 *  TRANSFER SETTINGS
 *
 *
 */


//STEP ONE DISCOVER THE PRIMARY SERVICES
void __ble_discover_primary_callback(GSList *services, 
		guint8 status, gpointer user_data) {

	params_t *_param = (params_t *) user_data; 

	GSList *l;

	if (status) {
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, "Discover all primary services failed!:");
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, att_ecode2str(status));
		return;
	}

	if (services == NULL) {
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, "No primary service found!");
		return;
	}
	
	char passed = 0;

	for (l = services; l; l = l->next) {
		struct gatt_primary *prim = l->data;
		if(strstr(prim->uuid, SLUMBER_BLE_SERVICE_STR_UUID) != NULL) {
			LOGBACK(_param, "Found a slumber band: ");
			
			char buff_line[100];
			
			sprintf(buff_line, "Handler: 0x%04x <-> End Handler: 0x%04x UUID: %s",
				prim->range.start, prim->range.end, prim->uuid);
		
			LOGBACK(_param, buff_line);
			
			passed = 1; //Set the passed flag to true
		}
	}
	
	if(!passed) {
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, "The selected bluetooth device is not a Slumber band!");
	} else {
		ble_discover_characteristics(_param); //Discover and check the characteristics
	}
}

void __ble_discover_char_callback(guint8 status, const guint8 *pdu, 
		guint16 plen, gpointer user_data) {
	
	params_t *_param = (params_t *) user_data;
	
	struct att_data_list *list;
	guint8 format;
	uint16_t handle = 0xffff;
	int i;
	char skipper = 1;

	if (status != 0) {
		LOGBACK(_param, "Finished with finding characteristics:");
		LOGBACK(_param, att_ecode2str(status));

		skipper = 0;
	}

	if(skipper) {
		list = dec_find_info_resp(pdu, plen, &format);
		
		if (list == NULL) {
			ERRORBACK(_param, SLUMBER_BLE_CHAR_ERROR, "No characteristics found on the device!");
			return;
		}
		
		char buff_line[200];
	
		//Loop through all of the characteristics
		for (i = 0; i < list->num; i++) {
			char uuidstr[MAX_LEN_UUID_STR];
			uint8_t *value;
			bt_uuid_t uuid;

			value = list->data[i];
			handle = att_get_u16(value);
	
			if (format == 0x01)
				uuid = att_get_uuid16(&value[2]);
			else
				uuid = att_get_uuid128(&value[2]);

			bt_uuid_to_string(&uuid, uuidstr, MAX_LEN_UUID_STR);
			
			if(strstr(uuidstr, SLUMBER_BLE_SERVICE_TX_ID) != NULL) {
				snprintf(buff_line, 200, "Found TX Handle: 0x%04x, UUID: %s", handle, uuidstr);
				LOGBACK(_param, buff_line);
				_param->char_passed++;
			} else if(strstr(uuidstr, SLUMBER_BLE_SERVICE_RX_ID) != NULL) {
				snprintf(buff_line, 200, "Found RX Handle: 0x%04x, UUID: %s", handle, uuidstr);
				LOGBACK(_param, buff_line);
				_param->char_passed++;
			}
			
			usleep(3); //Wait 3 nanoseconds
		}
	
		att_data_list_free(list); //Clear the characteristics service list
	}
	
	MUTEXLOCK(_param);
	
	if (handle != 0xffff && handle < _param->_end_t) {
		gatt_discover_char_desc(_param->attrib, handle + 1, 
			_param->_end_t, __ble_discover_char_callback, _param);
	} else {
		//The characteristic loop pull has finished check if the characteristics are correct
		if(_param->char_passed < 2) {
			ERRORBACK(_param, SLUMBER_BLE_CHAR_ERROR, "Failed to get the right characteristics from the band");
		} else {
			//Both TX and RX were found continue to set the RX flag
			LOGBACK(_param, "Found the band characteristics for the UART service");

			ble_read_characteristic(_param, SLUMBER_BLE_SERVICE_FLAGS);
		}
	}
	
	MUTEXUNLOCK(_param);
}


//Write a characteristic value
void __ble_write_char_callback(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data) {
	params_t *_param = (params_t *) user_data;
				
	if (status != 0) {
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, "Characteristics write failed:");
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, att_ecode2str(status));
		return;
	}

	if (!dec_write_resp(pdu, plen) && !dec_exec_write_resp(pdu, plen)) {
		ERRORBACK(_param, SLUMBER_BLE_WRITE_ERROR, "Protocol error");
		return;
	}

	LOGBACK(_param, "The new characteristic was written successfully");
	LOGBACK(_param, "Waiting for a response from the Slumber band");
	
	MUTEXLOCK(_param);
	_param->conn_state = STATE_CONNECTED;
	MUTEXUNLOCK(_param);
	
	if(_param->_connected_func != NULL) {
		_param->_connected_func(_param->_a_num); //Call the callback function
	} else {
		LOGBACK(_param, "The BLE device connected!");
	}
}

//Write a characteristic value
void __ble_read_char_uuid_callback(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data) {
	params_t *_param = (params_t *) user_data;
	
	struct att_data_list *list;
	int i;
	GString *s;

	if (status != 0) {
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, "Failed to read characteristic from the band:");
		ERRORBACK(_param, SLUMBER_BLE_DISCOVERY_ERROR, att_ecode2str(status));
		return;
	}

	list = dec_read_by_type_resp(pdu, plen);
	if (list == NULL)
		return;
		
	uint16_t last_handler = 0;
	s = g_string_new(NULL);
	for (i = 0; i < list->num; i++) {
		uint8_t *value = list->data[i];
		int j;

		last_handler = att_get_u16(value); //Get the handler location

		g_string_printf(s, "Characteristic read Handler: 0x%04x value: ", 
			last_handler);
		value += 2;
		for (j = 0; j < list->len - 2; j++, value++)
			g_string_append_printf(s, "%c ", ((char) *value));
		LOGBACK(_param, s->str);
	}
	
	sprintf(_param->rx_handler, "0x%04x", last_handler); //Update rx_handler flag for requests
	
	char buff_line[100];
	sprintf(buff_line, "Selected %s ad the RX handler", _param->rx_handler);

	att_data_list_free(list);
	g_string_free(s, TRUE);
	
	//Update RX characteristic with selected handler
	ble_write_characteristic(_param, _param->rx_handler, SLUMBER_BLE_SERVICE_RX_FLAG);
}








/*
 *
 *
 *  CALLBACK INITIALIZERS
 *
 *
 */
 
 
char ble_discover_services(params_t *_param) {
	if(_param->conn_state == STATE_DISCONNECTED) {
		ERRORBACK(_param, SLUMBER_BLE_CONNECTION_ERROR, "Not connected to a device!");
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	LOGBACK(_param, "Attempting to find services on the BLE device");

	if(gatt_discover_primary(_param->attrib, NULL, __ble_discover_primary_callback, _param) != 0)
		return SLUMBER_BLE_DISCOVERY_ERROR;
	return SLUMBER_BLE_OK;
}


/*void *__timeout_char_discover(void *data) {
	int killType;

	printf("FIVE\n");
	
	thread_args_t *t_args = (struct thread_args *) data;
	
	//Allow the thread to be killed anytime
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &killType);

	printf("SIX: %d\n", *t_args->a_num);

	//Start the BLE char discovery
	if(gatt_discover_char_desc(t_args->att, t_args->start_t, t_args->end_t, t_args->char_discovery, t_args->a_num) != 0) {
		printf("RETURN\n");
		//(*(((struct thread_args *) data)->ret_data)) = SLUMBER_BLE_CHAR_ERROR;
	}
	
	printf("SEVEN\n");
	
	//Notify the conditional state that the function finished
	pthread_cond_signal(&end_func);
	return NULL;
}*/


char ble_discover_characteristics(params_t *_param) {
	if(_param->conn_state == STATE_DISCONNECTED) {
		ERRORBACK(_param, SLUMBER_BLE_CONNECTION_ERROR, "Not connected to a device!");
		return SLUMBER_BLE_CONNECTION_ERROR;
	}

	LOGBACK(_param, "Attempting to find characteristics on the BLE device");
	
	_param->start = 0x0001; //Set start flag back to 0
	_param->end = 0xffff; //Set the end flag to end
	
	_param->char_passed = 0; //Set the flag checker back to zero
	
	//Start the BLE char discovery
	if(gatt_discover_char_desc(_param->attrib, _param->_start_t, _param->_end_t, __ble_discover_char_callback, _param) != 0) {
		return SLUMBER_BLE_CHAR_ERROR;
	}
	
	
	/*struct timespec abs_time, max_wait;
	pthread_t thread_id;
	int err;
	
	memset(&max_wait, 0, sizeof(max_wait));
	max_wait.tv_sec = SLUMBER_BLE_THREAD_TIMEOUT;
	
	pthread_mutex_lock(&start_func);
	
	//Absolute timeout so we don't hang if the device barely booted up
	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += max_wait.tv_sec;
	abs_time.tv_nsec += max_wait.tv_nsec;
	
	printf("ONE\n");
	
	int *ret_data = malloc(sizeof(*ret_data));
	
	(*ret_data) = SLUMBER_BLE_OK;
	
	printf("ONEHALF\n");
	
	//Create a new send struct
	thread_args_t *init_args = malloc(sizeof(*init_args));
	
	printf("ONEQUATERS\n");
	
	init_args->att = attrib[a_num];
	init_args->a_num = &a_num;
	init_args->ret_data = ret_data;
	init_args->start_t = _start_t;
	init_args->end_t = _end_t;
	init_args->char_discovery = __ble_discover_char_callback;
	
	printf("TWO\n");
	
	pthread_create(&thread_id, NULL, __timeout_char_discover, &init_args);
	
	printf("TWOHALF\n");
	
	err = pthread_cond_timedwait(&end_func, &start_func, &abs_time);
	
	printf("THREE\n");
	
	if (err == ETIMEDOUT) {
		ERRORBACK(a_num, SLUMBER_BLE_CHAR_ERROR, "Characteristics timeout!");
	}
	
	if(!err) {
		pthread_mutex_unlock(&start_func);
	}
	
	printf("FOUR\n");
	
	return (*init_args->ret_data);*/
	
	return SLUMBER_BLE_OK;
}

char ble_write_characteristic(params_t *_param, const char *uuid, const char *value_w) {
	if(_param->conn_state == STATE_DISCONNECTED) {
		ERRORBACK(_param, SLUMBER_BLE_CONNECTION_ERROR, "Not connected to a device!");
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	LOGBACK(_param, "Attempting to write a characteristic on the BLE device");
	
	if(uuid == NULL) {
		ERRORBACK(_param, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID cannot be NULL");
		return SLUMBER_BLE_UUID_ERROR;
	}
	
	if(value_w == NULL) {
		ERRORBACK(_param, SLUMBER_BLE_VALUE_ERROR, "The characteristic value cannot be NULL");
		return SLUMBER_BLE_VALUE_ERROR;
	}
	
	uint8_t *value;
	size_t plen;
	int handle;
	
	handle = ble_string_to_handle(uuid);
	if (handle <= 0) {
		ERRORBACK(_param, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID bad parsing");
		return SLUMBER_BLE_UUID_ERROR;
	}

	plen = ble_string_to_attr(value_w, &value);
	if (plen == 0) {
		ERRORBACK(_param, SLUMBER_BLE_VALUE_ERROR, "The characteristic value is invalid");
		return SLUMBER_BLE_VALUE_ERROR;
	}
	
	char buff_line[50];
	sprintf(buff_line, "Writing %s to 0x%04x", value_w, handle);

	LOGBACK(_param, buff_line);

	//Write the new characteristic value and call the callback on finish
	if(gatt_write_char(_param->attrib, handle, value, plen, __ble_write_char_callback, _param) != 0)
		return SLUMBER_BLE_WRITE_ERROR;
		
	g_free(value);
	
	return SLUMBER_BLE_OK;
}

char ble_read_characteristic(params_t *_param, const char *uuid) {
	if(_param->conn_state == STATE_DISCONNECTED) {
		ERRORBACK(_param, SLUMBER_BLE_CONNECTION_ERROR, "Not connected to a device!");
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	LOGBACK(_param, "Attempting to read a characteristic on the BLE device");
	
	if(uuid == NULL) {
		ERRORBACK(_param, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID cannot be NULL");
		return SLUMBER_BLE_UUID_ERROR;
	}
	
	int ret;
	
	ret = __ble_parse_uuid(_param, uuid);
	if (ret != 0) {
		ERRORBACK(_param, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID bad parsing");
		return SLUMBER_BLE_UUID_ERROR;
	}
	
	char buff_line[100];
	sprintf(buff_line, "Reading from uuid %s", uuid);
	LOGBACK(_param, buff_line);

	gatt_read_char_by_uuid(_param->attrib, _param->_start_t, _param->_end_t, 
			_param->_uuid_t, __ble_read_char_uuid_callback, _param);
	return SLUMBER_BLE_OK;
}

//Custom channel watch when wrong events happen
gboolean __ble_channel_watcher(GIOChannel *chan, GIOCondition cond,
				gpointer user_data) {
	//int a_num = *((int*)user_data); //The adapter number
	__ble_disconnect_channel((params_t *) user_data); //Discontinue the channel
	return FALSE;
}

void __ble_connect_main(GIOChannel *io, GError *err, gpointer user_data) {
	//int a_num = *((int*)user_data); //The adapter number
	
	params_t *_param = (params_t*) user_data;
	int a_num = _param->_a_num;
	
	
	//if there was an error detected print details
	if (err) {
		_param->conn_state = STATE_DISCONNECTED; //Set enum flag to disconnected
		printf("%s\n", err->message);
		return;
	}

	_param->attrib = g_attrib_new(_param->chan); //Create the attribute on the channel
	g_attrib_register(_param->attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
		__ble_events_handler, _param, NULL); //Handle notification events on channel
	//g_attrib_register(attrib[a_num], ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
	//					events_handler, attrib, NULL);
	LOGBACK(_param, "Connection successful. Configuring...");
	
	MUTEXLOCK(_param);
	_param->conn_state = STATE_CONNECTING; //Set enum flag to connecting
	MUTEXUNLOCK(_param);
	ble_discover_services(_param);
}

char __ble_parse_uuid(params_t *_param, const char *value) {
	if (!value)
		return SLUMBER_BLE_ERROR;

	//We first must allocate memory for the UUID struct or we will get a seg fault
	_param->_uuid_t = g_try_malloc(sizeof(_param->_uuid_t));
	if (_param->_uuid_t == NULL)
		return SLUMBER_BLE_ERROR;

	//Turn the string into the ble struct
	if (bt_string_to_uuid(_param->_uuid_t, value) < 0)
		return SLUMBER_BLE_ERROR;

	return SLUMBER_BLE_OK;
}

char ble_stop(int a_num) {ble_set(a_num, STATE_CONNECTING);
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	
	params_t *_param = &PROPS[a_num];
	
	MUTEXLOCK(_param);
	
	__ble_disconnect_channel(_param);
	
	g_main_loop_quit(_param->event_loop);
	g_main_loop_unref(_param->event_loop); //Cleanup the mainloop
	
	MUTEXUNLOCK(_param);
	
	return SLUMBER_BLE_OK;
}

//Start the glib mainloop in a separate thread
void *__ble_start(void *data) {
	g_main_loop_run(((params_t *) data)->event_loop); //Run the mainloop
	return NULL;
}

char ble_start(int a_num, const char *adapter, 
		const char *device, const char *sec_level) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR; //Return error not that many adapters available
	
	params_t *_init_param = &PROPS[a_num];
	
	MUTEXLOCK(_init_param);
	
	if(_init_param->conn_state == STATE_CONNECTED) {
		ERRORBACK(_init_param, SLUMBER_BLE_ALREADY_CONNECTED, "Already connected");
		MUTEXUNLOCK(_init_param);
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	//Setup the struct defaults
	_init_param->mutex = mutex_locks[a_num];
	_init_param->_a_num = a_num;
	_init_param->_start_t = _start_t;
	_init_param->_end_t = _end_t;
	_init_param->_handle_t = _handle_t;
	_init_param->_mtu = _mtu;
	_init_param->_psm = _psm;
	_init_param->_address_type = _address_type;
	_init_param->_src = _src;
	_init_param->_dst = _dst;
	_init_param->_sec_level = _sec_level;
	_init_param->_uuid_t = _uuid_t;
	_init_param->chan = chan;
	_init_param->attrib = attrib;
	_init_param->gerr = gerr;
	_init_param->char_passed = char_passed;
	
	//Set dynamic settings for each adapter
	_init_param->_src = g_strdup(adapter);
	_init_param->_dst = g_strdup(device);
	_init_param->_sec_level = g_strdup(sec_level);
	
	//Set the BLE io channel using the above custom function
	_init_param->chan = ble_gatt_connect(_init_param->_src, _init_param->_dst,
		_init_param->_address_type, _init_param->_sec_level, _init_param->_psm,
		_init_param->_mtu, __ble_connect_main, &_t_props);
	
	_init_param->conn_state = STATE_CONNECTING;
	
	if(_init_param->chan == NULL) {
		_init_param->conn_state = STATE_DISCONNECTED;
		char buff_print[200];
		sprintf(buff_print, "Failed connecting to the device: %s", device);
		
		ERRORBACK(_init_param, SLUMBER_BLE_CONNECTION_ERROR, buff_print);
		MUTEXUNLOCK(_init_param);
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	//Add channel watch to adapter
	g_io_add_watch(_init_param->chan, G_IO_HUP, __ble_channel_watcher, &_t_props);
	
	//Create a new mainloop to run the channel
	_init_param->event_loop = g_main_loop_new(NULL, FALSE);
	
	//Create the new starting thread
	pthread_create(&_init_param->thread, NULL, __ble_start, _init_param);
	
	MUTEXUNLOCK(_init_param);
	
	return SLUMBER_BLE_OK;
}

/*
void callback(int a, const char *response) {
	printf("Response recieved: %s\n", response);
}

void why(int code, const char *message) {
	printf("CODE: %d MESSAGE: %s\n", code, message); 
}

void logger(const char *tolog) {
	printf("LOGGER: %s\n", tolog);
}

void detach() {
	printf("Disconnected!\n");
	//ble_stop(0);
}

int main() {
	printf("A: %d\n", ble_attach_errorback(0, why));
	printf("B: %d\n", ble_attach_logback(0, logger));
	printf("C: %d\n", ble_reset_drivers(NULL));
	printf("D: %d\n", ble_attach_callback(0, callback));
	ble_attach_disconnected(0, detach);
	printf("E: %d\n", ble_start(0, NULL, "D0:41:31:31:40:BB", "low"));
	
	while(1);
}*/

