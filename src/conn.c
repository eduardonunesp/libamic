#ifndef AMIC_CONNECTION
#define AMIC_CONNECTION

#include "amic.h"

amic_status_t amic_init_conn(amic_conn_t **conn,
                             char *ip_addr, 
                             unsigned int port)
{
    amic_status_t status = AMIC_STATUS_SUCCESS;
    amic_conn_t *new_conn = 0;
    new_conn = (amic_conn_t*) malloc(sizeof(amic_conn_t));

    if (!new_conn)
        return AMIC_STATUS_BAD_ALLOC;

    new_conn->ip_addr = strdup(ip_addr);
    new_conn->port = port;
    new_conn->loop = uv_default_loop();
    new_conn->state = AMIC_STATE_INIT;

    AMIC_DBG("Created amic_conn %s:%d", ip_addr, port);

    //TODO: Check if is ipv4 or ipv6 connection
    uv_tcp_init(new_conn->loop, &new_conn->handler);
    status = uv_ip4_addr(new_conn->ip_addr, 
                         new_conn->port, 
                         &new_conn->t.addr4);

    if (status != 0) {
        AMIC_DBG("Get ip status %d:%s", status, uv_strerror(status));
        return AMIC_STATUS_CONNECTION_FAILED;
    }

    *conn = new_conn;
    return status;
}

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(size), size);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread == -1) {
        if (nread != UV_EOF)
            AMIC_ERR("Read error %s", uv_strerror(nread));
        uv_close((uv_handle_t*) stream, NULL);
        free(stream);
        return;
    }

    amic_conn_t *conn = (amic_conn_t*) stream->data;
    assert(conn != NULL);

    char *data = (char*) malloc(sizeof(char) * (nread+1));
    data[nread] = '\0';
    memcpy(data, buf->base, nread+1);
    free(buf->base);

    AMIC_DBG("On read: %s", data);

    switch (conn->state) {
        case AMIC_STATE_INIT: {
            if (amic_ast_banner(data) == AMIC_STATUS_SUCCESS) {
                conn->state = AMIC_STATE_OPEN;
                conn->conn_cb((struct amic_conn_t*)conn, AMIC_STATUS_SUCCESS);
            } else {
                conn->state = AMIC_STATE_CLOSE;
                conn->conn_cb((struct amic_conn_t*)conn, AMIC_STATUS_CONNECTION_FAILED);
                uv_read_stop(stream);
            }
        } break;
        case AMIC_STATE_OPEN: {
            switch (conn->cmd) {
                case AMIC_CMD_NONE:
                    break;
                case AMIC_CMD_LOGIN: {
                    if (amic_ast_check_auth(data) == AMIC_STATUS_SUCCESS)
                        conn->cmd_cb((struct amic_conn_t*)conn, AMIC_STATUS_SUCCESS);
                    else
                        conn->cmd_cb((struct amic_conn_t*)conn, AMIC_STATUS_CONNECTION_FAILED);
                } break;
            }

            conn->cmd = AMIC_CMD_NONE;
        } break;
        case AMIC_STATE_CLOSE:
            break;
        default:
            AMIC_ERR("Unknow state %d", conn->state);
            break;
    }

    free(data);
}

static void on_connect(uv_connect_t *req, int status) 
{
    if (status < 0) {
        AMIC_ERR("Connected status %d:%s", status, uv_strerror(status));
        return;
    }

    req->handle->data = req->data;
    uv_read_start(req->handle, alloc_buffer, on_read);
}

amic_status_t amic_open(amic_conn_t *conn,
                        amic_conn_cb conn_cb)
{
    amic_status_t status = AMIC_STATUS_SUCCESS;

    if (!conn)
        return AMIC_STATUS_NULL_PTR;
    
    AMIC_DBG("Will connect to %s:%d", conn->ip_addr, conn->port);

    conn->conn_cb = conn_cb;
    conn->tunnel.data = (void*) conn;

    status = uv_tcp_connect(&conn->tunnel, 
                            &conn->handler, 
                            &conn->t.addr, 
                            on_connect);

    if (status != 0) {
        AMIC_DBG("Get tcp connect status %d:%s", status, uv_strerror(status));
        return AMIC_STATUS_CONNECTION_FAILED;
    }

    return status;
}

void amic_close(amic_conn_t *conn)
{
}

#endif
