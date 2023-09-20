#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "jps.h"

#define MT_NAME ("_jps_search_metatable")

static inline int getfield(lua_State *L, const char *f) {
    if (lua_getfield(L, -1, f) != LUA_TNUMBER) {
        return luaL_error(L, "invalid type %s", f);
    }
    int v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

static int add_block(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (jps_set_obstacle(m, x, y, 1)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    return 0;
}

static int add_blockset(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_settop(L, 2);
    int i = 1;
    while (lua_geti(L, -1, i) == LUA_TTABLE) {
        lua_geti(L, -1, 1);
        int x = lua_tointeger(L, -1);
        lua_geti(L, -2, 2);
        int y = lua_tointeger(L, -1);
        if (jps_set_obstacle(m, x, y, 1)) {
            luaL_error(L, "Position (%d,%d) is out of map", x, y);
        }
        lua_pop(L, 3);
        ++i;
    }
    return 0;
}

static int is_block(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int ret = jps_is_obstacle(m, x, y);
    if (ret < 0) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
        return 0;
    }
    if (ret == 1) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int clear_block(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (jps_set_obstacle(m, x, y, 0)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    return 0;
}

static int clear_allblock(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    jps_clearall_obs(m);
    return 0;
}

static int set_start(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (jps_set_start(m, x, y)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    return 0;
}

static int set_end(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    if (jps_set_end(m, x, y)) {
        luaL_error(L, "Position (%d,%d) is out of map", x, y);
    }
    return 0;
}

static int mark_connected(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    jps_mark_connected(m);
    return 0;
}

static int path_to_stack(lua_State *L, IntList *path) {
    lua_newtable(L);
    int x, y;
    int len = il_size(path);
    for (int i = len - 1; i >= 0; i--) {
        lua_newtable(L);
        x = il_get(path, i, 0);
        lua_pushinteger(L, x);
        lua_rawseti(L, -2, 1);
        y = il_get(path, i, 1);
        lua_pushinteger(L, y);
        lua_rawseti(L, -2, 2);
        lua_rawseti(L, -2, len - i);
    }
    il_destroy(path);
    return 1;
}

static int find_path(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    int type = luaL_optinteger(L, 2, 1);
    IntList *path = il_create(2);
    switch (jps_path_finding(m, type, path)) {
    case 1:
        il_destroy(path);
        luaL_error(L, "start pos(%d,%d) is in block", m->start % m->width, m->start / m->width);
    case 2:
        il_destroy(path);
        luaL_error(L, "end pos(%d,%d) is in block", m->end % m->width, m->end / m->width);
    case 3:
        il_destroy(path);
        luaL_error(L, "path type(%d) error", type);
    case 4:
        il_destroy(path);
        // no path found
        return 0;
    }
    return path_to_stack(L, path);
}

static int dump_connected(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    jps_dump_connected(m);
    return 0;
}

static int dump(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    jps_dump(m);
    return 0;
}

static int gc(lua_State *L) {
    struct map *m = luaL_checkudata(L, 1, MT_NAME);
    jps_destory(m);
    return 0;
}

static int lmetatable(lua_State *L) {
    if (luaL_newmetatable(L, MT_NAME)) {
        luaL_Reg l[] = {
            {"add_block", add_block},
            {"add_blockset", add_blockset},
            {"is_block", is_block},
            {"clear_block", clear_block},
            {"clear_allblock", clear_allblock},
            {"set_start", set_start},
            {"set_end", set_end},
            {"find_path", find_path},
            {"mark_connected", mark_connected},
            {"dump_connected", dump_connected},
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

static int lnewmap(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    int width = getfield(L, "w");
    int height = getfield(L, "h");
    lua_assert(width > 0 && height > 0);
    int len = width * height;
    int map_men_len = jps_get_memory_len(len);
    struct map *m = lua_newuserdata(L, sizeof(struct map) + map_men_len * sizeof(m->m[0]));
    jps_init(m, width, height, map_men_len);
    if (lua_getfield(L, 1, "obstacle") == LUA_TTABLE) {
        int i = 1;
        while (lua_geti(L, -1, i) == LUA_TTABLE) {
            lua_geti(L, -1, 1);
            int x = lua_tointeger(L, -1);
            lua_geti(L, -2, 2);
            int y = lua_tointeger(L, -1);
            if (jps_set_obstacle(m, x, y, 1)) {
                luaL_error(L, "Position (%d,%d) is out of map", x, y);
            }
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

LUAMOD_API int luaopen_jps(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "new", lnewmap },
        { NULL, NULL },
    };
    luaL_newlib(L, l);
    return 1;
}
