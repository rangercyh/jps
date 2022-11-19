#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jps.h"

void main() {
    int w = 20; int h = 20;
    struct map *m = jps_create(w, h);

    for (int i = 0; i < w; i++) {
        jps_set_obstacle(m, i, 5, 1);
        jps_set_obstacle(m, i, 15, 1);
    }
    for (int j = 0; j < h; j++) {
        jps_set_obstacle(m, 5, j, 1);
        jps_set_obstacle(m, 15, j, 1);
    }

    jps_set_obstacle(m, 5, 3, 0);
    jps_set_obstacle(m, 15, 3, 0);
    jps_set_obstacle(m, 2, 5, 0);
    jps_set_obstacle(m, 7, 5, 0);
    jps_set_obstacle(m, 17, 5, 0);
    jps_set_obstacle(m, 5, 10, 0);
    jps_set_obstacle(m, 15, 10, 0);
    jps_set_obstacle(m, 2, 15, 0);
    jps_set_obstacle(m, 8, 15, 0);
    jps_set_obstacle(m, 18, 15, 0);
    jps_set_obstacle(m, 5, 18, 0);
    jps_set_obstacle(m, 15, 18, 0);

    jps_mark_connected(m);
    jps_dump_connected(m);

    jps_set_start(m, 1, 1);
    jps_set_end(m, 18, 18);

    IntList *il = il_create(2);
    jps_path_finding(m, 1, il);
    jps_dump(m);

    jps_destory(m);
    il_destroy(il);
    free(m);
}
