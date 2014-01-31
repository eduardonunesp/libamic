/**
 * Author: Eduardo Nunes Pereira (eduardonunesp@gmail.com)
 * License: BSD
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Wintermoon nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AMIC_HEADER
#define AMIC_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <uv.h>
#include "hashmap.h"

#ifndef NDEBUG
#define AMIC_DBG(STR, ...) fprintf(stdout, STR "\n", ##__VA_ARGS__)
#else
#define AMIC_DBG(STR, ...)
#endif
#define AMIC_ERR(STR, ...) fprintf(stderr, STR "\n", ##__VA_ARGS__)

/* Common return types */
typedef enum {
    AMIC_STATUS_SUCCESS = 0,
    AMIC_STATUS_FAIL,
    AMIC_STATUS_BAD_ALLOC,
    AMIC_STATUS_NULL_PTR, 
    AMIC_STATUS_CONNECTION_FAILED,
    AMIC_STATUS_BAD_ADDRESS,
    AMIC_STATUS_BAD_REGEX,
    AMIC_STATUS_NOT_FOUND,
    AMIC_STATUS_CMD_FAILED
} amic_status_t;

/* States of amic connetion */
typedef enum {
    AMIC_STATE_INIT = 0,
    AMIC_STATE_OPEN,
    AMIC_STATE_CLOSE
} amic_state_t;

/* Indicates the last command for a connection */
typedef enum {
    AMIC_CMD_NONE = 0,
    AMIC_CMD_LOGIN
} amic_cmd_t;

/* Just familiarize a foreign struct */
typedef map_t amic_map_t;
struct amic_conn_t;

/* Callback for a connection request, when open the connection with AMI */
typedef void (*amic_conn_cb)(struct amic_conn_t *conn, amic_status_t status);

/* Callback for a command request */
typedef void (*amic_cmd_cb)(struct amic_conn_t *conn, amic_status_t status);

/* Callback for a event registered */
typedef void (*amic_ev_cb)(struct amic_conn_t *conn, amic_map_t ev_keys);

/* Connection is the main struct, manages the sockets, connections 
   callbacks and commands */
typedef struct amic_conn_t {
    char *ip_addr;
    unsigned int port;
    amic_state_t state;

    uv_loop_t *loop;
    uv_tcp_t handler;
    uv_connect_t tunnel;
    uv_write_t w_req;

    union {
        struct sockaddr addr;
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
    } t;

    amic_cmd_t cmd;
    amic_conn_cb conn_cb;
    amic_cmd_cb cmd_cb;

    map_t ev_map;
} amic_conn_t;


/*
 * Initialize a connection struct, note that function does not
 * make the connection with remote AMI server, only just do the
 * important allocations for struct amic_conn_t.
 * 
 *  - amic_conn_t: Connection to be initialized
 *  - ip_addr: Remote ip address 
 *  - port: Remote port 
 */
amic_status_t amic_init_conn(amic_conn_t **conn,
                             char *ip_addr, 
                             unsigned int port);

/*
 * With the amic_conn_t struct properly initialized it's time to make 
 * the connection with AMI server, don't forget the callback 
 * function. Note: AMI need a login too, so you must call amic_cm_login
 * only if the status for the callback is AMIC_STATUS_SUCCESS
 *
 *  - amic_conn_t: Connection already initialized
 *  - amic_conn_cb: The callback to be called when connection is done
 */
amic_status_t amic_open(amic_conn_t *conn,
                        amic_conn_cb conn_cb);

/*
 * Close the connection with AMI server
 *  - amic_conn_t: Connection already initialized
 */
void amic_close(amic_conn_t *conn);

/*
 * The main loop of the libamic, this function blocks the execution
 * and starts to listen and process all libuv events, but if there is 
 * no event to execute the main loop exits
 */
void amic_run();

/*
 * Request a action login for AMI server, and response come in the 
 * callback. The login executed with success if the status at struct
 * amic_cmd_cb have the value of AMIC_STATUS_SUCCESS;
 *
 *  - amic_conn_t: Connection already initialized
 *  - username: The username for AMI server
 *  - secret: The password related to username
 *  - amic_cmd_cb: Callback for command login
 */
amic_status_t amic_cmd_login(amic_conn_t *conn,
                             const char *username,
                             const char *secret,
                             amic_cmd_cb cmd_cb);

/*
 * Register a new event to listen, if the command executed and dispatched
 * to AMI server, the callback amic_ev_cb will be called. The callback have
 * parameters with all keys to associated event, to call any key use the
 * the helper function amic_get_ev_value with amic_map_t struct
 *
 *  - amic_conn_t: Connection already initialized
 *  - event: The event to listen
 *  - amic_ev_cb: Callback for event registered
 */
amic_status_t amic_reg_event(amic_conn_t *conn,
                             const char *event,
                             amic_ev_cb ev_cb);

/*
 * Helper to get any key from amic_map_t
 * 
 *  - amic_map_t: The map to get the value
 *  - key: All map needs a key, so this is the key
 */
const char* amic_get_ev_value(amic_map_t amic_map, const char *key);

#endif
