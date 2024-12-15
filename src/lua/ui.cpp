#include <cstdlib>
#include <cstring>

extern "C" {
    #include <curses.h>
    #include <lua.h>
    #include <lauxlib.h>
}

bool initialized = false;
const char* color_names[] = {
    "black", "red", "green", "yellow",
    "blue", "magenta", "cyan", "white"
};

int lua_get_main_window(lua_State* L) {
    lua_getglobal(L, "ui");
    luaL_checktype(L, -1, LUA_TTABLE);

    lua_getfield(L, -1, "main_window");
    lua_remove(L, -2);
    luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);

    return 1;
}

WINDOW* get_subwindow(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "window");
    if (lua_isnil(L, -1)) {
        luaL_error(L, "'window' field is nil");
        return nullptr;
    }

    luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);

    WINDOW* win = (WINDOW*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    return win;
}

int lua_refresh_window(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if(!win) {
        luaL_error(L, "Attempt to refresh a null or invalid window");
        return 0;
    }

    wrefresh(win);

    return 0;
}

int lua_clear_window(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if(!win) {
        luaL_error(L, "Attempt to clear a null or invalid window");
        return 0;
    }

    wclear(win);
    box(win, 0, 0);
    wrefresh(win);

    return 0;
}

int lua_delete_window(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if(!win) {
        luaL_error(L, "Attempt to delete a null or invalid window");
        return 0;
    }

    werase(win);
    wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');

    wrefresh(win);
    delwin(win);

    lua_pushnil(L);
    lua_setfield(L, 1, "window");

    return 0;
}

int lua_print_window(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if(!win) {
        luaL_error(L, "Attempt to print to a null or invalid window");
        return 0;
    }

    int x = (int)luaL_checknumber(L, 2);
    int y = (int)luaL_checknumber(L, 3);
    const char* text = luaL_checkstring(L, 4);

    mvwaddstr(win, y, x, text);
    wrefresh(win);

    return 0;
}

int lua_move_cursor(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if(!win) {
        luaL_error(L, "Attempt to move cursor on a null or invalid window");
        return 0;
    }

    int x = (int)luaL_checknumber(L, 2);
    int y = (int)luaL_checknumber(L, 3);

    wmove(win, y, x);
    wrefresh(win);

    return 0;
}

int lua_set_cursor_type(lua_State* L) {
    int cursor_type = (int)luaL_checknumber(L, 1);

    if (cursor_type < 0 || cursor_type > 2) {
        luaL_error(L, "Invalid cursor type. Expected 0 (hidden), 1 (visible), or 2 (blink).");
        return 0;
    }

    curs_set(cursor_type);

    return 0;
}

int lua_move_window(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if (!win) {
        luaL_error(L, "Attempt to move on a null or invalid window");
        return 0;
    }

    int x = (int)luaL_checknumber(L, 2);
    int y = (int)luaL_checknumber(L, 3);

    lua_pushnumber(L, x);
    lua_setfield(L, -4, "x");

    lua_pushnumber(L, x);
    lua_setfield(L, -4, "y");

    int width = getmaxx(win);
    int height = getmaxy(win);

    char buffer[height][width];

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            buffer[i][j] = mvwinch(win, i, j);
        }
    }

    werase(win);
    wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(win);

    mvwin(win, y, x);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mvwaddch(win, i, j, buffer[i][j]);
        }
    }

    box(win, 0, 0);

    wrefresh(win);

    return 0;
}

int lua_resize_window(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if (!win) {
        luaL_error(L, "Attempt to move on a null or invalid window");
        return 0;
    }

    int width = (int)luaL_checknumber(L, 2);
    int height = (int)luaL_checknumber(L, 3);

    lua_pushnumber(L, width);
    lua_setfield(L, -4, "width");

    lua_pushnumber(L, height);
    lua_setfield(L, -4, "height");

    wresize(win, height, width);

    wrefresh(win);

    return 0;
}

int lua_window_set_color(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if (!win) {
        luaL_error(L, "Attempt to set color on a null or invalid window");
        return 0;
    }

    int pair = (int) luaL_checknumber(L, 2);

    if (pair <= 0 || pair > COLOR_PAIRS) {
        luaL_error(L, "Invalid color pair number: %d (%d)", pair, COLOR_PAIRS);
        return 0;
    }

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "color_pair");
        if(lua_isinteger(L, -1)) {
            int prev_pair = lua_tointeger(L, -1);
            wattroff(win, COLOR_PAIR(prev_pair));
        }
        lua_pop(L, 1);

        lua_pushinteger(L, pair);
        lua_setfield(L, 1, "color_pair");
    }

    wattron(win, COLOR_PAIR(pair));

    return 0;
}

