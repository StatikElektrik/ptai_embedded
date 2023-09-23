/// NOT FINISHED

/**
 * @brief
 *
 * @file
 *
 */

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/
#include <stdio.h>
#include "thingsboard_iot.h"
#include "coap_handler.h"
#include "cloud_codec/cloud_codec.h"

/* STEP 2.2 - Include the header file for the CoAP library */
#include <zephyr/net/coap.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <nrf_socket.h>

#include <zephyr/logging/log.h>

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/

#define MODULE thingsboard_iot
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

// #define THINGSBOARD_IOT_SETTINGS_KEY "thingsboard_iot"
// #define THINGSBOARD_IOT_SETTINGS_KEY_CONFIG "config"

#if defined(CONFIG_THINGSBOARD_IOT_DEVICE_TOKEN)
#define THINGSBOARD_IOT_DEVICE_TOKEN_SIZE 21
#define THINGSBOARD_IOT_IS_DEVICE_TOKEN_DEFINED true
#define THINGSBOARD_IOT_DEVICE_TOKEN CONFIG_THINGSBOARD_IOT_DEVICE_TOKEN
#else
#define THINGSBOARD_IOT_DEVICE_TOKEN_SIZE CONFIG_THINGSBOARD_IOT_DEVICE_TOKEN_SIZE
#define THINGSBOARD_IOT_IS_DEVICE_TOKEN_DEFINED false
#endif

#define CONFIG_THINGSBOARD_IOT_STACK_SIZE 1024
/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
enum connection_state
{
	/* The library is uninitialized. */
	STATE_UNINIT,
	/* The library is initialized, no connection established. */
	STATE_DISCONNECTED,
	/* Connecting to Azure IoT Hub. */
	STATE_CONNECTING,
	/* Connected to Azure IoT Hub on MQTT level. */
	STATE_CONNECTED,
	/* Disconnecting from Azure IoT Hub. */
	STATE_DISCONNECTING,

	STATE_COUNT
};

enum device_provision_state
{
	PROVISIONED,
	NOT_PROVISIONED
};

struct thingsboard_iot_dev_settings
{
	char device_token[THINGSBOARD_IOT_DEVICE_TOKEN_SIZE];
};

/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/
// static K_SEM_DEFINE(connection_poll_sem, 0, 1);
// static K_SEM_DEFINE(config_load_sem, 0, 1);

static enum connection_state connection_state = STATE_UNINIT;
static enum device_provision_state provision_state = NOT_PROVISIONED;

static thingsboard_iot_evt_handler_t evt_handler;
static struct thingsboard_iot_dev_settings dev_settings;

// static int config_settings_handler(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg);
// SETTINGS_STATIC_HANDLER_DEFINE(MODULE, THINGSBOARD_IOT_SETTINGS_KEY, NULL, config_settings_handler, NULL, NULL);

/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/
static void thingsboard_iot_notify_event(struct thingsboard_iot_evt *evt)
{
	if (evt_handler)
	{
		evt_handler(evt);
	}
}

static const char *device_provision_state_name_get(enum device_provision_state state)
{
	switch (state)
	{
	case PROVISIONED:
		return "PROVISIONED";
	case NOT_PROVISIONED:
		return "NOT_PROVISIONED";
	default:
		return "STATE_UNKNOWN";
	}
}

static const char *thingsboard_iot_message_type_name_get_lower_case(enum thingsboard_iot_message_type msg_type)
{
	switch (msg_type)
	{
	case TELEMETRY:
		return "telemetry";
	case ATTRIBUTE:
		return "attribute";
	default:
		return "";
	}
}

static const char *connection_state_name_get(enum connection_state state)
{
	switch (state)
	{
	case STATE_UNINIT:
		return "STATE_UNINIT";
	case STATE_DISCONNECTED:
		return "STATE_DISCONNECTED";
	case STATE_CONNECTING:
		return "STATE_CONNECTING";
	case STATE_CONNECTED:
		return "STATE_CONNECTED";
	case STATE_DISCONNECTING:
		return "STATE_DISCONNECTING";
	default:
		return "STATE_UNKNOWN";
	}
}

