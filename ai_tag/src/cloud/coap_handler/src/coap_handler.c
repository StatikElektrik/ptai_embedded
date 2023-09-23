/// NOT FINISHED

/**
 * @brief
 *
 * @file
 *
 */

/***********************************************************************************************************************
 * Header Includes
 **********************************************************************************************************************/
#include <stdio.h>

#include <zephyr/net/coap.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <nrf_socket.h>

#include "coap_handler.h"
#include <zephyr/logging/log.h>

/***********************************************************************************************************************
 * Macro Definitions
 *********************************************************************************************************************/

/* STEP 4.2 - Define the macros for the CoAP version and message length */
#define APP_COAP_VERSION 1
#define APP_COAP_MAX_MSG_LEN 1280

#define MODULE coap_handler
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

/***********************************************************************************************************************
 * Typedef Definitions
 **********************************************************************************************************************/
enum dns_state
{
    RESOLVED,
    NOT_RESOLVED
};
/***********************************************************************************************************************
 * Private Global Variables
 **********************************************************************************************************************/

static uint16_t next_token;

static int sock;
static struct sockaddr_storage server;

static enum dns_state dns_state = NOT_RESOLVED;

// static struct dns_server_lookup
// {
//     uint32_t addr;
//     uint8_t addr_u8[4];
// };

// static struct dns_server_lookup dns_servers[] =
//     {
//         // Google DNS primary
//         {
//             .addr = 134744072,
//             .addr_u8 = {8, 8, 8, 8}},
//         // Google DNS secondary
//         {
//             .addr = 134743044,
//             .addr_u8 = {8, 8, 4, 4}},
//         // OpenDNS primary
//         {
//             .addr = 3494108894,
//             .addr_u8 = {208, 67, 222, 222}},
//         // OpenDNS secondary
//         {
//             .addr = 3494108894,
//             .addr_u8 = {208, 67, 222, 222}},
//         {.addr = 0,
//          .addr_u8 = {
//              0,
//              0,
//              0,
//          }}};
/***********************************************************************************************************************
 * Private Function Definitions
 **********************************************************************************************************************/
static int coap_handler_client_put_send(enum coap_msgtype msg_type, const char *uri_path, const char *message, size_t message_size)
{
    int err;
    struct coap_packet request;

    next_token++;

    uint8_t coap_buf[CONFIG_COAP_HELPER_RX_BUFFER_SIZE];

    err = coap_packet_init(&request, coap_buf, sizeof(coap_buf),
                           APP_COAP_VERSION, msg_type,
                           sizeof(next_token), (uint8_t *)&next_token,
                           COAP_METHOD_POST, coap_next_id());
    if (err < 0)
    {
        LOG_ERR("Failed to create CoAP request, %d\n", err);
        return err;
    }

    err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                    (uint8_t *)uri_path,
                                    strlen(uri_path));
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

    err = coap_packet_append_payload(&request, (uint8_t *)message, message_size);
    if (err < 0)
    {
        LOG_ERR("Failed to append payload, %d\n", err);
        return err;
    }

    err = send(sock, request.data, request.offset, 0);
    if (err < 0)
    {
        LOG_ERR("Failed to send CoAP request, %d\n", errno);
        return -errno;
    }

    LOG_INF("CoAP POST request sent: Token 0x%04x\n", next_token);

    return 0;
}

static const char *dns_state_name_get(enum dns_state state)
{
    switch (state)
    {
    case RESOLVED:
        return "RESOLVED";
    case NOT_RESOLVED:
        return "NOT_RESOLVED";
    default:
        return "STATE_UNKNOWN";
    }
}

static void dns_state_set(enum dns_state new_state)
{
    LOG_DBG("DNS state transition: %s --> %s",
            dns_state_name_get(dns_state),
            dns_state_name_get(new_state));
    dns_state = new_state;
}

static bool is_dns_resolved()
{
    return (dns_state == RESOLVED) ? true : false;
}

