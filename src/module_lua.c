#include "module_lua.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h> // Added to define bool, true, false

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

int lua_utils_get_connections_count(lua_State *L) {
    lua_getglobal(L, "connections");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    int count = lua_rawlen(L, -1);
    lua_pop(L, 1);
    return count;
}

void lua_utils_get_connection(lua_State *L, int conn_index, int *from_node, int *from_output, int *to_node, int *to_input) {
    lua_getglobal(L, "connections");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        *from_node = *from_output = *to_node = *to_input = 0;
        return;
    }
    lua_rawgeti(L, -1, conn_index);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        *from_node = *from_output = *to_node = *to_input = 0;
        return;
    }
    lua_getfield(L, -1, "from_node");
    *from_node = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    lua_getfield(L, -1, "from_output");
    *from_output = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    lua_getfield(L, -1, "to_node");
    *to_node = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    lua_getfield(L, -1, "to_input");
    *to_input = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 2);
}

void lua_utils_add_connection(lua_State *L, int from_node, int from_output, int to_node, int to_input) {
    lua_getglobal(L, "connections");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setglobal(L, "connections");
        lua_getglobal(L, "connections");
    }
    int count = lua_rawlen(L, -1) + 1;
    lua_newtable(L);
    lua_pushinteger(L, from_node);
    lua_setfield(L, -2, "from_node");
    lua_pushinteger(L, from_output);
    lua_setfield(L, -2, "from_output");
    lua_pushinteger(L, to_node);
    lua_setfield(L, -2, "to_node");
    lua_pushinteger(L, to_input);
    lua_setfield(L, -2, "to_input");
    lua_rawseti(L, -2, count);
    lua_pop(L, 1);
}

void lua_utils_remove_connections(lua_State *L, int node_index, const char *type, int connector_index) {
    lua_getglobal(L, "connections");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    int count = lua_rawlen(L, -1);
    int new_count = 0;
    lua_newtable(L); // New connections table
    for (int i = 1; i <= count; i++) {
        lua_rawgeti(L, -2, i);
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "from_node");
            int from_node = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "from_output");
            int from_output = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "to_node");
            int to_node = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "to_input");
            int to_input = lua_tointeger(L, -1);
            lua_pop(L, 1);
            bool keep = true;
            if (strcmp(type, "input") == 0 && to_node == node_index && to_input == connector_index) {
                keep = false;
            } else if (strcmp(type, "output") == 0 && from_node == node_index && from_output == connector_index) {
                keep = false;
            }
            if (keep) {
                new_count++;
                lua_newtable(L);
                lua_pushinteger(L, from_node);
                lua_setfield(L, -2, "from_node");
                lua_pushinteger(L, from_output);
                lua_setfield(L, -2, "from_output");
                lua_pushinteger(L, to_node);
                lua_setfield(L, -2, "to_node");
                lua_pushinteger(L, to_input);
                lua_setfield(L, -2, "to_input");
                lua_rawseti(L, -3, new_count);
            }
        }
        lua_pop(L, 1);
    }
    lua_setglobal(L, "connections");
    lua_pop(L, 1);
}