extern "C" {
    #include <lua.h>
}

void load_request_library(lua_State* L);
int lua_delete_request(lua_State* L);
int lua_get_request(lua_State* L);
int lua_make_request(lua_State* L);
int lua_patch_request(lua_State* L);
int lua_post_request(lua_State* L);
int lua_put_request(lua_State* L);
