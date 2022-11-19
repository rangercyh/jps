#ifndef __JPS__
#define __JPS__

#include "heap.h"
#include "intlist.h"

struct map {
    int width;
    int height;
    int start;
    int end;
    int *comefrom;

//  for mark connected
    char mark_connected;
    int *connected;
    int *queue;
    char *visited;

    int *open_set_map;
    IntList *il;
    heap_t *h;

    int path_type;
/*
    [map] | [close_set] | [path]
*/
    char m[0];
};

struct map *jps_create(int w, int h);
void jps_init(struct map *m, int w, int h, int map_men_len);
void jps_destory(struct map *m);
int jps_get_memory_len(int len);

int jps_set_obstacle(struct map *m, int x, int y, int bit);
void jps_clearall_obs(struct map *m);
int jps_set_start(struct map *m, int x, int y);
int jps_set_end(struct map *m, int x, int y);
void jps_mark_connected(struct map *m);
int jps_path_finding(struct map *m, int type, IntList *path);

void jps_dump_connected(struct map *m);
void jps_dump(struct map *m);

#endif // __JPS__
