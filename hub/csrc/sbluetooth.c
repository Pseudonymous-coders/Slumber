#include <sbluetooth/sbluetooth.h>
#include <sbluetooth/bluetooth.h>
#include <sbluetooth/gatt.h>
#include <sbluetooth/gattrib.h>
#include <sbluetooth/hci.h>
#include <sbluetooth/hci_lib.h>
#include <sbluetooth/btio.h>
#include <sbluetooth/uuid.h>
#include <sbluetooth/rfcomm.h>

#define SLUMBER_BLE_SERVICE_STR_UUID	"6e400001" //BLE UART short service uuid
#define SLUMBER_BLE_SERVICE_TX_ID		"6e400002" //BLE UART transmit characteristic
#define SLUMBER_BLE_SERVICE_RX_ID		"6e400003" //BLE UART recieve characteristic
#define SLUMBER_BLE_SERVICE_RX_FLAG		"0100" //Flag to get requests from RX characteristic
#define SLUMBER_BLE_SERVICE_FLAGS		"2902" //Flag to get all service handlers

#define ERRORBACK(X, Y, Z) if(_errorback_func[X] != NULL) _errorback_func[X](Y, Z)
#define LOGBACK(X, Y) if(_logback_func[X] != NULL) _logback_func[X](Y)

//Connection definition settings
int _start_t = 0x0001;
int _end_t = 0xffff;
int _handle_t = -1;
int _mtu = 0;
int _psm = 0;
const char *_address_type = "random"; //Connect to the device with a random address

//Device and adapter definitions
char *_src[A_MAX] = {NULL}; //The local adapter to use
char *_dst[A_MAX] = {NULL}; //The BLE device to connect to (MAC address) 
char *_sec_level[A_MAX] = {NULL}; //The security level to use with the device
bt_uuid_t *_uuid_t = {NULL}; //The uuid to look for
bt_uuid_t _uuid_temp; //Temporary uuid holder
GMainLoop *event_loop[A_MAX]; //Mainloops per BLE device
GIOChannel *chan[A_MAX] = {NULL}; //Channels to attach BLE devices to
GAttrib *attrib[A_MAX] = {NULL}; //Attributes for the devices
GError *gerr[A_MAX] = {NULL}; //Error reporting

int start[A_MAX]; //Temporary adapter beginnning address
int end[A_MAX]; //Same as above except the final address
char char_passed[A_MAX] = {0}; //Characteristic passing flag
char rx_handler[A_MAX][50]; //The RX characteristic handler flag

callback_ptr_t _callback_func[A_MAX]; //Callback functions to call when a notification happens
errorback_ptr_t _errorback_func[A_MAX]; //Callback functions when a error occurs
logback_ptr_t _logback_func[A_MAX]; //Callback functions to log data on main logger

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
	return (conn_state[a_num] == STATE_CONNECTED);
}

void ble_set(int a_num, enum state st) {
	if(a_num > A_MAX) return;
	conn_state[a_num] = st;
}

//Reset the device drivers
char ble_reset_drivers(const char *adapter) {
	const char *execute_buff = "service bluetooth restart && hciconfig ";
	char adapter_buff[150];
	int ret;
	
	if(adapter == NULL) {
		sprintf(adapter_buff, "%s%s reset", execute_buff, "hci0");
	} else {
		sprintf(adapter_buff, "%s%s reset", execute_buff, adapter);
	}
	
	LOGBACK(0, "Unblocking all rfkill options");
	ret = system("rfkill unblock all");
	if(ret != 0) {
		ERRORBACK(0, SLUMBER_BLE_DRIVER_ERROR, "Failed to unblock bluetooth from rfkill");
	} else {
		LOGBACK(0, "Unblocked everything from rfkill");
	}

	LOGBACK(0, "Resetting the bluetooth drivers");
	ret = system(adapter_buff);
	sleep(1); //Wait one second to finish
	
	if(ret != 0) {
		ERRORBACK(0, SLUMBER_BLE_DRIVER_ERROR, "Failed to reset bluetooth drivers!");
		return SLUMBER_BLE_DRIVER_ERROR;
	} else {
		LOGBACK(0, "Bluetooth driver reset. Complete.");
	}
	return SLUMBER_BLE_OK;
}

