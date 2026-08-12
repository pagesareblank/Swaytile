#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "swaytile.h"
#include "tree.h"
#include "layout_map.h"
#include "flatten.h"

#define MAX_ALLOWED_WORKSPACES 32

static int split_limit = 0; /* 0 means unlimited */
static char *allowed_workspaces[MAX_ALLOWED_WORKSPACES];
static int allowed_workspace_count = 0;

static void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("Options:\n");
    printf("  -t, --toggle <MODE>  Change or toggle workspace layout mode:\n");
    printf("                       - toggle  : Toggle between tabbed and dynamic autotiling\n");
    printf("                       - tabbed  : Flatten windows into a single tabbed row\n");
    printf("                       - stacked : Flatten windows into a single stacked column (or 'stacking')\n");
    printf("                       - split   : Restore dynamic autotiling (or 'default')\n");
    printf("  -w, --workspace <WS> Limit autotiling to specific workspace(s)\n");
    printf("  -h, --help           Display this help message and exit\n");
}

static void add_allowed_workspace(const char *ws_name) {
    if (allowed_workspace_count < MAX_ALLOWED_WORKSPACES) {
        allowed_workspaces[allowed_workspace_count++] = strdup(ws_name);
    }
}

static void parse_workspace_arg(const char *arg) {
    char *copy = strdup(arg);
    char *token = strtok(copy, ",");
    while (token != NULL) {
        add_allowed_workspace(token);
        token = strtok(NULL, ",");
    }
    free(copy);
}

static bool is_workspace_allowed(const Node *focused) {
    if (allowed_workspace_count == 0) {
        return true;
    }

    const Node *ws = focused;
    while (ws) {
        if (ws->type && strcmp(ws->type, "workspace") == 0) {
            break;
        }
        ws = ws->parent;
    }

    if (!ws || !ws->name) return false;

    for (int i = 0; i < allowed_workspace_count; i++) {
        if (strcmp(ws->name, allowed_workspaces[i]) == 0) {
            return true;
        }
    }

    return false;
}

const char *calculate_next_split(const Node *root, const Node *focused) {
    (void)root;

    if (!focused || focused->is_floating) return NULL;

    if (!is_workspace_allowed(focused)) {
        return NULL;
    }

    const Node *ws = focused;
    while (ws && (ws->type == NULL || strcmp(ws->type, "workspace") != 0)) {
        ws = ws->parent;
    }

    if (ws && ws->name) {
        bool actual_is_flattened = (ws->layout == LAYOUT_TABBED || 
                                    ws->layout == LAYOUT_STACKED || 
                                    ws->layout == LAYOUT_STACKING);

        /* 1. If Sway tree confirms workspace is tabbed/stacked, keep map as TRUE */
        if (actual_is_flattened) {
            layout_map_set_flattened(ws->name, true);
            return NULL;
        }

        /* 2. If layout_map was set to TRUE by toggle command, suppress autotile */
        if (layout_map_is_flattened(ws->name)) {
            return NULL;
        }
    }

    if (focused->width > focused->height) {
        return "splith";
    } else {
        return "splitv";
    }
}

int main(int argc, char *argv[]) {
    int opt;

    static struct option long_options[] = {
        {"limit",         required_argument, 0, 'l'},
        {"workspace",     required_argument, 0, 'w'},
        {"toggle-layout", required_argument, 0, 't'},
        {"help",          no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "l:w:t:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'l':
                split_limit = atoi(optarg);
                if (split_limit <= 0) {
                    fprintf(stderr, "Error: Limit must be a positive integer.\n");
                    return 1;
                }
                break;
            case 'w':
                parse_workspace_arg(optarg);
                break;
            case 't':
                if (ipc_send_client_command(optarg) == 0) {
                    return 0;
                } else {
                    fprintf(stderr, "Error: swaytile daemon is not running.\n");
                    return 1;
                }
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    layout_map_init();

    int sway_fd = ipc_connect();
    if (sway_fd < 0) {
        fprintf(stderr, "Failed to connect to Sway IPC\n");
        return 1;
    }

    if (ipc_subscribe_windows(sway_fd) < 0) {
        fprintf(stderr, "Failed to subscribe to window events\n");
        close(sway_fd);
        return 1;
    }

    int server_fd = ipc_create_server_socket();
    if (server_fd < 0) {
        fprintf(stderr, "Failed to create control socket server\n");
        close(sway_fd);
        return 1;
    }

    printf("swaytile daemon active.\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sway_fd, &readfds);
        FD_SET(server_fd, &readfds);

        int max_fd = (sway_fd > server_fd) ? sway_fd : server_fd;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            break;
        }

        /* Handle client IPC toggle commands */
        if (FD_ISSET(server_fd, &readfds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            if (client_fd >= 0) {
                char mode[64] = {0};
                ssize_t n = read(client_fd, mode, sizeof(mode) - 1);
                close(client_fd);

                if (n > 0) {
                    mode[n] = '\0';
                    char *tree_json = NULL;

                    if (ipc_command(sway_fd, IPC_GET_TREE, NULL, NULL, &tree_json, NULL) == 0) {
                        Node *root = tree_parse(tree_json);
                        free(tree_json);

                        if (root) {
                            char *toggle_cmd = flatten_build_toggle_cmd(root, mode);
                            if (toggle_cmd && strlen(toggle_cmd) > 0) {
                                fprintf(stderr, "[TOGGLE EXEC] Mode '%s' -> Sending command: %s\n", mode, toggle_cmd);
                                ipc_command(sway_fd, IPC_RUN_COMMAND, toggle_cmd, NULL, NULL, NULL);
                                free(toggle_cmd);
                            }
                            tree_free(root);
                        }
                    }
                }
            }
        }

        /* Handle Sway window events */
        if (FD_ISSET(sway_fd, &readfds)) {
            uint32_t type = 0;
            char *payload = NULL;
            uint32_t length = 0;

            if (ipc_read(sway_fd, &type, &payload, &length) < 0) {
                break;
            }

            uint32_t event_type = MASK_EVENT_TYPE(type);

            if (event_type == IPC_EVENT_WINDOW) {
                char *change = tree_parse_event_change(payload);

                if (change && (strcmp(change, "focus") == 0 || strcmp(change, "new") == 0)) {
                    char *tree_json = NULL;

                    if (ipc_command(sway_fd, IPC_GET_TREE, NULL, NULL, &tree_json, NULL) == 0) {
                        Node *root = tree_parse(tree_json);
                        free(tree_json);

                        if (root) {
                            const Node *focused = tree_find_focused(root);

                            if (focused && !focused->is_floating && !focused->is_fullscreen) {
                                const char *cmd = calculate_next_split(root, focused);

                                if (cmd) {
                                    ipc_set_split(sway_fd, cmd);
                                }
                            }
                            tree_free(root);
                        }
                    }
                }

                free(change);
            }

            free(payload);
        }
    }

    close(server_fd);
    close(sway_fd);
    layout_map_cleanup();
    return 0;
}
