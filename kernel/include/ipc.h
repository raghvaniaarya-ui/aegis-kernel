#ifndef AEGIS_IPC_H
#define AEGIS_IPC_H

#include "types.h"

#define AEGIS_MSG_PAYLOAD_SIZE 48
#define AEGIS_MAX_ENDPOINTS    64

typedef enum {
    MSG_PING = 1,
    MSG_PONG = 2,
    MSG_LOG  = 3,
} aegis_msg_type_t;

typedef struct AEGIS_PACKED {
    endpoint_id_t sender;
    uint64_t      type;
    uint64_t      arg0;
    uint64_t      arg1;
    uint64_t      arg2;
    char          payload[AEGIS_MSG_PAYLOAD_SIZE];
} aegis_msg_t;

void ipc_init(void);
int  ipc_endpoint_create(endpoint_id_t *out_id);
int  ipc_send(endpoint_id_t to, const aegis_msg_t *msg);
int  ipc_recv(endpoint_id_t on, aegis_msg_t *out_msg);

#endif
