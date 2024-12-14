#include <json/json.h>

extern "C" {
    #include <lua.h>
}

void json_to_lua_table(lua_State* L, Json::Value json);
void load_json_library(lua_State* L);
