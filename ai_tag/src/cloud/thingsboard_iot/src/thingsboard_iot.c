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
LOG_MODULE_REGISTER(MODULE, CONFIG_THINGSBOARD_IOT_LOG_LEVEL);

#if defined(CONFIG_THINGSBOARD_IOT_DEVICE_TOKEN)
#define THINGSBOARD_IOT_DEVICE_TOKEN_MAX_LEN 21
#define THINGSBOARD_IOT_IS_DEVICE_TOKEN_DEFINED true
#define THINGSBOARD_IOT_DEVICE_TOKEN CONFIG_THINGSBOARD_IOT_DEVICE_TOKEN
#else
#define THINGSBOARD_IOT_DEVICE_TOKEN_MAX_LEN CONFIG_THINGSBOARD_IOT_DEVICE_TOKEN_SIZE
#define THINGSBOARD_IOT_IS_DEVICE_TOKEN_DEFINED false
#endif

#define THINGSBOARD_IOT_COMMON_URI "/api/v1"
#define THINGSBOARD_IOT_PROVISION_REQUEST_URI THINGSBOARD_IOT_COMMON_URI "/provision"

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
enum connection_state
{
	STATE_UNINIT,
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
	STATE_COUNT
};

enum device_provision_state
{
	PROVISIONED,
	NOT_PROVISIONED
};

struct thingsboard_iot_settings
{
	char device_token[THINGSBOARD_IOT_DEVICE_TOKEN_MAX_LEN];
};

/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/

static enum connection_state g_connection_state = STATE_UNINIT;
static enum device_provision_state g_provision_state = NOT_PROVISIONED;

static thingsboard_iot_evt_handler_t g_evt_handler;
static struct thingsboard_iot_settings g_dev_settings;

/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/

/**
 * @brief Notify an event through the event caller configured in the initialize function.
 *        This event caller will ensure that whoever is listening to the module will be
 *        notified of the event.
 *
 * @param evt Pointer to the event structure.
 */
static void thingsboard_iot_notify_event(struct thingsboard_iot_evt *p_evt)
{
	if (g_evt_handler)
	{
		g_evt_handler(p_evt);
	}
}

/* State name related functions. */

/**
 * @brief Get the name associated with a device provision state.
 *
 * @param state The device provision state.
 * @return The name of the state as a string.
 */
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

/**
 * @brief Get the name associated with a connection state.
 *
 * @param state The connection state.
 * @return The name of the state as a string.
 */
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

/**
 * @brief Get the lower case name associated with a ThingsBoard IoT message type.
 *
 * @param msg_type The message type.
 * @return The lower case name of the message type as a string.
 */
static const char *thingsboard_iot_message_type_name_get_lower_case(enum thingsboard_iot_message_type msg_type)
{
	switch (msg_type)
	{
	case TELEMETRY:
		return "telemetry";
	case ATTRIBUTE:
		return "attribute";
	default:
		LOG_ERR("Unkown message type.");
		return "";
	}
}

/* State functions. */

/**
 * @brief Set the new connection state and perform state transition checks.
 *
 * @param new_state The new connection state.
 */
static void thingsboard_iot_connection_state_set(enum connection_state new_state)
{
	bool notify_error = false;

	if (g_connection_state == new_state)
	{
		LOG_DBG("Skipping transition to the same state (%s)",
				connection_state_name_get(g_connection_state));
		return;
	}

	/* Check for legal state transitions. */
	switch (g_connection_state)
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

	if (notify_error)
	{
		struct thingsboard_iot_evt evt = {
			.type = THINGSBOARD_IOT_EVT_ERROR,
			.data.err = -EINVAL};

		LOG_ERR("Invalid connection state transition, %s --> %s",
				connection_state_name_get(g_connection_state),
				connection_state_name_get(new_state));

		thingsboard_iot_notify_event(&evt);
		return;
	}

	LOG_DBG("Connection state transition: %s --> %s",
			connection_state_name_get(g_connection_state),
			connection_state_name_get(new_state));

	g_connection_state = new_state;
}

/**
 * @brief Set the device provision state.
 *
 * @param new_state The new device provision state.
 */
static void thingsboard_iot_device_provision_state_set(enum device_provision_state new_state)
{
	LOG_DBG("Device provision state transition: %s --> %s",
			device_provision_state_name_get(g_provision_state),
			device_provision_state_name_get(new_state));
	g_provision_state = new_state;
}

