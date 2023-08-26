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

//Third-party Libraries
#include "sht4x.h"
#include "sensor_buf_config.h"

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

enum thingsboard_iot_hub_evt_type {
	THINGSBOARD_IOT_HUB_EVT_CONNECTING = 0x1,
	THINGSBOARD_IOT_HUB_EVT_CONNECTED,
	THINGSBOARD_IOT_HUB_EVT_CONNECTION_FAILED,
	THINGSBOARD_IOT_HUB_ERROR
};

enum thingsboard_confirm_type {
    CONFIRMABLE_MESSAGE,
    NON_CONFIRMABLE_MESSAGE
}

enum thingsboard_message_type {
    TELEMETRY,
    ATTRIBUTE
}

struct thingsboard_iot_hub_data {
	enum thingsboard_message_type type;
    enum thingsboard_confirm_type type;
	char *ptr;
	size_t len;
};

struct thingsboard_iot_hub_evt {
	/** Type of event. */
    int dummy_data;
};

typedef void (*thingsboard_iot_hub_evt_handler_t)(struct thingsboard_iot_hub_evt *evt);

/***********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/

int thingsboard_iot_hub_init(const struct thingsboard_iot_hub_config *config,
		       thingsboard_iot_hub_evt_handler_t event_handler);

int thingsboard_iot_hub_connect(void);

int thingsboard_iot_hub_disconnect(void);

int thingsboard_iot_hub_send_data(const struct thingsboard_iot_hub_data *const tx_data);


#ifdef __cplusplus
}
#endif

#endif /* SRC_CLOUD_THINGSBOARD_IOT_ */
