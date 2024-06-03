/**
 * @file function.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief LUA function interface.
 * @version 0.1.0
 * @date 2024-05-25
 * @copyright Copyright (c) 2024
 */

#ifndef _FUNCTION_H

#include <json-c/json_object.h>
#include <lua.h>

/** 
 * @brief Initialize the LUA state.
 * @return 0 on success, 1 on failure.
 */
extern int function_init(void);

/** 
 * @brief Invoke the given function.
 * @param name Name of the function to invoke.
 * @param parameters JSON object containing the function parameters.
 * @return string containing the function result.
 */
extern const char *function_invoke(const char *name, json_object *parameters);

/** 
 * @brief Load the given LUA file.
 * @param fn Full path/name of the file to load.
 * @param settings JSON object to store the function settings.
 * @return 0 on success, 1 on failure.
 */
extern int function_load(const char *fn, json_object *settings);

#endif // _FUNCTION_H
