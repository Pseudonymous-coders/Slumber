//#include <sbluetooth/att.h>

extern "C" {
#include <sbluetooth/gatt.h>
#include <sbluetooth/gattrib.h>
#include <sbluetooth/hci.h>
#include <sbluetooth/btio.h>
#include <sbluetooth/uuid.h>
#include <sbluetooth/rfcomm.h>
};


#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <glib.h>


static int opt_start = 0x0001;
static int opt_end = 0xffff;
static int opt_handle = -1;
static int opt_mtu = 0;
static int opt_psm = 0;

static int start;
static int end;

static char *opt_src = NULL;
static char *opt_sec_level = "low";


static bt_uuid_t *opt_uuid = NULL;
static gboolean r_primary = FALSE;
static GMainLoop *event_loop;
static gboolean got_error = FALSE;
static GSourceFunc operation;

static GIOChannel *iochannel = NULL;
static GAttrib *attrib = NULL;
static GString *prompt;

struct characteristic_data {
	GAttrib *attrib;
	uint16_t start;
	uint16_t end;
};


size_t gatt_attr_data_from_string(const char *str, uint8_t **data)
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

/*static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	GAttrib *attrib;

	if (err) {
		g_printerr("%s\n", err->message);
		got_error = TRUE;
		g_main_loop_quit(event_loop);
	}

	attrib = g_attrib_new(io);

	operation(attrib);
}*/


static int strtohandle(const char *src)
{
	char *e;
	int dst;

	errno = 0;
	dst = strtoll(src, &e, 16);
	if (errno != 0 || *e != '\0')
		return -EINVAL;

	return dst;
}



static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	uint8_t *opdu;
	uint16_t handle, i, olen;
	size_t plen;
	GString *s;

	handle = att_get_u16(&pdu[1]);

	switch (pdu[0]) {
	case ATT_OP_HANDLE_NOTIFY:
		s = g_string_new(NULL);
		g_string_printf(s, "Notification handle = 0x%04x value: ",
									handle);
		break;
	case ATT_OP_HANDLE_IND:
		s = g_string_new(NULL);
		g_string_printf(s, "Indication   handle = 0x%04x value: ",
									handle);
		break;
	default:
		printf("Invalid opcode\n");
		return;
	}

	for (i = 3; i < len; i++)
		g_string_append_printf(s, "%c", (char) pdu[i]);

	printf("%s\n", s->str);
	g_string_free(s, TRUE);

	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
		return;

	opdu = g_attrib_get_buffer(attrib, &plen);
	olen = enc_confirmation(opdu, plen);

	if (olen > 0)
		g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	if (err) {
		printf("%s\n", err->message);
		return;
	}

	attrib = g_attrib_new(iochannel);
	g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
						events_handler, attrib, NULL);
	g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
						events_handler, attrib, NULL);
	printf("Connection successful\n");
	
	operation(attrib);
}

static void mainloop_quit(gpointer user_data)
{
	uint8_t *value = user_data;

	g_free(value);
	g_main_loop_quit(event_loop);
}


static void primary_all_cb(GSList *services, guint8 status, gpointer user_data)
{
	GSList *l;

	if (status) {
		g_printerr("Discover all primary services failed: %s\n",
							att_ecode2str(status));
		goto done;
	}

	for (l = services; l; l = l->next) {
		struct gatt_primary *prim = l->data;
		g_print("attr handle = 0x%04x, end grp handle = 0x%04x "
			"uuid: %s\n", prim->range.start, prim->range.end, prim->uuid);
	}

done:
	g_main_loop_quit(event_loop);
}

static void primary_by_uuid_cb(GSList *ranges, guint8 status,
							gpointer user_data)
{
	GSList *l;

	if (status != 0) {
		g_printerr("Discover primary services by UUID failed: %s\n",
							att_ecode2str(status));
		goto done;
	}

	for (l = ranges; l; l = l->next) {
		struct att_range *range = l->data;
		g_print("Starting handle: %04x Ending handle: %04x\n",
						range->start, range->end);
	}

done:
	g_main_loop_quit(event_loop);
}

static void char_discovered_cb(GSList *characteristics, guint8 status,
							gpointer user_data)
{
	GSList *l;

	if (status) {
		g_printerr("Discover all characteristics failed: %s\n",
							att_ecode2str(status));
		goto done;
	}

	for (l = characteristics; l; l = l->next) {
		struct gatt_char *chars = l->data;

		g_print("handle = 0x%04x, char properties = 0x%02x, char value "
			"handle = 0x%04x, uuid = %s\n", chars->handle,
			chars->properties, chars->value_handle, chars->uuid);
	}

done:
	g_main_loop_quit(event_loop);
}

static gboolean characteristics(gpointer user_data)
{
	GAttrib *attrib = user_data;

	gatt_discover_char(attrib, opt_start, opt_end, opt_uuid,
						char_discovered_cb, NULL);

	return FALSE;
}


static void char_read_by_uuid_cb(guint8 status, const guint8 *pdu,
					guint16 plen, gpointer user_data)
{
	struct att_data_list *list;
	int i;
	GString *s;

	if (status != 0) {
		printf("Read characteristics by UUID failed: %s\n",
							att_ecode2str(status));
		return;
	}

	list = dec_read_by_type_resp(pdu, plen);
	if (list == NULL)
		return;

	s = g_string_new(NULL);
	for (i = 0; i < list->num; i++) {
		uint8_t *value = list->data[i];
		int j;

		g_string_printf(s, "handle: 0x%04x \t value: ",
							att_get_u16(value));
		value += 2;
		for (j = 0; j < list->len - 2; j++, value++)
			g_string_append_printf(s, "%02x ", *value);

		printf("%s\n", s->str);
	}

	att_data_list_free(list);
	g_string_free(s, TRUE);
}

