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
	double error_type_1;
	double error_type_2;
	double error_type_3;
	double error_type_4;
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
