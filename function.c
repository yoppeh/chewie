/**
 * @file function.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief LUA function implmentation.
 * @version 0.1.0
 * @date 2024-05-25
 * @copyright Copyright (c) 2024
 */

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/linkhash.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "chewie.h"
#include "file.h"
#include "function.h"
#include "setting.h"

static lua_State *state_ptr = NULL;

static void function_exit(void);
static void *alloc(void *ud, void *ptr, size_t osize, size_t nsize);
static const char *lua_reader(lua_State *state, void *data, size_t *size);
static json_object *parse_enum(void);
static json_object *parse_functions(void);
static json_object *parse_parameters(void);

int function_init(void) {
    debug_enter();
    int result = 1;
    atexit(function_exit);
    state_ptr = lua_newstate(alloc, NULL);
    if (state_ptr == NULL) {
        fprintf(stderr, "Error initializing LUA state\n");
        goto term;
    }
    result = 0;
term:
    debug_return result;
}

const char *function_invoke(const char *name, json_object *args) {
    debug_enter();
    const char *result = NULL;
    size_t len = 0;
    bool found = false;
    lua_pushnil(state_ptr);
    while (lua_next(state_ptr, -2) != 0) {
        int lt = lua_getfield(state_ptr, -1, "name");
        if (lt != LUA_TSTRING) {
            fprintf(stderr, "Error parsing function table: name is not a string\n");
            goto term;
        }
        const char *ln = lua_tostring(state_ptr, -1);
        lua_pop(state_ptr, 1);
        if (strcmp(name, ln) == 0) {
            debug("Function found: %s\n", name);
            lua_getfield(state_ptr, -1, "code");
            found = true;
            break;
        }
        lua_pop(state_ptr, 1);
    }
    if (!found) {
        fprintf(stderr, "Function not found: %s\n", name);
        goto term;
    }
    json_object *arguments = json_tokener_parse(json_object_get_string(args));
    debug("Arguments: %s\n", json_object_to_json_string(arguments));
    debug("Arguments type: %i\n", json_object_get_type(arguments));
    if (arguments != NULL) {
        if (json_object_is_type(arguments, json_type_array)) {
            len = json_object_array_length(arguments);
            debug("Arguments length: %zu\n", len);
            for (size_t i = 0; i < len; i++) {
                lua_pushstring(state_ptr, json_object_get_string(json_object_array_get_idx(arguments, i)));
            }
        } else if (json_object_is_type(arguments, json_type_object)) {
            len = 0;
            json_object_object_foreach(arguments, key, val) {
                len++;
                debug("Pushing argument value %s\n", json_object_get_string(val));
                lua_pushstring(state_ptr, json_object_get_string(val));
            }
        } else {
            fprintf(stderr, "Error parsing arguments: not an array or object\n");
            goto term;
        }
    }
    debug("Invoking function: %s with %zu arguments\n", name, len);
    int res = lua_pcall(state_ptr, len, 1, 0);
    if (res != LUA_OK) {
        fprintf(stderr, "Error executing function (%s): %i\n", name, res);
        goto term;
    }
    result = lua_tostring(state_ptr, -1);
    lua_pop(state_ptr, 1);
    debug("Function result: %s\n", result);
term:
    debug_return result;
}

int function_load(const char *fn, json_object *settings) {
    debug_enter();
    json_object *tools = NULL;
    int res = 1;
    int lua_res = lua_load(state_ptr, lua_reader, (void *)fn, NULL, NULL);
    if (lua_res != LUA_OK) {
        fprintf(stderr, "Error loading LUA file (%s)\n", fn);
        goto term;
    }
    luaL_openlibs(state_ptr);
    debug("Loaded LUA file (%s)\n", fn);
    res = lua_pcall(state_ptr, 0, LUA_MULTRET, 0);
    if (res != LUA_OK) {
        fprintf(stderr, "Error executing code in LUA file (%s): %i\n", fn, res);
        goto term;
    }
    tools = parse_functions();
    if (tools != NULL) {
        res = 0;
        json_object_object_add(settings, SETTING_KEY_TOOLS, tools);
    }
term:
    debug_return res;
}

static void *alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize;
    if (nsize == 0) {
        free(ptr);
        return NULL;;
    }
    return realloc(ptr, nsize);
}

static void function_exit(void) {
    debug_enter();
    if (state_ptr != NULL) {
        lua_close(state_ptr);
        state_ptr = NULL;
    }
    debug_return;
}

static const char *lua_reader(lua_State *state, void *data, size_t *size) {
    debug_enter();
    static bool file_read_done = false;
    if (file_read_done) {
        file_read_done = false;
        *size = 0;
        debug_return NULL;
    }
    char *lua = file_read((char *)data);
    (void)state;
    if (lua == NULL) {
        *size = 0;
        debug_return NULL;
    }
    *size = strlen(lua);
    file_read_done = true;
    debug_return lua;
}

static json_object *parse_enum(void) {
    debug_enter();
    json_object *enums = json_object_new_array();
    json_object *results = NULL;
    if (enums == NULL) {
        fprintf(stderr, "Error creating JSON enum object\n");
        goto term;
    }
    size_t l = luaL_len(state_ptr, -1) + 1;
    for (size_t i = 1; i < l; i++) {
        lua_rawgeti(state_ptr, -1, i);
        switch (lua_type(state_ptr, -1)) {
            case LUA_TSTRING:
                json_object_array_add(enums, json_object_new_string(lua_tostring(state_ptr, -1)));
                break;
            case LUA_TNUMBER:
                json_object_array_add(enums, json_object_new_double(lua_tonumber(state_ptr, -1)));
                break;
            default:
                fprintf(stderr, "Error parsing choices: value is not a string or number\n");
                goto term;
        }
        lua_pop(state_ptr, 1);
    }
    results = enums;
term:
    debug_return results;
}

