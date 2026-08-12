#include "layout_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct WorkspaceState {
    char *ws_name;
    bool flattened;
    Node *saved_tree; /* Stores the nested container hierarchy */
    struct WorkspaceState *next;
} WorkspaceState;

static WorkspaceState *map_head = NULL;

static Node *node_copy(const Node *src) {
    if (!src) return NULL;

    Node *dst = calloc(1, sizeof(Node));
    if (!dst) return NULL;

    dst->id = src->id;
    if (src->name) dst->name = strdup(src->name);
    if (src->type) dst->type = strdup(src->type);
    dst->layout = src->layout;
    dst->focused = src->focused;
    dst->is_floating = src->is_floating;
    dst->is_fullscreen = src->is_fullscreen;
    dst->width = src->width;
    dst->height = src->height;

    if (src->num_children > 0) {
        dst->children = malloc(src->num_children * sizeof(Node *));
        dst->num_children = src->num_children;
        for (int i = 0; i < src->num_children; i++) {
            dst->children[i] = node_copy(src->children[i]);
            if (dst->children[i]) {
                dst->children[i]->parent = dst;
            }
        }
    }

    return dst;
}

void layout_map_init(void) {
    map_head = NULL;
}

void layout_map_cleanup(void) {
    WorkspaceState *curr = map_head;
    while (curr) {
        WorkspaceState *next = curr->next;
        free(curr->ws_name);
        if (curr->saved_tree) {
            tree_free(curr->saved_tree);
        }
        free(curr);
        curr = next;
    }
    map_head = NULL;
}

static WorkspaceState *get_or_create_node(const char *ws_name) {
    WorkspaceState *curr = map_head;
    while (curr) {
        if (strcmp(curr->ws_name, ws_name) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    WorkspaceState *node = calloc(1, sizeof(WorkspaceState));
    if (!node) return NULL;

    node->ws_name = strdup(ws_name);
    node->flattened = false;
    node->saved_tree = NULL;
    node->next = map_head;
    map_head = node;

    return node;
}

bool layout_map_is_flattened(const char *ws_name) {
    if (!ws_name) return false;
    WorkspaceState *node = get_or_create_node(ws_name);
    return node ? node->flattened : false;
}

void layout_map_set_flattened(const char *ws_name, bool flattened) {
    if (!ws_name) return;
    WorkspaceState *node = get_or_create_node(ws_name);
    if (node && node->flattened != flattened) {
        node->flattened = flattened;
        fprintf(stderr, "[SAVED STATE] Workspace '%s' -> flattened = %s\n",
                ws_name, flattened ? "TRUE" : "FALSE");
    }
}

void layout_map_save_tree(const char *ws_name, const Node *ws_node) {
    if (!ws_name || !ws_node) return;
    WorkspaceState *node = get_or_create_node(ws_name);
    if (!node) return;

    if (node->saved_tree) {
        tree_free(node->saved_tree);
    }
    node->saved_tree = node_copy(ws_node);

    /* Print saved tree details */
    fprintf(stderr, "\n--- [SAVED TREE SNAPSHOT FOR WS '%s'] ---\n", ws_name);
    tree_dump(node->saved_tree, 0);
    fprintf(stderr, "-------------------------------------------\n\n");
}

Node *layout_map_get_tree(const char *ws_name) {
    if (!ws_name) return NULL;
    WorkspaceState *curr = map_head;
    while (curr) {
        if (strcmp(curr->ws_name, ws_name) == 0) {
            return curr->saved_tree;
        }
        curr = curr->next;
    }
    return NULL;
}

void layout_map_clear_tree(const char *ws_name) {
    if (!ws_name) return;
    WorkspaceState *curr = map_head;
    while (curr) {
        if (strcmp(curr->ws_name, ws_name) == 0) {
            if (curr->saved_tree) {
                tree_free(curr->saved_tree);
                curr->saved_tree = NULL;
                fprintf(stderr, "[CLEARED TREE] Freed saved layout tree for workspace '%s'\n", ws_name);
            }
            return;
        }
        curr = curr->next;
    }
}
