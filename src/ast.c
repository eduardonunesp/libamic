#ifndef AMIC_AST
#define AMIC_AST

#include "amic.h"
#include <regex.h>

#define CMD_LOGIN "action: login\r\n"  \
                  "username: %s\r\n"   \
                  "secret: %s\r\n\r\n"

amic_status_t regex_test(const char *regex_test, const char *need)
{
    regex_t regex;
    int reti;
    char msgbuf[100];
    amic_status_t status = AMIC_STATUS_SUCCESS;

    reti = regcomp(&regex, regex_test, 0);
    if (reti) { 
        AMIC_ERR("Could not compile regex"); 
        return AMIC_STATUS_BAD_REGEX;
    }

    /* Execute regular expression */
    reti = regexec(&regex, need, 0, NULL, 0);
    if( !reti ) 
        status = AMIC_STATUS_SUCCESS;
    else if( reti == REG_NOMATCH)
        status = AMIC_STATUS_NOT_FOUND;
    else {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        AMIC_ERR("Regex match failed: %s", msgbuf);
        return AMIC_STATUS_BAD_REGEX;
    }

    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(&regex);
    return status;
}

amic_status_t amic_ast_banner(const char *banner)
{
    return regex_test("^Asterisk Call Manager.*$", banner);
}

amic_status_t amic_ast_success(const char *resp)
{
    return regex_test("^Response: Success$", resp);
}

amic_status_t amic_ast_check_auth(const char *resp)
{
    return regex_test("^Message: Authentication accepted$", resp);
}

amic_status_t amic_ast_event(const char *resp)
{
    return regex_test("^Event:.*$", resp);
}

static void on_amic_cmd_login(uv_write_t *req, int status) 
{
}

amic_status_t amic_cmd_login(amic_conn_t *conn,
                             const char *username, 
                             const char *secret,
                             amic_cmd_cb cmd_cb)
{
    amic_status_t status = AMIC_STATUS_SUCCESS;
    if (!conn)
        return AMIC_STATUS_NULL_PTR;

    char cmd[255];
    memset(cmd, '\0', sizeof(cmd));
    sprintf(cmd, CMD_LOGIN, username, secret);

    AMIC_DBG("SEND COMMAND: \r\n%s", cmd);

    int len = sizeof(cmd);
    char buffer[sizeof(cmd)]; 

    uv_buf_t buff = uv_buf_init(buffer, sizeof(buffer));
    buff.len = len;
    buff.base = cmd;

    conn->cmd_cb = cmd_cb;
    conn->cmd = AMIC_CMD_LOGIN;

    status = uv_write(&conn->w_req,
                      (uv_stream_t*) &conn->handler,
                      &buff, 1, on_amic_cmd_login); 
    if (status != 0) {
        AMIC_ERR("Error on send login command: %s", uv_strerror(status));
        return AMIC_STATUS_CMD_FAILED;
    }

    return status;
}

amic_status_t amic_add_event(amic_conn_t *conn,
                             const char *event,
                             amic_ev_cb ev_cb)
{
    amic_status_t status = AMIC_STATUS_SUCCESS;
    if (!conn)
        return AMIC_STATUS_NULL_PTR;
    int err = hashmap_put(conn->ev_map, strdup(event), (void*) ev_cb);
    if (err != MAP_OK) {
        return AMIC_STATUS_FAIL;
    }

    AMIC_DBG("Added event listener %s", event);
    return status;
}

const char* amic_get_ev_value(amic_map_t amic_map, const char *key)
{
    char *ret = 0;
    if (!amic_map)
        return 0;

#ifndef NDEBUG
    AMIC_DBG("Test key %s", key);
#endif

    int map_err = hashmap_get(amic_map, (char*) key, (void**) (&ret));
    if (map_err != MAP_OK) {
#ifndef NDEBUG
    AMIC_DBG("Test key not found %s", key);
#endif
        return 0;
    }

    return ret;
}

#endif