static json_object *parse_functions(void) {
    debug_enter();
    json_object *result = NULL;
    json_object *tools = json_object_new_array();
    if (tools == NULL) {
        fprintf(stderr, "Error creating JSON array\n");
        goto term;
    }
    debug("Parsing function table\n");
    lua_pushnil(state_ptr);
    while (lua_next(state_ptr, -2) != 0) {
        const char *description = NULL;
        const char *name = NULL;
        json_object *tool = json_object_new_object();
        json_object *function = json_object_new_object();
        json_object *parameters = NULL;
        if (tool == NULL) {
            fprintf(stderr, "Error creating JSON tool object\n");
            goto term;
        }
        if (function == NULL) {
            fprintf(stderr, "Error creating JSON function object\n");
            goto term;
        }
        int lt = lua_getfield(state_ptr, -1, "name");
        if (lt == LUA_TSTRING) {
            name = lua_tostring(state_ptr, -1);
        } else if (lt != LUA_TNIL) {
            fprintf(stderr, "Error parsing function table: name is not a string\n");
            goto term;
        } else {
            fprintf(stderr, "Error parsing function table: name is missing\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        lt = lua_getfield(state_ptr, -1, "description");
        if (lt == LUA_TSTRING) {
            description = lua_tostring(state_ptr, -1);
        } else if (lt != LUA_TNIL) {
            fprintf(stderr, "Error parsing function table: description is not a string\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        lt = lua_getfield(state_ptr, -1, "parameters");
        if (lt == LUA_TTABLE) {
            parameters = parse_parameters();
       } else if (lt != 0) {
            fprintf(stderr, "Error parsing function table: parameters is not a table\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        debug("Function name .......... %s\n", name);
        debug("Function description ... %s\n", description);
        json_object_object_add(tool, "type", json_object_new_string("function"));
        json_object_object_add(function, "name", json_object_new_string(name));
        json_object_object_add(function, "description", json_object_new_string(description));
        if (parameters != NULL) {
            json_object_object_add(function, "parameters", parameters);
        } 
        json_object_object_add(tool, "function", function);
        json_object_array_add(tools, tool);
        lua_pop(state_ptr, 1);
        debug("Function %s\n", json_object_to_json_string(tool));
    }
    result = tools;
term:
    debug_return result;
}

static json_object *parse_parameters(void) {
    debug_enter();
    json_object *properties = json_object_new_object();
    json_object *parameters = json_object_new_object();
    json_object *required = json_object_new_array();
    json_object *results = NULL;
    if (parameters == NULL || properties == NULL) {
        fprintf(stderr, "Error creating JSON parameters object\n");
        goto term;
    }
    lua_pushnil(state_ptr);
    while (lua_next(state_ptr, -2) != 0) {
        json_object *parameter = json_object_new_object();
        const char *pname = NULL;
        const char *ptype = NULL;
        json_object *penum = NULL;
        const char *pdesc = NULL;
        if (parameter == NULL) {
            fprintf(stderr, "Error creating JSON parameter object\n");
            goto term;
        }
        int lt = lua_getfield(state_ptr, -1, "name");
        if (lt == LUA_TSTRING) {
            pname = lua_tostring(state_ptr, -1);
        } else if (lt != LUA_TNIL) {
            fprintf(stderr, "Error parsing function table: parameter name is not a string\n");
            goto term;
        } else {
            fprintf(stderr, "Error parsing function table: parameter name is missing\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        lt = lua_getfield(state_ptr, -1, "type");
        if (lt == LUA_TSTRING) {
            ptype = lua_tostring(state_ptr, -1);
        } else if (lt != LUA_TNIL) {
            fprintf(stderr, "Error parsing function table: parameter type is not a string\n");
            goto term;
        } else {
            fprintf(stderr, "Error parsing function table: parameter type is missing\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        lt = lua_getfield(state_ptr, -1, "choices");
        if (lt == LUA_TTABLE) {
            penum = parse_enum();
        } else if (lt != LUA_TNIL) {
            fprintf(stderr, "Error parsing function table: parameter enum is not a string\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        lt = lua_getfield(state_ptr, -1, "description");
        if (lt != LUA_TSTRING) {
            fprintf(stderr, "Error parsing function table: parameter description is not a string\n");
            goto term;
        }
        pdesc = lua_tostring(state_ptr, -1);
        lua_pop(state_ptr, 1);
        lt = lua_getfield(state_ptr, -1, "required");
        if (lt == LUA_TBOOLEAN) {
            if (lua_toboolean(state_ptr, -1)) {
                json_object_array_add(required, json_object_new_string(pname));
            }
        } else if (lt != LUA_TNIL) {
            fprintf(stderr, "Error parsing function table: parameter required is not a boolean\n");
            goto term;
        }
        lua_pop(state_ptr, 1);
        debug("Parameter name .......... %s\n", pname);
        debug("Parameter type .......... %s\n", ptype);
        debug("Parameter enum .......... %s\n", json_object_to_json_string(penum));
        debug("Parameter description ... %s\n", pdesc);
        json_object_object_add(parameter, "type", json_object_new_string(ptype));
        if (penum != NULL) {
            json_object_object_add(parameter, "enum", penum);
        }
        json_object_object_add(parameter, "description", json_object_new_string(pdesc));
        json_object_object_add(properties, pname, parameter);
        lua_pop(state_ptr, 1);
    } 
    json_object_object_add(parameters, "properties", properties);
    json_object_object_add(parameters, "type", json_object_new_string("object"));
    if (required != NULL && json_object_array_length(required) > 0) {
        json_object_object_add(parameters, "required", required);
    }
    results = parameters;
term:
    debug_return results;
}