char ble_attach_callback(int a_num, callback_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	_callback_func[a_num] = func;
	
	return SLUMBER_BLE_OK;
}

char ble_attach_errorback(int a_num, errorback_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	_errorback_func[a_num] = func;
	
	return SLUMBER_BLE_OK;
}

char ble_attach_logback(int a_num, logback_ptr_t func) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	_logback_func[a_num] = func;
	
	return SLUMBER_BLE_OK;
}

//Handle glib events such as when the library does get a response
void __ble_events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data) {
	
	//for(int ind = 0; ind < A_MAX; ind++) {
	//	if(chan
	//}
	
	int a_num = *((int*)user_data); //The adapter number
	
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
		ERRORBACK(a_num, SLUMBER_BLE_VALUE_ERROR, "Invalid opcode");
		return;
	}

	//Complete the returned string
	for (i = 3; i < len; i++) {
		g_string_append_printf(s, "%c", (char) pdu[i]);
	}
	
	if(_callback_func[a_num] == NULL) {
		LOGBACK(a_num, "No callback function provided response: ");
		LOGBACK(a_num, s->str);
	} else {
		_callback_func[a_num](s->str); //Call the callback function
	}
	
	g_string_free(s, TRUE); //Free the gstring

	//Only respond on attrib if it's an indication
	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
		return;

	opdu = g_attrib_get_buffer(attrib[a_num], &plen);
	olen = enc_confirmation(opdu, plen);

	if (olen > 0) {
		g_attrib_send(attrib[a_num], 0, opdu, olen, NULL, NULL, NULL);
	}
}

//Disconnect the specified channel adapter
void __ble_disconnect_channel(int a_num) {
	if (!ble_connected) {
		ERRORBACK(a_num, SLUMBER_BLE_ALREADY_CONNECTED, "Already disconnected");
		return;
	}

	g_attrib_unref(attrib[a_num]); //Dereference the attributes
	attrib[a_num] = NULL;

	g_io_channel_shutdown(chan[a_num], FALSE, NULL);
	g_io_channel_unref(chan[a_num]);
	chan[a_num] = NULL;
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
	int a_num = *((int*)user_data); //The adapter number
	GSList *l;

	if (status) {
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, "Discover all primary services failed!:");
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, att_ecode2str(status));
		return;
	}

	if (services == NULL) {
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, "No primary service found!");
		return;
	}
	
	char passed = 0;

	for (l = services; l; l = l->next) {
		struct gatt_primary *prim = l->data;
		if(strstr(prim->uuid, SLUMBER_BLE_SERVICE_STR_UUID) != NULL) {
			LOGBACK(a_num, "Found a slumber band: ");
			
			char buff_line[100];
			
			sprintf(buff_line, "Handler: 0x%04x <-> End Handler: 0x%04x UUID: %s",
				prim->range.start, prim->range.end, prim->uuid);
		
			LOGBACK(a_num, buff_line);
			
			passed = 1; //Set the passed flag to true
		}
	}
	
	if(!passed) {
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, "The selected bluetooth device is not a Slumber band!");
	} else {
		ble_discover_characteristics(a_num); //Discover and check the characteristics
	}
}

