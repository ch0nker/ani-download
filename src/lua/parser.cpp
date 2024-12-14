#include "libxml/parser.h"
#include <cstring>

extern "C" {
    #include <libxml/HTMLparser.h>
    #include <libxml/xpath.h>
    #include <lua.h>
    #include <lauxlib.h>
}

int lua_node_search(lua_State* L);

void populate_node_table(lua_State* L, xmlNodePtr node) {
    lua_newtable(L);

    lua_pushstring(L, (const char*)node->name);
    lua_setfield(L, -2, "name");

    xmlChar* content = xmlNodeGetContent(node);
    lua_pushstring(L, content ? (const char*)content : "");
    lua_setfield(L, -2, "text");
    if (content) xmlFree(content);

    lua_newtable(L);
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
        if (value) {
            lua_pushstring(L, (const char*)value);
            lua_setfield(L, -2, (const char*)attr->name);
            xmlFree(value);
        }
    }
    lua_setfield(L, -2, "attributes");

    lua_newtable(L);
    int index = 1;
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE) {
            populate_node_table(L, child); 
            lua_rawseti(L, -2, index++);
        }
    }
    lua_setfield(L, -2, "children");

    lua_pushlightuserdata(L, node);
    lua_setfield(L, -2, "node_ptr");

    lua_pushcfunction(L, lua_node_search);
    lua_setfield(L, -2, "xpath");
}

int lua_node_search(lua_State* L) {
    if (!lua_istable(L, 1)) {
        luaL_error(L, "Expected self to be a table.");
        return 0;
    }

    if (!lua_isstring(L, 2)) {
        luaL_error(L, "Expected XPath to be a string.");
        return 0;
    }

    xmlChar* xpath = (xmlChar*) lua_tostring(L, 2);

    lua_getfield(L, 1, "node_ptr");
    if (!lua_islightuserdata(L, -1)) {
        luaL_error(L, "Expected 'node_ptr' to be a light userdata.");
        return 0;
    }

    xmlNodePtr node = (xmlNodePtr)lua_touserdata(L, -1);
    lua_pop(L, 1);

    // Create a new XPath context for each evaluation
    xmlXPathContextPtr ctx = xmlXPathNewContext(node->doc);
    if (ctx == NULL) {
        luaL_error(L, "Failed to create XPath context.");
        return 0;
    }

    // Set the context node for each XPath evaluation
    if (xmlXPathSetContextNode(node, ctx) != 0) {
        xmlXPathFreeContext(ctx);
        luaL_error(L, "Failed to set context node.");
        return 0;
    }

    xmlXPathObjectPtr obj = xmlXPathEvalExpression(xpath, ctx);
    lua_newtable(L);

    if (!obj->nodesetval || obj->nodesetval->nodeNr == 0) {
        xmlXPathFreeObject(obj);
        xmlXPathFreeContext(ctx);
        return 1;
    }

    // Iterate over the result nodes
    for (int i = 0; i < obj->nodesetval->nodeNr; i++) {
        xmlNodePtr node = obj->nodesetval->nodeTab[i];
        if (node) {
            populate_node_table(L, node);  // Populate the node's data into the Lua table
            lua_rawseti(L, -2, i + 1);     // Store the node result in the Lua table
        }
    }

    xmlXPathFreeObject(obj);
    xmlXPathFreeContext(ctx);

    return 1;
}

int lua_xml_xpath(lua_State* L) {
    if (!lua_istable(L, 1)) {
        luaL_error(L, "Expected self to be a table.");
        return 0;
    }

    if(!lua_isstring(L, 2)) {
        luaL_error(L, "Expected to XPath to be a string.");
        return 0;
    }

    xmlChar* xpath = (xmlChar*) lua_tostring(L, 2);

    lua_getfield(L, 1, "root");
    if (!lua_islightuserdata(L, -1)) {
        luaL_error(L, "Expected 'root' to be a light userdata.");
        return 0;
    }

    htmlDocPtr doc = (htmlDocPtr) lua_touserdata(L, -1);
    lua_pop(L, 1);

    xmlXPathContextPtr ctx = xmlXPathNewContext(doc); 
    xmlXPathObjectPtr elements = xmlXPathEvalExpression(xpath, ctx);

    if (!elements->nodesetval || elements->nodesetval->nodeNr == 0) {
        xmlXPathFreeObject(elements);
        xmlXPathFreeContext(ctx);
        lua_newtable(L);
        return 1;
    }

    lua_newtable(L);
    for(int i = 0; i < elements->nodesetval->nodeNr; i++) {
        xmlNodePtr node = elements->nodesetval->nodeTab[i];
        if(node) {
            populate_node_table(L, node);
            lua_rawseti(L, -2, i + 1);
        }
    }
    xmlXPathFreeObject(elements);
    xmlXPathFreeContext(ctx);
    return 1;
}

int lua_parse_html(lua_State* L) {
    if(!lua_isstring(L, -1)) {
        luaL_error(L, "First argument needs to be a string.");
        return 0;
    }

    const char* html = lua_tostring(L, -1);
    htmlDocPtr doc = htmlReadMemory(html, strlen(html), nullptr, nullptr, HTML_PARSE_NOERROR);

    if(doc == nullptr) {
        luaL_error(L, "Failed to parse HTML.");
        return 0;
    }

    lua_newtable(L);

    lua_pushlightuserdata(L, doc);
    lua_setfield(L, -2, "root");

    lua_pushcfunction(L, lua_xml_xpath);
    lua_setfield(L, -2, "xpath");

    return 1;
}

void load_html_library(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, lua_parse_html);
    lua_setfield(L, -2, "parse");

    lua_setglobal(L, "html");
}