static gboolean cmd_read(gpointer user_data) {
	GAttrib *attrib = user_data;
	
	gatt_read_char_by_uuid(attrib, opt_start, opt_end, opt_uuid, char_read_by_uuid_cb, NULL);
}

static void char_desc_cb(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data)
{
	struct att_data_list *list;
	guint8 format;
	uint16_t handle = 0xffff;
	int i;

	if (status != 0) {
		printf("Discover descriptors finished: %s\n",
						att_ecode2str(status));
		return;
	}

	list = dec_find_info_resp(pdu, plen, &format);
	if (list == NULL)
		return;

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
		printf("handle: 0x%04x, uuid: %s\n", handle, uuidstr);
	}

	att_data_list_free(list);

	if (handle != 0xffff && handle < end)
		gatt_discover_char_desc(attrib, handle + 1, end, char_desc_cb,
									NULL);
}

static void cmd_char_desc(int argcp, char **argvp)
{
	gatt_discover_char_desc(attrib, opt_start, opt_end, char_desc_cb, NULL);
}

static gboolean primary(gpointer user_data)
{
	GAttrib *attrib = user_data;

	if (opt_uuid)
		gatt_discover_primary(attrib, opt_uuid, primary_by_uuid_cb,
									NULL);
	else
		gatt_discover_primary(attrib, NULL, primary_all_cb, NULL);

	return FALSE;
}

static void char_read_cb(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data)
{
	uint8_t value[plen];
	ssize_t vlen;
	int i;
	GString *s;

	if (status != 0) {
		printf("Characteristic value/descriptor read failed: %s\n",
							att_ecode2str(status));
		return;
	}

	vlen = dec_read_resp(pdu, plen, value, sizeof(value));
	if (vlen < 0) {
		printf("Protocol error\n");
		return;
	}

	s = g_string_new("Characteristic value/descriptor: ");
	for (i = 0; i < vlen; i++)
		g_string_append_printf(s, "%02x ", value[i]);

	printf("%s\n", s->str);
	g_string_free(s, TRUE);
}

static gboolean cmd_read_hnd(gpointer user_data)
{
	int handle;

	handle = strtohandle("0x0023");
	if (handle < 0) {
		printf("Invalid handle");
		return FALSE;
	}

	gatt_read_char(attrib, handle, char_read_cb, attrib);
	return TRUE;
}

static void char_write_req_cb(guint8 status, const guint8 *pdu, guint16 plen,
							gpointer user_data)
{
	if (status != 0) {
		printf("Characteristic Write Request failed: "
						"%s\n", att_ecode2str(status));
		return;
	}

	if (!dec_write_resp(pdu, plen) && !dec_exec_write_resp(pdu, plen)) {
		printf("Protocol error\n");
		return;
	}

	printf("Characteristic value was written successfully\n");
	cmd_read_hnd(NULL);
}

static gboolean cmd_char_write(gpointer user_data)
{
	GAttrib *attrib = user_data;
	uint8_t *value;
	size_t plen;
	int handle;
	
	handle = strtohandle("0x0023");
	if (handle <= 0) {
		printf("A valid handle is required\n");
		return FALSE;
	}

	plen = gatt_attr_data_from_string("0300", &value);
	if (plen == 0) {
		printf("Invalid value\n");
		return FALSE;
	}

	//if (g_strcmp0("char-write-req", argvp[0]) == 0)
	gatt_write_char(attrib, handle, value, plen, char_write_req_cb, NULL);
	//else
	//	gatt_write_cmd(attrib, handle, value, plen, NULL, NULL);

	g_free(value);
	
	return TRUE;
}

static gboolean characteristics_desc(gpointer user_data)
{
	GAttrib *attrib = user_data;

	gatt_discover_char_desc(attrib, opt_start, opt_end, char_desc_cb, NULL);

	return FALSE;
}

static void disconnect_io()
{
	g_attrib_unref(attrib);
	attrib = NULL;
	opt_mtu = 0;

	g_io_channel_shutdown(iochannel, FALSE, NULL);
	g_io_channel_unref(iochannel);
	iochannel = NULL;
}

static gboolean channel_watcher(GIOChannel *chan, GIOCondition cond,
				gpointer user_data)
{
	disconnect_io();

	return FALSE;
}

static gboolean parse_uuid(const char *value,
				gpointer user_data, GError **error)
{
	if (!value)
		return FALSE;

	opt_uuid = g_try_malloc(sizeof(bt_uuid_t));
	if (opt_uuid == NULL)
		return FALSE;

	if (bt_string_to_uuid(opt_uuid, value) < 0)
		return FALSE;

	return TRUE;
}

bt_uuid_t uuid;

int main() {
	GError *gerr = NULL;

	operation = cmd_char_write;
	
	
	if (bt_string_to_uuid(&uuid, "0x0023") < 0) {
		printf("Invalid UUID\n");
		return 1;
	}
	
	opt_uuid = &uuid;

	iochannel = gatt_connect(NULL, "D0:41:31:31:40:BB", "random", "low", 0, 0, connect_cb, &gerr);
	
	if (iochannel == NULL) {
		g_printerr("%s\n", gerr->message);
		g_clear_error(&gerr);
		got_error = TRUE;
		goto done;
	}
	
	g_io_add_watch(iochannel, G_IO_HUP, channel_watcher);

	event_loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(event_loop);
	g_main_loop_unref(event_loop);

done:
	printf("done!\n");
	//g_option_context_free(context);
	return 0;
}
