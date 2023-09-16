/// NOT FINISHED

/**
* @brief 
*
* @file 
*/

#ifndef SRC_CLOUD_COAP_HANDLER_
#define SRC_CLOUD_COAP_HANDLER_

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/

//Standard Libraries

//Third-party Libraries

/***********************************************************************************************************************
 * Macro Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/
int coap_handler_resolve_dns_address(void);

int coap_handler_client_put_confirmable_send(const char *message, size_t message_size);

int coap_handler_client_put_non_confirmable_send(const char *message, size_t message_size);

int coap_handler_client_init(void);

int coap_handler_client_handle_response(uint8_t *buf, int received);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CLOUD_COAP_HANDLER_ */
