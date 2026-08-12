/* tree.h */
#ifndef TREE_H
#define TREE_H

#include <stdint.h>
#include <stdbool.h>

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

Node *tree_parse(const char *json_str);
void tree_free(Node *root);
const Node *tree_find_focused(const Node *root);
int tree_count_workspace_layout_changes(const Node *root);
char *tree_parse_event_change(const char *payload_json);
void tree_dump(const Node *node, int depth);

#endif /* TREE_H */
