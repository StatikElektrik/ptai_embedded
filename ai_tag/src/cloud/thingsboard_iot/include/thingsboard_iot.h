/// NOT FINISHED

/**
* @brief 
*
* @file 
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

// Borda Libraries

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/

// typedef enum
// {
//     TEMP_HUM_ERROR_NONE,
//     TEMP_HUM_ERROR_GENERAL,
//     TEMP_HUM_ERROR_BUFFER
// } temp_hum_error_t;

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
	THINGSBOARD_IOT_ERROR
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
	/** Type of event. */
	enum thingsboard_iot_evt_type type;
    int dummy_data;
};

/** @brief Structure for Azure IoT Hub connection parameters. */
struct thingsboard_iot_config {
	/** Hostname to IoT Hub to connect to.
	 *  If the buffer size is zero, the device ID provided by
	 *  @kconfig{CONFIG_AZURE_IOT_HUB_HOSTNAME} is used. If DPS is enabled and `use_dps` is
	 *  set to true, the provided hostname is ignored.
	 */
	struct thingsboard_iot_buf hostname;
	/** Device id for the Azure IoT Hub connection.
	 *  If the buffer size is zero, the device ID provided by Kconfig is used.
	 */
	struct thingsboard_iot_buf device_id;

	/** Use DPS to obtain hostname and device ID if true.
	 *  Using DPS requires that @kconfig{CONFIG_AZURE_IOT_HUB_DPS} is enabled and DPS
	 *  configured accordingly.
	 *  If a hostname and device ID have already been obtained previously, the stored values
	 *  will be used. To re-run DPS, the DPS information must be reset first.
	 *  Note that using this option will use the device ID as DPS registration ID and the
	 *  ID cope from @kconfig{CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE}.
	 *  For more fine-grained control over DPS, use the azure_iot_hub_dps APIs directly insted.
	 */
	bool use_dps;
};

typedef void (*thingsboard_iot_evt_handler_t)(struct thingsboard_iot_evt *evt);

/***********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/

int thingsboard_iot_init(thingsboard_iot_evt_handler_t event_handler);

int thingsboard_iot_connect(const struct thingsboard_iot_config *config);

int thingsboard_iot_disconnect(void);

int thingsboard_iot_send_data(const struct thingsboard_iot_msg *const tx_data);


#ifdef __cplusplus
}
#endif

#endif /* SRC_CLOUD_THINGSBOARD_IOT_ */
