#ifndef _AI_MODULE_EVENT_H_
#define _AI_MODULE_EVENT_H_

/**
 * @brief AI module event
 * @defgroup ai_module_event ai module event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Sensor event types submitted by the Sensor module. */
    enum ai_module_event_type
    {
        AI_MODULE_EVT_ANALYSIS_RESULT_READY,
        AI_MODULE_EVT_ANAYLSIS_NOT_SUPPORTED,
        AI_MODULE_EVT_SHUTDOWN_READY,
        AI_MODULE_EVT_ERROR
    };

    /** @brief Structure used to provide acceleration data. */
    struct ai_module_analysis_data
    {
        /** Uptime when the data was sampled. */
        int64_t timestamp;
        double normal_mode;
        double prs_red_intake_manifold;
        double comp_rat_red_cylinder;
        double fuel_inject_red_cylinder;
    };

    /** @brief Sensor module event. */
    struct ai_module_event
    {
        struct app_event_header header;
        enum ai_module_event_type type;

        union
        {
            struct ai_module_analysis_data results;
            uint32_t id;
            int err;
        } data;
    };

    APP_EVENT_TYPE_DECLARE(ai_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _AI_MODULE_EVENT_H_ */
