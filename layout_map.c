#include "layout_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static WorkspaceState *map_head = NULL;

static bool verbose_logging = false;

void layout_map_set_verbose(bool verbose) {
    verbose_logging = verbose;
}

static Node *node_copy(const Node *src) {
    if (!src) return NULL;

    /* If this is an intermediate layout container (type 'con', name 'NULL' or similar) 
       that only has 1 child, skip (collapse) it entirely to avoid nesting bloat! */
    if (src->type && strcmp(src->type, "con") == 0 && 
        (!src->name || strcmp(src->name, "NULL") == 0) && 
        src->num_children == 1) {
        
        /* Recursively copy its single child instead of creating this wrapper node */
        return node_copy(src->children[0]);
    }

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
        /* Filter out any single-child redundancies within children lists */
        dst->children = malloc(src->num_children * sizeof(Node *));
        int real_count = 0;
        
        for (int i = 0; i < src->num_children; i++) {
            Node *copied_child = node_copy(src->children[i]);
            if (copied_child) {
                dst->children[real_count] = copied_child;
                copied_child->parent = dst;
                real_count++;
            }
        }
        dst->num_children = real_count;
        
        /* If a node ended up with 0 children after filtering, handle safely */
        if (dst->num_children == 0) {
            free(dst->children);
            dst->children = NULL;
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
        if (verbose_logging) {
            fprintf(stderr, "[SAVED STATE] Workspace '%s' -> flattened = %s\n",
                    ws_name, flattened ? "TRUE" : "FALSE");
        }
    }
}

void layout_map_save_tree(const char *ws_name, const Node *ws_node) {
    if (!ws_name || !ws_node) return;
    WorkspaceState *node = get_or_create_node(ws_name);
    if (!node) return;

    if (node->saved_tree != NULL) return;

    Node *copied = node_copy(ws_node);

    if (copied && copied->num_children == 1 && 
        copied->children[0]->type && strcmp(copied->children[0]->type, "con") == 0 &&
        (!copied->children[0]->name || strcmp(copied->children[0]->name, "NULL") == 0)) {
        
        Node *old_root = copied;
        copied = old_root->children[0];
        
        copied->id = old_root->id;
        if (old_root->name) {
            free(copied->name);
            copied->name = strdup(old_root->name);
        }
        if (old_root->type) {
            free(copied->type);
            copied->type = strdup(old_root->type);
        }
        copied->parent = NULL;

        free(old_root->children);
        free(old_root);
    }

    node->saved_tree = copied;

    if (verbose_logging) {
        fprintf(stderr, "\n--- [SAVED TREE SNAPSHOT FOR WS '%s'] ---\n", ws_name);
        tree_dump(node->saved_tree, 0);
        fprintf(stderr, "-------------------------------------------\n\n");
    }
}

void layout_map_clear_tree(const char *ws_name) {
    if (!ws_name) return;
    WorkspaceState *curr = map_head;
    while (curr) {
        if (strcmp(curr->ws_name, ws_name) == 0) {
            if (curr->saved_tree) {
                tree_free(curr->saved_tree);
                curr->saved_tree = NULL;
                if (verbose_logging) {
                    fprintf(stderr, "[CLEARED TREE] Freed saved layout tree for workspace '%s'\n", ws_name);
                }
            }
            return;
        }
        curr = curr->next;
    }
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

const char *layout_map_get_last_split(const char *ws_name) {
    if (!ws_name) return "splitv"; // Safe fallback default
    WorkspaceState *node = get_or_create_node(ws_name);
    if (node && node->last_split[0] != '\0') {
        return node->last_split;
    }
    return "splitv"; // Default fallback if none recorded yet
}

bool layout_map_check_and_update_split(const char *ws_name, const char *new_split, int limit) {
    if (!ws_name || !new_split) return true;
    WorkspaceState *node = get_or_create_node(ws_name);
    if (!node) return true;

    /* If limit is 0 or unlimited, always allow */
    if (limit <= 0) return true;

    /* If this is the first split, record it without counting a change */
    if (node->last_split[0] == '\0') {
        strncpy(node->last_split, new_split, sizeof(node->last_split) - 1);
        node->split_changes = 0;
        return true;
    }

    /* If the direction changed, increment the change counter */
    if (strcmp(node->last_split, new_split) != 0) {
        if (node->split_changes >= limit) {
            return false; /* Cap reached! Disallow further direction changes */
        }
        node->split_changes++;
        strncpy(node->last_split, new_split, sizeof(node->last_split) - 1);
    }

    return true;
}