int lua_window_clear_color(lua_State* L) {
    WINDOW* win = get_subwindow(L);
    if (!win) {
        luaL_error(L, "Attempt to set color on a null or invalid window");
        return 0;
    }

    lua_getfield(L, 1, "color_pair");
    if(lua_isinteger(L, -1)) {
        int prev_pair = lua_tointeger(L, -1);
        wattroff(win, COLOR_PAIR(prev_pair));
    }
    lua_pop(L, 1);

    lua_pushnil(L);
    lua_setfield(L, 1, "color_pair");

    return 0;
}

int lua_create_window(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    if(!initialized) {
        if (initscr() == nullptr) {
            luaL_error(L, "Error initializing ncurses.\n");
            return 0;
        }

        noecho();
        cbreak();
        start_color();
        keypad(stdscr, TRUE);
        initialized = true;
        
        lua_getglobal(L, "ui");
        lua_pushlightuserdata(L, stdscr);
        lua_setfield(L, -2, "main_window");
    }
    
    if(!lua_get_main_window(L))
        return 0;

    WINDOW* main = (WINDOW*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if(!main)
        main = stdscr;

    if(!main) {
        luaL_error(L, "Failed to retrieve main_window");
        return 0;
    }

    lua_getfield(L, 1, "x");
    int x = lua_isnumber(L, -1) ? (int)lua_tonumber(L, -1) : 0;
    lua_pop(L, 1);
    lua_getfield(L, 1, "y");
    int y = lua_isnumber(L, -1) ? (int)lua_tonumber(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, 1, "width");
    int width = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "height");
    int height = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    WINDOW* new_window = subwin(main, height, width, x, y);
    if (!new_window) {
        luaL_error(L, "Failed to create subwindow: dimensions out of bounds");
        return 0;
    }

    box(new_window, 0, 0);

    lua_newtable(L);

    lua_pushlightuserdata(L, new_window);
    lua_setfield(L, -2, "window");

    lua_pushcfunction(L, lua_refresh_window);
    lua_setfield(L, -2, "refresh");

    lua_pushcfunction(L, lua_delete_window);
    lua_setfield(L, -2, "destroy");

    lua_pushcfunction(L, lua_print_window);
    lua_setfield(L, -2, "print");

    lua_pushcfunction(L, lua_clear_window);
    lua_setfield(L, -2, "clear");

    lua_pushcfunction(L, lua_move_window);
    lua_setfield(L, -2, "move");

    lua_pushcfunction(L, lua_resize_window);
    lua_setfield(L, -2, "resize");

    lua_pushcfunction(L, lua_move_cursor);
    lua_setfield(L, -2, "set_cursor");

    lua_pushcfunction(L, lua_window_set_color);
    lua_setfield(L, -2, "set_color");

    lua_pushcfunction(L, lua_window_clear_color);
    lua_setfield(L, -2, "clear_color");

    lua_pushnil(L);
    lua_setfield(L, -2, "color_pair");

    lua_pushnumber(L, height);
    lua_setfield(L, -2, "height");

    lua_pushnumber(L, width);
    lua_setfield(L, -2, "width");

    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");

    return 1;
}

