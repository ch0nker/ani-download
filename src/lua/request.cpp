#include <cstring>
#include <vector>
#include <string>
#include <sstream>

#include <vector>
#include <json/json.h>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
    #include <curl/curl.h>
    #include <curl/easy.h>
}

#include "json.h"

std::vector<std::string> split_string(const char* value, char delim) {
    std::vector<std::string> result;
    std::stringstream sstream(value);
    std::string item;

    while (std::getline(sstream, item, delim)) {
        result.push_back(item);
    }
    return result;
}

int lua_json_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "data");
    if(!lua_isstring(L, -1)) {
        lua_pop(L, -1);
        return luaL_error(L, "Expected string for 'data' field.");
    }

    const char* data = lua_tostring(L, -1);
    lua_pop(L, 1);
    Json::CharReaderBuilder reader;
    std::istringstream isstream(data);
    std::string errors;
    Json::Value result;

    if (Json::parseFromStream(reader, isstream, &result, &errors)) {
        json_to_lua_table(L, result);
    } else {
        return luaL_error(L, "Failed to parse JSON: %s", errors.c_str());
    }

    return 1;
}

size_t curl_write_function(void* ptr, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append((char*) ptr, total_size);
    return total_size;
}

int lua_make_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "url");
    if(!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        return luaL_error(L, "Expected string for 'url' field.");
    }

    const char* url = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "method");
    if(!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        return luaL_error(L, "Expected string for 'method' field.");
    }

    const char* method = lua_tostring(L, -1);
    lua_pop(L, 1);

    CURL* curl;
    CURLcode response;

    curl = curl_easy_init();

    if(!curl)
        return luaL_error(L, "Failed to initialize CURL.");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);

    lua_getfield(L, 1, "headers");
    if(lua_istable(L, -1)) {
        struct curl_slist *headers = nullptr;

        lua_pushnil(L);
        while(lua_next(L, -2)) {
            if(lua_isstring(L, -2) && lua_isstring(L, -1)) {
                std::string header = std::string(lua_tostring(L, -2)) + ": " + lua_tostring(L, -1);
                headers = curl_slist_append(headers, header.c_str());
            }
            lua_pop(L, 1);
        }

        if (headers != nullptr) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        } 
    }
    lua_pop(L, 1);

    if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0) {
        lua_getfield(L, 1, "body");
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return luaL_error(L, "Expected string for 'body' field.");
        }

        const char* body = lua_tostring(L, -1);
        lua_pop(L, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }

    std::string response_data;
    std::string header_data;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &header_data);

    response = curl_easy_perform(curl);
    if(response != CURLE_OK) {
        curl_easy_cleanup(curl);
        return luaL_error(L, "CURL request failed: %s", curl_easy_strerror(response));
    }

    std::vector<std::string> headers_list = split_string(header_data.c_str(), '\n');
    std::vector<std::string> data_line = split_string(headers_list[0].c_str(), ' ');

    curl_easy_cleanup(curl);

    lua_createtable(L, 0, 5);

    lua_pushstring(L, response_data.c_str());
    lua_setfield(L, -2, "data");

    lua_newtable(L);
    for(int i = 1; i < headers_list.size(); i++) {
        std::string line = headers_list[i];

        if(line.length() < 3)
            continue;

        size_t key_size = line.find(": ") + 1;
        size_t value_size = line.size() - key_size;

        char* key = (char*) calloc(1, key_size);
        char* value = (char*) calloc(1, value_size);
        
        memcpy(key, line.c_str(), key_size - 1);
        memcpy(value, line.c_str() + key_size + 1, value_size - 1);

        lua_pushstring(L, value);
        lua_setfield(L, -2, key);

        free(key);
        free(value);
    }

    lua_setfield(L, -2, "headers");
    lua_pushstring(L, url);
    lua_setfield(L, -2, "url");
    lua_pushinteger(L, std::stoi(data_line[1]));
    lua_setfield(L, -2, "status_code");
    std::string status = data_line[2];
    lua_pushstring(L, status.erase(status.length() - 1, 1).c_str());
    lua_setfield(L, -2, "status");
    lua_pushcfunction(L, lua_json_request);
    lua_setfield(L, -2, "json");

    return 1;
}

int lua_get_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, "GET");
    lua_setfield(L, 1, "method");

    lua_make_request(L);

    return 1;
}

int lua_post_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, "POST");
    lua_setfield(L, 1, "method");

    lua_make_request(L);

    return 1;
}

int lua_patch_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, "PATCH");
    lua_setfield(L, 1, "method");

    lua_make_request(L);

    return 1;
}

int lua_put_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, "PUT");
    lua_setfield(L, 1, "method");

    lua_make_request(L);

    return 1;
}

int lua_delete_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, "DELETE");
    lua_setfield(L, 1, "method");

    lua_make_request(L);

    return 1;
}

std::string urlencode(const std::string& str) {
    CURL *curl = curl_easy_init();
    if (curl) {
        char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
        std::string result = encoded;
        curl_free(encoded);
        curl_easy_cleanup(curl);
        return result;
    }
    return "";
}

int lua_url_decode(lua_State* L) {
    if(!lua_isstring(L, 1)) {
        luaL_error(L, "Expected string as first argument.");
        return 0;
    }

    CURL* curl = curl_easy_init();
    if(!curl) {
        luaL_error(L, "Failed to allocate CURL pointer.");
        return 0;
    }

    const char* path = lua_tostring(L, 1);
    int decodelen;
    char* decoded = curl_easy_unescape(curl, path, strlen(path), &decodelen);
    if(!decoded) {
        curl_easy_cleanup(curl);
        luaL_error(L, "Failed to allocate decoded string.");
        return 0;
    }

    lua_pushstring(L, decoded);

    curl_free(decoded);
    curl_easy_cleanup(curl);

    return 1;
}

int lua_url_encode(lua_State* L) {
    if(!lua_isstring(L, 1)) {
        luaL_error(L, "Expected string as first argument.");
        return 0;
    }

    CURL* curl = curl_easy_init();
    if(!curl) {
        luaL_error(L, "Failed to allocate CURL pointer.");
        return 0;
    }

    const char* path = lua_tostring(L, 1);
    char* encoded = curl_easy_escape(curl, path, strlen(path));

    if(!encoded) {
        curl_easy_cleanup(curl);
        luaL_error(L, "Failed to allocate encoded string.");
        return 0;
    }

    lua_pushstring(L, encoded);

    curl_free(encoded);
    curl_easy_cleanup(curl);

    return 1;
}

void load_request_library(lua_State* L) {
    lua_createtable(L, 0, 6);
    lua_pushcfunction(L, lua_make_request);
    lua_setfield(L, -2, "make");
    lua_pushcfunction(L, lua_post_request);
    lua_setfield(L, -2, "post");
    lua_pushcfunction(L, lua_get_request);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, lua_put_request);
    lua_setfield(L, -2, "put");
    lua_pushcfunction(L, lua_patch_request);
    lua_setfield(L, -2, "patch");
    lua_pushcfunction(L, lua_delete_request);
    lua_setfield(L, -2, "delete");
    lua_pushcfunction(L, lua_url_encode);
    lua_setfield(L, -2, "url_encode");
    lua_pushcfunction(L, lua_url_decode);
    lua_setfield(L, -2, "url_decode");
    lua_setglobal(L, "requests");
}
