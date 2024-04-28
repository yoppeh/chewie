/**
 * @file api.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Interface to the backend API modules.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#ifndef _API_H
#define _API_H

#include <json-c/json_object.h>

#include "action.h"
#include "option.h"

/** @brief API function that returns a json object. */
typedef action_t **(*api_get_action_func_t)(void);
/** @brief API function that returns a json object. */
typedef option_t **(*api_get_option_func_t)(void);
/** @brief API functions that return a string. */
typedef const char *(*api_get_func_t)(void);
/** @brief API functions that print to stdout. */
typedef int (*api_print_func_t)(json_object *options);
/** @brief API function that queries the host. */
typedef const char *(*api_query_func_t)(json_object *options);

/**
 * @brief AIP API ID.
 * @note API = Application Programming Interface. AIP = A.I. Provider
 */
typedef enum api_id_t {
    api_id_none = 0,
    api_id_ollama = 1,
    api_id_openai = 2,
    api_id_groq = 3,
    api_id_max = api_id_groq,
    api_id_default = api_id_ollama
} api_id_t;

/** @brief API interface. */
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

/** @brief List of AIP API interface functions, ordered by api_id_t. */
extern const get_api_interface_func_t api_interfaces[];
/** @brief The AIP API interface currently in use. */
extern api_interface_t *api_interface;

/** 
 * @brief Convert an API name to an api_id_t. 
 * @param name The name of the API.
 * @return api_id_t
 */
extern api_id_t api_name_to_id(const char *name);

/**
 * @brief  Get the API interface for the given api_id_t.
 * @param id API ID.
 * @return * const api_interface_t* 
 */
extern const api_interface_t *api_get_aip_interface(api_id_t id);

#endif // _API_H
