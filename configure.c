/**
 * configure.c
 *
 * Configure according to comm and line arguments and environment. Uses getopt()
 * to parse command line arguments and json-c to store the results in a JSON
 * object. That object is then sent to the action processor to execute any
 * actions specified, such as dumping the query. The object is then passed on
 * to the API interface to be used to send the proper query to the API.
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
    .arg_type = option_arg_none,
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
    debug("configure()\n");
    if (merge_api_options()) {
        return 1;
    }
    program_name = av[0];
    if (option_parse_args(options, ac, av, actions_obj, settings_obj) != 0) {
        debug("configure() option_parse_args() failed\n");
        return 1;
    }
    option_set_missing(options, actions_obj, settings_obj);
    debug("actions_obj = %s\n", json_object_to_json_string(actions_obj));
    if (context_set_ai_host(json_object_get_string(json_object_object_get(settings_obj, SETTING_KEY_AI_HOST)))) {
        return 1;
    }
    if (context_set_ai_provider(json_object_get_string(json_object_object_get(settings_obj, SETTING_KEY_AI_PROVIDER)))) {
        return 1;
    }
    if (context_set_model(json_object_get_string(json_object_object_get(settings_obj, SETTING_KEY_AI_MODEL)))) {
        return 1;
    }
    debug("settings_obj = %s\n", json_object_to_json_string(settings_obj)); 
#ifdef DEBUG
    printf("configure() all options:\n");
    json_object_object_foreach(actions_obj, key, val) {
        printf("        %s = \"%s\"\n", key, json_object_get_string(val));
    }
#endif
    return 0;
}

static int set_missing_aih(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_aih()\n");
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_HOST, &value)) {
        return 0;
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
                return 1;
            }
            const api_interface_t *api_interface = api_get_aip_interface(id);
            host = (char *)api_interface->get_default_host();
        }
    }
    if (host != NULL) {
        json_object_object_add(settings_obj, SETTING_KEY_AI_HOST, json_object_new_string(host));
        free(host);
    } else {
        fprintf(stderr, "Error setting an API backend host\n");
        return 1;
    }
    return 0;
}

static int set_missing_aip(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_aip()\n");
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_PROVIDER, &value)) {
        return 0;
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
            return 1;
        }
    }
    const api_interface_t *api_interface = api_get_aip_interface(id);
    char *api_name = (char *)api_interface->get_api_name();
    json_object_object_add(settings_obj, SETTING_KEY_AI_PROVIDER, json_object_new_string(api_name));
    return 0;
}

static int set_missing_ctx(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_ctx()\n");
    char *context_fn = NULL;
    json_object *value;
    int result = 1;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_CONTEXT_FILENAME, &value)) {
        context_fn = strdup(json_object_get_string(value));
        result = 0;
        goto term;
    }
    if (getenv(ENV_KEY_CONTEXT_FILENAME) != NULL) {
        context_fn = strdup(getenv(ENV_KEY_CONTEXT_FILENAME));
        if (context_fn == NULL) printf("context filename not set in env\n");
        if (context_fn != NULL) {
            json_object_object_add(settings_obj, SETTING_KEY_CONTEXT_FILENAME, json_object_new_string(context_fn));
            result = 0;
            goto term;
        }
    }
    char *d = (char *)context_dir_default;
    char *h = getenv("HOME");
    int l = strlen(h);
    l++;
    l += strlen(d);
    l++;
    l += strlen(context_fn_default);
    l++;
    context_fn = malloc(l);
    if (context_fn == NULL) {
        fprintf(stderr, "Error setting context file path/name\n");
        return 1;
    }
    strcpy(context_fn, h);
    strcat(context_fn, d);
    if (file_create_path(context_fn) != 0) {
        fprintf(stderr, "Error creating default path %s for context file\n", context_fn);
        goto term;
    }
    strcat(context_fn, "/");
    strcat(context_fn, context_fn_default);
    json_object_object_add(settings_obj, SETTING_KEY_CONTEXT_FILENAME, json_object_new_string(context_fn));
    result = 0;
term:
    context_load(context_fn);
    if (context_fn != NULL) {
        free(context_fn);
    }
    return result;
}

static int set_missing_mdl(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_mdl()\n");
    json_object *value;
    const api_interface_t *api_interface = NULL;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_AI_MODEL, &value)) {
        debug("model is set to %s\n", json_object_get_string(value));
        return 0;
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
    return 0;
}

static int set_missing_qry(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_qry()\n");
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_PROMPT, &value)) {
        return 0;
    }
    if (json_object_object_get_ex(actions_obj, ACTION_KEY_DUMP_QUERY_HISTORY, &value)) {
        return 0;
    }
    if (json_object_object_get_ex(actions_obj, ACTION_KEY_GET_EMBEDDINGS, &value)) {
        return 0;
    }
    if (json_object_object_get_ex(actions_obj, ACTION_KEY_LIST_MODELS, &value)) {
        return 0;
    }
    json_object_object_add(actions_obj, ACTION_KEY_QUERY, NULL);
    return 0;
}

static int set_missing_sys(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_sys()\n");
    json_object *value;
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_SYSTEM_PROMPT, &value)) {
        context_set_system_prompt(json_object_get_string(value));
        return 0;
    }
    const char *s = context_get_system_prompt();
    if (s == NULL) {
        return 0;
    }
    json_object_object_add(settings_obj, SETTING_KEY_SYSTEM_PROMPT, json_object_new_string(s));
    return 0;
}

static int option_aih_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_aih_validate()\n");
    json_object_object_add(settings_obj, SETTING_KEY_AI_HOST, json_object_new_string(option->value));
    return 0;
}

static int option_aip_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_aip_validate()\n");
    if (strcmp(option->value, list_argument) == 0) {
        debug("    list_apis requested\n");
        json_object_object_add(actions_obj, ACTION_KEY_LIST_APIS, json_object_new_boolean(true));
        return 0;
    } 
    api_id_t id = api_name_to_id(option->value);
    if (id != api_id_none) {
        json_object_object_add(settings_obj, SETTING_KEY_AI_PROVIDER, json_object_new_string(option->value));
    } else {
        fprintf(stderr, "Invalid api name: \"%s\"\n", option->value);
        return 1;
    }
    return 0;
}

static int option_buf_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_buf_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_BUFFERED, json_object_new_boolean(true));
    return 0;
}

static int option_ctx_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_ctx_validate()\n");
    json_object_object_add(settings_obj, SETTING_KEY_CONTEXT_FILENAME, json_object_new_string(option->value));
    return 0;
}

static int option_emb_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_emb_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_GET_EMBEDDINGS, json_object_new_boolean(true));
    return 0;
}

static int option_his_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_his_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_DUMP_QUERY_HISTORY, json_object_new_boolean(true));
    return 0;
}

static int option_mdl_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_mdl_validate()\n");
    if (strcmp(option->value, list_argument) == 0) {
        debug("    list_models requested\n");
        json_object_object_add(actions_obj, ACTION_KEY_LIST_MODELS, json_object_new_boolean(true));
        return 0;
    }
    json_object_object_add(settings_obj, SETTING_KEY_AI_MODEL, json_object_new_string(option->value));
    return 0;
}

static int option_qry_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_qry_validate()\n");
    if (option->value != NULL) {
        json_object_object_add(settings_obj, SETTING_KEY_PROMPT, json_object_new_string(option->value));
        json_object_object_add(actions_obj, ACTION_KEY_QUERY, json_object_new_string(option->value));
    } else {
        json_object_object_add(actions_obj, ACTION_KEY_QUERY, NULL);
    }
    return 0;
}

static int option_sys_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_sys_validate()\n");
    json_object_object_add(settings_obj, SETTING_KEY_SYSTEM_PROMPT, json_object_new_string(option->value));
    return 0;
}

static int option_h_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_h_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_HELP, json_object_new_int64((int64_t)options));
    return 0;
}

static int option_r_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_r_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_RESET_CONTEXT, json_object_new_boolean(true));
    return 0;
}

static int option_u_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_u_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_UPDATE_CONTEXT, json_object_new_boolean(true));
    return 0;
}

static int option_v_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_v_validate()\n");
    json_object_object_add(actions_obj, ACTION_KEY_VERSION, json_object_new_boolean(true));
    return 0;
}

static int merge_api_options(void) {
    debug("merge_api_options()\n");
    for (int i = 0; i < api_id_max; i++) {
        const api_interface_t *api_interface = api_get_aip_interface(i + 1);
        if (api_interface != NULL) {
            if (api_interface->get_options != NULL) {
                option_t **api_options = api_interface->get_options();
                if (api_options != NULL) {
                    option_t **new = option_merge(options, api_options);
                    if (new != NULL) {
                        if (options != NULL) {
                            free(options);
                        }
                        options = new;
                    } else {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}