void __ble_discover_char_callback(guint8 status, const guint8 *pdu, 
		guint16 plen, gpointer user_data) {
	int a_num = *((int*)user_data); //The adapter number
	
	struct att_data_list *list;
	guint8 format;
	uint16_t handle = 0xffff;
	int i;
	char skipper = 1;

	if (status != 0) {
		LOGBACK(a_num, "Finished with finding characteristics:");
		LOGBACK(a_num, att_ecode2str(status));

		skipper = 0;
	}

	if(skipper) {
		list = dec_find_info_resp(pdu, plen, &format);
		
		if (list == NULL) {
			ERRORBACK(a_num, SLUMBER_BLE_CHAR_ERROR, "No characteristics found on the device!");
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
				snprintf(buff_line, 200, "Found TX Handle: 0x%04x, UUID: %s\n", handle, uuidstr);
				LOGBACK(a_num, buff_line);
				char_passed[a_num]++;
			} else if(strstr(uuidstr, SLUMBER_BLE_SERVICE_RX_ID) != NULL) {
				snprintf(buff_line, 200, "Found RX Handle: 0x%04x, UUID: %s\n", handle, uuidstr);
				LOGBACK(a_num, buff_line);
				char_passed[a_num]++;
			}
			usleep(300); //Wait 300 nanoseconds
		}
	
		att_data_list_free(list); //Clear the characteristics service list
	}
	
	if (handle != 0xffff && handle < end[a_num]) {
		gatt_discover_char_desc(attrib[a_num], handle + 1, 
			end[a_num], __ble_discover_char_callback, &a_num);
	} else {
		//The characteristic loop pull has finished check if the characteristics are correct
		if(char_passed[a_num] < 2) {
			ERRORBACK(a_num, SLUMBER_BLE_CHAR_ERROR, "Failed to get the right characteristics from the band");
		} else {
			//Both TX and RX were found continue to set the RX flag
			LOGBACK(a_num, "Found the band characteristics for the UART service");

			ble_read_characteristic(a_num, SLUMBER_BLE_SERVICE_FLAGS);
		}
	}
}


//Write a characteristic value
void __ble_write_char_callback(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data) {
	int a_num = *((int*)user_data); //The adapter number
				
	if (status != 0) {
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, "Characteristics write failed:");
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, att_ecode2str(status));
		return;
	}

	if (!dec_write_resp(pdu, plen) && !dec_exec_write_resp(pdu, plen)) {
		ERRORBACK(a_num, SLUMBER_BLE_WRITE_ERROR, "Protocol error");
		return;
	}

	LOGBACK(a_num, "The new characteristic was written successfully");
	LOGBACK(a_num, "Waiting for a response from the Slumber band");
	
	ble_set(a_num, STATE_CONNECTED); //Set the flag that the device is connected
}

//Write a characteristic value
void __ble_read_char_uuid_callback(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data) {
	int a_num = *((int*)user_data); //The adapter number
	
	struct att_data_list *list;
	int i;
	GString *s;

	if (status != 0) {
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, "Failed to read characteristic from the band:");
		ERRORBACK(a_num, SLUMBER_BLE_DISCOVERY_ERROR, att_ecode2str(status));
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
		LOGBACK(a_num, s->str);
	}
	
	sprintf(rx_handler[a_num], "0x%04x", last_handler); //Update rx_handler flag for requests
	
	char buff_line[100];
	sprintf(buff_line, "Selected %s ad the RX handler", rx_handler[a_num]);

	att_data_list_free(list);
	g_string_free(s, TRUE);
	
	//Update RX characteristic with selected handler
	ble_write_characteristic(a_num, rx_handler[a_num], SLUMBER_BLE_SERVICE_RX_FLAG);
}







/*
 *
 *
 *  CALLBACK INITIALIZERS
 *
 *
 */
char ble_discover_services(int a_num) {
	if(a_num > A_MAX) {
		ERRORBACK(a_num, SLUMBER_BLE_ADAPTER_ERROR, "Input invalid adapter number!");
		return SLUMBER_BLE_ADAPTER_ERROR;
	}

	if(gatt_discover_primary(attrib[a_num], NULL, __ble_discover_primary_callback, &a_num) != 0)
		return SLUMBER_BLE_DISCOVERY_ERROR;
	return SLUMBER_BLE_OK;
}

