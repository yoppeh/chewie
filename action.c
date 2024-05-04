/**
 * @file action.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Actions that are performed in response to certain command line arguments.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#include <stdio.h>
#include <string.h>

#include <json-c/json_object.h>

#include "chewie.h"
#include "action.h"
#include "api.h"
#include "configure.h"
#include "context.h"
#include "file.h"
#include "input.h"
#include "setting.h"

static action_result_t dump_query_history(json_object *settings, json_object *data);
static action_result_t get_embeddings(json_object *settings, json_object *data);
static action_result_t list_apis(json_object *settings, json_object *data);
static action_result_t list_models(json_object *settings, json_object *data);
static action_result_t query(json_object *settings, json_object *data);
static action_result_t show_help(json_object *settings, json_object *data);
static action_result_t show_version(json_object *settings, json_object *data);
static action_result_t reset_context(json_object *settings, json_object *data);
static action_result_t update_context(json_object *settings, json_object *data);

static action_t action_version = {
    .name = ACTION_KEY_VERSION,
    .callback = show_version,
};
static action_t action_help = {
    .name = ACTION_KEY_HELP,
    .callback = show_help,
};
static action_t action_list_apis = {
    .name = ACTION_KEY_LIST_APIS,
    .callback = list_apis,
};
static action_t action_list_models = {
    .name = ACTION_KEY_LIST_MODELS,
    .callback = list_models,
};
static action_t action_query = {
    .name = ACTION_KEY_QUERY,
    .callback = query,
};
static action_t action_reset_context = {
    .name = ACTION_KEY_RESET_CONTEXT,
    .callback = reset_context,
};
static action_t action_dump_query_history = {
    .name = ACTION_KEY_DUMP_QUERY_HISTORY,
    .callback = dump_query_history
};
static action_t action_update_context = {
    .name = ACTION_KEY_UPDATE_CONTEXT,
    .callback = update_context
};
static action_t action_get_embeddings = {
    .name = ACTION_KEY_GET_EMBEDDINGS,
    .callback = get_embeddings
};
static action_t *action_templates[] = {
    &action_version,
    &action_help,
    &action_list_apis,
    &action_list_models,
    &action_reset_context,
    &action_dump_query_history,
    &action_update_context,
    &action_get_embeddings,
    &action_query,
    NULL
};
static action_t **merged_actions = action_templates;

static action_t **action_merge(action_t **actions1, action_t **actions2);

action_result_t action_execute_all(json_object *actions, json_object *settings) {
    debug_enter();
    debug("actions: %s\n", json_object_to_json_string_ext(actions, JSON_C_TO_STRING_PRETTY));
    for (api_id_t i = 0; i < api_id_max; i++) {
        const api_interface_t *api_interface = api_interfaces[i + 1]();
        action_t **b = api_interface->get_actions();
        if (b != NULL) {
            action_t **c = action_merge(merged_actions, b);
            if (c == NULL) {
                debug_return ACTION_ERROR;
            }
            if (merged_actions != action_templates) {
                free(merged_actions);
            }
            merged_actions = c;
        }
    }
    json_object *json_obj = NULL;
    action_result_t result = ACTION_CONTINUE;
    for (int i = 0; merged_actions[i] != NULL; i++) {
        action_t *action_template = merged_actions[i];
        if (json_object_object_get_ex(actions, action_template->name, &json_obj)) {
            if (action_template->callback != NULL) {
                action_result_t r = action_template->callback(settings, json_obj);
                if (r == ACTION_ERROR) {
                    result = ACTION_ERROR;
                    break;
                }
                if (r == ACTION_END) {
                    result = ACTION_END;
                    break;
                }
            }
        } 
    }
    debug_return result;
}

static action_result_t dump_query_history(json_object *actions, json_object *settings_obj) {
    debug_enter();
    context_dump_history();
    debug_return ACTION_END;
}

static action_result_t get_embeddings(json_object *settings, json_object *data) {
    debug_enter();
    char *query = NULL;
    json_object *prompt = NULL;
    if (json_object_object_get_ex(settings, SETTING_KEY_PROMPT, &prompt) && prompt != NULL) {
        json_object_object_add(settings, SETTING_KEY_PROMPT, prompt);
    } else {
        query = input_get();
        if (query != NULL) {
            json_object_object_add(settings, SETTING_KEY_PROMPT, json_object_new_string(query));
            free(query);
            query = NULL;
        }
    }
    if (query != NULL) {
        json_object_object_add(settings, SETTING_KEY_PROMPT, json_object_new_string(query));
        free(query);
        query = NULL;
    }
    if (api_interface->get_embeddings(settings)) {
        debug_return ACTION_ERROR;
    }
    debug_return ACTION_END;
}

static action_result_t list_apis(json_object *settings, json_object *data) {
    debug_enter();
    printf("Available APIs:\n");
    for (api_id_t i = 0; i < api_id_max; i++) {
        const api_interface_t *api_interface = api_interfaces[i + 1]();
        const char *s = api_interface->get_api_name();
        printf("    %s\n", s);
    }
    debug_return ACTION_END;
}

static action_result_t list_models(json_object *settings, json_object *data) {
    debug_enter();
    debug("data = %s\n", json_object_to_json_string_ext(data, JSON_C_TO_STRING_PRETTY));
    if (api_interface->print_model_list(settings)) {
        debug_return ACTION_ERROR;
    }
    debug_return ACTION_END;
}

static action_result_t query(json_object *settings, json_object *data) {
    debug_enter();
    char *query = NULL;
    json_object *prompt = NULL;
    json_object_object_get_ex(settings, SETTING_KEY_PROMPT, &prompt);
    if (prompt == NULL) {
        query = input_get();
        if (query != NULL) {
            json_object_object_add(settings, SETTING_KEY_PROMPT, json_object_new_string(query));
            free(query);
            query = NULL;
        }
    }
    api_interface->query(settings);
    debug_return ACTION_END;
}

static action_result_t show_help(json_object *settings, json_object *data) {
    debug_enter();
    option_show_help((option_t **)json_object_get_int64(data));
    debug_return ACTION_END;
}

static action_result_t show_version(json_object *settings, json_object *data) {
    debug_enter();
    printf("%s v. %i.%i.%i\n", program_name, VERSION_MAJOR, VERSION_MINOR, VERSION_REV);
    debug_return ACTION_END;
}

static action_result_t reset_context(json_object *settings, json_object *options) {
    debug_enter();
    json_object *context_fn = NULL;
    if (json_object_object_get_ex(settings, SETTING_KEY_CONTEXT_FILENAME, &context_fn)) {
        const char *fn = json_object_get_string(context_fn);
        file_truncate(fn);
        debug_return ACTION_CONTINUE;
    }
    debug_return ACTION_ERROR;
}

static action_result_t update_context(json_object *settings, json_object *data) {
    debug_enter();
    debug("settings = %s\n", json_object_to_json_string_ext(settings, JSON_C_TO_STRING_PRETTY));
    debug_return ACTION_END;
}

static action_t **action_merge(action_t **actions1, action_t **actions2) {
    debug_enter();
    int n1 = 0;
    int n2 = 0;
    if (actions1 != NULL) {
        while (actions1[n1] != NULL) {
            n1++;
        }
    }
    if (actions2 != NULL) {
        while (actions2[n2] != NULL) {
            n2++;
        }
    }
    int n = n1 + n2 + 1;
    action_t **actions = malloc(n * sizeof(action_t *));
    if (actions == NULL) {
        debug_return NULL;
    }
    if (actions1 != NULL) {
        memcpy(actions, actions1, n1 * sizeof(action_t *));
    }
    if (actions2 != NULL) {
        memcpy(actions + n1, actions2, n2 * sizeof(action_t *));
    }
    actions[n - 1] = NULL;
    debug_return actions;    
}
