/**
 * @brief Handles all the coap related operations including sending a p_message and receiving the response.
 *
 * @file coap_handler.c
 *
 * @todo Response handler is not implemeted, here is three different examples for response handling,
 *
 *  https://github.com/nrfconnect/sdk-nrf/blob/4f612c9527bfad994eff14b63608f227d16c3581/samples/openthread/coap_client/src/coap_client_utils.c
 *  https://github.com/nrfconnect/sdk-nrf/blob/4f612c9527bfad994eff14b63608f227d16c3581/subsys/net/lib/coap_utils/coap_utils.c
 *  https://github.com/nrfconnect/sdk-zephyr/tree/a47925f081693cfef963af4f29b2f9f0ccc80768/samples/net/sockets/coap_client
 */

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/
// Standard Libraries
#include <stdio.h>

// Third Party Libraries
#include <zephyr/net/coap.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>
#include <zephyr/logging/log.h>

// PTAI Libraries
#include "coap_handler.h"

/***********************************************************************************************************************
 * Macro Definitions
 *********************************************************************************************************************/

#define APP_COAP_VERSION 1

#define MODULE coap_handler
LOG_MODULE_REGISTER(MODULE, CONFIG_COAP_HANDLER_LOG_LEVEL);

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
enum dns_state
{
    DNS_RESOLVED,
    DNS_NOT_RESOLVED
};

enum connection_state
{
    STATE_DISCONNECTED,
    STATE_CONNECTED
};
/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/
static uint16_t g_next_token;

static int g_sock;
static struct sockaddr_storage g_server;

static enum dns_state g_dns_state = DNS_NOT_RESOLVED;
static enum connection_state g_connection_state = STATE_DISCONNECTED;

/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/

/* State Related Functions */

/**
 * @brief Get the name of the DNS state as a string.
 *
 * @param state DNS state
 * @return const char* name of the state as a string.
 */
static const char *coap_handler_dns_state_name_get(enum dns_state state)
{
    switch (state)
    {
    case DNS_RESOLVED:
        return "DNS_RESOLVED";
    case DNS_NOT_RESOLVED:
        return "DNS_NOT_RESOLVED";
    default:
        return "STATE_UNKNOWN";
    }
}

/**
 * @brief Get the name associated with a connection state.
 *
 * @param state The connection state.
 * @return The name of the state as a string.
 */
static const char *connection_state_name_get(enum connection_state state)
{
    switch (state)
    {
    case STATE_DISCONNECTED:
        return "STATE_DISCONNECTED";
    case STATE_CONNECTED:
        return "STATE_CONNECTED";
    default:
        return "STATE_UNKNOWN";
    }
}

/**
 * @brief Set the current DNS state with the provided one.
 *
 * @param new_state New DNS state.
 */
static void coap_handler_dns_state_set(enum dns_state new_state)
{
    LOG_DBG("DNS state transition: %s --> %s",
            coap_handler_dns_state_name_get(g_dns_state),
            coap_handler_dns_state_name_get(new_state));
    g_dns_state = new_state;
}

/**
 * @brief Check if the DNS is resolved and solved to a IP address.
 *
 * @return true If DNS is resolved to an IP address.
 * @return fals If DNS is not resolved to an IP address.
 */
static bool coap_handler_is_dns_resolved()
{
    return (g_dns_state == DNS_RESOLVED) ? true : false;
}

/**
 * @brief Set the new connection state and perform state transition checks.
 *
 * @param new_state The new connection state.
 */
static void coap_handler_connection_state_set(enum connection_state new_state)
{
    if (g_connection_state == new_state)
    {
        LOG_DBG("Skipping transition to the same state (%s)",
                connection_state_name_get(g_connection_state));
        return;
    }

    switch (g_connection_state)
    {

    case STATE_DISCONNECTED:
        break;
    case STATE_CONNECTED:
        break;
    default:
        LOG_ERR("Invalid connection state transition, %s --> %s",
                connection_state_name_get(g_connection_state),
                connection_state_name_get(new_state));
        break;
    }

    LOG_DBG("Connection state transition: %s --> %s",
            connection_state_name_get(g_connection_state),
            connection_state_name_get(new_state));

    g_connection_state = new_state;
}

/**
 * @brief Verify the current connection state.
 *
 * @param state The connection state to verify.
 * @return true if the current connection state matches the given state, false otherwise.
 */
static bool coap_handler_connection_state_verify(enum connection_state state)
{
    return (g_connection_state == state);
}

/**
 * @brief Send a POST request to the already initialized client with the given URI path.
 *
 * @param msg_type Message Type of the COAP.
 * @param p_uri_path URI path that the request will be send to, @example api/v1/provision
 * @param p_message The payload which will send with the request.
 * @param message_size The size of the payload.
 * @return int 0 if successfull else failure.
 */
