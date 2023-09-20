#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#include "jps.h"

#ifdef DEBUG
#include <stdio.h>
#define print(format,...) printf(format, ##__VA_ARGS__)
#else
#define print(format,...)
#endif

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))

enum {
    NODE_POS = 0,
    NODE_G,
    NODE_F,
    NODE_DIR,
    NODE_SIZE
};

enum {
    TYPE_INIT = 0,
    OBS_CONNER_OK = 1,
    OBS_CONNER_AVOID,
    TYPE_NUM
};

int jps_get_memory_len(int len) {
#ifdef DEBUG
    return (BITSLOT(len) + 1) * 3;
#else
    return (BITSLOT(len) + 1) * 2;
#endif
}

static inline int check_in_map(int x, int y, int w, int h) {
    return x >= 0 && y >= 0 && x < w && y < h;
}

static inline int check_in_map_pos(int pos, int limit) {
    return pos >= 0 && pos < limit;
}

int jps_set_obstacle(struct map *m, int x, int y, int bit) {
    if (!check_in_map(x, y, m->width, m->height)) {
        return -1;
    }
    if (bit) {
        BITSET(m->m, m->width * y + x);
    } else {
        BITCLEAR(m->m, m->width * y + x);
    }
    m->mark_connected = 0;
#ifdef DEBUG
    int len = m->width * m->height;
    memset(&m->m[(BITSLOT(len) + 1) * 2], 0, (BITSLOT(len) + 1) * sizeof(m->m[0]));
#endif //DEBUG
    return 0;
}

int jps_is_obstacle(struct map *m, int x, int y) {
    int pos = m->width * y + x;
    if (!check_in_map(x, y, m->width, m->height)) {
        return -1;
    }
    if (BITTEST(m->m, pos)) {
        return 1;
    } else {
        return 0;
    }
}

void jps_clearall_obs(struct map *m) {
    int i;
    for (i = 0; i < m->width * m->height; i++) {
        BITCLEAR(m->m, i);
    }
    m->mark_connected = 0;
#ifdef DEBUG
    int len = m->width * m->height;
    memset(&m->m[(BITSLOT(len) + 1) * 2], 0, (BITSLOT(len) + 1) * sizeof(m->m[0]));
#endif //DEBUG
}

int jps_set_start(struct map *m, int x, int y) {
    if (!check_in_map(x, y, m->width, m->height)) {
        return -1;
    }
    int pos = m->width * y + x;
    if (BITTEST(m->m, pos)) {
        return -1;
    }
    m->start = pos;
#ifdef DEBUG
    int len = m->width * m->height;
    memset(&m->m[(BITSLOT(len) + 1) * 2], 0, (BITSLOT(len) + 1) * sizeof(m->m[0]));
#endif //DEBUG
    return 0;
}

int jps_set_end(struct map *m, int x, int y) {
    if (!check_in_map(x, y, m->width, m->height)) {
        return -1;
    }
    int pos = m->width * y + x;
    if (BITTEST(m->m, pos)) {
        return -1;
    }
    m->end = pos;
#ifdef DEBUG
    int len = m->width * m->height;
    memset(&m->m[(BITSLOT(len) + 1) * 2], 0, (BITSLOT(len) + 1) * sizeof(m->m[0]));
#endif //DEBUG
    return 0;
}

static int dist(int one, int two, int w) {
    int ex = one % w, ey = one / w;
    int px = two % w, py = two / w;
    int dx = ex - px, dy = ey - py;
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    if (dx < dy) {
        return dx *  7 + (dy-dx) * 5;
    }
    else {
        return dy * 7 + (dx - dy) * 5;
    }
}

static void add_to_openset(struct map *m, int pos, int g_value, unsigned char dir) {
    int idx = il_push_back(m->il);
    il_set(m->il, idx, NODE_POS, pos);
    il_set(m->il, idx, NODE_G, g_value);
    il_set(m->il, idx, NODE_F, g_value + dist(m->end, pos, m->width));
    il_set(m->il, idx, NODE_DIR, dir);
    heap_insert(&m->h, idx);
    m->open_set_map[pos] = idx;
}

