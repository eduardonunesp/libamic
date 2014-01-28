#ifndef AMIC_HEADER
#define AMIC_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <uv.h>

#ifndef NDEBUG
#define AMIC_DBG(STR, ...) fprintf(stdout, STR "\n", ##__VA_ARGS__)
#else
#define AMIC_DBG(STR, ...)
#endif
#define AMIC_ERR(STR, ...) fprintf(stderr, STR "\n", ##__VA_ARGS__)

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

typedef enum {
    AMIC_STATE_INIT = 0,
    AMIC_STATE_OPEN,
    AMIC_STATE_CLOSE
} amic_state_t;

typedef enum {
    AMIC_CMD_NONE = 0,
    AMIC_CMD_LOGIN
} amic_cmd_t;

struct amic_conn_t;
typedef void (*amic_conn_cb)(struct amic_conn_t *conn, amic_status_t status);
typedef void (*amic_cmd_cb)(struct amic_conn_t *conn, amic_status_t status);

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
} amic_conn_t;

amic_status_t amic_init_conn(amic_conn_t **conne,
                             char *ip_addr, 
                             unsigned int port);

amic_status_t amic_open(amic_conn_t *conn,
                        amic_conn_cb conn_cb);

void amic_close(amic_conn_t *conn);

void amic_run();

amic_status_t amic_ast_banner(const char *banner);
amic_status_t amic_ast_success(const char *response);
amic_status_t amic_ast_check_auth(const char *response);
amic_status_t amic_cmd_login(amic_conn_t *conn,
                             const char *username,
                             const char *secret,
                             amic_cmd_cb cmd_cb);

#endif
