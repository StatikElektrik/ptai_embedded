/**
* @brief 
* 
* @file
*/

#ifndef CLOUD_CLOUD_CODEC_THINGSBOARD_IOT_THINGSBOARD_IOT_CODEC_HELPERS
#define CLOUD_CLOUD_CODEC_THINGSBOARD_IOT_THINGSBOARD_IOT_CODEC_HELPERS

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

struct cloud_data_device_provision_request {
    char device_name[CONFIG_THINGSBOARD_IOT_DEVICE_ID_MAX_LEN];
	char device_provision_key[CONFIG_THINGSBOARD_IOT_DEVICE_PROVISION_KEY_MAX_LEN];
	char device_provision_secret[CONFIG_THINGSBOARD_IOT_DEVICE_PROVISION_SECRET_MAX_LEN];
};

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/

// Forward declaration
struct cloud_codec_data;

/***********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/

int cloud_codec_encode_device_provision_request_data(struct cloud_codec_data *output,
				   struct cloud_data_device_provision_request *dev_provision_buf);


#ifdef __cplusplus
}
#endif

#endif // CLOUD_CLOUD_CODEC_THINGSBOARD_IOT_THINGSBOARD_IOT_CODEC_HELPERS