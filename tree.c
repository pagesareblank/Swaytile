#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h"
#include "tree.h"

void tree_dump(const Node *node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) fprintf(stderr, "  ");

    fprintf(stderr, "Node[id=%ld, type='%s', name='%s', layout=%d, focused=%s, size=%dx%d]\n",
            (long)node->id,
            node->type ? node->type : "NULL",
            node->name ? node->name : "NULL",
            node->layout,
            node->focused ? "YES" : "no",
            node->width, node->height);

    for (int i = 0; i < node->num_children; i++) {
        tree_dump(node->children[i], depth + 1);
    }
}

static LayoutType parse_layout_string(const char *str) {
    if (!str) return LAYOUT_UNKNOWN;
    if (strcmp(str, "splith") == 0) return LAYOUT_SPLITH;
    if (strcmp(str, "splitv") == 0) return LAYOUT_SPLITV;
    if (strcmp(str, "stacked") == 0) return LAYOUT_STACKED;
    if (strcmp(str, "stacking") == 0) return LAYOUT_STACKING;
    if (strcmp(str, "tabbed") == 0) return LAYOUT_TABBED;
    return LAYOUT_UNKNOWN;
}

static Node *tree_parse_node_internal(const cJSON *json, Node *parent) {
    if (!json) return NULL;

    Node *node = calloc(1, sizeof(Node));
    if (!node) return NULL;

    node->parent = parent;

    cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id");
    if (cJSON_IsNumber(id)) node->id = (int64_t)id->valuedouble;

    cJSON *focused = cJSON_GetObjectItemCaseSensitive(json, "focused");
    if (cJSON_IsBool(focused)) node->focused = cJSON_IsTrue(focused);

    cJSON *percent = cJSON_GetObjectItemCaseSensitive(json, "percent");
    if (cJSON_IsNumber(percent)) node->percent = percent->valuedouble;

    cJSON *layout = cJSON_GetObjectItemCaseSensitive(json, "layout");
    if (cJSON_IsString(layout) && layout->valuestring) {
        node->layout = parse_layout_string(layout->valuestring);
    }

    cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (cJSON_IsString(type) && type->valuestring) {
        node->type = strdup(type->valuestring);
        if (strcmp(type->valuestring, "floating_con") == 0) {
            node->is_floating = true;
        }
    }

    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (cJSON_IsString(name) && name->valuestring) {
        node->name = strdup(name->valuestring);
    }

    cJSON *fullscreen_mode = cJSON_GetObjectItemCaseSensitive(json, "fullscreen_mode");
    if (cJSON_IsNumber(fullscreen_mode) && fullscreen_mode->valueint > 0) {
        node->is_fullscreen = true;
    }

    cJSON *rect = cJSON_GetObjectItemCaseSensitive(json, "rect");
    if (rect) {
        cJSON *w = cJSON_GetObjectItemCaseSensitive(rect, "width");
        cJSON *h = cJSON_GetObjectItemCaseSensitive(rect, "height");
        if (cJSON_IsNumber(w)) node->width = w->valueint;
        if (cJSON_IsNumber(h)) node->height = h->valueint;
    }

    cJSON *nodes = cJSON_GetObjectItemCaseSensitive(json, "nodes");
    if (cJSON_IsArray(nodes)) {
        node->num_children = cJSON_GetArraySize(nodes);
        if (node->num_children > 0) {
            node->children = calloc(node->num_children, sizeof(Node *));
            int i = 0;
            cJSON *child_json = NULL;
            cJSON_ArrayForEach(child_json, nodes) {
                node->children[i++] = tree_parse_node_internal(child_json, node);
            }
        }
    }

    cJSON *floating_nodes = cJSON_GetObjectItemCaseSensitive(json, "floating_nodes");
    if (cJSON_IsArray(floating_nodes) && cJSON_GetArraySize(floating_nodes) > 0) {
        cJSON *fchild = NULL;
        cJSON_ArrayForEach(fchild, floating_nodes) {
            Node *fnode = tree_parse_node_internal(fchild, node);
            if (fnode) fnode->is_floating = true;
        }
    }

    return node;
}

Node *tree_parse(const char *json_str) {
    if (!json_str) return NULL;
    cJSON *root_json = cJSON_Parse(json_str);
    if (!root_json) return NULL;

    Node *root = tree_parse_node_internal(root_json, NULL);
    cJSON_Delete(root_json);
    return root;
}

const Node *tree_find_focused(const Node *node) {
    if (!node) return NULL;
    if (node->focused) return node;

    for (int i = 0; i < node->num_children; i++) {
        const Node *target = tree_find_focused(node->children[i]);
        if (target) return target;
    }

    return NULL;
}

static void count_switches_in_subtree(const Node *node, LayoutType parent_layout, int *count) {
    if (!node) return;

    LayoutType current_layout = parent_layout;

    if (node->layout == LAYOUT_SPLITH || node->layout == LAYOUT_SPLITV) {
        if (parent_layout != LAYOUT_UNKNOWN && node->layout != parent_layout) {
            (*count)++;
        }
        current_layout = node->layout;
    }

    for (int i = 0; i < node->num_children; i++) {
        count_switches_in_subtree(node->children[i], current_layout, count);
    }
}

int tree_count_workspace_layout_changes(const Node *root) {
    const Node *focused = tree_find_focused(root);
    if (!focused) return 0;

    const Node *ws = focused;
    while (ws && ws->parent) {
        if (ws->type && strcmp(ws->type, "workspace") == 0) break;
        ws = ws->parent;
    }

    if (!ws) return 0;

    int total_changes = 0;
    count_switches_in_subtree(ws, LAYOUT_UNKNOWN, &total_changes);
    return total_changes;
}

void tree_free(Node *node) {
    if (!node) return;

    for (int i = 0; i < node->num_children; i++) {
        tree_free(node->children[i]);
    }

    free(node->type);
    free(node->name);
    free(node->children);
    free(node);
}

char *tree_parse_event_change(const char *json_str) {
    if (!json_str) return NULL;

    cJSON *root = cJSON_Parse(json_str);
    if (!root) return NULL;

    cJSON *change = cJSON_GetObjectItemCaseSensitive(root, "change");
    char *result = NULL;

    if (cJSON_IsString(change) && change->valuestring) {
        result = strdup(change->valuestring);
    }

    cJSON_Delete(root);
    return result;
}
