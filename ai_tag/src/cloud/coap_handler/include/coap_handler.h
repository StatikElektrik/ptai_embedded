/**
* @brief Interface for handler functions that simplifies to use of CoAp Library. 
*
* @file coap_handler.h
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

/**
 * @brief Initalize the coap client and connect to the given server hostname by opening a socket.
 * 
 * @param server_hostname Server host name to connect.
 * @return int 0 if successful else failure.
 */
int coap_handler_client_connect(const char *p_server_hostname);

/**
 * @brief Disconnects the CoAP client socket.
 *
 *
 * @return 0 on success, a negative error code on failure.
 *   - 0: Socket disconnected successfully.
 *   - -ENOTCONN: The socket was not connected or is already disconnected.
 *   - Another error code in case of a failure.
 */
int coap_handler_client_disconnect(void);

/**
 * @brief Send a confirmable post request to the given URI path.
 * 
 * @param p_uri_path URI path the request will sent to.
 * @param p_message Payload that will be send with the request.
 * @param message_size Payload size.
 * @return int 0 if sucessful else failure.
 */
int coap_handler_client_post_confirmable_send(const char *p_uri_path, const char *p_message, size_t message_size);

/**
 * @brief Send a non-confirmable post request to the given URI path.  
 * 
 * @param p_uri_path URI path the request will sent to. 
 * @param p_message Payload that will be send with the request.
 * @param message_size Payload size.
 * @return int 0 if sucessful else failure.
 */
int coap_handler_client_post_non_confirmable_send(const char *p_uri_path, const char *p_message, size_t message_size);

/**
 * @brief Parse the given received CoAp packet.
 * @warning Not implemented fully.
 * 
 * @param p_received_coap_packet_buf Received CoAp packet.
 * @param buf_size Size of the received Coap packet.
 * @return int 0 if sucessful else failure.
 */
int coap_handler_client_handle_response(uint8_t *p_received_coap_packet_buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CLOUD_COAP_HANDLER_ */
