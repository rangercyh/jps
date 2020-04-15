#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

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
    char m[0];
};

static int
getfield(lua_State *L, const char *f) {
    if (lua_getfield(L, -1, f) != LUA_TNUMBER) {
        return luaL_error(L, "invalid type %s", f);
    }
    int v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

static int
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

static int
add_block(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (x < 0 || x >= m->width ||
        y < 0 || y >= m->height) {
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
    if (x < 0 || x >= m->width ||
        y < 0 || y >= m->height) {
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
    if (x < 0 || x >= m->width ||
        y < 0 || y >= m->height) {
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
    if (x < 0 || x >= m->width ||
        y < 0 || y >= m->height) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    int pos = m->width * y + x;
    if (BITTEST(m->m, pos)) {
        luaL_error(L, "Position (%d,%d) is in block", x, y);
    }
    m->end = pos;
    return 0;
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
            if (BITTEST(m->m, i + m->width * m->height)) {
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
    printf("%s\n", s);
    return 0;
}

static int
gc(lua_State * L) {
    printf("may be something need to free\n");
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