static void connection_state_set(enum connection_state new_state)
{
	bool notify_error = false;

	if (connection_state == new_state)
	{
		LOG_DBG("Skipping transition to the same state (%s)",
				connection_state_name_get(connection_state));
		return;
	}

	/* Check for legal state transitions. */
	switch (connection_state)
	{
	case STATE_UNINIT:
		if (new_state != STATE_DISCONNECTED)
		{
			notify_error = true;
		}
		break;
	case STATE_DISCONNECTED:
		if (new_state != STATE_CONNECTING)
		{
			notify_error = true;
		}
		break;
	case STATE_CONNECTING:
		if (new_state != STATE_CONNECTED &&
			new_state != STATE_DISCONNECTED)
		{
			notify_error = true;
		}
		break;
	case STATE_CONNECTED:
		if (new_state != STATE_DISCONNECTING &&
			new_state != STATE_DISCONNECTED)
		{
			notify_error = true;
		}
		break;
	case STATE_DISCONNECTING:
		if ((new_state != STATE_DISCONNECTED))
		{
			notify_error = true;
		}
		break;
	default:
		LOG_ERR("New state is unknown");
		notify_error = true;
		break;
	}

	LOG_DBG("Connection state transition: %s --> %s",
			connection_state_name_get(connection_state),
			connection_state_name_get(new_state));

	connection_state = new_state;
}

static void device_provision_state_set(enum device_provision_state new_state)
{
	LOG_DBG("Device provision state transition: %s --> %s",
			device_provision_state_name_get(provision_state),
			device_provision_state_name_get(new_state));
	provision_state = new_state;
}

static bool is_device_registered()
{
	return (provision_state == PROVISIONED) ? true : false;
}

static bool connection_state_verify(enum connection_state state)
{
	return (connection_state == state);
}

static void request_provision(const char *device_name, const char *device_provision_key, const char *device_provision_secret)
{

	struct cloud_data_device_provision_request cloud_data;
	// @todo Change with strnlen.
	strcpy(cloud_data.device_name, device_name);
	strcpy(cloud_data.device_provision_key, device_provision_key);
	strcpy(cloud_data.device_provision_secret, device_provision_secret);

	struct cloud_codec_data codec = {0};
	int err = cloud_codec_encode_device_provision_request_data(&codec, &cloud_data);
	switch (err)
	{
	case 0:
		LOG_DBG("Device provision data encoded successfully");
		break;
	case -ENOTSUP:
		/* Cloud location data encoding not supported */
		return;
		break;
	case -ENODATA:
		LOG_DBG("No cloud location data to encode, error: %d", err);
		return;
		break;
	default:
		LOG_ERR("Error encoding cloud location data: %d", err);
		return;
	}

	err = coap_handler_client_put_confirmable_send("/api/v1/provision", codec.buf, codec.len);
	if (err < 0)
	{
		LOG_ERR("Connect failed : %d\n", err);
	}
}

// static void send_attribute_data(const char *device_token, const char *data, size_t data_len)
// {
// }

// static void send_telemetry_data(const char *device_token, const char *data, size_t data_len)
// {
// }

static bool is_dev_token_saved()
{
	size_t token_len = strlen(dev_settings.device_token);
	if (token_len < THINGSBOARD_IOT_DEVICE_TOKEN_SIZE)
	{
		LOG_DBG("Device token is not saved to the flash.");
		return false;
	}
	return true;
}

static int read_device_token()
{
	// @todo Implement later.
	// If config is defined, read it from there, if not read it from the flash.
	int err = 0;
	if (THINGSBOARD_IOT_IS_DEVICE_TOKEN_DEFINED)
	{
		strncpy(dev_settings.device_token, THINGSBOARD_IOT_DEVICE_TOKEN, THINGSBOARD_IOT_DEVICE_TOKEN_SIZE);
	}
	else
	{
		err = is_dev_token_saved();
	}
	return err;
}

static void register_device()
{
	request_provision("device_name", "provision_key", "provision_secret");
}

// static void thingsboard_iot_run(void)
// {
// 	while (true)
// 	{
// 		LOG_DBG("Waiting on connection..");
// 		k_sem_take(&connection_poll_sem, K_FOREVER);

