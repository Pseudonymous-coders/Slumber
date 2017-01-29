#ifndef SLUMBER_SBLUETOOTHC_H
#define SLUMBER_SBLUETOOTHC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <glib.h>

#define A_MAX 1 //Maximum ammount of adapters allowed

#define SLUMBER_BLE_OK 0
#define SLUMBER_BLE_ERROR -1
#define SLUMBER_BLE_ALREADY_CONNECTED -2
#define SLUMBER_BLE_ADAPTER_ERROR -3
#define SLUMBER_BLE_CONNECTION_ERROR -4
#define SLUMBER_BLE_DRIVER_ERROR -5
#define SLUMBER_BLE_DISCOVERY_ERROR -6
#define SLUMBER_BLE_UUID_ERROR -7
#define SLUMBER_BLE_VALUE_ERROR -8
#define SLUMBER_BLE_READ_ERROR -9
#define SLUMBER_BLE_WRITE_ERROR -10
#define SLUMBER_BLE_CHAR_ERROR -11

//Function pointer for on recieve callback, on error and general logging
typedef void (*callback_ptr_t)(int, const char *);
typedef void (*errorback_ptr_t)(int, const char *);
typedef void (*logback_ptr_t)(const char *);
typedef void (*disconnected_ptr_t)(int);
typedef void (*connected_ptr_t)(int);

//Create the new params struct to not confuse locals
typedef struct params params_t;

//Current connection state
enum state {
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED
};

//PUBLIC FUNCTIONS
char ble_start(int, const char *, const char *, const char *);
size_t ble_string_to_attr(const char *, uint8_t **);
char ble_string_to_handle(const char *);
char ble_reset_drivers(const char *);
char ble_stop(int a_num);
char ble_connected(int);
char ble_connecting(int);
char ble_disconnecting(int);
void ble_set(int, enum state);

//PUBLIC INITIALIZERS
char ble_discover_services(params_t *);
char ble_discover_characteristics(params_t *);
char ble_write_characteristic(params_t *, const char *, const char *);
char ble_read_characteristic(params_t *, const char *);
char ble_attach_callback(int, callback_ptr_t);
char ble_attach_errorback(int, errorback_ptr_t);
char ble_attach_logback(int, logback_ptr_t);
char ble_attach_disconnected(int, disconnected_ptr_t);
char ble_attach_connected(int, connected_ptr_t);

//PRIVATE FUNCTIONS
void __ble_events_handler(const uint8_t *, uint16_t, gpointer);
char __ble_parse_uuid(params_t *, const char *value);
//void __ble_disconnect_channel(int);
gboolean __ble_channel_watcher(GIOChannel *, GIOCondition, gpointer);
void __ble_connect_main(GIOChannel *, GError *, gpointer);

//PRIVATE CALLBACKS
void __ble_discover_primary_callback(GSList *, guint8, gpointer);
void __ble_discover_char_callback(guint8, const guint8 *, guint16, gpointer);
void __ble_write_char_callback(guint8, const guint8 *, guint16, gpointer);
void __ble_read_char_uuid_callback(guint8, const guint8 *, guint16, gpointer);

#ifdef __cplusplus
};
#endif

#endif
