#ifndef AMIC_CONNECTION
#define AMIC_CONNECTION

#include "amic.h"
#include <regex.h>

extern amic_status_t amic_ast_banner(const char *banner);
extern amic_status_t amic_ast_success(const char *response);
extern amic_status_t amic_ast_check_auth(const char *response);
extern amic_status_t amic_ast_event(const char *resp);
extern void remove_carriage_return(char *str);
extern void get_event_param(char *str, char *key, char *value);
extern void get_event_name(char *str, char *ev);

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
    new_conn->ev_map = hashmap_new();

#ifndef NDEBUG
    AMIC_DBG("Created amic_conn [%s:%d]", new_conn->ip_addr, new_conn->port);
#endif

    //TODO: Check if is ipv4 or ipv6 connection
    uv_tcp_init(new_conn->loop, &new_conn->handler);
    status = uv_ip4_addr(new_conn->ip_addr, 
                         new_conn->port, 
                         &new_conn->t.addr4);

    if (status != 0) {
        AMIC_ERR("Get ip status %d:%s", status, uv_strerror(status));
        return AMIC_STATUS_CONNECTION_FAILED;
    }

    *conn = new_conn;
    return status;
}

typedef struct {
    amic_conn_t *conn;
    char *data;
} process_event_pair_t;

void process_event(uv_work_t *req)  
{
    assert(req);
    assert(req->data);

    process_event_pair_t *pep = 0;
    pep = (process_event_pair_t*) req->data;

    amic_conn_t *conn = 0;
    conn = pep->conn;
    char *data = pep->data;

    char *cur_line = data;
    int count = 0;
    amic_ev_cb cb;
    map_t map_keys = 0;
    int map_err = MAP_MISSING;
    while (cur_line) {
        char *key = 0, *value = 0;
        if (!cur_line) break;

        char *next_line = strchr(cur_line, '\n');
        if (next_line) *next_line = '\0';

        if (cur_line != '\0' && *cur_line != '\r' && strlen(cur_line) > 0) {
            const size_t kv_size = strlen(cur_line);
            key = (char*) malloc(kv_size);
            value = (char*) malloc(kv_size);

            remove_carriage_return(cur_line);
            get_event_param(cur_line, key, value);

            if (!strncmp("Event", key, strlen(key))) {
                count++;
                if (count > 1) break;

                map_err = hashmap_get(conn->ev_map, value, (void**) (&cb));
                if (map_err != MAP_OK) {
                    free(key); free(value);
                    break;
                }
            }

            if (map_err == MAP_OK) {
                if (!map_keys)
                    map_keys = hashmap_new();
                    //TODO: Must delete key and value from mem someday
                int put_err = hashmap_put(map_keys, key, value);
                if (put_err != MAP_OK) {
                    AMIC_ERR("Error on put key %s", key);
                }
            }
        }

        if (next_line) *next_line = '\n';    
        cur_line = next_line ? (next_line+1) : NULL;
    }

    if (map_err == MAP_OK && map_keys) {
        cb(conn, map_keys);
        int index = 0;
        hashmap_free(map_keys);
    }
    
    free(pep->data);
    free(pep);
}

void after_process_event(uv_work_t *req, int status) 
{
    assert(req);
    assert(req->data);
    free(req);
}

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) 
{
    if (nread <= -1) {
        if (nread != UV_EOF)
            AMIC_ERR("Read error %s", uv_strerror(nread));
        return;
    }

    amic_conn_t *conn = (amic_conn_t*) stream->data;
    assert(conn != NULL);

    char *data = (char*) malloc(sizeof(char) * (nread+1));
    data[nread] = '\0';
    memcpy(data, buf->base, nread+1);

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
                case AMIC_CMD_QUEUE_STATUS: {
                    AMIC_DBG("DATA: %s", data);
                } break;
            }

            if (strstr(data, "Event:")) {
               uv_work_t *queue_req = 0;
               queue_req = (uv_work_t*) malloc(sizeof(uv_work_t));

               if (!queue_req) {
                    AMIC_DBG("Cannot alloc work queue");
               } else {
                   process_event_pair_t *pep = 0;
                   pep = (process_event_pair_t*) malloc(sizeof(process_event_pair_t));
                   pep->conn = conn;
                   pep->data = data;
                   queue_req->data = (void*) pep;
                   uv_queue_work(conn->loop, queue_req, process_event, after_process_event);
               }
            }

            conn->cmd = AMIC_CMD_NONE;

        } break;
        case AMIC_STATE_CLOSE:
            break;
        default:
            AMIC_ERR("Unknow state %d", conn->state);
            break;
    }

    free(buf->base);
}

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(size), size);
}

void on_connect(uv_connect_t *req, int status) 
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
    
#ifndef NDEBUG
    AMIC_DBG("Will connect to %s:%d", conn->ip_addr, conn->port);
#endif

    conn->conn_cb = conn_cb;
    conn->tunnel.data = (void*) conn;

    status = uv_tcp_connect(&conn->tunnel, 
                            &conn->handler, 
                            &conn->t.addr, 
                            on_connect);

    if (status != 0) {
#ifndef NDEBUG
        AMIC_DBG("Get tcp connect status %d:%s", status, uv_strerror(status));
#endif
        return AMIC_STATUS_CONNECTION_FAILED;
    }

    return status;
}

void on_amic_close(uv_handle_t *handle) 
{
    amic_conn_t *conn = (amic_conn_t*) handle->data;
    if (!conn)
        return;

    free(conn->ip_addr);
    hashmap_free(conn->ev_map);
    free(conn);
}

void amic_close(amic_conn_t *conn)
{
    if (!conn)
        return;
    
    conn->handler.data = (void*) conn;
    uv_close((uv_handle_t*) &conn->handler, on_amic_close);
}

#endif
