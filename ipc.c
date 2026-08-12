#include "swaytile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>

#define IPC_MAGIC "i3-ipc"

#define IPC_RUN_COMMAND 0
#define IPC_GET_WORKSPACES 1
#define IPC_SUBSCRIBE 2
#define IPC_GET_OUTPUTS 3
#define IPC_GET_TREE 4
#define IPC_GET_MARKS 5
#define IPC_GET_BAR_CONFIG 6
#define IPC_GET_VERSION 7
#define IPC_GET_BINDING_MODES 8
#define IPC_GET_CONFIG 9
#define IPC_SEND_TICK 10
#define SOCKET_PATH "/tmp/swaytile.sock"

int ipc_create_server_socket(void) {
    unlink(SOCKET_PATH);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    listen(fd, 5);
    return fd;
}

int ipc_send_client_command(const char *cmd) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    write(fd, cmd, strlen(cmd));
    close(fd);
    return 0;
}

int ipc_set_split(int fd, const char *split_cmd)
{
    if (!split_cmd) return -1;

    uint32_t resp_type = 0;
    char *resp = NULL;

    /* IPC_RUN_COMMAND is payload type 0 */
    if (ipc_command(fd, IPC_RUN_COMMAND, split_cmd, &resp_type, &resp, NULL) < 0) {
        return -1;
    }

    free(resp);
    return 0;
}

int ipc_subscribe_windows(int fd)
{
    /* Sway expects a JSON array of string event names */
    const char *payload = "[\"window\"]";
    uint32_t resp_type = 0;
    char *resp = NULL;

    if (ipc_command(fd, IPC_SUBSCRIBE, payload, &resp_type, &resp, NULL) < 0) {
        return -1;
    }

    /* Verify {"success": true} */
    bool success = (resp && strstr(resp, "\"success\": true") != NULL);
    free(resp);

    return success ? 0 : -1;
}

int ipc_command(int fd, uint32_t type, const char *payload,
                uint32_t *out_type, char **out_payload, uint32_t *out_length)
{
    uint32_t payload_len = payload ? (uint32_t)strlen(payload) : 0;

    if (ipc_send(fd, type, payload, payload_len) < 0) {
        return -1;
    }

    uint32_t resp_type = 0;
    uint32_t resp_len = 0;
    char *resp_payload = NULL;

    if (ipc_read(fd, &resp_type, &resp_payload, &resp_len) < 0) {
        return -1;
    }

    if (out_type)    *out_type = resp_type;
    if (out_length)  *out_length = resp_len;
    if (out_payload) *out_payload = resp_payload;
    else             free(resp_payload);

    return 0;
}

int ipc_connect(void)
{
    const char *socket_path = getenv("SWAYSOCK");

    if (socket_path == NULL) {
        fprintf(stderr, "SWAYSOCK is not set\n");
        return -1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;

    if (strlen(socket_path) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "Sway socket path is too long\n");
        close(fd);
        return -1;
    }

    strcpy(addr.sun_path, socket_path);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

int ipc_send(int fd, uint32_t type, const void *payload, uint32_t length)
{
    uint8_t header[14];

    memcpy(header, "i3-ipc", 6);
    memcpy(header + 6, &length, sizeof(length));
    memcpy(header + 10, &type, sizeof(type));

    if (write(fd, header, sizeof(header)) != sizeof(header)) {
        return -1;
    }

    if (length > 0) {
        if (write(fd, payload, length) != length) {
            return -1;
        }
    }

    return 0;
}

static int read_full(int fd, void *buffer, size_t size)
{
    size_t total = 0;

    while (total < size) {
        ssize_t n = read(fd, (char *)buffer + total, size - total);

        if (n == 0) {
            return -1;
        }

        if (n < 0) {
            return -1;
        }

        total += n;
    }

    return 0;
}

int ipc_read(int fd, uint32_t *type, char **payload, uint32_t *length)
{
    uint8_t header[14];

    if (read_full(fd, header, sizeof(header)) == -1) {
        return -1;
    }

    if (memcmp(header, "i3-ipc", 6) != 0) {
        fprintf(stderr, "Invalid IPC magic\n");
        return -1;
    }

    memcpy(length, header + 6, sizeof(*length));
    memcpy(type, header + 10, sizeof(*type));

    *payload = malloc(*length + 1);

    if (*payload == NULL) {
        return -1;
    }

    if (read_full(fd, *payload, *length) == -1) {
        free(*payload);
        *payload = NULL;
        return -1;
    }

    (*payload)[*length] = '\0';

    return 0;
}
