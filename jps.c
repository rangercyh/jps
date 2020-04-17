#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "fibheap.h"

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

static inline int
setobstacle(lua_State *L, struct map *m, int x, int y) {
    if (y >= m->height) {
        luaL_error(L, "add obstacle (y = %d) fail", y);
    }
    if (x >= m->width) {
        luaL_error(L, "add obstacle (%d, %d) fail", x, y);
    }
    BITSET(m->m, m->width * y + x);
    return 0;
}

static inline int check_in_map(int x, int y, int w, int h) {
    return x >= 0 && y >= 0 && x < w && y < h;
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
    int ex = one % w;
    int ey = one / w;
    int px = two % w;
    int py = two / w;
    int dx = ex - px;
    int dy = ey - py;
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
    int len = m->width * m->height;
    while (m->comefrom[pos] != -1) {
#ifdef PATH_DEBUG
        BITSET(m->m, pos + len);
#endif
        x = pos % w;
        y = pos / w;
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

// N, NE, E, SE, S, SW, W, NW
typedef unsigned char direction;
#define NO_DIRECTION 8
typedef unsigned char directionset;

static int neighbour_calc(int from, int to, int w) {
    if (from == -1) { // start

    } else {
        switch (to - from) {
            case 1:  return;
            case -1:
            case w:
            case -w:
            case -w-1:
            case -w+1:
            case w-1:
            case w+1:
        }
    }
}

static int jump() {

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

    struct heap *open_set = fibheap_init(len, compare);
    struct node_data *node = construct(m, m->start, 0);
    fibheap_insert(open_set, node);
    while ((node = fibheap_pop(open_set))) {
        m->close_set[node->pos] = node;
        if (node->pos == m->end) {
            fibheap_destroy(open_set);
            return form_path(L, node->pos, m);
        }
        unsigned char check_dirs = neighbour_calc(m->comefrom[node->pos], node->pos, m->width);
        int dir = next_dir(&check_dirs);
        while (dir != NO_DIRECTION) {
            int new_jump_point = jump(node->pos, dir, m->width, m->height);
            if (new_jump_point) {
                struct node_data *p = m->close_set[new_jump_point];
                if (p) {
                    if () {

                    }
                } else {
                    p = construct(m, new_jump_point, dist(new_jump_point, node->pos));
                    fibheap_insert(open_set, p);
                }
            }
            dir = next_dir(&check_dirs);
        }

    }

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
#ifdef PATH_DEBUG
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
    printf("may be something need to free\n");
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    free(m->comefrom);
    free(m->close_set);
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
