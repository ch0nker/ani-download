#include <json/json.h>

extern "C" {
    #include <lua.h>
}

void json_to_lua_table(lua_State* L, Json::Value json) {
    if (json.isObject()) {
        lua_createtable(L, 0, json.size());
        for (const auto& key : json.getMemberNames()) {
            const Json::Value& value = json[key];

            lua_pushstring(L, key.c_str());

            if (value.isString()) {
                lua_pushstring(L, value.asString().c_str());
            } else if (value.isInt()) {
                lua_pushinteger(L, value.asInt());
            } else if (value.isDouble()) {
                lua_pushnumber(L, value.asDouble());
            } else if (value.isBool()) {
                lua_pushboolean(L, value.asBool());
            } else if (value.isObject() || value.isArray()) {
                json_to_lua_table(L, value);
            } else {
                lua_pushnil(L);
            }

            lua_settable(L, -3);
        }
    } else if (json.isArray()) {
        lua_createtable(L, json.size(), 0);
        for (Json::ArrayIndex i = 0; i < json.size(); ++i) {
            json_to_lua_table(L, json[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else {
        lua_pushnil(L);
    }
}

int is_array(lua_State* L, int index) {
    if (!lua_istable(L, index)) {
        return 0;
    }

    size_t len = lua_rawlen(L, index);
    if (len > 0) {
        return 1;
    }

    lua_pushnil(L);
    if (lua_next(L, index) != 0) {
        lua_pop(L, 2);
        return 0;
    }

    return 1;
}
void encode_lua_table(lua_State* L, int index, Json::Value& json_value) {
    if (is_array(L, index)) {
        json_value = Json::Value(Json::arrayValue);
        size_t len = lua_rawlen(L, index);
        for (size_t i = 1; i <= len; ++i) {
            lua_rawgeti(L, index, i);
            if (lua_istable(L, -1)) {
                Json::Value nested_value;
                encode_lua_table(L, lua_gettop(L), nested_value);
                json_value.append(nested_value);
            } else if (lua_isboolean(L, -1)) {
                json_value.append(lua_toboolean(L, -1) != 0);
            } else if (lua_isinteger(L, -1)) {
                json_value.append((Json::Int)lua_tointeger(L, -1));
            } else if (lua_isnumber(L, -1)) {
                json_value.append(lua_tonumber(L, -1));
            } else if (lua_isstring(L, -1)) {
                json_value.append(lua_tostring(L, -1));
            } else {
                json_value.append(Json::Value());
            }
            lua_pop(L, 1);
        }
    } else {
        json_value = Json::Value(Json::objectValue);
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
            const char* key = lua_tostring(L, -2);
            if (lua_istable(L, -1)) {
                Json::Value nested_value;
                encode_lua_table(L, lua_gettop(L), nested_value);
                json_value[key] = nested_value;
            } else if (lua_isboolean(L, -1)) {
                json_value[key] = lua_toboolean(L, -1) != 0;
            } else if (lua_isinteger(L, -1)) {
                json_value[key] = (Json::Int)lua_tointeger(L, -1);
            } else if (lua_isnumber(L, -1)) {
                json_value[key] = lua_tonumber(L, -1);
            } else if (lua_isstring(L, -1)) {
                json_value[key] = lua_tostring(L, -1);
            } else {
                json_value[key] = Json::Value();
            }
            lua_pop(L, 1);
        }
    }
}

int lua_json_encode(lua_State* L) {
    if (!lua_istable(L, 1)) {
        lua_pushstring(L, "Expected a table to encode to JSON");
        lua_error(L);
    }

    Json::Value root;
    encode_lua_table(L, 1, root);

    Json::StreamWriterBuilder writer;
    std::string jsonString = Json::writeString(writer, root);

    lua_pushstring(L, jsonString.c_str());
    return 1;
}

int lua_json_decode(lua_State* L) {
    const char* json_str = lua_tostring(L, 1);

    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errors;

    std::istringstream s(json_str);
    if (!Json::parseFromStream(reader, s, &root, &errors)) {
        lua_pushstring(L, errors.c_str());
        lua_error(L);
    }

    lua_newtable(L);

    if (root.isObject()) {
        for (const auto& key : root.getMemberNames()) {
            lua_pushstring(L, key.c_str());

            const Json::Value& value = root[key];

            if (value.isString()) {
                lua_pushstring(L, value.asString().c_str());
            } else if (value.isInt()) {
                lua_pushinteger(L, value.asInt());
            } else if (value.isBool()) {
                lua_pushboolean(L, value.asBool());
            } else if (value.isDouble()) {
                lua_pushnumber(L, value.asDouble());
            } else if (value.isArray() || value.isObject()) {
                lua_pushcfunction(L, lua_json_decode);
                lua_pushstring(L, value.toStyledString().c_str());
                lua_call(L, 1, 1);
            } else {
                lua_pushnil(L);
            }

            lua_settable(L, -3);
        }
    }
    else if (root.isArray()) {
        for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
            lua_pushinteger(L, i + 1);

            const Json::Value& value = root[i];

            if (value.isString()) {
                lua_pushstring(L, value.asString().c_str());
            } else if (value.isInt()) {
                lua_pushinteger(L, value.asInt());
            } else if (value.isBool()) {
                lua_pushboolean(L, value.asBool());
            } else if (value.isDouble()) {
                lua_pushnumber(L, value.asDouble());
            } else if (value.isArray() || value.isObject()) {
                lua_pushcfunction(L, lua_json_decode);
                lua_pushstring(L, value.toStyledString().c_str());
                lua_call(L, 1, 1);
            } else {
                lua_pushnil(L);
            }

            lua_settable(L, -3);
        }
    } else {
        lua_pushnil(L);
    }

    return 1;
}

void load_json_library(lua_State* L) {
    lua_createtable(L, 0, 2);
    lua_pushcfunction(L, lua_json_decode);
    lua_setfield(L, -2, "decode");
    lua_pushcfunction(L, lua_json_encode);
    lua_setfield(L, -2, "encode");
    lua_setglobal(L, "json");
}
