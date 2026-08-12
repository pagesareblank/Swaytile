#ifndef SWAYTILE_H
#define SWAYTILE_H

#include <stdint.h>
#include <stdbool.h>

#define IPC_EVENT_BIT        (1U << 31)
#define IPC_EVENT_WORKSPACE  0
#define IPC_EVENT_OUTPUT     1
#define IPC_EVENT_MODE       2
#define IPC_EVENT_WINDOW     3

#define IS_IPC_EVENT(type)    (((type) & IPC_EVENT_BIT) != 0)
#define MASK_EVENT_TYPE(type) ((type) & ~IPC_EVENT_BIT)

#define IPC_RUN_COMMAND 0
#define IPC_GET_TREE    4
#define IPC_SUBSCRIBE   2

typedef enum {
    LAYOUT_UNKNOWN,
    LAYOUT_SPLITH,
    LAYOUT_SPLITV,
    LAYOUT_STACKED,
    LAYOUT_STACKING,
    LAYOUT_TABBED
} LayoutType;

typedef struct Node Node;
struct Node {
    int64_t id;
    char *type;
    char *name;
    LayoutType layout;
    double percent;
    bool focused;
    bool is_floating;
    bool is_fullscreen;
    int width;
    int height;
    Node *parent;
    Node **children;
    int num_children;
};

typedef struct WorkspaceState {
    char *ws_name;
    bool flattened;
    Node *saved_tree;
    char last_split[8];
    int split_changes;
    struct WorkspaceState *next;
} WorkspaceState;

int ipc_connect(void);
int ipc_send(int fd, uint32_t type, const void *payload, uint32_t length);
int ipc_read(int fd, uint32_t *out_type, char **out_payload, uint32_t *out_length);
int ipc_command(int fd, uint32_t type, const char *payload,
                uint32_t *out_type, char **out_payload, uint32_t *out_length);
int ipc_subscribe_windows(int fd);
int ipc_set_split(int fd, const char *split_cmd);
int ipc_send_client_command(const char *cmd);
int ipc_create_server_socket(void);


char *flatten_build_toggle_cmd(const Node *root, const char *mode);


void layout_map_init(void);
void layout_map_cleanup(void);
bool layout_map_is_flattened(const char *ws_name);
void layout_map_set_flattened(const char *ws_name, bool flattened);
void layout_map_save_tree(const char *ws_name, const Node *ws_node);
Node *layout_map_get_tree(const char *ws_name);
void layout_map_clear_tree(const char *ws_name);
bool layout_map_check_and_update_split(const char *ws_name, const char *new_split, int limit);
const char *layout_map_get_last_split(const char *ws_name);
void layout_map_set_verbose(bool verbose);


Node *tree_parse(const char *json_str);
void tree_free(Node *root);
const Node *tree_find_focused(const Node *root);
int tree_count_workspace_layout_changes(const Node *root);
char *tree_parse_event_change(const char *payload_json);
void tree_dump(const Node *node, int depth);

#endif
