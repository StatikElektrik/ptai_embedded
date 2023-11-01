/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ai_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum ai_module_event_type type)
{
    switch (type)
    {
    case AI_MODULE_EVT_ANALYSIS_RESULT_READY:
        return "AI_MODULE_EVT_ANALYSIS_RESULT_READY";
    case AI_MODULE_EVT_ANAYLSIS_NOT_SUPPORTED:
        return "AI_MODULE_EVT_ANAYLSIS_NOT_SUPPORTED";
    case AI_MODULE_EVT_SHUTDOWN_READY:
        return "AI_MODULE_EVT_SHUTDOWN_READY";
    case AI_MODULE_EVT_ERROR:
        return "AI_MODULE_EVT_ERROR";
    default:
        return "Unknown event";
    }
}

static void log_event(const struct app_event_header *aeh)
{
    const struct ai_module_event *event = cast_ai_module_event(aeh);

    if (event->type == AI_MODULE_EVT_ERROR)
    {
        APP_EVENT_MANAGER_LOG(aeh, "%s - Error code %d",
                              get_evt_type_str(event->type), event->data.err);
    }
    else
    {
        APP_EVENT_MANAGER_LOG(aeh, "%s", get_evt_type_str(event->type));
    }
}

#if defined(CONFIG_NRF_PROFILER)

static void profile_event(struct log_event_buf *buf,
                          const struct app_event_header *aeh)
{
    const struct ai_module_event *event = cast_ai_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
    nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
    nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(ai_module_event,
                             profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(ai_module_event,
                             log_event,
                             &ai_module_event_info,
                             APP_EVENT_FLAGS_CREATE(
                                 IF_ENABLED(CONFIG_AI_EVENTS_LOG,
                                            (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
