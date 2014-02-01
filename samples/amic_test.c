#include <stdio.h>
#include <stdlib.h>
#include <amic.h>

char *username = 0;
char *secret = 0;

static void on_login(amic_conn_t *conn,
                     amic_status_t status) 
{
    /* Nothing to do, but you can do something after login */
}

static void on_connection(amic_conn_t *conn, 
                          amic_status_t status) 
{
    fprintf(stdout, "CONNECT STATUS %d\n", status);
    if (conn->state == AMIC_STATE_OPEN) {
        /* Great ! will connected, now must login in to AMI */
        amic_cmd_login(conn, username, secret, on_login);
    }
}

static void on_queue_status(amic_conn_t *conn, 
                     amic_status_t status) 
{
    AMIC_DBG("AAA %d", status);
}

static void on_event(amic_conn_t *conn, 
                     amic_map_t keys) 
{
    /* Great ! out registered event was triggered */
    AMIC_DBG("ON EV %s", amic_get_ev_value(keys, "Event"));

    amic_cmd_queue_status(conn, on_queue_status);
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

    /* We use the amic_conn_t struct for a lot of calls */
    amic_conn_t *conn = 0;
    amic_status_t status = AMIC_STATUS_SUCCESS;

    /* Now we must init our connection */
    status = amic_init_conn(&conn, address, port);
    if (status != AMIC_STATUS_SUCCESS) {
        AMIC_ERR("Fail to create connection %d.\n", status);
        return 1;
    }

    /* Ok trying to open a connection with AMI */
    status = amic_open(conn, on_connection);

    if (status != AMIC_STATUS_SUCCESS) {
        AMIC_ERR("Fail to open connection %d.\n", status);
    }

    /* Everytime QueueMemberStatus happen the call back is called */
    amic_reg_event(conn, "QueueMemberStatus", on_event);

    /* Our main loop, and will block until close connection our fail to login */
    amic_run(conn);

    AMIC_DBG("Exit app %s\n", argv[0]);
    return 0;
}
