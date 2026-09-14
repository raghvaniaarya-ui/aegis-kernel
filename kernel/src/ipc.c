#include "ipc.h"

#include <stddef.h>

typedef struct {
    bool        used;
    aegis_msg_t pending;
    bool        has_pending;
} endpoint_t;

static endpoint_t endpoints[AEGIS_MAX_ENDPOINTS];

void ipc_init(void) {
    for (size_t i = 0; i < AEGIS_MAX_ENDPOINTS; i++) {
        endpoints[i].used = false;
        endpoints[i].has_pending = false;
    }
}

int ipc_endpoint_create(endpoint_id_t *out_id) {
    if (!out_id) {
        return -1;
    }

    for (size_t i = 0; i < AEGIS_MAX_ENDPOINTS; i++) {
        if (!endpoints[i].used) {
            endpoints[i].used = true;
            endpoints[i].has_pending = false;
            *out_id = (endpoint_id_t)i;
            return 0;
        }
    }

    return -1;
}

int ipc_send(endpoint_id_t to, const aegis_msg_t *msg) {
    if (!msg || to >= AEGIS_MAX_ENDPOINTS || !endpoints[to].used) {
        return -1;
    }

    if (endpoints[to].has_pending) {
        return -2;
    }

    endpoints[to].pending = *msg;
    endpoints[to].has_pending = true;
    return 0;
}

int ipc_recv(endpoint_id_t on, aegis_msg_t *out_msg) {
    if (!out_msg || on >= AEGIS_MAX_ENDPOINTS || !endpoints[on].used) {
        return -1;
    }

    if (!endpoints[on].has_pending) {
        return -2;
    }

    *out_msg = endpoints[on].pending;
    endpoints[on].has_pending = false;
    return 0;
}
