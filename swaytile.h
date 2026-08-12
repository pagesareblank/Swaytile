/* swaytile.h */
#ifndef SWAYTILE_H
#define SWAYTILE_H

#include <stdint.h>
#include <stdbool.h>
#include "tree.h"

/* IPC Event Types */
#define IPC_EVENT_BIT        (1U << 31)
#define IPC_EVENT_WORKSPACE  0
#define IPC_EVENT_OUTPUT     1
#define IPC_EVENT_MODE       2
#define IPC_EVENT_WINDOW     3

#define IS_IPC_EVENT(type)    (((type) & IPC_EVENT_BIT) != 0)
#define MASK_EVENT_TYPE(type) ((type) & ~IPC_EVENT_BIT)

/* IPC Command Types */
#define IPC_RUN_COMMAND 0
#define IPC_GET_TREE    4
#define IPC_SUBSCRIBE   2

/* IPC functions */
int ipc_connect(void);
int ipc_send(int fd, uint32_t type, const void *payload, uint32_t length);
int ipc_read(int fd, uint32_t *out_type, char **out_payload, uint32_t *out_length);
int ipc_command(int fd, uint32_t type, const char *payload,
                uint32_t *out_type, char **out_payload, uint32_t *out_length);
int ipc_subscribe_windows(int fd);
int ipc_set_split(int fd, const char *split_cmd);
int ipc_send_client_command(const char *cmd);
int ipc_create_server_socket(void);

#endif /* SWAYTILE_H */