// 		while (true)
// 		{
// 			LOG_DBG("Waiting for response..");
// 			if (connection_state_verify(STATE_DISCONNECTED))
// 			{
// 				LOG_DBG("The socket is already closed");
// 				break;
// 			}

// 			// received = recv(sock, coap_buf, sizeof(coap_buf), 0);

// 			// if (received < 0)
// 			// {
// 			// 	LOG_ERR("Socket error: %d, exit\n", errno);
// 			// 	break;
// 			// }
// 			// else if (received == 0)
// 			// {
// 			// 	LOG_INF("Empty datagram\n");
// 			// 	continue;
// 			// }

// 			// /* STEP 12 - Parse the received CoAP packet */
// 			// err = client_handle_response(coap_buf, received);
// 			// if (err < 0)
// 			// {
// 			// 	LOG_ERR("Invalid response, exit\n");
// 			// 	break;
// 			// }
// 		}

// 		/* Always revert to the initialization state if the socket has been
// 		 * closed.
// 		 */
// 		connection_state_set(STATE_DISCONNECTED);
// 		//(void)close(sock);
// 	}
// }

/* Forward declarations */

/* Private API */

// static int config_settings_handler(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
// {
// 	if (!key)
// 	{
// 		LOG_ERR("Invalid key");
// 		return -EINVAL;
// 	}

// 	LOG_DBG("Settings key: %s, size: %d", key, len_rd);

// 	if (!strncmp(key, THINGSBOARD_IOT_SETTINGS_KEY_CONFIG, strlen(THINGSBOARD_IOT_SETTINGS_KEY_CONFIG)))
// 	{
// 		size_t len = read_cb(cb_arg, &dev_settings, sizeof(dev_settings));
// 		if (len > 0)
// 		{
// 			return 0;
// 		}

// 		LOG_ERR("No settings data read.");
// 		return -ENODATA;
// 	}

// 	k_sem_give(&config_load_sem);
// 	return -ENOTSUP;
// }

// static int enable_settings(void)
// {
// 	int err = settings_subsys_init();
// 	if (err)
// 	{
// 		LOG_ERR("settings_subsys_init, error: %d", err);
// 		return err;
// 	}

// 	err = settings_load_subtree(THINGSBOARD_IOT_SETTINGS_KEY);
// 	if (err)
// 	{
// 		LOG_ERR("settings_load_subtree, error: %d", err);
// 		return err;
// 	}

// 	/* Wait up to 1 seconds for the settings API to load the device configuration stored
// 	 * to flash, if any.
// 	 */
// 	if (k_sem_take(&config_load_sem, K_SECONDS(1)) != 0)
// 	{
// 		LOG_DBG("Failed retrieveing the device configuration from flash in time");
// 	}

// 	return err;
// }

// static void save_settings(const struct thingsboard_iot_dev_settings *dev_settings, size_t settings_size)
// {
// 	LOG_INF("Storing state to flash");

// 	int err = settings_save_one(THINGSBOARD_IOT_SETTINGS_KEY "/" THINGSBOARD_IOT_SETTINGS_KEY_CONFIG, dev_settings, settings_size);

// 	if (err)
// 	{
// 		LOG_ERR("Storing state to flash failed");
// 	}
// }

static void create_uri_path(char *uri_path, size_t uri_path_size, const char *device_token, const char *path_ending)
{
	snprintf(uri_path, uri_path_size, "/api/v1/%s/%s", device_token, path_ending);
}

// static int init_device_register()
// {
// 	return read_device_token();
// }

/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/

int thingsboard_iot_init(thingsboard_iot_evt_handler_t event_handler)
{
	if (event_handler == NULL)
	{
		LOG_ERR("Event handler must be provided");
		return -EINVAL;
	}

	// int err = enable_settings();
	// if (err)
	// {
	// 	LOG_ERR("enable_settings, error: %d", err);
	// 	return err;
	// }

	// err = init_device_register();
	// if (err)
	// {
	// 	LOG_ERR("init_device_register, error: %d", err);
	// 	return err;
	// }

	// char device_token[THINGSBOARD_IOT_DEVICE_TOKEN_SIZE];
	// device_token[0] = '\0';

	int err = read_device_token();
	if (err != 0)
	{
		LOG_ERR("Could not read the device token\n");
		return -1;
	}

	// if (strlen(device_token) == 0)
	// {
	// 	LOG_DBG("Device token is empty.");
	// 	return -1;
	// }

	// LOG_DBG("Device token read as : %s", device_token);

	evt_handler = event_handler;

	device_provision_state_set(PROVISIONED);
	connection_state_set(STATE_DISCONNECTED);
	return 0;
}