/**
 * @brief Check if the device is provisioned.
 *
 * @return true if the device is provisioned, false otherwise.
 */
static bool thingsboard_is_device_provisioned()
{
	return (g_provision_state == PROVISIONED) ? true : false;
}

/**
 * @brief Verify the current connection state.
 *
 * @param state The connection state to verify.
 * @return true if the current connection state matches the given state, false otherwise.
 */
static bool thingsboard_iot_connection_state_verify(enum connection_state state)
{
	return (g_connection_state == state);
}

/**
 * @brief Request device provisioning.
 *
 * @param[in] p_device_name The device name.
 * @param[in] p_device_provision_key The device provisioning key.
 * @param[in] p_device_provision_secret The device provisioning secret.
 * @param[out] p_device_token Pointer to store the device token.
 * @return 0 on success, else failure.
 */
static int thingsboard_iot_request_provision(const char *p_device_name, const char *p_device_provision_key, const char *p_device_provision_secret, char *p_device_token)
{
	if (thingsboard_is_device_provisioned())
	{
		return 0;
	}

	struct cloud_data_device_provision_request cloud_data;

	strncpy(cloud_data.device_name, p_device_name, CONFIG_THINGSBOARD_IOT_DEVICE_ID_MAX_LEN);
	strncpy(cloud_data.device_provision_key, p_device_provision_key, CONFIG_THINGSBOARD_IOT_DEVICE_PROVISION_KEY_MAX_LEN);
	strncpy(cloud_data.device_provision_secret, p_device_provision_secret, CONFIG_THINGSBOARD_IOT_DEVICE_PROVISION_SECRET_MAX_LEN);

	struct cloud_codec_data codec = {0};
	int err = cloud_codec_encode_device_provision_request_data(&codec, &cloud_data);
	switch (err)
	{
	case 0:
		LOG_DBG("Provision request data data encoded successfully");
		break;
	case -ENOTSUP:
		LOG_ERR("Encoding provision request data is not supported: %d", err);
		return err;
		break;
	case -ENODATA:
		LOG_DBG("No provision reques data to encode, error: %d", err);
		return err;
		break;
	default:
		LOG_ERR("Error encoding provision request data location data: %d", err);
		return err;
	}

	err = coap_handler_client_post_confirmable_send(THINGSBOARD_IOT_PROVISION_REQUEST_URI, codec.buf, codec.len);
	if (err < 0)
	{
		LOG_ERR("Connect failed : %d\n", err);
		return -1;
	}

	// @todo Here a function will wait the response from the CoAp that returns the
	// device_token. And will fill the device_token.
	(void)p_device_token;

	return 0;
}

/**
 * @brief Read the device token from configuration.
 *
 * @return 0 on success, else failure.
 */
static int thingsboard_iot_read_device_token()
{
	if (THINGSBOARD_IOT_IS_DEVICE_TOKEN_DEFINED)
	{
		strncpy(g_dev_settings.device_token, THINGSBOARD_IOT_DEVICE_TOKEN, THINGSBOARD_IOT_DEVICE_TOKEN_MAX_LEN);
	}
	else
	{
		// @todo If configuration is not defined, read it from the saved settings such as flash.
	}
	return 0;
}

/**
 * @brief Create a URI path based on device token and path ending.
 *
 * @param[out] uri_path Pointer to store the created URI path.
 * @param[in] uri_path_size Size of the uri_path buffer.
 * @param[in] device_token The device token.
 * @param[in] path_ending The path ending.
 */
static void thingsboard_iot_create_uri_path(char *p_uri_path, size_t uri_path_size, const char *p_device_token, const char *p_path_ending)
{
	snprintf(p_uri_path, uri_path_size, THINGSBOARD_IOT_COMMON_URI "/%s/%s", p_device_token, p_path_ending);
}

/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/

int thingsboard_iot_init(thingsboard_iot_evt_handler_t event_handler)
{
	if (NULL == event_handler)
	{
		LOG_ERR("Event handler must be provided.");
		return -EINVAL;
	}

	int err = thingsboard_iot_read_device_token();
	if (err)
	{
		LOG_ERR("Could not read the device token\n");
		return -1;
	}

	g_evt_handler = event_handler;

	thingsboard_iot_device_provision_state_set(PROVISIONED);
	thingsboard_iot_connection_state_set(STATE_DISCONNECTED);
	return 0;
}

