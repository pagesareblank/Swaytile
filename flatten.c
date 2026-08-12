#include "flatten.h"
#include "layout_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void collect_leaf_window_ids(const Node *node, int64_t *ids, int *count, int max_ids) {
    if (!node || *count >= max_ids) return;

    if (node->num_children == 0) {
        if (!node->is_floating && node->id > 0) {
            ids[(*count)++] = node->id;
        }
        return;
    }

    for (int i = 0; i < node->num_children; i++) {
        collect_leaf_window_ids(node->children[i], ids, count, max_ids);
    }
}

static int64_t get_first_leaf_id(const Node *node) {
    if (!node) return -1;
    if (node->num_children == 0 && !node->is_floating && node->id > 0) {
        return node->id;
    }
    for (int i = 0; i < node->num_children; i++) {
        int64_t id = get_first_leaf_id(node->children[i]);
        if (id > 0) return id;
    }
    return -1;
}

static void build_restore_commands_topdown(const Node *node, char *buf, size_t buf_size) {
    if (!node || node->num_children <= 1) return;

    int64_t anchor_id = get_first_leaf_id(node);
    if (anchor_id <= 0) return;

    const char *split_dir = (node->layout == LAYOUT_SPLITH || node->layout == 1) ? "splith" : "splitv";

    char cmd[512];
    
    snprintf(cmd, sizeof(cmd), "[con_id=%ld] focus; [con_id=%ld] %s; ", 
             (long)anchor_id, (long)anchor_id, split_dir);
    strncat(buf, cmd, buf_size - strlen(buf) - 1);

    char mark_name[64];
    snprintf(mark_name, sizeof(mark_name), "_swaytile_%ld", (long)node->id);

    snprintf(cmd, sizeof(cmd), "[con_id=%ld] focus; [con_id=%ld] mark --add \"%s\"; ", 
             (long)anchor_id, (long)anchor_id, mark_name);
    strncat(buf, cmd, buf_size - strlen(buf) - 1);

    for (int i = 1; i < node->num_children; i++) {
        int64_t child_anchor = get_first_leaf_id(node->children[i]);
        if (child_anchor <= 0) continue;

        snprintf(cmd, sizeof(cmd), "[con_id=%ld] move container to mark \"%s\"; ", 
                 (long)child_anchor, mark_name);
        strncat(buf, cmd, buf_size - strlen(buf) - 1);
    }

    snprintf(cmd, sizeof(cmd), "[con_id=%ld] focus; [con_id=%ld] unmark \"%s\"; ", 
             (long)anchor_id, (long)anchor_id, mark_name);
    strncat(buf, cmd, buf_size - strlen(buf) - 1);

    for (int i = 0; i < node->num_children; i++) {
        build_restore_commands_topdown(node->children[i], buf, buf_size);
    }
}

char *flatten_build_toggle_cmd(const Node *root, const char *mode) {
    if (!root || !mode) return NULL;

    const Node *focused = tree_find_focused(root);
    if (!focused) return NULL;

    const Node *ws = focused;
    while (ws && (ws->type == NULL || strcmp(ws->type, "workspace") != 0)) {
        ws = ws->parent;
    }

    if (!ws || !ws->name) return NULL;

    int64_t window_ids[256];
    int window_count = 0;
    collect_leaf_window_ids(ws, window_ids, &window_count, 256);

    if (window_count == 0) return NULL;

    bool currently_flattened = layout_map_is_flattened(ws->name);

    size_t buf_size = 16384;
    char *cmd_buf = malloc(buf_size);
    if (!cmd_buf) return NULL;
    cmd_buf[0] = '\0';

    if (strcmp(mode, "split") == 0 || strcmp(mode, "default") == 0 ||
       (strcmp(mode, "toggle") == 0 && currently_flattened)) {

        layout_map_set_flattened(ws->name, false);

        char reset_cmd[128];
        snprintf(reset_cmd, sizeof(reset_cmd), "[workspace=\"%s\"] layout splitv; ", ws->name);
        strncat(cmd_buf, reset_cmd, buf_size - strlen(cmd_buf) - 1);

        Node *saved_tree = layout_map_get_tree(ws->name);
        if (saved_tree) {
            build_restore_commands_topdown(saved_tree, cmd_buf, buf_size);
            
            layout_map_clear_tree(ws->name);
        }

        return cmd_buf;
    }

    const char *target_layout = (strcmp(mode, "stacked") == 0 || strcmp(mode, "stacking") == 0) 
                              ? "stacking" : "tabbed";

    if (!currently_flattened) {
        layout_map_save_tree(ws->name, ws);
    }

    layout_map_set_flattened(ws->name, true);

    for (int i = 0; i < window_count; i++) {
        char move_cmd[128];
        snprintf(move_cmd, sizeof(move_cmd), "[con_id=%ld] move container to workspace \"%s\"; ", 
                 (long)window_ids[i], ws->name);
        strncat(cmd_buf, move_cmd, buf_size - strlen(cmd_buf) - 1);
    }

    char layout_cmd[128];
    snprintf(layout_cmd, sizeof(layout_cmd), "[workspace=\"%s\"] layout %s", ws->name, target_layout);
    strncat(cmd_buf, layout_cmd, buf_size - strlen(cmd_buf) - 1);

    return cmd_buf;
}
