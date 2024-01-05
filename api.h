/**
 * api.h
 *
 * Interface to the backend API modules.
 */

#ifndef _API_H
#define _API_H

#include <json-c/json_object.h>

#include "action.h"
#include "option.h"

/// API function that returns a json object.
typedef action_t **(*api_get_action_func_t)(void);
/// API function that returns a json object.
typedef option_t **(*api_get_option_func_t)(void);
/// API functions that return a string. get_default_host(), for example.
typedef const char *(*api_get_func_t)(void);
/// API functions that print to stdout.
typedef int (*api_print_func_t)(json_object *options);
/// API function that queries the host.
typedef const char *(*api_query_func_t)(json_object *options);

/// AIP API ID.
typedef enum api_id_t {
    api_id_none = 0,
    api_id_ollama = 1,
    api_id_openai = 2,
    api_id_max = api_id_openai,
    api_id_default = api_id_ollama
} api_id_t;
/// API interface.
typedef struct api_interface_t {
    api_get_action_func_t   get_actions;        // Get actions for AI API.
    api_get_option_func_t   get_options;        // Get option templates for AI API.
    api_get_func_t          get_default_host;   // Get default host for AI API.
    api_get_func_t          get_default_model;  // Get default model.
    api_get_func_t          get_api_name;       // Get API name.
    api_print_func_t        get_embeddings;     // Get embeddings.
    api_print_func_t        print_model_list;   // Print list of models.
    api_query_func_t        query;              // Query the host.
} api_interface_t;

typedef const api_interface_t *(*get_api_interface_func_t)(void);

/// List of AIP API interface functions, ordered by api_id_t.
extern const get_api_interface_func_t api_interfaces[];
/// The AIP API interface currently in use.
extern api_interface_t *api_interface;
/// Convert an API name to an api_id_t.
extern api_id_t api_name_to_id(const char *name);
/// Get the API interface for the given api_id_t.
extern const api_interface_t *api_get_aip_interface(api_id_t id);

#endif // _API_H
