#ifndef LAYOUT_MAP_H
#define LAYOUT_MAP_H

#include <stdbool.h>
#include "tree.h"

typedef struct WorkspaceState {
    char *ws_name;
    bool flattened;
    Node *saved_tree;
    char last_split[8];       /* "splith" or "splitv" */
    int split_changes;        /* Number of times direction has alternated */
    struct WorkspaceState *next;
} WorkspaceState;

void layout_map_init(void);
void layout_map_cleanup(void);

bool layout_map_is_flattened(const char *ws_name);
void layout_map_set_flattened(const char *ws_name, bool flattened);

/* Save and retrieve the pre-flattened tree structure */
void layout_map_save_tree(const char *ws_name, const Node *ws_node);
Node *layout_map_get_tree(const char *ws_name);
void layout_map_clear_tree(const char *ws_name);
bool layout_map_check_and_update_split(const char *ws_name, const char *new_split, int limit);
const char *layout_map_get_last_split(const char *ws_name);
void layout_map_set_verbose(bool verbose);

#endif /* LAYOUT_MAP_H */
