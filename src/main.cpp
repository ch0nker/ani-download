#include "json/writer.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>

#include <json/value.h>

#include "args.h"

#include "lua/request.h"
#include "lua/json.h"
#include "lua/parser.h"

extern "C" {
    #include <curl/curl.h>
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
    #include <dirent.h>
}

#define USAGE "ani-download <name> [tags]\n\n" \
              "Flags:\n" \
              "\t-c, --core:\tWhich core to use.\n" \
              "\t-l, --list-cores:\tLists all of the available cores.\n"

std::string home_dir = getenv("HOME");
std::string config_dir = home_dir + "/.config/ani-downloader";
std::string cores_dir = config_dir + "/cores";
std::string modules_dir = config_dir + "/modules";
std::string configs_dir = config_dir + "/configs";
std::string settings_path = config_dir + "/settings.json";

void cleanup(lua_State* L) {
    lua_close(L);
    curl_global_cleanup();
    exit(EXIT_SUCCESS);
}

typedef struct Flags {
    std::string name;
    std::string core;
} Flags;

Flags flags = {
    .name = std::string(),
    .core = std::string(),
};

void help_func(char*) {
    printf("%s", USAGE);
}

void core_func(char* core) {
    if(core == nullptr)
        return;

    flags.core = core;
}

void list_cores_func(char*) {
    for(const auto& entry : std::filesystem::directory_iterator(cores_dir)) {
        std::filesystem::path path = entry.path();
        if(path.extension().compare(".lua") != 0)
            continue;

        const char* dir_name = path.filename().replace_extension("").c_str();
        if(dir_name[0] == '.')
            continue;

        printf("%s\n", dir_name);
    }
}

int load_core(lua_State*L, std::string core) {
    if (luaL_dofile(L, (cores_dir + "/" + core + ".lua").c_str()) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        printf("Error loading core: %s\n", error);
        lua_pop(L, 1);
        return 0;
    }

    return 1;
}

int call_core(lua_State* L) {
    load_core(L, flags.core);
    
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "download");
        lua_pushstring(L, flags.name.c_str());
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            printf("Error calling download function: %s\n", error);
            lua_pop(L, 1);
            lua_pop(L, 1);
            return 0;
        }

        lua_pop(L, 1);
        return 1;
    }

    printf("Error: Returned value is not a table.\n");
    lua_pop(L, 1);
    return 0;
}

void load_system_paths(lua_State* L) {
    lua_newtable(L);

    lua_pushstring(L, home_dir.c_str());
    lua_setfield(L, -2, "home");

    lua_pushstring(L, config_dir.c_str());
    lua_setfield(L, -2, "config");

    lua_pushstring(L, modules_dir.c_str());
    lua_setfield(L, -2, "modules");

    lua_pushstring(L, cores_dir.c_str());
    lua_setfield(L, -2, "cores");

    lua_pushstring(L, configs_dir.c_str());
    lua_setfield(L, -2, "configs");

    lua_setglobal(L, "system_paths");
}

void set_settings(Json::Value settings) {
    std::ofstream file(settings_path);
    Json::StreamWriterBuilder writer;

    writer["indentation"] = "    ";
    file << Json::writeString(writer, settings);
}

Json::Value get_settings() {
    Json::Value val;
    if (!std::filesystem::exists(settings_path)) {
        set_settings(val);
        return val;
    }
    std::ifstream file(settings_path);

    file >> val;
    file.close();

    return val;
}

void add_package_path(lua_State*L, std::string dir) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");

    const char* current_path = lua_tostring(L, -1);

    std::string new_path = std::string(current_path) + ";" + dir + "/?.lua";

    lua_pop(L, 1);
    lua_pushstring(L, new_path.c_str());
    lua_setfield(L, -2, "path");

    lua_pop(L, 1);
}

int main(int argc, char** argv) {
    if(!std::filesystem::is_directory(config_dir)) {
        std::filesystem::create_directory(config_dir);
        std::filesystem::create_directory(cores_dir);
        std::filesystem::create_directory(modules_dir);
        std::filesystem::create_directory(configs_dir);
    }
        
    if(argc == 1) {
        help_func(nullptr);
        return EXIT_FAILURE;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    lua_State* L = luaL_newstate();
    if(L == nullptr) {
        fprintf(stderr, "Failed to allocate lua state.");
        return EXIT_FAILURE;
    }

    luaL_openlibs(L);
    load_json_library(L);
    load_request_library(L);
    load_html_library(L);
    load_system_paths(L);

    add_package_path(L, modules_dir);

    if(*argv[0] != '-')
        flags.name = argv[1];

    if(argc == 2 && !flags.name.empty()) {
        flags.name = argv[1];
        Json::Value settings = get_settings();

        if(!settings.isMember("default_core")) {
            settings["default_core"] = "nyaa";
            set_settings(settings);
        }

        flags.core = settings["default_core"].asString();
    }

    FlagContainer* container = create_container();
    add_flag(container, "core", core_func, nullptr);
    add_flag(container, "list-cores", list_cores_func, "l");
    handle_args(container, argc, argv, 1);

    if(!flags.core.empty())
        call_core(L);

    cleanup(L);

    return EXIT_FAILURE;
}