char ble_discover_characteristics(int a_num) {
	if(a_num > A_MAX) {
		ERRORBACK(a_num, SLUMBER_BLE_ADAPTER_ERROR, "Input invalid adapter number!");
		return SLUMBER_BLE_ADAPTER_ERROR;
	}
	
	start[a_num] = 0x0001; //Set start flag back to 0
	end[a_num] = 0xffff; //Set the end flag to end
	char_passed[a_num] = 0; //Set the flag checker back to zero
	
	if(gatt_discover_char_desc(attrib[a_num], _start_t, _end_t, __ble_discover_char_callback, &a_num) != 0)
		return SLUMBER_BLE_CHAR_ERROR;
	
	return SLUMBER_BLE_OK;
}

char ble_write_characteristic(int a_num, const char *uuid, const char *value_w) {
	if(a_num > A_MAX) {
		ERRORBACK(a_num, SLUMBER_BLE_ADAPTER_ERROR, "Input invalid adapter number!");
		return SLUMBER_BLE_ADAPTER_ERROR;
	}
	
	if(uuid == NULL) {
		ERRORBACK(a_num, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID cannot be NULL");
		return SLUMBER_BLE_UUID_ERROR;
	}
	
	if(value_w == NULL) {
		ERRORBACK(a_num, SLUMBER_BLE_VALUE_ERROR, "The characteristic value cannot be NULL");
		return SLUMBER_BLE_VALUE_ERROR;
	}
	
	uint8_t *value;
	size_t plen;
	int handle;
	
	handle = ble_string_to_handle(uuid);
	if (handle <= 0) {
		ERRORBACK(a_num, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID bad parsing");
		return SLUMBER_BLE_UUID_ERROR;
	}

	plen = ble_string_to_attr(value_w, &value);
	if (plen == 0) {
		ERRORBACK(a_num, SLUMBER_BLE_VALUE_ERROR, "The characteristic value is invalid");
		return SLUMBER_BLE_VALUE_ERROR;
	}
	
	char buff_line[50];
	sprintf(buff_line, "Writing %s to 0x%04x", value_w, handle);

	LOGBACK(a_num, buff_line);

	//Write the new characteristic value and call the callback on finish
	if(gatt_write_char(attrib[a_num], handle, value, plen, __ble_write_char_callback, &a_num) != 0)
		return SLUMBER_BLE_WRITE_ERROR;
		
	g_free(value);
	
	return SLUMBER_BLE_OK;
}

char ble_read_characteristic(int a_num, const char *uuid) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	
	if(uuid == NULL) {
		ERRORBACK(a_num, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID cannot be NULL");
		return SLUMBER_BLE_UUID_ERROR;
	}
	
	int ret;
	
	ret = __ble_parse_uuid(uuid);
	if (ret != 0) {
		ERRORBACK(a_num, SLUMBER_BLE_UUID_ERROR, "Placed invalid UUID bad parsing");
		return SLUMBER_BLE_UUID_ERROR;
	}
	
	char buff_line[100];
	sprintf(buff_line, "Reading from uuid %s", uuid);
	LOGBACK(a_num, buff_line);

	gatt_read_char_by_uuid(attrib[a_num], _start_t, _end_t, _uuid_t,
		 __ble_read_char_uuid_callback, &a_num);
	return SLUMBER_BLE_OK;
}

//Custom channel watch when wrong events happen
gboolean __ble_channel_watcher(GIOChannel *chan, GIOCondition cond,
				gpointer user_data) {
	int a_num = *((int*)user_data); //The adapter number
	__ble_disconnect_channel(a_num); //Discontinue the channel
	return FALSE;
}

void __ble_connect_main(GIOChannel *io, GError *err, gpointer user_data) {
	int a_num = *((int*)user_data); //The adapter number
	
	//if there was an error detected print details
	if (err) {
		ble_set(a_num, STATE_DISCONNECTED); //Set enum flag to disconnected
		printf("%s\n", err->message);
		return;
	}

	attrib[a_num] = g_attrib_new(chan[a_num]); //Create the attribute on the channel
	g_attrib_register(attrib[a_num], ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
		__ble_events_handler, &a_num, NULL); //Handle notification events on channel
	//g_attrib_register(attrib[a_num], ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
	//					events_handler, attrib, NULL);
	LOGBACK(a_num, "Connection successful. Configuring...");
	
	ble_set(a_num, STATE_CONNECTING); //Set enum flag to connecting
	
	ble_discover_services(a_num);
}

char __ble_parse_uuid(const char *value) {
	if (!value)
		return SLUMBER_BLE_ERROR;

	//We first must allocate memory for the UUID struct or we will get a seg fault
	_uuid_t = g_try_malloc(sizeof(_uuid_t));
	if (_uuid_t == NULL)
		return SLUMBER_BLE_ERROR;

	//Turn the string into the ble struct
	if (bt_string_to_uuid(_uuid_t, value) < 0)
		return SLUMBER_BLE_ERROR;

	return SLUMBER_BLE_OK;
}

char ble_stop(int a_num) {ble_set(a_num, STATE_CONNECTING);
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR;
	
	g_main_loop_quit(event_loop[a_num]);
	g_main_loop_unref(event_loop[a_num]); //Cleanup the mainloop
	return SLUMBER_BLE_OK;
}

char ble_start(int a_num, const char *adapter, 
		const char *device, const char *sec_level) {
	if(a_num > A_MAX) return SLUMBER_BLE_ADAPTER_ERROR; //Return error not that many adapters available
	
	if(ble_connected(a_num)) {
		ERRORBACK(a_num, SLUMBER_BLE_ALREADY_CONNECTED, "Already connected");
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	//Set dynamic settings for each adapter
	_src[a_num] = g_strdup(adapter);
	_dst[a_num] = g_strdup(device);
	_sec_level[a_num] = g_strdup(sec_level);
	
	//Set the BLE io channel using the above custom function
	chan[a_num] = ble_gatt_connect(_src[a_num], _dst[a_num], _address_type, 
		_sec_level[a_num], _psm, _mtu, __ble_connect_main, &a_num);
	
	ble_set(a_num, STATE_CONNECTING);
	
	if(chan[a_num] == NULL) {
		ble_set(a_num, STATE_DISCONNECTED);
		char buff_print[200];
		sprintf(buff_print, "Failed connecting to the device: %s", device);
		
		ERRORBACK(a_num, SLUMBER_BLE_CONNECTION_ERROR, buff_print);
		printf("%s\n", buff_print);
		return SLUMBER_BLE_CONNECTION_ERROR;
	}
	
	//Add channel watch to adapter
	g_io_add_watch(chan[a_num], G_IO_HUP, __ble_channel_watcher, &a_num);
	
	//Create a new mainloop to run the channel
	event_loop[a_num] = g_main_loop_new(NULL, FALSE);
	
	g_main_loop_run(event_loop[a_num]); //Run the mainloop
	
	return SLUMBER_BLE_OK;
}

/*void callback(const char *response) {
	printf("Response recieved: %s\n", response);
}

void why(int code, const char *message) {
	printf("CODE: %d MESSAGE: %s\n", code, message); 
}

void logger(const char *tolog) {
	printf("LOGGER: %s\n", tolog);
}

int main() {
	printf("A: %d\n", ble_attach_errorback(0, why));
	printf("B: %d\n", ble_attach_logback(0, logger));
	printf("C: %d\n", ble_reset_drivers(NULL));
	printf("D: %d\n", ble_attach_callback(0, callback));
	printf("E: %d\n", ble_start(0, NULL, "D0:41:31:31:40:BB", "low"));
}*/

