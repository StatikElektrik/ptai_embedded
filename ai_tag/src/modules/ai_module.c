#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/sensor.h>
#include <app_event_manager.h>

#if defined(CONFIG_EXTERNAL_SENSORS)
#include "ext_sensors.h"
#endif

#if defined(CONFIG_ADP536X)
#include <adp536x.h>
#endif

#define MODULE ai_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/ai_module_event.h"
#include "events/util_module_event.h"

#include "ai/ai_analyzer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_AI_MODULE_LOG_LEVEL);

struct ai_msg_data
{
	union
	{
		struct app_module_event app;
		struct data_module_event data;
		struct util_module_event util;
	} module;
};

/* AI module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN
} state;

/* Sensor module message queue. */
#define AI_QUEUE_ENTRY_COUNT 10
#define AI_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_ai, sizeof(struct ai_msg_data),
			  AI_QUEUE_ENTRY_COUNT, AI_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "ai",
	.msg_q = &msgq_ai,
	.supports_shutdown = true,
};

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type new_state)
{
	switch (new_state)
	{
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_RUNNING:
		return "STATE_RUNNING";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state)
	{
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
			state2str(state),
			state2str(new_state));

	state = new_state;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct ai_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(aeh))
	{
		struct app_module_event *event = cast_app_module_event(aeh);

		msg.module.app = *event;
		enqueue_msg = true;
	}

	if (is_data_module_event(aeh))
	{
		struct data_module_event *event = cast_data_module_event(aeh);

		msg.module.data = *event;
		enqueue_msg = true;
	}

	if (is_util_module_event(aeh))
	{
		struct util_module_event *event = cast_util_module_event(aeh);

		msg.module.util = *event;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = module_enqueue_msg(&self, &msg);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(ai, AI_MODULE_EVT_ERROR, err);
		}
	}

	return false;
}

/* Static module functions. */

#if defined(CONFIG_EXTERNAL_SENSORS)
static void ext_sensor_handler(const struct ext_sensor_evt *const evt)
{
	switch (evt->type)
	{
	case EXT_SENSOR_EVT_ACCELEROMETER_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_ACCELEROMETER_ERROR");
		break;
	case EXT_SENSOR_EVT_TEMPERATURE_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_TEMPERATURE_ERROR");
		break;
	case EXT_SENSOR_EVT_HUMIDITY_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_HUMIDITY_ERROR");
		break;
	case EXT_SENSOR_EVT_PRESSURE_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_PRESSURE_ERROR");
		break;
	case EXT_SENSOR_EVT_BME680_BSEC_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_BME680_BSEC_ERROR");
		break;
	default:
		break;
	}
}
#endif /* CONFIG_EXTERNAL_SENSORS */

// static void accelerometer_data_get(void)
// {
// 	struct ai_module_event *ai_module_event;

// #if defined(CONFIG_ADP536X)
// 	int err;
// 	uint8_t percentage;

// 	err = adp536x_fg_soc(&percentage);
// 	if (err)
// 	{
// 		LOG_ERR("Failed to get battery level: %d", err);
// 		return;
// 	}

// 	ai_module_event = new_ai_module_event();

// 	__ASSERT(ai_module_event, "Not enough heap left to allocate event");

// 	ai_module_event->data.bat.timestamp = k_uptime_get();
// 	ai_module_event->data.bat.battery_level = percentage;
// 	ai_module_event->type = SENSOR_EVT_FUEL_GAUGE_READY;
// #else
// 	ai_module_event = new_ai_module_event();

// 	__ASSERT(ai_module_event, "Not enough heap left to allocate event");

// 	ai_module_event->type = SENSOR_EVT_FUEL_GAUGE_NOT_SUPPORTED;
// #endif
// 	APP_EVENT_SUBMIT(ai_module_event);
// }