/***********************************************************************************************************************
 * Public Function Definitions
 **********************************************************************************************************************/

int coap_handler_resolve_dns_address(const char *server_hostname)
{

    if (is_dns_resolved())
    {
        LOG_WRN("DNS address is already resolved.");
        return 0;
    }

    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM};
    char ipv4_addr[NET_IPV4_ADDR_LEN];

    // @todo Add DNS config to it is own file. Also seperate from the CoAp because it calls a nrf specific function.
    // struct nrf_in_addr dns;
    // dns.s_addr = dns_servers[0].addr;
    // int dns_err = nrf_setdnsaddr(NRF_AF_INET, &dns, sizeof(dns));
    // LOG_DBG("DNS is set to %d - result %d", dns.s_addr, dns_err);
    // Todo End

    err = getaddrinfo(server_hostname, NULL, &hints, &result);
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

    struct sockaddr_in *server4 = ((struct sockaddr_in *)&server);

    server4->sin_addr.s_addr =
        ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
    server4->sin_family = AF_INET;
    server4->sin_port = htons(CONFIG_COAP_HELPER_PORT);

    inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr,
              sizeof(ipv4_addr));
    LOG_INF("IPv4 Address found %s\n", ipv4_addr);

    freeaddrinfo(result);

    dns_state_set(RESOLVED);
    return 0;
}

int coap_handler_client_put_confirmable_send(const char *uri_path, const char *message, size_t message_size)
{
    return coap_handler_client_put_send(COAP_TYPE_CON, uri_path, message, message_size);
}

int coap_handler_client_put_non_confirmable_send(const char *uri_path, const char *message, size_t message_size)
{
    return coap_handler_client_put_send(COAP_TYPE_NON_CON, uri_path, message, message_size);
}

int coap_handler_client_init(void)
{
    int err;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        LOG_ERR("Failed to create CoAP socket: %d.\n", errno);
        return -errno;
    }

    err = connect(sock, (struct sockaddr *)&server,
                  sizeof(struct sockaddr_in));
    if (err < 0)
    {
        LOG_ERR("Connect failed : %d\n", errno);
        return -errno;
    }

    LOG_INF("Successfully connected to server");

    /* STEP 6.2 - Generate a random token after the socket is connected */
    next_token = sys_rand32_get();

    return 0;
}

/**@brief Handles responses from the remote CoAP server. */
int coap_handler_client_handle_response(uint8_t *buf, int received)
{
    struct coap_packet reply;
    uint8_t token[8];
    uint16_t token_len;
    const uint8_t *payload;
    uint16_t payload_len;
    uint8_t temp_buf[128];
    /* STEP 9.1 - Parse the received CoAP packet */
    int err = coap_packet_parse(&reply, buf, received, NULL, 0);
    if (err < 0)
    {
        LOG_ERR("Malformed response received: %d\n", err);
        return err;
    }

    /* STEP 9.2 - Confirm the token in the response matches the token sent */
    token_len = coap_header_get_token(&reply, token);
    if ((token_len != sizeof(next_token)) ||
        (memcmp(&next_token, token, sizeof(next_token)) != 0))
    {
        LOG_ERR("Invalid token received: 0x%02x%02x\n",
                token[1], token[0]);
        return 0;
    }

    /* STEP 9.3 - Retrieve the payload and confirm it's nonzero */
    payload = coap_packet_get_payload(&reply, &payload_len);

    if (payload_len > 0)
    {
        snprintf(temp_buf, MIN(payload_len + 1, sizeof(temp_buf)), "%s", payload);
    }
    else
    {
        strcpy(temp_buf, "EMPTY");
    }

    /* STEP 9.4 - Log the header code, token and payload of the response */
    LOG_INF("CoAP response: Code 0x%x, Token 0x%02x%02x, Payload: %s\n",
            coap_header_get_code(&reply), token[1], token[0], (char *)temp_buf);

    return 0;
}