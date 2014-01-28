#ifndef AMIC_LOOP
#define AMIC_LOOP

#include "amic.h"

void amic_run(amic_conn_t *conn) 
{
    if (!conn) {
        fprintf(stderr, "Connection ins't created yet\n");
        return;
    }

    uv_run(conn->loop, UV_RUN_DEFAULT);
#ifndef NDEBUG 
    fprintf(stdout, "Exit main loop lib\n");
#endif 
    uv_loop_delete(conn->loop);
}

#endif