static void insert_mid_jump_point(struct map *m, int cur, int father, int w, IntList *out) {
    int dx = cur % w - father % w;
    int dy = cur / w - father / w;
    if (dx == 0 || dy == 0) {
        return;
    }
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    if (dx == dy) {
        return;
    }
    int span = dx;
    if (dy < dx) {
        span = dy;
    }
    int mx = 0, my = 0;
    if (cur % w < father % w && cur / w < father / w) {
        mx = father % w - span;
        my = father / w - span;
    } else if (cur % w < father % w && cur / w > father / w) {
        mx = father % w - span;
        my = father / w + span;
    } else if (cur % w > father % w && cur / w < father / w) {
        mx = father % w + span;
        my = father / w - span;
    } else if (cur % w > father % w && cur / w > father / w) {
        mx = father % w + span;
        my = father / w + span;
    }
#ifdef DEBUG
    int len = m->width * m->height;
    BITSET(m->m, (BITSLOT(len) + 1) * 2 * CHAR_BIT + mx + my * w);
#endif // DEBUG
    int idx = il_push_back(out);
    il_set(out, idx, 0, mx);
    il_set(out, idx, 1, my);
}

static void form_path(int last, struct map *m, IntList *out) {
    int pos = last;
    int x, y;
    int w = m->width;
#ifdef DEBUG
    int len = m->width * m->height;
    memset(&m->m[(BITSLOT(len) + 1) * 2], 0, (BITSLOT(len) + 1) * sizeof(m->m[0]));
#endif //DEBUG
    int idx;
    while (m->comefrom[pos] != -1) {
#ifdef DEBUG
        BITSET(m->m, (BITSLOT(len) + 1) * 2 * CHAR_BIT + pos);
#endif //DEBUG
        x = pos % w;
        y = pos / w;
        idx = il_push_back(out);
        il_set(out, idx, 0, x);
        il_set(out, idx, 1, y);
        insert_mid_jump_point(m, pos, m->comefrom[pos], w, out);
        pos = m->comefrom[pos];
    }
}

#define NO_DIRECTION 8
#define FULL_DIRECTIONSET 255
#define EMPTY_DIRECTIONSET 0

// N, NE, E, SE, S, SW, W, NW
/*
   7  0  1
    \ | /
  6 -   - 2
    / | \
   5  4  3
*/

static inline void dir_add(unsigned char *dirs, unsigned char dir)
{
    *dirs |= (1 << dir);
}

static inline int dir_is_diagonal(unsigned char dir)
{
    return (dir % 2) != 0;
}

static inline int map_walkable(int pos, int limit, struct map *m) {
    return check_in_map_pos(pos, limit) && !BITTEST(m->m, pos);
}

static int get_next_pos(int pos, unsigned char dir, int w, int h) {
    int x = pos % w;
    int y = pos / w;
    switch (dir) {
        case 0:
            y = y - 1;
            break;
        case 1:
            x = x + 1;
            y = y - 1;
            break;
        case 2:
            x = x + 1;
            break;
        case 3:
            x = x + 1;
            y = y + 1;
            break;
        case 4:
            y = y + 1;
            break;
        case 5:
            x = x - 1;
            y = y + 1;
            break;
        case 6:
            x = x - 1;
            break;
        case 7:
            x = x - 1;
            y = y - 1;
            break;
        default: return -1;
    }
    if (!check_in_map(x, y, w, h)) {
        return -1;
    }
    return x + y * w;
}

static inline int walkable(struct map *m, int pos, int cur_dir, int next_dir) {
    return map_walkable(
        get_next_pos(pos, (cur_dir + (next_dir)) % 8, m->width, m->height),
        m->width * m->height, m);
}

