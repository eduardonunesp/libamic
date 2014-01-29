#include <stdio.h>
#include <stdlib.h>
#include <amic.h>

char *username = 0;
char *secret = 0;

static void on_login(amic_conn_t *conn,
                     amic_status_t status) 
{

}

static void on_connection(amic_conn_t *conn, 
                          amic_status_t status) 
{
    fprintf(stdout, "CONNECT STATUS %d\n", status);
    if (conn->state == AMIC_STATE_OPEN) {
        amic_cmd_login(conn, username, secret, on_login);
    }
}

static void on_event(amic_conn_t *conn, 
                     amic_map_t keys) 
{
    AMIC_DBG("ON EV %s", amic_get_ev_value(keys, "Event"));

}

int main(int argc, char *argv[]) 
{
    if (argc < 5) {
        fprintf(stdout, "Usage: amic_test <address> <port> <username> <secret>\n");
        return 1;
    }

    char *address = strdup(argv[1]);
    unsigned int port = atoi(argv[2]);

    username = strdup(argv[3]);
    secret = strdup(argv[4]);

    amic_conn_t *conn = 0;
    amic_status_t status = AMIC_STATUS_SUCCESS;

    status = amic_init_conn(&conn, address, port);
    if (status != AMIC_STATUS_SUCCESS) {
        AMIC_ERR("Fail to create connection %d.\n", status);
        return 1;
    }

    status = amic_open(conn, on_connection);

    if (status != AMIC_STATUS_SUCCESS) {
        AMIC_ERR("Fail to open connection %d.\n", status);
    }

    amic_add_event(conn, "QueueMemberStatus", on_event);

    amic_run(conn);

    AMIC_DBG("Exit app %s\n", argv[0]);
    return 0;
}