int lua_input_loop(lua_State* L) {
    if(!lua_isfunction(L, 1)) {
        luaL_error(L, "Expected a function as the first argument got %s.", lua_typename(L, 1));
        return 0;
    }

    char input[256];
    int index = 0;
    bool loop = true;

    while(loop) {
        int ch = getch();
        if (ch == ERR) {
            luaL_error(L, "Error occured while getting input.");
            return 0;
        }

        if (index > 0 && (ch == KEY_DL || ch == KEY_BACKSPACE)) {
            input[--index] = '\0';
        } else if (index < sizeof(input) - 1) {
            input[index++] = (char)ch;
            input[index] = '\0';
        }

        lua_pushvalue(L, 1);
        lua_pushstring(L, input);

        if(lua_pcall(L, 1, 1, 0) != LUA_OK) {
            luaL_error(L, "Error calling input loop: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
            return 0;
        }

        if(!lua_isboolean(L, -1)) {
            luaL_error(L, "Expected input loop to return a boolean.");
            lua_pop(L, 1);
            return 0;
        }

        loop = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }

    return 0;
}

int lua_char_loop(lua_State* L) {
    if(!lua_isfunction(L, 1)) {
        luaL_error(L, "Expected a function as the first argument got %s.", lua_typename(L, 1));
        return 0;
    }

    bool loop = true;

    while(loop) {
        int ch = getch();
        if (ch == ERR) {
            luaL_error(L, "Error occured while getting input.");
            return 0;
        }

        lua_pushvalue(L, 1);

        char *key_str;
        switch(ch) {
            case KEY_UP:
                key_str = strdup("UP");
                break;
            case KEY_DOWN:
                key_str = strdup("DOWN");
                break;
            case KEY_LEFT:
                key_str = strdup("LEFT");
                break;
            case KEY_RIGHT:
                key_str = strdup("RIGHT");
                break;
            case KEY_BACKSPACE:
                key_str = strdup("BACKSPACE");
                break;
            case KEY_DC:
                key_str = strdup("DELETE");
                break;
            case KEY_IC:
                key_str = strdup("INSERT");
                break;
            case KEY_ENTER:
                key_str = strdup("ENTER");
                break;
            case KEY_BTAB:
                key_str = strdup("BTAB");
                break;
            case KEY_HOME:
                key_str = strdup("HOME");
                break;
            case KEY_END:
                key_str = strdup("END");
                break;
            case KEY_PPAGE:
                key_str = strdup("PAGE_UP");
                break;
            case KEY_NPAGE:
                key_str = strdup("PAGE_DOWN");
                break;
            case KEY_F(1):
                key_str = strdup("F1");
                break;
            case KEY_F(2):
                key_str = strdup("F2");
                break;
            case KEY_F(3):
                key_str = strdup("F3");
                break;
            case KEY_F(4):
                key_str = strdup("F4");
                break;
            case KEY_F(5):
                key_str = strdup("F5");
                break;
            case KEY_F(6):
                key_str = strdup("F6");
                break;
            case KEY_F(7):
                key_str = strdup("F7");
                break;
            case KEY_F(8):
                key_str = strdup("F8");
                break;
            case KEY_F(9):
                key_str = strdup("F9");
                break;
            case KEY_F(10):
                key_str = strdup("F10");
                break;
            case KEY_F(11):
                key_str = strdup("F11");
                break;
            case KEY_F(12):
                key_str = strdup("F12");
                break;
            default:
                key_str = (char*)malloc(2);
                key_str[0] = ch;
                key_str[1] = '\0';
        }

        lua_pushstring(L, key_str);
        free(key_str);

        if(lua_pcall(L, 1, 1, 0) != LUA_OK) {
            luaL_error(L, "Error calling input loop: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
            return 0;
        }

        if(!lua_isboolean(L, -1)) {
            luaL_error(L, "Expected input loop to return a boolean.");
            lua_pop(L, 1);
            return 0;
        }

        loop = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }

    return 0;
}

int mt_main_index(lua_State* L) {
    const char* key = lua_tostring(L, 2);

    if(strcmp(key, "cols") == 0) {
        int row, col;
        getmaxyx(stdscr, row, col);

        lua_pushnumber(L, col);
        return 1;
    } else if(strcmp(key, "rows") == 0) {
        int row, col;
        getmaxyx(stdscr, row, col);

        lua_pushnumber(L, row);
        return 1;
    }

    lua_gettable(L, 1);
    return 1;
}

int lua_init_pair(lua_State* L) {
    int pair = (int) luaL_checknumber(L, 1);
    int color_1 = (int) luaL_checknumber(L, 2);
    int color_2 = (int) luaL_checknumber(L, 3);

    init_extended_pair(pair, color_1, color_2);
    return 0;
}

int lua_init_curse(lua_State* L) {
    if(!initialized) {
        if (initscr() == nullptr) {
            luaL_error(L, "Error initializing ncurses.\n");
            return 0;
        }

        noecho();
        cbreak();
        start_color();
        keypad(stdscr, TRUE);
        initialized = true;
        
        lua_getglobal(L, "ui");
        lua_pushlightuserdata(L, stdscr);
        lua_setfield(L, -2, "main_window");
    }

    return 0;
}

void load_ui_library(lua_State* L) {
    lua_newtable(L);

    lua_pushlightuserdata(L, nullptr);
    lua_setfield(L, -2, "main_window");

    lua_pushcfunction(L, lua_input_loop);
    lua_setfield(L, -2, "on_input");

    lua_pushcfunction(L, lua_char_loop);
    lua_setfield(L, -2, "on_key");

    lua_pushcfunction(L, lua_set_cursor_type);
    lua_setfield(L, -2, "set_cursor_type");

    lua_pushcfunction(L, lua_create_window);
    lua_setfield(L, -2, "create");

    lua_pushcfunction(L, lua_init_curse);
    lua_setfield(L, -2, "init");

    lua_newtable(L);
    for (int i = 0; i < 8; ++i) {
        lua_pushinteger(L, i);
        lua_setfield(L, -2, color_names[i]);
    }   
    lua_setfield(L, -2, "color");

    lua_pushcfunction(L, lua_init_pair);
    lua_setfield(L, -2, "init_pair");

    lua_newtable(L);

    lua_pushcfunction(L, mt_main_index);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    lua_setglobal(L, "ui");
}