static unsigned char natural_dir(int pos, unsigned char cur_dir, struct map *m) {
    unsigned char dir_set = EMPTY_DIRECTIONSET;
    if (cur_dir == NO_DIRECTION) {
        return FULL_DIRECTIONSET;
    }
    if (m->path_type == OBS_CONNER_AVOID) {
        if (dir_is_diagonal(cur_dir)) {
            if (!walkable(m, pos, cur_dir, 7)) {
                dir_add(&dir_set, (cur_dir + 1) % 8);
            } else if (!walkable(m, pos, cur_dir, 1)) {
                dir_add(&dir_set, (cur_dir + 7) % 8);
            } else {
                dir_add(&dir_set, cur_dir);
                dir_add(&dir_set, (cur_dir + 1) % 8);
                dir_add(&dir_set, (cur_dir + 7) % 8);
            }
        } else {
            dir_add(&dir_set, cur_dir);
        }
    } else {
        dir_add(&dir_set, cur_dir);
        if (dir_is_diagonal(cur_dir)) {
            dir_add(&dir_set, (cur_dir + 1) % 8);
            dir_add(&dir_set, (cur_dir + 7) % 8);
        }
    }
    return dir_set;
}

static unsigned char force_dir(int pos, unsigned char cur_dir, struct map *m) {
    if (cur_dir == NO_DIRECTION) {
        return EMPTY_DIRECTIONSET;
    }
    unsigned char dir_set = EMPTY_DIRECTIONSET;
#define WALKABLE(n) walkable(m, pos, cur_dir, n)
    if (m->path_type == OBS_CONNER_OK) {
        if (dir_is_diagonal(cur_dir)) {
            if (WALKABLE(6) && !WALKABLE(5)) {
                dir_add(&dir_set, (cur_dir + 6) % 8);
            }
            if (WALKABLE(2) && !WALKABLE(3)) {
                dir_add(&dir_set, (cur_dir + 2) % 8);
            }
        } else {
            if (WALKABLE(1) && !WALKABLE(2)) {
                dir_add(&dir_set, (cur_dir + 1) % 8);
            }
            if (WALKABLE(7) && !WALKABLE(6)) {
                dir_add(&dir_set, (cur_dir + 7) % 8);
            }
        }
    } else {
        if (!dir_is_diagonal(cur_dir)) {
            if (WALKABLE(2) && !(WALKABLE(3))) {
                dir_add(&dir_set, (cur_dir + 2) % 8);
                if (WALKABLE(1)) {
                    dir_add(&dir_set, (cur_dir + 1) % 8);
                }
            }
            if (WALKABLE(6) && !(WALKABLE(5))) {
                dir_add(&dir_set, (cur_dir + 6) % 8);
                if (WALKABLE(7)) {
                    dir_add(&dir_set, (cur_dir + 7) % 8);
                }
            }
        }
    }
#undef WALKABLE
    return dir_set;
}

static unsigned char next_dir(unsigned char *dirs) {
    unsigned char dir;
    for (dir = 0 ; dir < 8 ; dir++) {
        char dirBit = 1 << dir;
        if (*dirs & dirBit) {
            *dirs ^= dirBit;
            return dir;
        }
    }
    return NO_DIRECTION;
}

static void put_in_open_set(struct map *m, int pos, int len, int from, unsigned char dir) {
    if (!BITTEST(m->m, (BITSLOT(len) + 1) * CHAR_BIT + pos)) {
        int g_value = il_get(m->il, from, NODE_G);
        int f_pos = il_get(m->il, from, NODE_POS);
        int ng_value = g_value + dist(pos, f_pos, m->width);
        int p = m->open_set_map[pos];
        if (p < 0) {
            m->comefrom[pos] = f_pos;
            add_to_openset(m, pos, ng_value, dir);
        } else {
            int p_g_value = il_get(m->il, p, NODE_G);
            if (p_g_value > ng_value) {
                m->comefrom[pos] = f_pos;
                int p_f_value = il_get(m->il, p, NODE_F);
                il_set(m->il, p, NODE_F, p_f_value - (p_g_value - ng_value));
                il_set(m->il, p, NODE_G, ng_value);
                il_set(m->il, p, NODE_DIR, dir);
                push_up(m->h, p);
            }
        }
    }
}

static int conner_ok_obs(struct map *m, int pos, int new_pos, unsigned char dir) {
    switch (dir) {
        case 1:
            return BITTEST(m->m, pos + 1) || BITTEST(m->m, new_pos - 1);
        case 3:
            return BITTEST(m->m, pos + 1) || BITTEST(m->m, new_pos - 1);
        case 5:
            return BITTEST(m->m, pos - 1) || BITTEST(m->m, new_pos + 1);
        case 7:
            return BITTEST(m->m, pos - 1) || BITTEST(m->m, new_pos + 1);
        default: return 0;
    }
    return 0;
}