static void ai_analysis_result_data_get(void)
{
	struct ai_module_event *ai_module_event;
#if defined(CONFIG_EXTERNAL_SENSORS)
	struct ai_analysis_error_types error_types = {0};
	int err;

	// int err = ext_sensors_acc_data_get();
	// if (err)
	// {
	// 	LOG_ERR("ext_sensors_acc_data_get, error: %d", err);
	// }

	// err = ai_module_analyse_acc_data();
	// if (err)
	// {
	// 	LOG_ERR("ai_module_analyse_acc_data, error: %d", err);
	// }

	err = ai_module_analyse_acc_data_dummy(&error_types);
	if (err)
	{
		LOG_ERR("ai_module_analyse_acc_data_dummy, error: %d", err);
	}

	ai_module_event = new_ai_module_event();

	__ASSERT(ai_module_event, "Not enough heap left to allocate event");

	ai_module_event->data.results.timestamp = k_uptime_get();
	ai_module_event->data.results.error_type_1 = error_types.error_type_1;
	ai_module_event->data.results.error_type_2 = error_types.error_type_2;
	ai_module_event->data.results.error_type_3 = error_types.error_type_3;
	ai_module_event->data.results.error_type_4 = error_types.error_type_4;
	ai_module_event->type = AI_MODULE_EVT_ANALYSIS_RESULT_READY;
#else

	/* This event must be sent even though environmental sensors are not
	 * available on the nRF9160DK. This is because the Data module expects
	 * responses from the different modules within a certain amount of time
	 * after the APP_EVT_DATA_GET event has been emitted.
	 */
	LOG_DBG("No sensors found to analyze the data.");

	/* Set this entry to false signifying that the event carries no data.
	 * This makes sure that the entry is not stored in the circular buffer.
	 */
	ai_module_event = new_ai_module_event();

	__ASSERT(ai_module_event, "Not enough heap left to allocate event");

	ai_module_event->type = AI_MODULE_EVT_ANAYLSIS_NOT_SUPPORTED;
#endif
	APP_EVENT_SUBMIT(ai_module_event);
}

static int setup(void)
{
#if defined(CONFIG_EXTERNAL_SENSORS)
	int err;

	err = ext_sensors_init(ext_sensor_handler);
	if (err)
	{
		LOG_ERR("ext_sensors_init, error: %d", err);
		return err;
	}
#endif
	return 0;
}

static bool ai_analysis_result_data_requested(enum app_module_data_type *data_list,
											  size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		if (data_list[i] == APP_DATA_AI_RESULT)
		{
			return true;
		}
	}

	return false;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct ai_msg_data *msg)
{
	// if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
	// 	apply_config(msg);
	// 	state_set(STATE_RUNNING);
	// }
	state_set(STATE_RUNNING);
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct ai_msg_data *msg)
{
	// if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
	// 	apply_config(msg);
	// }

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET))
	{
		// if (environmental_data_requested(msg->module.app.data_list,
		// 				 msg->module.app.count)) {
		// 	environmental_data_get();
		// }

		// if (battery_data_requested(msg->module.app.data_list, msg->module.app.count)) {
		// 	battery_data_get();
		// }

		if (ai_analysis_result_data_requested(msg->module.app.data_list, msg->module.app.count))
		{
			ai_analysis_result_data_get();
		}
	}
}

/* Message handler for all states. */
static void on_all_states(struct ai_msg_data *msg)
{
	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST))
	{
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_SHUTDOWN_ACK(ai, AI_MODULE_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}
}

static void module_thread_fn(void)
{
	int err;
	struct ai_msg_data msg = {0};

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err)
	{
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(ai, AI_MODULE_EVT_ERROR, err);
	}

	state_set(STATE_INIT);

	err = setup();
	if (err)
	{
		LOG_ERR("setup, error: %d", err);
		SEND_ERROR(ai, AI_MODULE_EVT_ERROR, err);
	}

	while (true)
	{
		module_get_next_msg(&self, &msg);

		switch (state)
		{
		case STATE_INIT:
			on_state_init(&msg);
			break;
		case STATE_RUNNING:
			on_state_running(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_ERR("Unknown state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(ai_module_thread, CONFIG_AI_THREAD_STACK_SIZE,
				module_thread_fn, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE(MODULE, util_module_event);
