#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "fibheap.h"

#ifdef __DEEP_DEBUG__
    #define deep_print(format,...) printf(format, ##__VA_ARGS__)
#else
    #define deep_print(format,...)
#endif

#define MT_NAME ("_jps_search_metatable")

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))

struct map {
    int width;
    int height;
    int start;
    int end;
    int *comefrom;
    struct node_data **close_set;
    struct heap_node **open_set_map;
    char m[0];
};

static inline int
getfield(lua_State *L, const char *f) {
    if (lua_getfield(L, -1, f) != LUA_TNUMBER) {
        return luaL_error(L, "invalid type %s", f);
    }
    int v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

static inline int check_in_map(int x, int y, int w, int h) {
    return x >= 0 && y >= 0 && x < w && y < h;
}

static inline int check_in_map_pos(int pos, int limit) {
    return pos >= 0 && pos < limit;
}

static inline int
setobstacle(lua_State *L, struct map *m, int x, int y) {
    if (!check_in_map(x, y, m->width, m->height)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    BITSET(m->m, m->width * y + x);
    return 0;
}

static int
add_block(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (!check_in_map(x, y, m->width, m->height)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    BITSET(m->m, m->width * y + x);
    return 0;
}

static int
add_blockset(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_settop(L, 2);
    int i = 1;
    while (lua_geti(L, -1, i) == LUA_TTABLE) {
        lua_geti(L, -1, 1);
        int x = lua_tointeger(L, -1);
        lua_geti(L, -2, 2);
        int y = lua_tointeger(L, -1);
        setobstacle(L, m, x, y);
        lua_pop(L, 3);
        ++i;
    }
    return 0;
}

static int
clear_block(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (!check_in_map(x, y, m->width, m->height)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    BITCLEAR(m->m, m->width * y + x);
    return 0;
}

static int
clear_allblock(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int i;
    for (i = 0; i < m->width * m->height; i++) {
        BITCLEAR(m->m, i);
    }
    return 0;
}

static int
set_start(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (!check_in_map(x, y, m->width, m->height)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    int pos = m->width * y + x;
    if (BITTEST(m->m, pos)) {
        luaL_error(L, "Position (%d,%d) is in block", x, y);
    }
    m->start = pos;
    return 0;
}

static int
set_end(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (!check_in_map(x, y, m->width, m->height)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    int pos = m->width * y + x;
    if (BITTEST(m->m, pos)) {
        luaL_error(L, "Position (%d,%d) is in block", x, y);
    }
    m->end = pos;
    return 0;
}

int compare(struct node_data *old, struct node_data *new)
{
    if (new->f_value < old->f_value) {
        return 1;
    }
    else {
        return -1;
    }
}

static inline int dist(int one, int two, int w) {
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

struct node_data *construct(struct map *m, int pos, int g_value) {
    struct node_data *node = (struct node_data *)malloc(sizeof(struct node_data));
    node->pos = pos;
    node->g_value = g_value;
    node->f_value = g_value + dist(m->end, pos, m->width);
    return node;
}

static inline int
form_path(lua_State *L, int last, struct map *m) {
    int pos = last;
    lua_newtable(L);
    int num = 0;
    int x, y;
    int w = m->width;
#ifdef __PATH_DEBUG__
    int len = m->width * m->height;
#endif
    while (m->comefrom[pos] != -1) {
#ifdef __PATH_DEBUG__
        BITSET(m->m, pos + len);
#endif
        x = pos % w;
        y = pos / w;
        deep_print("jump path = %d %d\n", x, y);
        lua_newtable(L);
        lua_pushnumber(L, x);
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, y);
        lua_rawseti(L, -2, 2);
        lua_rawseti(L, -2, ++num);
        pos = m->comefrom[pos];
    }
    return 1;
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

static void dir_add(unsigned char *dirs, unsigned char dir)
{
    *dirs |= (1 << dir);
}

static int dir_is_diagonal(unsigned char dir)
{
    return (dir % 2) != 0;
}

static unsigned char calc_dir(int from, int to, int w) {
    if (from == -1) {
        return NO_DIRECTION;
    } else {
        int fx = from % w, fy = from / w;
        int tx = from % w, ty = from / w;
        if (fx == tx && fy > ty) {
            return 0;
        } else if (fx == tx && fy < ty) {
            return 4;
        } else if (fx > tx && fy == ty) {
            return 6;
        } else if (fx < tx && fy == ty) {
            return 2;
        } else if (fx < tx && fy > ty) {
            return 1;
        } else if (fx < tx && fy < ty) {
            return 3;
        } else if (fx > tx && fy > ty) {
            return 7;
        } else if (fx > tx && fy < ty) {
            return 5;
        } else {
            // error path
            return NO_DIRECTION;
        }
    }
}

static unsigned char natural_dir(unsigned char cur_dir) {
    unsigned char dir_set = EMPTY_DIRECTIONSET;
    if (cur_dir == NO_DIRECTION) {
        return FULL_DIRECTIONSET;
    }
    dir_add(&dir_set, cur_dir);
    if (dir_is_diagonal(cur_dir)) {
        dir_add(&dir_set, (cur_dir + 1) % 8);
        dir_add(&dir_set, (cur_dir + 7) % 8);
    }
    return dir_set;
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

static int
map_walkable(int pos, int limit, struct map *m) {
    return check_in_map_pos(pos, limit) && !BITTEST(m->m, pos);
}

static unsigned char force_dir(int pos, unsigned char cur_dir, struct map *m) {
    if (cur_dir == NO_DIRECTION) {
        return EMPTY_DIRECTIONSET;
    }
    unsigned char dir_set = EMPTY_DIRECTIONSET;
    int w = m->width;
    int h = m->height;
    int limit = w * h;
#define WALKABLE(n) map_walkable(get_next_pos(pos, (cur_dir + (n)) % 8, w, h), limit, m)
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
#undef WALKABLE
    return dir_set;
}

static unsigned char next_dir(unsigned char *dirs) {
    unsigned char dir;
    // first check 0 2 4 6
    for (dir = 0 ; dir < 8 ; dir++) {
        char dirBit = 1 << dir;
        if (*dirs & dirBit) {
            *dirs ^= dirBit;
            return dir;
        }
    }
    return NO_DIRECTION;
}

static int jump(int end, int pos, unsigned char dir, struct map *m) {
    int w = m->width;
    int h = m->height;
    int next_pos = get_next_pos(pos, dir, w, h);
    deep_print("^^^^^^^ get next pos = %d %d   %d   %d %d\n", pos % w, pos / w, dir, next_pos %w, next_pos / w);
    if (!map_walkable(next_pos, w * h, m)) {
        deep_print("not walk = %d %d %d\n", next_pos % w, next_pos / w, dir);
        return -1;
    }
    if (next_pos == end) {
        deep_print("jump end = %d %d %d\n", next_pos % w, next_pos / w, dir);
        return next_pos;
    }
    if (force_dir(next_pos, dir, m) != EMPTY_DIRECTIONSET) {
        deep_print("force dir = %d %d %d\n", next_pos % w, next_pos / w, dir);
        return next_pos;
    }
    if (dir_is_diagonal(dir)) { // diagonal dir explore first check straight dir
        int i;
        i = jump(end, next_pos, (dir + 7) % 8, m);
        if (i > -1) {
            deep_print("jump found 7 = %d %d %d\n", next_pos % w, next_pos / w, dir);
            return next_pos;
        }
        i = jump(end, next_pos, (dir + 1) % 8, m);
        if (i > -1) {
            deep_print("jump found 1 = %d %d %d\n", next_pos % w, next_pos / w, dir);
            return next_pos;
        }
    }
    return jump(end, next_pos, dir, m);
}

static int
find_path(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    if (BITTEST(m->m, m->start)) {
        luaL_error(L, "start pos(%d,%d) is in block", m->start % m->width, m->start / m->width);
    }
    if (BITTEST(m->m, m->end)) {
        luaL_error(L, "end pos(%d,%d) is in block", m->end % m->width, m->end / m->width);
    }
    int len = m->width * m->height;
    if (m->start == m->end) {
        lua_newtable(L);
        return 1;
    }
    memset(&m->m[BITSLOT(len)], 0, BITSLOT(len) * sizeof(m->m[0]));
    memset(m->comefrom, -1, len * sizeof(int));
    memset(m->close_set, 0, len * sizeof(struct node_data *));
    memset(m->open_set_map, 0, len * sizeof(struct heap_node *));

    struct heap *open_set = fibheap_init(len, compare);
    struct node_data *node = construct(m, m->start, 0);
    m->open_set_map[m->start] = fibheap_insert(open_set, node);;
    deep_print("put in open set point: %d %d %d %d\n", node->pos % m->width, node->pos / m->width, node->g_value, node->f_value);
    while ((node = fibheap_pop(open_set))) {
        m->open_set_map[node->pos] = NULL;
        m->close_set[node->pos] = node;
        deep_print("==================check new jump point: %d %d %d %d\n", node->pos % m->width, node->pos / m->width, node->g_value, node->f_value);
        if (node->pos == m->end) {
            fibheap_destroy(open_set);
            deep_print("xxx found path\n");
            for (int k = 0; k < m->height; k++) {
                for (int s = 0; s < m->width; s++) {
                    deep_print("%d ", m->comefrom[k * m->height + s]);
                }
                deep_print("\n");
            }
            return form_path(L, node->pos, m);
        }
        unsigned char cur_dir = calc_dir(m->comefrom[node->pos], node->pos, m->width);
        unsigned char check_dirs = natural_dir(cur_dir) | force_dir(node->pos, cur_dir, m);
        unsigned char dir = next_dir(&check_dirs);
        while (dir != NO_DIRECTION) {
            int new_jump_point = jump(m->end, node->pos, dir, m);
            deep_print("------check dir = %d %d %d %d %d\n", node->pos % m->width, node->pos / m->width, dir, new_jump_point % m->width, new_jump_point / m->width);
            if (new_jump_point != -1 && !m->close_set[new_jump_point]) {
                int ng_value = node->g_value + dist(new_jump_point, node->pos, m->width);
                struct heap_node *p = m->open_set_map[new_jump_point];
                if (!p) {
                    deep_print("come1: %d %d =  %d %d\n", new_jump_point % m->width, new_jump_point / m->width, node->pos % m->width, node->pos / m->width);
                    m->comefrom[new_jump_point] = node->pos;
                    struct node_data *test = construct(m, new_jump_point, ng_value);
                    m->open_set_map[new_jump_point] = fibheap_insert(open_set, test);
                    deep_print("found jump point: %d %d\n", new_jump_point % m->width, new_jump_point / m->width);
                    deep_print("put in open set point: %d %d %d %d\n", new_jump_point % m->width, new_jump_point / m->width, test->g_value, test->f_value);
                } else if (p->data->g_value > ng_value) {
                    deep_print("come2: %d %d =  %d %d\n", new_jump_point % m->width, new_jump_point / m->width, node->pos % m->width, node->pos / m->width);
                    m->comefrom[new_jump_point] = node->pos;
                    p->data->f_value = p->data->f_value - (p->data->g_value - ng_value);
                    p->data->g_value = ng_value;
                    fibheap_decrease(open_set, p);
                }
            }
            dir = next_dir(&check_dirs);
        }
    }
    deep_print("no found path\n");

    fibheap_destroy(open_set);
    return 0;
}

static int
dump(lua_State * L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    printf("dump map state!!!!!!\n");
    int i, pos;
    char s[m->width * 2 + 2];
    for (pos = 0, i = 0; i < m->width * m->height; i++) {
        if (i > 0 && i % m->width == 0) {
            s[pos - 1] = '\0';
            printf("%s\n", s);
            pos = 0;
        }
        int mark = 0;
        if (BITTEST(m->m, i)) {
            s[pos++] = '*';
            mark = 1;
        } else {
#ifdef __PATH_DEBUG__
            if (BITTEST(m->m, i + m->width * m->height)) {
                s[pos++] = '0';
                mark = 1;
            }
#endif
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
    printf("%s\n", s);
    return 0;
}

static int
gc(lua_State * L) {
    deep_print("may be something need to free\n");
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    free(m->comefrom);
    free(m->close_set);
    free(m->open_set_map);
    return 0;
}

static int
lmetatable(lua_State *L) {
    if (luaL_newmetatable(L, MT_NAME)) {
        luaL_Reg l[] = {
            {"add_block", add_block},
            {"add_blockset", add_blockset},
            {"clear_block", clear_block},
            {"clear_allblock", clear_allblock},
            {"set_start", set_start},
            {"set_end", set_end},
            {"find_path", find_path},
            {"dump", dump},
            { NULL, NULL }
        };
        luaL_newlib(L, l);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, gc);
        lua_setfield(L, -2, "__gc");
    }
    return 1;
}

static int
lnewmap(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    int width = getfield(L, "w");
    int height = getfield(L, "h");
    struct map *m = lua_newuserdata(L, sizeof(struct map) + BITSLOT(width * height) * 2 * sizeof(m->m[0]));
    m->width = width;
    m->height = height;
    m->start = -1;
    m->end = -1;
    memset(m->m, 0, BITSLOT(width * height) * 2 * sizeof(m->m[0]));
    int len = width * height;
    m->comefrom = (int *)malloc(len * sizeof(int));
    m->close_set = (struct node_data **)malloc(len * sizeof(struct node_data *));
    m->open_set_map = (struct heap_node **)malloc(len * sizeof(struct heap_node *));
    if (lua_getfield(L, 1, "obstacle") == LUA_TTABLE) {
        int i = 1;
        while (lua_geti(L, -1, i) == LUA_TTABLE) {
            lua_geti(L, -1, 1);
            int x = lua_tointeger(L, -1);
            lua_geti(L, -2, 2);
            int y = lua_tointeger(L, -1);
            setobstacle(L, m, x, y);
            lua_pop(L, 3);
            ++i;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lmetatable(L);
    lua_setmetatable(L, -2);
    return 1;
}

LUAMOD_API int
luaopen_jps(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "new", lnewmap },
        { NULL, NULL },
    };
    luaL_newlib(L, l);
    return 1;
}
