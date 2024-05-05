/**
 * @file configure.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Configure environment and setup actions.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 * @details
 * Configure according to command line arguments and environment. Uses 
 * getopt() to parse command line arguments and json-c to store the results in 
 * a JSON object. That object is then sent to the action processor to execute 
 * any actions specified, such as dumping the query. The object is then passed 
 * on to the API interface to be used to send the proper query to the API.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <json-c/json_object.h>

#include "chewie.h"
#include "action.h"
#include "api.h"
#include "configure.h"
#include "context.h"
#include "file.h"
#include "option.h"
#include "setting.h"

const char *list_argument = "?";
const char *program_name = NULL;

static int set_missing_aih(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_aip(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_ctx(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_mdl(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_qry(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_sys(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_aih_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_aip_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_ctx_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_buf_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_emb_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_his_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_mdl_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_qry_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_sys_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_h_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_r_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_u_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int option_v_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static option_t option_ctx = {
    .name = "ctx",
    .description = "Set the context file path/name.",
    .arg_type = option_arg_required,
    .value = NULL,
    .validate = option_ctx_validate,
    .set_missing = set_missing_ctx
};
static option_t option_aip = {
    .name = "aip",
    .description = "Set the AI provider. Use \"?\" to list available providers.",
    .arg_type = option_arg_required,
    .value = NULL,
    .validate = option_aip_validate,
    .set_missing = set_missing_aip
};
static option_t option_aih = {
    .name = "aih",
    .description = "Set the AI provider host.",
    .arg_type = option_arg_required,
    .value = NULL,
    .validate = option_aih_validate,
    .set_missing = set_missing_aih
};
static option_t option_mdl = {
    .name = "mdl",
    .description = "Set the language model. Use \"?\" to list available models.",
    .arg_type = option_arg_required,
    .value = NULL,
    .validate = option_mdl_validate,
    .set_missing = set_missing_mdl
};
static option_t option_qry = {
    .name = "qry",
    .description = "Set the query.",
    .arg_type = option_arg_optional,
    .value = NULL,
    .validate = option_qry_validate,
    .set_missing = set_missing_qry
};
static option_t option_h = {
    .name = "h",
    .description = "Print this help message.",
    .arg_type = option_arg_none,
    .value = NULL,
    .validate = option_h_validate
};
static option_t option_r = {
    .name = "r",
    .description = "Reset the context file.",
    .arg_type = option_arg_none,
    .value = NULL,
    .validate = option_r_validate
};
static option_t option_u = {
    .name = "u",
    .description = "Update context file and exit.",
    .arg_type = option_arg_none,
    .value = NULL,
    .validate = option_u_validate
};
static option_t option_v = {
    .name = "v",
    .description = "Print the version.",
    .arg_type = option_arg_none,
    .value = NULL,
    .validate = option_v_validate
};
static option_t option_buf = {
    .name = "buf",
    .description = "Buffer the response instead of printing as it comes in.",
    .arg_type = option_arg_none,
    .value = NULL,
    .validate = option_buf_validate,
};
static option_t option_his = {
    .name = "his",
    .description = "Print the query/response history.",
    .arg_type = option_arg_none,
    .value = NULL,
    .validate = option_his_validate
};
static option_t option_sys = {
    .name = "sys",
    .description = "Set the system prompt.",
    .arg_type = option_arg_required,
    .value = NULL,
    .validate = option_sys_validate,
    .set_missing = set_missing_sys
};
static option_t option_emb = {
    .name = "emb",
    .description = "Generate embeddings for the input text.",
    .arg_type = option_arg_optional,
    .value = NULL,
    .validate = option_emb_validate
};
static option_t *common_options[] = {
    &option_buf,
    &option_aip,
    &option_aih,
    &option_ctx,
    &option_emb,
    &option_his,
    &option_mdl,
    &option_qry,
    &option_sys,
    &option_h,
    &option_r,
    &option_u,
    &option_v,
    NULL
};
static option_t **options = common_options;

static int merge_api_options(void);

int configure(json_object *actions_obj, json_object *settings_obj, int ac, char **av) {
    json_object *context_fn_obj = NULL;
    debug_enter();
    if (merge_api_options()) {
        debug_return 1;
    }
    program_name = av[0];
    if (option_parse_args(options, ac, av, actions_obj, settings_obj) != 0) {
        debug("configure() option_parse_args() failed\n");
        debug_return 1;
    }
    if ((context_fn_obj = json_object_object_get(settings_obj, SETTING_KEY_CONTEXT_FILENAME)) == NULL) {
        debug("context filename not found\n");
        if (set_missing_ctx(NULL, actions_obj, settings_obj)) {
            debug("configure() set_missing_ctx() failed\n");
            debug_return 1;
        }
    } else {
        context_load(json_object_get_string(context_fn_obj));
    }   
    option_set_missing(options, actions_obj, settings_obj);
    debug("actions_obj = %s\n", json_object_to_json_string(actions_obj));
    if (context_set_ai_host(json_object_get_string(json_object_object_get(settings_obj, SETTING_KEY_AI_HOST)))) {
        debug_return 1;
    }
    if (context_set_ai_provider(json_object_get_string(json_object_object_get(settings_obj, SETTING_KEY_AI_PROVIDER)))) {
        debug_return 1;
    }
    if (context_set_model(json_object_get_string(json_object_object_get(settings_obj, SETTING_KEY_AI_MODEL)))) {
        debug_return 1;
    }
    debug("settings_obj = %s\n", json_object_to_json_string(settings_obj)); 
#ifdef DEBUG
    debug("configure() all options:\n"); debug_indent_inc();
    json_object_object_foreach(actions_obj, key, val) {
        debug("%s = \"%s\"\n", key, json_object_get_string(val));
    }
    debug_indent_dec();
#endif
    debug_return 0;
}

static int set_missing_aih(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_HOST, &value)) {
        debug("aih set on command line to %s\n", json_object_get_string(value));
        debug_return 0;
    }
    char *host = NULL;
    const char *h = context_get_ai_host();
    if (h == NULL) {
        h = getenv(ENV_KEY_AI_HOST);\
    }
    if (h != NULL) {
        host = strdup(h);
        int l = strlen(host);
        if (l > 1) {
            l--;
            if (host[l] == '/') {
                host[l] = 0;
            }
        }
    } else {
        if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_PROVIDER, &value)) {
            api_id_t id = api_name_to_id(json_object_get_string(value));
            if (id == api_id_none) {
                fprintf(stderr, "Invalid api name in environment variable \"%s\": \"%s\"\n", ENV_KEY_AI_PROVIDER, json_object_get_string(value));
                debug_return 1;
                
            }
            const api_interface_t *api_interface = api_get_aip_interface(id);
            host = (char *)api_interface->get_default_host();
        }
    }
    if (host != NULL) {
        debug("setting aih to %s\n", host);
        json_object_object_add(settings_obj, SETTING_KEY_AI_HOST, json_object_new_string(host));
        free(host);
    } else {
        fprintf(stderr, "Error setting an API backend host\n");
        debug_return 1;
    }
    debug_return 0;
}

static int set_missing_aip(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_PROVIDER, &value)) {
        debug("aip set on command line to %s\n", json_object_get_string(value));
        debug_return 0;
    }
    api_id_t id = api_id_default;
    const char *a = context_get_ai_provider();
    if (a == NULL) {
        a = getenv(ENV_KEY_AI_PROVIDER);
    }
    if (a != NULL) {
        id = api_name_to_id(a);
        if (id == api_id_none) {
            fprintf(stderr, "Invalid api name in environment variable \"%s\": \"%s\"\n", ENV_KEY_AI_PROVIDER, a);
            debug_return 1;
        }
    }
    const api_interface_t *api_interface = api_get_aip_interface(id);
    char *api_name = (char *)api_interface->get_api_name();
    debug("setting aip to %s\n", api_name);
    json_object_object_add(settings_obj, SETTING_KEY_AI_PROVIDER, json_object_new_string(api_name));
    debug_return 0;
}

static int set_missing_ctx(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    char *context_fn = NULL;
    json_object *value;
    int result = 1;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_CONTEXT_FILENAME, &value)) {
        debug("context filename set on command line to %s\n", json_object_get_string(value));
        context_fn = strdup(json_object_get_string(value));
        result = 0;
        goto term;
    }
    if (getenv(ENV_KEY_CONTEXT_FILENAME) != NULL) {
        context_fn = strdup(getenv(ENV_KEY_CONTEXT_FILENAME));
        if (context_fn == NULL) 
            debug("context filename not set in env\n");
        if (context_fn != NULL) {
            debug("setting context filename to %s from environment variable %s\n", context_fn, ENV_KEY_CONTEXT_FILENAME);
            json_object_object_add(settings_obj, SETTING_KEY_CONTEXT_FILENAME, json_object_new_string(context_fn));
            result = 0;
            goto term;
        }
    }
    const char *d = (char *)context_dir_default;
    const char *h = getenv("HOME");
    int l = strlen(h);
    l++;
    l += strlen(d);
    l++;
    l += strlen(context_fn_default);
    l++;
    context_fn = malloc(l);
    if (context_fn == NULL) {
        fprintf(stderr, "Error setting context file path/name\n");
        debug_return 1;
    }
    strcpy(context_fn, h);
    strcat(context_fn, d);
    if (file_create_path(context_fn) != 0) {
        fprintf(stderr, "Error creating default path %s for context file\n", context_fn);
        goto term;
    }
    strcat(context_fn, "/");
    strcat(context_fn, context_fn_default);
    debug("setting context filename to %s\n", context_fn);
    json_object_object_add(settings_obj, SETTING_KEY_CONTEXT_FILENAME, json_object_new_string(context_fn));
    result = 0;
term:
    context_load(context_fn);
    if (context_fn != NULL) {
        free(context_fn);
    }
    debug_return result;
}

static int set_missing_mdl(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *value;
    const api_interface_t *api_interface = NULL;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_MODEL, &value)) {
        debug("model is set to %s\n", json_object_get_string(value));
        debug_return 0;
    }
    const char *m = context_get_model();
    debug("model in context file is %s\n", m);
    if (m == NULL) {
        m = getenv(ENV_KEY_MODEL);
        if (m == NULL) {
            api_id_t id = api_id_default;
            if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_PROVIDER, &value)) {
                id = api_name_to_id(json_object_get_string(value));
                if (id == api_id_none) {
                    id = api_id_default;
                }
            }
            api_interface = api_get_aip_interface(id);
            m = (char *)api_interface->get_default_model();
        }
    }
    json_object_object_add(settings_obj, SETTING_KEY_AI_MODEL, json_object_new_string(m));
    debug_return 0;
}

static int set_missing_qry(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_PROMPT, &value)) {
        debug_return 0;
    }
    if (json_object_object_get_ex(actions_obj, ACTION_KEY_DUMP_QUERY_HISTORY, &value)) {
        debug_return 0;
    }
    if (json_object_object_get_ex(actions_obj, ACTION_KEY_GET_EMBEDDINGS, &value)) {
        debug_return 0;
    }
    if (json_object_object_get_ex(actions_obj, ACTION_KEY_LIST_MODELS, &value)) {
        debug_return 0;
    }
    json_object_object_add(actions_obj, ACTION_KEY_QUERY, NULL);
    debug_return 0;
}

static int set_missing_sys(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_SYSTEM_PROMPT, &value)) {
        context_set_system_prompt(json_object_get_string(value));
        debug_return 0;
    }
    const char *s = context_get_system_prompt();
    if (s == NULL) {
         debug_return 0;
    }
    json_object_object_add(settings_obj, SETTING_KEY_SYSTEM_PROMPT, json_object_new_string(s));
    debug_return 0;
}

static int option_aih_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(settings_obj, SETTING_KEY_AI_HOST, json_object_new_string(option->value));
    debug_return 0;
}

static int option_aip_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    if (strcmp(option->value, list_argument) == 0) {
        debug("list_apis requested\n");
        json_object_object_add(actions_obj, ACTION_KEY_LIST_APIS, json_object_new_boolean(true));
        debug_return 0;
    } 
    api_id_t id = api_name_to_id(option->value);
    if (id != api_id_none) {
        json_object_object_add(settings_obj, SETTING_KEY_AI_PROVIDER, json_object_new_string(option->value));
    } else {
        fprintf(stderr, "Invalid api name: \"%s\"\n", option->value);
        debug_return 1;
    }
    debug_return 0;
}

static int option_buf_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_BUFFERED, json_object_new_boolean(true));
    debug_return 0;
}

static int option_ctx_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(settings_obj, SETTING_KEY_CONTEXT_FILENAME, json_object_new_string(option->value));
    debug_return 0;
}

static int option_emb_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_GET_EMBEDDINGS, json_object_new_string(option->value));
    debug_return 0;
}

static int option_his_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_DUMP_QUERY_HISTORY, json_object_new_boolean(true));
    debug_return 0;
}

static int option_mdl_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    if (strcmp(option->value, list_argument) == 0) {
        debug("list_models requested\n");
        json_object_object_add(actions_obj, ACTION_KEY_LIST_MODELS, json_object_new_boolean(true));
        debug_return 0;
    }
    json_object_object_add(settings_obj, SETTING_KEY_AI_MODEL, json_object_new_string(option->value));
    debug_return 0;
}

static int option_qry_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    if (option->value != NULL) {
        json_object_object_add(settings_obj, SETTING_KEY_PROMPT, json_object_new_string(option->value));
        json_object_object_add(actions_obj, ACTION_KEY_QUERY, json_object_new_string(option->value));
    } else {
        json_object_object_add(actions_obj, ACTION_KEY_QUERY, NULL);
    }
    debug_return 0;
}

static int option_sys_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(settings_obj, SETTING_KEY_SYSTEM_PROMPT, json_object_new_string(option->value));
    debug_return 0;
}

static int option_h_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_HELP, json_object_new_int64((int64_t)options));
    debug_return 0;
}

static int option_r_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_RESET_CONTEXT, json_object_new_boolean(true));
    debug_return 0;
}

static int option_u_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_UPDATE_CONTEXT, json_object_new_boolean(true));
    debug_return 0;
}

static int option_v_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object_object_add(actions_obj, ACTION_KEY_VERSION, json_object_new_boolean(true));
    debug_return 0;
}

static int merge_api_options(void) {
    debug_enter();
    for (int i = 0; i < api_id_max; i++) {
        const api_interface_t *api_interface = api_get_aip_interface(i + 1);
        if (api_interface != NULL) {
            if (api_interface->get_options != NULL) {
                option_t **api_options = api_interface->get_options();
                if (api_options != NULL) {
                    option_t **new = option_merge(options, api_options);
                    if (new != NULL) {
                        if (options != NULL && options != common_options) {
                            free(options);
                        }
                        options = new;
                    } else {
                        debug_return 1;
                    }
                }
            }
        }
    }
    debug_return 0;
}
