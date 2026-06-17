#include "balloc.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    char name[32];
} item_t;

int main(void) {
    ba_arena *scratch = ba_arena_create(64 * 1024);
    item_t *items = (item_t *)ba_arena_calloc(scratch, 4, sizeof(item_t));
    items[0].id = 1; strcpy(items[0].name, "alpha");
    items[1].id = 2; strcpy(items[1].name, "beta");
    for (int i = 0; i < 4; i++) {
        if (items[i].id) {
            printf("item %d: %s\n", items[i].id, items[i].name);
        }
    }
    ba_arena_reset(scratch);

    ba_pool *nodes = ba_pool_create(sizeof(item_t), 8);
    item_t *n = (item_t *)ba_pool_alloc(nodes);
    n->id = 99; strcpy(n->name, "from-pool");
    printf("pool: %d %s\n", n->id, n->name);
    ba_pool_free(nodes, n);
    ba_pool_destroy(nodes);
    ba_arena_destroy(scratch);
    return 0;
}
