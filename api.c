/**
 * api.c
 * 
 * Interface to the backend API modules.
 */

#include <string.h>

#include "api.h"

#include "ollama.h"
#include "openai.h"

/**
 * List of API interfaces, indexed by api_id_t. These functions are called to
 * initialize the api_interface global variable, which contains pointers to the
 * functions that are used to interact with the API.
 */
const get_api_interface_func_t api_interfaces[] = {
    NULL,
    ollama_get_aip_interface,
    openai_get_aip_interface
};
api_interface_t *api_interface;

api_id_t api_name_to_id(const char *name) {
    if (name != NULL) {
        for (api_id_t i = 0; i < api_id_max; i++) {
            const api_interface_t *api_interface = api_interfaces[i + 1]();
            const char *s = api_interface->get_api_name();
            if (strncmp(name, s, strlen(s)) == 0) {
                return i + 1;
            }
        }
    }
    return api_id_none;
}

const api_interface_t *api_get_aip_interface(api_id_t id) {
    return api_interfaces[id]();
}
