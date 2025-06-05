#ifndef MODULE_LUA_H
#define MODULE_LUA_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Initialize Lua and load script
lua_State* lua_utils_init(const char *script_path);

// Cleanup Lua
void lua_utils_cleanup(lua_State *L);

// Get string from table
const char* lua_utils_get_string(lua_State *L, const char *table, const char *key, const char *default_value);

// Get integer from table
int lua_utils_get_integer(lua_State *L, const char *table, const char *key, int default_value);

// Get number from table
float lua_utils_get_number(lua_State *L, const char *table, const char *key, float default_value);

// Set number in table
void lua_utils_set_number(lua_State *L, const char *table, const char *key, float value);

// Get node count
int lua_utils_get_nodes_count(lua_State *L);

// Get node number
float lua_utils_get_node_number(lua_State *L, int node_index, const char *key, float default_value);

// Set node number
void lua_utils_set_node_number(lua_State *L, int node_index, const char *key, float value);

// Get node text
const char* lua_utils_get_node_text(lua_State *L, int node_index, const char *default_value);

// Get node connector counts
int lua_utils_get_node_connectors(lua_State *L, int node_index, const char *key, int default_value);

// Get connection count
int lua_utils_get_connections_count(lua_State *L);

// Get connection details
void lua_utils_get_connection(lua_State *L, int conn_index, int *from_node, int *from_output, int *to_node, int *to_input);

// Add connection
void lua_utils_add_connection(lua_State *L, int from_node, int from_output, int to_node, int to_input);

// Remove connections involving a connector
void lua_utils_remove_connections(lua_State *L, int node_index, const char *type, int connector_index);

#endif // MODULE_LUA_H