int thingsboard_iot_connect(const struct thingsboard_iot_config *config)
{
	LOG_DBG("Connecting to the platform...");
	if (connection_state_verify(STATE_CONNECTING))
	{
		LOG_WRN("Thingsboard IoT connection establishment in progress");
		return -EINPROGRESS;
	}
	else if (connection_state_verify(STATE_CONNECTED))
	{
		LOG_WRN("Thingsboard IoT is already connected");
		return -EALREADY;
	}
	else if (!connection_state_verify(STATE_DISCONNECTED))
	{
		LOG_WRN("Thingsboard IoT is not in initialized and disconnected state");
		return -ENOENT;
	}

	connection_state_set(STATE_CONNECTING);

	struct thingsboard_iot_evt evt = {
		evt.type = THINGSBOARD_IOT_EVT_CONNECTING,
	};

	thingsboard_iot_notify_event(&evt);

	int err = coap_handler_resolve_dns_address(CONFIG_THINGSBOARD_IOT_HOSTNAME);
	if (err != 0)
	{
		LOG_WRN("Could not resolve the dns address for %s", CONFIG_THINGSBOARD_IOT_HOSTNAME);
		return -1;
	}

	if (coap_handler_client_init() != 0)
	{
		LOG_INF("Failed to initialize client");
		return 0;
	}

	// k_sem_give(&connection_poll_sem);

	if (!is_device_registered())
	{
		register_device();
	}

	connection_state_set(STATE_CONNECTED);

	evt.type = THINGSBOARD_IOT_EVT_CONNECTED;
	thingsboard_iot_notify_event(&evt);
	return err;
}

int thingsboard_iot_disconnect(void)
{
	if (!connection_state_verify(STATE_CONNECTED))
	{
		LOG_WRN("Thingsboard IoT is not connected");
		return -ENOTCONN;
	}

	connection_state_set(STATE_DISCONNECTING);

	// err = mqtt_disconnect(&client);
	// if (err) {
	// 	LOG_ERR("Failed to disconnect MQTT client, error: %d", err);
	// 	connection_state_set(STATE_INIT);
	// 	return err;
	// }

	connection_state_set(STATE_DISCONNECTED);

	struct thingsboard_iot_evt evt = {
		.type = THINGSBOARD_IOT_EVT_DISCONNECTED,
	};

	thingsboard_iot_notify_event(&evt);

	/* The MQTT library only propagates the MQTT_DISCONNECT event
	 * if the call to mqtt_disconnect() is successful. In that case the
	 * setting of STATE_INIT is carried out in the mqtt_evt_handler.
	 */

	return 0;
}

int thingsboard_iot_send_data(const struct thingsboard_iot_msg *const tx_data)
{
	if (!connection_state_verify(STATE_CONNECTED))
	{
		LOG_WRN("Thingsboard IoT Hub is not connected");
		return -ENOTCONN;
	}

	char uri_path[CONFIG_THINGSBOARD_IOT_COAP_URI_PATH_LEN];
	create_uri_path(uri_path, sizeof(uri_path), dev_settings.device_token,
					thingsboard_iot_message_type_name_get_lower_case(tx_data->msg_type));

	int err = 0;
	switch (tx_data->confirm_type)
	{
	case CONFIRMABLE_MESSAGE:
		err = coap_handler_client_put_confirmable_send(uri_path, tx_data->payload.ptr, tx_data->payload.size);
		break;

	case NON_CONFIRMABLE_MESSAGE:
		err = coap_handler_client_put_non_confirmable_send(uri_path, tx_data->payload.ptr, tx_data->payload.size);
		break;

	default:
		break;
	}

	return err;
}

// K_THREAD_DEFINE(connection_poll_thread, CONFIG_THINGSBOARD_IOT_STACK_SIZE,
// 				thingsboard_iot_run, NULL, NULL, NULL,
// 				K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