static int coap_handler_client_post_send(enum coap_msgtype msg_type, const char *p_uri_path, const char *p_message, size_t message_size)
{
    int err;
    struct coap_packet request;

    g_next_token++;

    uint8_t coap_buf[CONFIG_COAP_HANDLER_RX_BUFFER_SIZE];

    err = coap_packet_init(&request, coap_buf, sizeof(coap_buf),
                           APP_COAP_VERSION, msg_type,
                           sizeof(g_next_token), (uint8_t *)&g_next_token,
                           COAP_METHOD_POST, coap_next_id());
    if (err < 0)
    {
        LOG_ERR("Failed to create CoAP request, %d\n", err);
        return err;
    }

    err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                    (uint8_t *)p_uri_path,
                                    strlen(p_uri_path));
    if (err < 0)
    {
        LOG_ERR("Failed to encode CoAP option, %d\n", err);
        return err;
    }

    const uint8_t text_plain = COAP_CONTENT_FORMAT_TEXT_PLAIN;
    err = coap_packet_append_option(&request, COAP_OPTION_CONTENT_FORMAT,
                                    &text_plain,
                                    sizeof(text_plain));
    if (err < 0)
    {
        LOG_ERR("Failed to encode CoAP option, %d\n", err);
        return err;
    }

    err = coap_packet_append_payload_marker(&request);
    if (err < 0)
    {
        LOG_ERR("Failed to append payload marker, %d\n", err);
        return err;
    }

    err = coap_packet_append_payload(&request, (uint8_t *)p_message, message_size);
    if (err < 0)
    {
        LOG_ERR("Failed to append payload, %d\n", err);
        return err;
    }

    err = send(g_sock, request.data, request.offset, 0);
    if (err < 0)
    {
        LOG_ERR("Failed to send CoAP request, %d\n", errno);
        return -errno;
    }

    LOG_DBG("CoAP POST request sent: Token 0x%04x\n", g_next_token);

    return 0;
}

static int coap_handler_resolve_dns_address(const char *p_server_hostname)
{

    if (coap_handler_is_dns_resolved())
    {
        LOG_DBG("DNS address is already resolved.");
        return 0;
    }

    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM};
    char ipv4_addr[NET_IPV4_ADDR_LEN];

    err = getaddrinfo(p_server_hostname, NULL, &hints, &result);
    if (err != 0)
    {
        LOG_ERR("ERROR: getaddrinfo failed %d\n", err);
        return -EIO;
    }

    if (result == NULL)
    {
        LOG_ERR("ERROR: Address not found\n");
        return -ENOENT;
    }

    struct sockaddr_in *server4 = ((struct sockaddr_in *)&g_server);

    server4->sin_addr.s_addr =
        ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
    server4->sin_family = AF_INET;
    server4->sin_port = htons(CONFIG_COAP_HANDLER_PORT);

    inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr,
              sizeof(ipv4_addr));
    LOG_DBG("IPv4 Address found %s\n", ipv4_addr);

    freeaddrinfo(result);

    coap_handler_dns_state_set(DNS_RESOLVED);
    return 0;
}

/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/

int coap_handler_client_connect(const char *p_server_hostname)
{

    if (coap_handler_connection_state_verify(STATE_CONNECTED))
    {
        LOG_WRN("CoAP socket is already connected.");
        return -EALREADY;
    }

    int err = coap_handler_resolve_dns_address(p_server_hostname);
    if (err != 0)
    {
        LOG_ERR("Could not resolve the dns address for %s", p_server_hostname);
        return err;
    }

    g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_sock < 0)
    {
        LOG_ERR("Failed to create CoAP socket: %d.\n", errno);
        return -errno;
    }

    err = connect(g_sock, (struct sockaddr *)&g_server,
                  sizeof(struct sockaddr_in));
    if (err < 0)
    {
        LOG_ERR("Connect failed : %d\n", errno);
        return -errno;
    }

    LOG_DBG("Successfully connected to server");
    coap_handler_connection_state_set(STATE_CONNECTED);

    // Generate a random token after the socket is connected.
    g_next_token = sys_rand32_get();

    return 0;
}

int coap_handler_client_disconnect(void)
{
    if (!coap_handler_connection_state_verify(STATE_CONNECTED))
    {
        LOG_WRN("CoAP socket is already disconnected.");
        return -ENOTCONN;
    }

    int err = close(g_sock);
    if (err)
    {
        LOG_ERR("Failed to disconnect CoAP socket, error: %d", err);
        return err;
    }

    coap_handler_connection_state_set(STATE_DISCONNECTED);
    return 0;
}

int coap_handler_client_handle_response(uint8_t *p_received_coap_packet_buf, size_t buf_size)
{
    struct coap_packet reply;
    uint8_t token[8];
    uint16_t token_len;
    const uint8_t *payload;
    uint16_t payload_len;
    uint8_t temp_buf[128];

    // Parse the received CoAP packet.
    int err = coap_packet_parse(&reply, p_received_coap_packet_buf, buf_size, NULL, 0);
    if (err < 0)
    {
        LOG_ERR("Malformed response received: %d\n", err);
        return err;
    }

    // Confirm the token in the response matches the token sent.
    token_len = coap_header_get_token(&reply, token);
    if ((token_len != sizeof(g_next_token)) ||
        (memcmp(&g_next_token, token, sizeof(g_next_token)) != 0))
    {
        LOG_ERR("Invalid token received: 0x%02x%02x\n",
                token[1], token[0]);
        return 0;
    }

    // Retrieve the payload and confirm it's nonzero.
    payload = coap_packet_get_payload(&reply, &payload_len);

    if (payload_len > 0)
    {
        snprintf(temp_buf, MIN(payload_len + 1, sizeof(temp_buf)), "%s", payload);
    }
    else
    {
        strcpy(temp_buf, "EMPTY");
    }

    // Log the header code, token and payload of the response.
    LOG_DBG("CoAP response: Code 0x%x, Token 0x%02x%02x, Payload: %s\n",
            coap_header_get_code(&reply), token[1], token[0], (char *)temp_buf);

    return 0;
}

int coap_handler_client_post_confirmable_send(const char *p_uri_path, const char *p_message, size_t message_size)
{
    return coap_handler_client_post_send(COAP_TYPE_CON, p_uri_path, p_message, message_size);
}

int coap_handler_client_post_non_confirmable_send(const char *p_uri_path, const char *p_message, size_t message_size)
{
    return coap_handler_client_post_send(COAP_TYPE_NON_CON, p_uri_path, p_message, message_size);
}
