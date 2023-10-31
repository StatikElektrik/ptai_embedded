/**
* @brief Interface to use Thingsboard IoT Device Management Platform 
*
* @file thingsboard_iot.h
*/

#ifndef SRC_CLOUD_THINGSBOARD_IOT_
#define SRC_CLOUD_THINGSBOARD_IOT_

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/

//Standard Libraries
#include <stdint.h>
#include <stdbool.h>

//Third-party Libraries

//PTAI Libraries

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
struct thingsboard_iot_buf {
	char *ptr;
	size_t size;
};


enum thingsboard_iot_evt_type {
	THINGSBOARD_IOT_EVT_CONNECTING = 0x1,
	THINGSBOARD_IOT_EVT_CONNECTED,
	THINGSBOARD_IOT_EVT_DISCONNECTED,
	THINGSBOARD_IOT_EVT_CONNECTION_FAILED,
	THINGSBOARD_IOT_EVT_DATA_RECEIVED,
	THINGSBOARD_IOT_EVT_ERROR
};

enum thingsboard_iot_confirm_type {
    CONFIRMABLE_MESSAGE,
    NON_CONFIRMABLE_MESSAGE
};

enum thingsboard_iot_message_type {
    TELEMETRY,
    ATTRIBUTE
};

struct thingsboard_iot_msg {
	struct thingsboard_iot_buf payload;
	enum thingsboard_iot_message_type msg_type;
    enum thingsboard_iot_confirm_type confirm_type;
};

struct thingsboard_iot_evt {
	enum thingsboard_iot_evt_type type;
    union {
		int err;
	} data;
};

struct thingsboard_iot_config {
	struct thingsboard_iot_buf hostname;
	struct thingsboard_iot_buf device_id;
};

/** @brief Thingsboard IoT library event handler.
 *
 *  @param p_evt Pointer to event structure.
 */
typedef void (*thingsboard_iot_evt_handler_t)(struct thingsboard_iot_evt *p_evt);

/***********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/

/**
 * @brief Initialize the ThingsBoard IoT library.
 *
 * This function initializes the library and sets up the event handler. It also reads the device token
 * and initializes the device provision and connection states.
 *
 * @param event_handler The event handler function to handle ThingsBoard IoT events.
 * @return 0 on success, a negative error code on failure.
 */
int thingsboard_iot_init(thingsboard_iot_evt_handler_t event_handler);

/**
 * @brief Connect to the ThingsBoard IoT platform.
 *
 * This function initiates a connection to the ThingsBoard IoT platform using the provided configuration.
 *
 * @param p_config Pointer to the configuration for connecting to the platform.
 * @return 0 on success, a negative error code on failure.
 */
int thingsboard_iot_connect(const struct thingsboard_iot_config *p_config);

/**
 * @brief Disconnect from the ThingsBoard IoT platform.
 *
 * This function disconnects from the ThingsBoard IoT platform if connected.
 * 
 * @warning Not implemented yet.
 *
 * @return 0 on success, a negative error code on failure.
 */
int thingsboard_iot_disconnect(void);

/**
 * @brief Send data to the ThingsBoard IoT platform.
 *
 * This function sends data to the ThingsBoard IoT platform based on the provided message type
 * and confirmation type.
 *
 * @param p_tx_data Pointer to the data to be sent.
 * @return 0 on success, a negative error code on failure.
 */
int thingsboard_iot_send_data(const struct thingsboard_iot_msg *p_tx_data);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CLOUD_THINGSBOARD_IOT_ */