static int jump_prune(int end, int pos, unsigned char dir, struct map *m, int from) {
    int w = m->width;
    int h = m->height;
    int len = w * h;
    int next_pos = get_next_pos(pos, dir, w, h);
    if (!map_walkable(next_pos, len, m)) {
        return 0;
    }
    if (m->path_type == OBS_CONNER_AVOID) {
        if (dir_is_diagonal(dir) && conner_ok_obs(m, pos, next_pos, dir)) { // conner solve OBS_CONNER_OK stop by obs
            return 0;
        }
    }
    if (next_pos == end) {
        put_in_open_set(m, next_pos, len, from, dir);
        return 1;
    }
    if (force_dir(next_pos, dir, m) != EMPTY_DIRECTIONSET) {
        put_in_open_set(m, next_pos, len, from, dir);
        return 0;
    }
    if (dir_is_diagonal(dir)) {
        int i;
        i = jump_prune(end, next_pos, (dir + 7) % 8, m, from);
        if (i == 1) {
            return 1;
        }
        i = jump_prune(end, next_pos, (dir + 1) % 8, m, from);
        if (i == 1) {
            return 1;
        }
    }
    return jump_prune(end, next_pos, dir, m, from);
}

static void flood_mark(struct map *m, int pos, int connected_num, int limit) {
    char *visited = m->visited;
    if (visited[pos]) {
        return;
    }
    int *queue = m->queue;
    memset(queue, 0, limit * sizeof(int));
    int pop_i = 0, push_i = 0;
    visited[pos] = 1;
    m->connected[pos] = connected_num;
    queue[push_i++] = pos;

#define CHECK_POS(n) do { \
    if (!BITTEST(m->m, n)) { \
        if (!visited[n]) { \
            visited[n] = 1; \
            m->connected[n] = connected_num; \
            queue[push_i++] = n; \
        } \
    } \
} while(0);
    int cur, left;
    while (pop_i < push_i) {
        cur = queue[pop_i++];
        left = cur % m->width;
        if (left != 0) {
            CHECK_POS(cur - 1);
        }
        if (left != m->width - 1) {
            CHECK_POS(cur + 1);
        }
        if (cur >= m->width) {
            CHECK_POS(cur - m->width);
        }
        if (cur < limit - m->width) {
            CHECK_POS(cur + m->width);
        }
    }
#undef CHECK_POS
}

void jps_mark_connected(struct map *m) {
    if (!m->mark_connected) {
        m->mark_connected = 1;
        int len = m->width * m->height;
        memset(m->connected, 0, len * sizeof(int));
        memset(m->visited, 0, len * sizeof(char));
        int i, connected_num = 0;
        for (i = 0; i < len; i++) {
            if (!m->visited[i] && !BITTEST(m->m, i)) {
                flood_mark(m, i, ++connected_num, len);
            }
        }
    }
}

static int compare(int a, int b, void *il) {
    il = (IntList *)il;
    int af = il_get(il, a, NODE_F);
    int bf = il_get(il, b, NODE_F);
    if (af < bf) {
        return 1;
    } else {
        return -1;
    }
}

