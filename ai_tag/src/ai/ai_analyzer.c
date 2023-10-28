/**
 * @brief Analyze the given data and return the updated error rates.
 *
 * @file ai_analyzer.c
 *
 * @copyright Copyright (C) PTAI. All rights reserved.
 */

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/

// Standard Libraries

// Third-party Libraries
#include <zephyr/random/rand32.h>

// PTAI Libraries
#include "ai_analyzer.h"

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/

/**
 * @brief Generate a random double value within a specified range.
 *
 * @param upper_range The upper limit of the desired range.
 * @param lower_range The lower limit of the desired range.
 * @return A random double value within the specified range.
 */
static double get_random_value_range(double upper_range, double lower_range)
{
    uint32_t rand_val = sys_rand32_get();                 // Get a 32-bit random value
    double random_double = (double)rand_val / UINT32_MAX; // Scale to [0.0, 1.0]

    return (lower_range + random_double * upper_range); // Add range.
}
/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/

int ai_module_analyse_acc_data_dummy(struct ai_analysis_error_types *p_error_types)
{
    static struct ai_analysis_error_types error_types_s = {0};

    error_types_s.error_type_1 += get_random_value_range(1, 10);
    error_types_s.error_type_2 += get_random_value_range(1, 2);
    error_types_s.error_type_3 += get_random_value_range(0, 1);
    error_types_s.error_type_4 += get_random_value_range(1, 50);

    *p_error_types = error_types_s;
    return 0;
}