#include "module_lua.h"
#include <SDL3/SDL.h>

// Initialize Lua state and load script
lua_State* lua_utils_init(const char* script_path) {
    lua_State* L = luaL_newstate();
    if (!L) {
        SDL_Log("Failed to create Lua state");
        return NULL;
    }
    luaL_openlibs(L);

    if (luaL_dofile(L, script_path) != LUA_OK) {
        SDL_Log("Lua script error: %s", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }
    return L;
}

// Get string from Lua table field
const char* lua_utils_get_string(lua_State* L, const char* table, const char* field, const char* default_value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: %s is not a table", table);
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, field);
    const char* result = lua_isstring(L, -1) ? lua_tostring(L, -1) : default_value;
    lua_pop(L, 2); // Pop field and table
    return result;
}

// Get number from Lua table field
float lua_utils_get_number(lua_State* L, const char* table, const char* field, float default_value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: %s is not a table", table);
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, field);
    float result = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : default_value;
    lua_pop(L, 2); // Pop field and table
    return result;
}

// Get integer from Lua table field
int lua_utils_get_integer(lua_State* L, const char* table, const char* field, int default_value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: %s is not a table", table);
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, field);
    int result = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : default_value;
    lua_pop(L, 2); // Pop field and table
    return result;
}

// Set number in Lua table field
void lua_utils_set_number(lua_State* L, const char* table, const char* field, float value) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: %s is not a table", table);
        lua_pop(L, 1);
        return;
    }
    lua_pushnumber(L, value);
    lua_setfield(L, -2, field);
    lua_pop(L, 1); // Pop table
}

// Get number of nodes in config.nodes table
int lua_utils_get_nodes_count(lua_State* L) {
    lua_getglobal(L, "config");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config is not a table");
        lua_pop(L, 1);
        return 0;
    }
    lua_getfield(L, -1, "nodes");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes is not a table");
        lua_pop(L, 2);
        return 0;
    }
    int count = (int)lua_rawlen(L, -1);
    lua_pop(L, 2); // Pop nodes and config
    return count;
}

// Get number from config.nodes[i].field
float lua_utils_get_node_number(lua_State* L, int index, const char* field, float default_value) {
    lua_getglobal(L, "config");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config is not a table");
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, "nodes");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes is not a table");
        lua_pop(L, 2);
        return default_value;
    }
    lua_rawgeti(L, -1, index);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes[%d] is not a table", index);
        lua_pop(L, 3);
        return default_value;
    }
    lua_getfield(L, -1, field);
    float result = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : default_value;
    lua_pop(L, 4); // Pop field, node, nodes, config
    return result;
}

// Get string from config.nodes[i].text
const char* lua_utils_get_node_text(lua_State* L, int index, const char* default_value) {
    lua_getglobal(L, "config");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config is not a table");
        lua_pop(L, 1);
        return default_value;
    }
    lua_getfield(L, -1, "nodes");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes is not a table");
        lua_pop(L, 2);
        return default_value;
    }
    lua_rawgeti(L, -1, index);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes[%d] is not a table", index);
        lua_pop(L, 3);
        return default_value;
    }
    lua_getfield(L, -1, "text");
    const char* result = lua_isstring(L, -1) ? lua_tostring(L, -1) : default_value;
    lua_pop(L, 4); // Pop text, node, nodes, config
    return result;
}

// Set number in config.nodes[i].field
void lua_utils_set_node_number(lua_State* L, int index, const char* field, float value) {
    lua_getglobal(L, "config");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config is not a table");
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "nodes");
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes is not a table");
        lua_pop(L, 2);
        return;
    }
    lua_rawgeti(L, -1, index);
    if (!lua_istable(L, -1)) {
        SDL_Log("Lua error: config.nodes[%d] is not a table", index);
        lua_pop(L, 3);
        return;
    }
    lua_pushnumber(L, value);
    lua_setfield(L, -2, field);
    lua_pop(L, 3); // Pop node, nodes, config
}

// Cleanup Lua state
void lua_utils_cleanup(lua_State* L) {
    if (L) {
        lua_close(L);
    }
}