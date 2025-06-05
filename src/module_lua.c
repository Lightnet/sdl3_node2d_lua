#include "module_lua.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdio.h>

lua_State* lua_utils_init(const char *script_path) {
    lua_State *L = luaL_newstate();
    if (!L) {
        printf("Failed to create Lua state\n");
        return NULL;
    }
    luaL_openlibs(L);
    if (luaL_dofile(L, script_path) != LUA_OK) {
        printf("Failed to load Lua script '%s': %s\n", script_path, lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }
    return L;
}

void lua_utils_cleanup(lua_State *L) {
    if (L) {
        lua_close(L);
    }
}

const char* lua_utils_get_string(lua_State *L, const char *table, const char *key, const char *default_value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, key);
    const char *value = lua_isstring(L, -1) ? lua_tostring(L, -1) : default_value;
    lua_pop(L, 2);
    return value;
}

int lua_utils_get_integer(lua_State *L, const char *table, const char *key, int default_value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, key);
    int value = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : default_value;
    lua_pop(L, 2);
    return value;
}

float lua_utils_get_number(lua_State *L, const char *table, const char *key, float default_value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, key);
    float value = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : default_value;
    lua_pop(L, 2);
    return value;
}

void lua_utils_set_number(lua_State *L, const char *table, const char *key, float value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setglobal(L, table);
        lua_getglobal(L, table);
    }
    lua_pushnumber(L, value);
    lua_setfield(L, -2, key);
    lua_pop(L, 1);
}

int lua_utils_get_nodes_count(lua_State *L) {
    lua_getglobal(L, "nodes");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    int count = lua_rawlen(L, -1);
    lua_pop(L, 1);
    return count;
}

float lua_utils_get_node_number(lua_State *L, int node_index, const char *key, float default_value) {
    lua_getglobal(L, "nodes");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    lua_rawgeti(L, -1, node_index);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return default_value;
    }
    lua_getfield(L, -1, key);
    float value = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : default_value;
    lua_pop(L, 3);
    return value;
}

void lua_utils_set_node_number(lua_State *L, int node_index, const char *key, float value) {
    lua_getglobal(L, "nodes");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setglobal(L, "nodes");
        lua_getglobal(L, "nodes");
    }
    lua_rawgeti(L, -1, node_index);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_rawseti(L, -2, node_index);
        lua_rawgeti(L, -1, node_index);
    }
    lua_pushnumber(L, value);
    lua_setfield(L, -2, key);
    lua_pop(L, 2);
}

const char* lua_utils_get_node_text(lua_State *L, int node_index, const char *default_value) {
    lua_getglobal(L, "nodes");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    lua_rawgeti(L, -1, node_index);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return default_value;
    }
    lua_getfield(L, -1, "text");
    const char *value = lua_isstring(L, -1) ? lua_tostring(L, -1) : default_value;
    lua_pop(L, 3);
    return value;
}

int lua_utils_get_node_connectors(lua_State *L, int node_index, const char *key, int default_value) {
    lua_getglobal(L, "nodes");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    lua_rawgeti(L, -1, node_index);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return default_value;
    }
    lua_getfield(L, -1, key);
    int value = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : default_value;
    lua_pop(L, 3);
    return value;
}