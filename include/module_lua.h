#ifndef MODULE_LUA_H
#define MODULE_LUA_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// Initialize Lua state and load script
lua_State* lua_utils_init(const char* script_path);

// Get string from Lua table field (e.g., config.window_title)
const char* lua_utils_get_string(lua_State* L, const char* table, const char* field, const char* default_value);

// Get number from Lua table field (e.g., square.x)
float lua_utils_get_number(lua_State* L, const char* table, const char* field, float default_value);

// Get integer from Lua table field (e.g., window_width)
int lua_utils_get_integer(lua_State* L, const char* table, const char* field, int default_value);

// Set number in Lua table field (e.g., square.x = value)
void lua_utils_set_number(lua_State* L, const char* table, const char* field, float value);

// Cleanup Lua state
void lua_utils_cleanup(lua_State* L);

#endif // MODULE_LUA_H