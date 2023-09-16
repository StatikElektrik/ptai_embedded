/// NOT FINISHED

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <hw_id.h>
#include "thingsboard_iot.h"

#include "cloud/cloud_wrapper.h"

#define MODULE thingsboard_iot_integration

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define CLIENT_ID_LEN HW_ID_LEN
#else
#define CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif

static char client_id_buf[CLIENT_ID_LEN + 1];
static struct thingsboard_iot_config config;

static cloud_wrap_evt_handler_t wrapper_evt_handler;

static void cloud_wrapper_notify_event(const struct cloud_wrap_event *evt)
{
	if ((wrapper_evt_handler != NULL) && (evt != NULL)) {
		wrapper_evt_handler(evt);
	} else {
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

/*
 * @brief Function that handles incoming data from the Azure IoT Hub.
 *	  This function notifies the cloud module with the appropriate event based on the
 *	  contents of the included topic and property bags.
 *
 * @param[in] event  Pointer to Azure IoT Hub event.
 */
static void incoming_message_handle(struct thingsboard_iot_evt *event)
{

}

static void thingsboard_iot_event_handler(struct thingsboard_iot_evt *const evt)
{
	struct cloud_wrap_event cloud_wrap_evt = { 0 };
	bool notify = false;

	switch (evt->type) {
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
		incoming_message_handle((struct thingsboard_iot_evt *)evt);
		break;
	case THINGSBOARD_IOT_ERROR:
		LOG_DBG("THINGSBOARD_IOT_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		// cloud_wrap_evt.err = evt->data.err;
		notify = true;
		break;
	default:
		LOG_ERR("Unknown Thingsboard IoT event type: %d", evt->type);
		break;
	}

	if (notify) {
		cloud_wrapper_notify_event(&cloud_wrap_evt);
	}
}

//////////////////////////////////////////////////////////////////////////////

int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler)
{
	int err;

	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));

	if (err) {
		LOG_ERR("Failed to retrieve device ID");
		return err;
	}

	snprintk(client_id_buf, sizeof(client_id_buf), "%s", hw_id_buf);

	config.device_id.ptr = client_id_buf;
	config.device_id.size = strlen(client_id_buf);

	err = thingsboard_iot_init(thingsboard_iot_event_handler);
	if (err) {
		LOG_ERR("thingsboard_iot_init, error: %d", err);
		return err;
	}

	LOG_DBG("********************************************");
	LOG_DBG(" The Asset Tracker v2 has started");
	LOG_DBG(" Version:      %s", CONFIG_ASSET_TRACKER_V2_APP_VERSION);
	LOG_DBG(" Client ID:    %s", client_id_buf);
	LOG_DBG(" Cloud:        %s", "Thingsboard IoT");
	LOG_DBG("********************************************");

	wrapper_evt_handler = event_handler;

	return 0;
}

int cloud_wrap_connect(void)
{
	int err;

	err = thingsboard_iot_connect(&config);
	if (err) {
		LOG_ERR("thingsboard_iot_connect, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_disconnect(void)
{
	int err;

	err = thingsboard_iot_disconnect();
	if (err) {
		LOG_ERR("thingsboard_iot_disconnect, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_state_get(bool ack, uint32_t id)
{
	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = REQUEST_DEVICE_TWIN_STRING,
	// 	.payload.size = sizeof(REQUEST_DEVICE_TWIN_STRING) - 1,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_state_send(char *buf, size_t len, bool ack, uint32_t id)
{
	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = buf,
	// 	.payload.size = len,
	// 	.message_id = id,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id,
			 const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	int err;
	struct thingsboard_iot_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.msg_type = TELEMETRY,
		.confirm_type = NON_CONFIRMABLE_MESSAGE
	};

	LOG_DBG("cloud_wrap_data_send - payload %s, size %u",msg.payload.ptr, msg.payload.size);

	err = thingsboard_iot_send_data(&msg);
	if (err) {
		LOG_ERR("thingsboard_iot_send_data, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id)
{
	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = buf,
	// 	.payload.size = len,
	// 	.message_id = id,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
	// 	.topic.properties = prop_bag_batch,
	// 	.topic.property_count = ARRAY_SIZE(prop_bag_batch)
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id,
		       const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = buf,
	// 	.payload.size = len,
	// 	.message_id = id,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
	// 	.topic.properties = prop_bag_message,
	// 	.topic.property_count = ARRAY_SIZE(prop_bag_message)
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_cloud_location_send(char *buf, size_t len, bool ack, uint32_t id)
{
	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = buf,
	// 	.payload.size = len,
	// 	.message_id = id,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
	// 	.topic.properties = prop_bag_ground_fix,
	// 	.topic.property_count = ARRAY_SIZE(prop_bag_ground_fix)
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_agps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = buf,
	// 	.payload.size = len,
	// 	.message_id = id,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
	// 	.topic.properties = prop_bag_agps,
	// 	.topic.property_count = ARRAY_SIZE(prop_bag_agps)
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	// int err;
	// struct thingsboard_iot_msg msg = {
	// 	.payload.ptr = buf,
	// 	.payload.size = len,
	// 	.message_id = id,
	// 	.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
	// 	.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
	// 	.topic.properties = prop_bag_pgps,
	// 	.topic.property_count = ARRAY_SIZE(prop_bag_pgps)
	// };

	// err = thingsboard_iot_send_data(&msg);
	// if (err) {
	// 	LOG_ERR("thingsboard_iot_send_data, error: %d", err);
	// 	return err;
	// }

	return 0;
}

int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	/* Not supported */
	return -ENOTSUP;
}
