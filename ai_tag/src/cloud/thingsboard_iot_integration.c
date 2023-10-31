/**
 * @brief Cloud wrapper integration for thingsboard iot.
 *
 * @file thingsboard_iot_integration.c
 *
 */

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/

// Standard Libraries

// Third-party Libraries
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hw_id.h>
#include "cloud/cloud_wrapper.h"
 
// PTAI Libraries
#include "thingsboard_iot.h"

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/
#define MODULE thingsboard_iot_integration
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define CLIENT_ID_LEN HW_ID_LEN
#else
#define CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif
/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/
static char g_client_id_buf[CLIENT_ID_LEN + 1];
static struct thingsboard_iot_config g_config;

static cloud_wrap_evt_handler_t g_wrapper_evt_handler;
/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/

/**
 * @brief Notify an event through the ThingsBoard IoT integration event handler.
 *
 * This function notifies an event through the registered event handler.
 *
 * @param evt Pointer to the cloud wrapper event to notify.
 */
static void thingsboard_iot_integration_cloud_wrapper_notify_event(const struct cloud_wrap_event *evt)
{
	if ((g_wrapper_evt_handler != NULL) && (evt != NULL))
	{
		g_wrapper_evt_handler(evt);
	}
	else
	{
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

/**
 * @brief Handle incoming ThingsBoard IoT messages.
 *
 * @warning This function is currently not supported and does nothing.
 *
 * @param p_event Pointer to the ThingsBoard IoT event to handle.
 */
static void thingsboard_iot_integration_incoming_message_handle(struct thingsboard_iot_evt *p_event)
{
	(void)p_event;
	/* Not supported */
}

/**
 * @brief Handle ThingsBoard IoT events.
 *
 * This function handles ThingsBoard IoT events and converts them into cloud wrapper events
 * for further processing. It maps specific event types to cloud wrapper event types and
 * notifies the cloud wrapper through the registered event handler.
 *
 * @param evt Pointer to the ThingsBoard IoT event to handle.
 */
static void thingsboard_iot_event_handler(struct thingsboard_iot_evt *const evt)
{
	struct cloud_wrap_event cloud_wrap_evt = {0};
	bool notify = false;

	switch (evt->type)
	{
	case THINGSBOARD_IOT_EVT_CONNECTING:
		LOG_DBG("THINGSBOARD_IOT_EVT_CONNECTING");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTING;
		notify = true;
		break;
	case THINGSBOARD_IOT_EVT_CONNECTED:
		LOG_DBG("THINGSBOARD_IOT_EVT_CONNECTED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTED;
		notify = true;
		break;
	case THINGSBOARD_IOT_EVT_CONNECTION_FAILED:
		LOG_DBG("THINGSBOARD_IOT_EVT_CONNECTION_FAILED");
		break;
	case THINGSBOARD_IOT_EVT_DISCONNECTED:
		LOG_DBG("THINGSBOARD_IOT_EVT_DISCONNECTED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		break;
	case THINGSBOARD_IOT_EVT_DATA_RECEIVED:
		LOG_DBG("THINGSBOARD_IOT_EVT_DATA_RECEIVED");
		thingsboard_iot_integration_incoming_message_handle((struct thingsboard_iot_evt *)evt);
		break;
	case THINGSBOARD_IOT_EVT_ERROR:
		LOG_DBG("THINGSBOARD_IOT_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		cloud_wrap_evt.err = evt->data.err;
		notify = true;
		break;
	default:
		LOG_ERR("Unknown Thingsboard IoT event type: %d", evt->type);
		break;
	}

	if (notify)
	{
		thingsboard_iot_integration_cloud_wrapper_notify_event(&cloud_wrap_evt);
	}
}
/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/
int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler)
{
	char hw_id_buf[HW_ID_LEN];

	int err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));

	if (err)
	{
		LOG_ERR("Failed to retrieve device ID");
		return err;
	}

	snprintk(g_client_id_buf, sizeof(g_client_id_buf), "%s", hw_id_buf);

	g_config.device_id.ptr = g_client_id_buf;
	g_config.device_id.size = strlen(g_client_id_buf);

	err = thingsboard_iot_init(thingsboard_iot_event_handler);
	if (err)
	{
		LOG_ERR("thingsboard_iot_init, error: %d", err);
		return err;
	}

	LOG_DBG("********************************************");
	LOG_DBG(" The Asset Tracker v2 has started");
	LOG_DBG(" Version:      %s", CONFIG_ASSET_TRACKER_V2_APP_VERSION);
	LOG_DBG(" Client ID:    %s", g_client_id_buf);
	LOG_DBG(" Cloud:        %s", "Thingsboard IoT");
	LOG_DBG("********************************************");

	g_wrapper_evt_handler = event_handler;

	return 0;
}

int cloud_wrap_connect(void)
{
	int err;

	err = thingsboard_iot_connect(&g_config);
	if (err)
	{
		LOG_ERR("Error occured while connecting to the Thingsboard IoT platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_disconnect(void)
{
	int err;

	err = thingsboard_iot_disconnect();
	if (err)
	{
		LOG_ERR("Error occured while disconnecting from Thingsboard IoT platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_state_get(bool ack, uint32_t id)
{
	ARG_UNUSED(ack);
	ARG_UNUSED(id);

	/* Not supported */
	return -ENOTSUP;
}

int cloud_wrap_state_send(char *buf, size_t len, bool ack, uint32_t id)
{
	struct thingsboard_iot_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.msg_type = TELEMETRY,
		.confirm_type = NON_CONFIRMABLE_MESSAGE};

	LOG_DBG("cloud_wrap_state_send - payload %s, size %u", msg.payload.ptr, msg.payload.size);

	int err = thingsboard_iot_send_data(&msg);
	if (err)
	{
		LOG_ERR("Error occured while sending state data to the Thingsboard IoT Platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id,
						 const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	struct thingsboard_iot_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.msg_type = TELEMETRY,
		.confirm_type = NON_CONFIRMABLE_MESSAGE};

	LOG_DBG("cloud_wrap_data_send - payload %s, size %u", msg.payload.ptr, msg.payload.size);

	int err = thingsboard_iot_send_data(&msg);
	if (err)
	{
		LOG_ERR("Error occured while sending data to the Thingsboard IoT Platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id)
{
	struct thingsboard_iot_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.msg_type = TELEMETRY,
		.confirm_type = NON_CONFIRMABLE_MESSAGE};

	LOG_DBG("cloud_wrap_batch_send - payload %s, size %u", msg.payload.ptr, msg.payload.size);

	int err = thingsboard_iot_send_data(&msg);
	if (err)
	{
		LOG_ERR("Error occured while sending batch data to the Thingsboard IoT Platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id,
					   const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	struct thingsboard_iot_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.msg_type = TELEMETRY,
		.confirm_type = NON_CONFIRMABLE_MESSAGE};

	LOG_DBG("cloud_wrap_ui_send - payload %s, size %u", msg.payload.ptr, msg.payload.size);

	int err = thingsboard_iot_send_data(&msg);
	if (err)
	{
		LOG_ERR("Error occured while sending UI data to the Thingsboard IoT Platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_cloud_location_send(char *buf, size_t len, bool ack, uint32_t id)
{
	struct thingsboard_iot_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.msg_type = TELEMETRY,
		.confirm_type = NON_CONFIRMABLE_MESSAGE};

	LOG_DBG("cloud_wrap_cloud_location_send - payload %s, size %u", msg.payload.ptr, msg.payload.size);

	int err = thingsboard_iot_send_data(&msg);
	if (err)
	{
		LOG_ERR("Error occured while sending location data to the Thingsboard IoT Platform, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_agps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(ack);
	ARG_UNUSED(id);

	/* Not supported */
	return -ENOTSUP;
}

int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(ack);
	ARG_UNUSED(id);

	/* Not supported */
	return -ENOTSUP;
}

int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(ack);
	ARG_UNUSED(id);

	/* Not supported */
	return -ENOTSUP;
}