int jps_path_finding(struct map *m, int type, IntList *out) {
    if (BITTEST(m->m, m->start)) {
        return 1;
    }
    if (BITTEST(m->m, m->end)) {
        return 2;
    }
    if (m->start == m->end) {
        return 0;
    }
    if (type <= TYPE_INIT || type >= TYPE_NUM) {
        return 3;
    }
    m->path_type = type;
    if (!m->mark_connected) {
        jps_mark_connected(m);
    }
    if (m->connected[m->start] != m->connected[m->end]) {
        return 4;
    }

    int len = m->width * m->height;
    // closeset clear
    memset(&m->m[BITSLOT(len) + 1], 0, (BITSLOT(len) + 1) * sizeof(m->m[0]));
    memset(m->comefrom, -1, len * sizeof(int));
    memset(m->open_set_map, -1, len * sizeof(int));
    il_clear(m->il);
    heap_init(m->h, compare, m->il);
    heap_clear(m->h);

    add_to_openset(m, m->start, 0, NO_DIRECTION);
    int cur, cpos, cdir;
    unsigned char check_dirs, dir;
    while ((cur = heap_pop(m->h)) >= 0) {
        cpos = il_get(m->il, cur, NODE_POS);
        m->open_set_map[cpos] = -1;
        BITSET(m->m, (BITSLOT(len) + 1) * CHAR_BIT + cpos);
        if (cpos == m->end) {
            form_path(cpos, m, out);
            return 0;
        }
        cdir = il_get(m->il, cur, NODE_DIR);
        check_dirs = natural_dir(cpos, cdir, m) | force_dir(cpos, cdir, m);
        dir = next_dir(&check_dirs);
        while (dir != NO_DIRECTION) {
            if (jump_prune(m->end, cpos, dir, m, cur) == 1) { // found end
                break;
            }
            dir = next_dir(&check_dirs);
        }
    }
    return 4;
}

void jps_dump_connected(struct map *m) {
#ifdef DEBUG
    print("dump map connected state!!!!!!\n");
    if (!m->mark_connected) {
        print("have not mark connected.\n");
        return;
    }
    int i;
    for (i = 0; i < m->width * m->height; i++) {
        int mark = m->connected[i];
        if (mark > 0) {
            print("%d ", mark);
        } else {
            print("* ");
        }
        if ((i + 1) % m->width == 0) {
            print("\n");
        }
    }
#endif // DEBUG
}

void jps_dump(struct map *m) {
#ifdef DEBUG
    print("dump map state!!!!!!\n");
    int i, pos;
    int len = m->width * m->height;
    char s[m->width * 2 + 2];
    for (pos = 0, i = 0; i < m->width * m->height; i++) {
        if (i > 0 && i % m->width == 0) {
            s[pos - 1] = '\0';
            print("%s\n", s);
            pos = 0;
        }
        int mark = 0;
        if (BITTEST(m->m, i)) {
            s[pos++] = '*';
            mark = 1;
        } else {
            if (BITTEST(m->m, (BITSLOT(len) + 1) * 2 * CHAR_BIT + i)) {
                s[pos++] = '0';
                mark = 1;
            }
        }
        if (i == m->start) {
            s[pos++] = 'S';
            mark = 1;
        }
        if (i == m->end) {
            s[pos++] = 'E';
            mark = 1;
        }
        if (mark) {
            s[pos++] = ' ';
        } else {
            s[pos++] = '.';
            s[pos++] = ' ';
        }
    }
    s[pos - 1] = '\0';
    print("%s\n", s);
#endif // DEBUG
}

struct map *jps_create(int w, int h) {
    assert(w > 0 && h > 0);
    int len = w * h;
    int map_men_len = jps_get_memory_len(len);
    struct map *m = (struct map *)malloc(sizeof(struct map) + map_men_len * sizeof(m->m[0]));
    jps_init(m, w, h, map_men_len);
    return m;
}

void jps_init(struct map *m, int w, int h, int map_men_len) {
    int len = w * h;
    m->width = w;
    m->height = h;
    m->start = -1;
    m->end = -1;
    m->mark_connected = 0;
    m->comefrom = (int *)malloc(len * sizeof(int));
    m->connected = (int *)malloc(len * sizeof(int));
    m->queue = (int *)malloc(len * sizeof(int));
    m->visited = (char *)malloc(len * sizeof(char));
    m->open_set_map = (int *)malloc(len * sizeof(int));
    m->il = il_create(NODE_SIZE);
    m->h = heap_new();
    m->path_type = OBS_CONNER_OK;
    memset(m->m, 0, map_men_len * sizeof(m->m[0]));
}

void jps_destory(struct map *m) {
    free(m->comefrom);
    free(m->connected);
    free(m->queue);
    free(m->visited);
    free(m->open_set_map);
    il_destroy(m->il);
    heap_free(m->h);
}
