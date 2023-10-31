/**
* @brief Analyze the given data and return the updated error rates.
* 
* @file ai_analyzer.h
*
* @copyright Copyright (C) PTAI. All rights reserved.
*/

#ifndef SRC_AI_AI_ANALYZER_H_
#define SRC_AI_AI_ANALYZER_H_

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/

//Standard Libraries

//Third-party Libraries

//PTAI Libraries

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
/**
 * @brief Structure to store different error types in unit of percentages.
 * 
 */
struct ai_analysis_error_types
{
	double normal_mode;
	double prs_red_intake_manifold;
	double comp_rat_red_cylinder;
	double fuel_inject_red_cylinder;
};
/***********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/

/**
 * @brief Returns cumulative randomized error values in unit of percentages for error types.
 *
 * @param p_error_types A pointer to the 'ai_analysis_error_types' structure to store error types.
 * @return 0 if the analysis is successful.
 */
int ai_module_analyse_acc_data_dummy(struct ai_analysis_error_types *p_error_types);

#ifdef __cplusplus
}
#endif

#endif // SRC_AI_AI_ANALYZER_H_
