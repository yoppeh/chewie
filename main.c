/**
 * @file main.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Main program.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 * @details
 * Main program. Parses command line arguments into a JSON object. The results
 * of that are used first to execute any actions that are specified, such as
 * dumping the query history or listing the available models. Then, assuming 
 * the given command line actions don't result in exiting the program (listing
 * models for example, exits after printing the list), the correct API 
 * interface is selected and the query is sent to the API.
 */
 
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <json-c/json.h>

#include "chewie.h"
#include "action.h"
#include "api.h"
#include "configure.h"
#include "context.h"
#include "file.h"
#include "groq.h"
#include "input.h"
#include "option.h"
#include "setting.h"
#include "ollama.h"
#include "openai.h"

#ifdef DEBUG
int indent_level = 0;
char *indent_string = "                                                                                ";
#endif

int main(int ac, char **av) {
    debug_enter();
    json_object *actions_obj = json_object_new_object();
    json_object *settings_obj = json_object_new_object();
    int result = 1;
    if (actions_obj == NULL) {
        fprintf(stderr, "Error initializing JSON object\n");
        goto term;
    }
    if (configure(actions_obj, settings_obj, ac, av)) {
        goto term;
    }
    json_object *api_opt = NULL;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_PROVIDER, &api_opt)) {
        const api_id_t api = api_name_to_id(json_object_get_string(api_opt));
        api_interface = (api_interface_t *)api_interfaces[api]();
    }
    if (!json_object_object_get_ex(actions_obj, ACTION_KEY_QUERY, NULL)) {
        json_object_object_add(actions_obj, ACTION_KEY_QUERY, NULL);
    }
    action_result_t action_result = action_execute_all(actions_obj, settings_obj);
    debug("Completed actions with result = %i\n", action_result);
    if (action_result == ACTION_END || action_result == ACTION_CONTINUE) {
        result = 0;
        goto term;
    } else if (action_result == ACTION_ERROR) {
        result = 1;
        goto term;
    }
term:
    if (result == 0) {
        context_update();
    }
    if (actions_obj != NULL) {
        json_object_put(actions_obj);
    }
    if (settings_obj != NULL) {
        json_object_put(settings_obj);
    }
    debug("Exiting with result = %i\n", result);
    debug_return result;
}