int thingsboard_iot_connect(const struct thingsboard_iot_config *p_config)
{
	(void)p_config;

	LOG_DBG("Connecting to the platform...");
	if (thingsboard_iot_connection_state_verify(STATE_CONNECTING))
	{
		LOG_WRN("Thingsboard IoT connection establishment is already in progress.");
		return -EINPROGRESS;
	}
	else if (thingsboard_iot_connection_state_verify(STATE_CONNECTED))
	{
		LOG_WRN("Thingsboard IoT is already connected.");
		return -EALREADY;
	}
	else if (!thingsboard_iot_connection_state_verify(STATE_DISCONNECTED))
	{
		LOG_WRN("Thingsboard IoT is not in initialized and disconnected state.");
		return -ENOENT;
	}

	thingsboard_iot_connection_state_set(STATE_CONNECTING);

	struct thingsboard_iot_evt evt = {
		evt.type = THINGSBOARD_IOT_EVT_CONNECTING,
	};

	thingsboard_iot_notify_event(&evt);

	int err = coap_handler_client_connect(CONFIG_THINGSBOARD_IOT_HOSTNAME);
	if (err)
	{
		LOG_ERR("Failed to initialize the coap client, error %d", err);
		thingsboard_iot_connection_state_set(STATE_DISCONNECTED);
		return -ENOTCONN;
	}

	err = thingsboard_iot_request_provision(p_config->device_id.ptr, CONFIG_THINGSBOARD_IOT_DEVICE_PROVISION_KEY, CONFIG_THINGSBOARD_IOT_DEVICE_PROVISION_SECRET, g_dev_settings.device_token);
	if (err)
	{
		LOG_ERR("Failed to register the device to the thingsboard. Err : %d\n", err);
		thingsboard_iot_connection_state_set(STATE_DISCONNECTED);
		return -1;
	}

	thingsboard_iot_connection_state_set(STATE_CONNECTED);

	evt.type = THINGSBOARD_IOT_EVT_CONNECTED;
	thingsboard_iot_notify_event(&evt);
	return 0;
}

int thingsboard_iot_disconnect(void)
{
	if (!thingsboard_iot_connection_state_verify(STATE_CONNECTED))
	{
		LOG_WRN("Thingsboard IoT is not connected");
		return -ENOTCONN;
	}

	thingsboard_iot_connection_state_set(STATE_DISCONNECTING);

	int err = coap_handler_client_disconnect();
	if (err)
	{
		LOG_ERR("Failed to disconnect CoAP, error: %d", err);
		thingsboard_iot_connection_state_set(STATE_UNINIT);
		return err;
	}

	thingsboard_iot_connection_state_set(STATE_DISCONNECTED);

	struct thingsboard_iot_evt evt = {
		.type = THINGSBOARD_IOT_EVT_DISCONNECTED,
	};

	thingsboard_iot_notify_event(&evt);

	return 0;
}

int thingsboard_iot_send_data(const struct thingsboard_iot_msg *const p_tx_data)
{
	if (!thingsboard_iot_connection_state_verify(STATE_CONNECTED))
	{
		LOG_WRN("Thingsboard IoT Hub is not connected to send data.");
		return -ENOTCONN;
	}

	char uri_path[CONFIG_THINGSBOARD_IOT_COAP_URI_PATH_MAX_LEN];
	thingsboard_iot_create_uri_path(uri_path, sizeof(uri_path), g_dev_settings.device_token,
									thingsboard_iot_message_type_name_get_lower_case(p_tx_data->msg_type));

	int err = 0;
	switch (p_tx_data->confirm_type)
	{
	case CONFIRMABLE_MESSAGE:
		err = coap_handler_client_post_confirmable_send(uri_path, p_tx_data->payload.ptr, p_tx_data->payload.size);
		break;

	case NON_CONFIRMABLE_MESSAGE:
		err = coap_handler_client_post_non_confirmable_send(uri_path, p_tx_data->payload.ptr, p_tx_data->payload.size);
		break;

	default:
		LOG_ERR("Unknown confirm type.");
		err = -ENOTSUP;
		break;
	}

	return err;
}