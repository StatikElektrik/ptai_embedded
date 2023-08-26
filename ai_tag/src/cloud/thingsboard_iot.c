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
#include <net/azure_iot_hub.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(azure_iot_hub, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);
/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/


/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
enum connection_state {
	/* The library is uninitialized. */
	STATE_IDLE,
	/* The library is initialized, no connection established. */
	STATE_INIT,
	/* Connecting to Azure IoT Hub. */
	STATE_CONNECTING,
	/* Connected to Azure IoT Hub. */
	STATE_CONNECTED,
	/* Disconnecting from Azure IoT Hub. */
	STATE_DISCONNECTING
};

enum device_provision_state {
    PROVISIONED,
    NOT_PROVISIONED
};

enum dns_state {
    RESOLVED,
    NOT_RESOLVED
};


/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/
static enum connection_state connection_state = STATE_IDLE;

/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/
static void connection_state_set(enum connection_state new_state)
{
	bool notify_error = false;

	/* Check for legal state transitions. */
	switch (connection_state) {
	case STATE_IDLE:
		if (new_state != STATE_INIT) {
			notify_error = true;
		}
		break;
	case STATE_INIT:
		if (new_state != STATE_CONNECTING &&
		    new_state != STATE_INIT) {
			notify_error = true;
		}
		break;
	case STATE_CONNECTING:
		if (new_state != STATE_CONNECTED &&
		    new_state != STATE_INIT) {
			notify_error = true;
		}
		break;
	case STATE_CONNECTED:
		if (new_state != STATE_DISCONNECTING &&
		    new_state != STATE_INIT) {
			notify_error = true;
		}
		break;
	case STATE_DISCONNECTING:
		if (new_state != STATE_INIT) {
			notify_error = true;
		}
		break;
	default:
		LOG_ERR("New connection state unknown");
		notify_error = true;
		break;
	}

	if (notify_error) {
		struct azure_iot_hub_evt evt = {
			.type = AZURE_IOT_HUB_EVT_ERROR,
			.data.err = -EINVAL
		};

		LOG_ERR("Invalid connection state transition, %s --> %s",
			log_strdup(state_name_get(connection_state)),
			log_strdup(state_name_get(new_state)));

		azure_iot_hub_notify_event(&evt);
		return;
	}

	connection_state = new_state;

	LOG_DBG("New connection state: %s",
		log_strdup(state_name_get(connection_state)));
}

static bool connection_state_verify(enum connection_state state)
{
	return (connection_state == state);
}

static void request_provision(const char* device_name, const char* device_provision_key, const char* device_provision_secret);

static void send_attribute_data(const char* device_token, const char* data, size_t data_len);

static void send_telemetry_data(const char* device_token, const char* data, size_t data_len);

/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/

