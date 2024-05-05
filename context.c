/**
 * @file context.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Manage context file.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */
 
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <json-c/json_tokener.h>

#include "chewie.h"
#include "context.h"
#include "file.h"

/** @brief Strings used as context object keys */
#define CONTEXT_KEY_PROMPT              "prompt"
#define CONTEXT_KEY_AI_HOST             "ai-host"
#define CONTEXT_KEY_AI_PROVIDER         "ai-provider"
#define CONTEXT_KEY_MODEL               "model"
#define CONTEXT_KEY_CONTEXT_FILENAME    "context-filename"
#define CONTEXT_KEY_RESPONSE            "response"
#define CONTEXT_KEY_HISTORY             "history"
#define CONTEXT_KEY_TIMESTAMP           "timestamp"
#define CONTEXT_KEY_SYSTEM_PROMPT       "system-prompt"

const char context_dir_default[] = "/.cache/chewie";
const char context_fn_default[] = "default-context.json";

static const char *context_fn = NULL;
static json_object *context_obj = NULL;

static json_object *read_context_file(const char *fn);
static int write_context_file(const char *fn, json_object *context_obj);

void context_add_history(const char *prompt, const char *response, const int64_t timestamp) {
    debug_enter();
    json_object *history_obj = NULL;
    json_object *new_entry = NULL;
    json_object *prompt_obj = NULL;
    json_object *response_obj = NULL;
    json_object *timestamp_obj = NULL;
    if (context_obj == NULL) {
        debug_return;
    }
    if (!json_object_object_get_ex(context_obj, CONTEXT_KEY_HISTORY, &history_obj)) {
        history_obj = json_object_new_array();
        json_object_object_add(context_obj, CONTEXT_KEY_HISTORY, history_obj);
    }
    if (prompt != NULL) {
        while (isspace(*prompt) && (*prompt != '\0')) prompt++;
        char *s = (char *)prompt + strlen(prompt) - 1;
        while (isspace(*s)) s--;
        *(s + 1) = '\0';
        prompt_obj = json_object_new_string(prompt);
    }
    if (response != NULL) {
        while (isspace(*response) && (*response != '\0')) response++;
        char *s = (char *)response + strlen(response) - 1;
        while (isspace(*s) && (s > response)) s--;
        *(s + 1) = '\0';
        response_obj = json_object_new_string(response);
    }
    timestamp_obj = json_object_new_int64(timestamp);
    if (timestamp_obj != NULL && prompt_obj != NULL && response_obj != NULL) {
        new_entry = json_object_new_object();
        json_object_object_add(new_entry, CONTEXT_KEY_TIMESTAMP, timestamp_obj);
        json_object_object_add(new_entry, CONTEXT_KEY_PROMPT, prompt_obj);
        json_object_object_add(new_entry, CONTEXT_KEY_RESPONSE, response_obj);
    }
    if (new_entry != NULL) {
        json_object_array_add(history_obj, new_entry);
    }
    debug_return;
}

json_object *context_get(const char *field) {
    debug_enter();
    json_object *result = NULL;
    if (context_obj == NULL) {
        debug_return NULL;
    }
    json_object_object_get_ex(context_obj, field, &result);
    debug_return result;
}

void context_load(const char *fn) {
    debug_enter();
    if (fn != NULL) {
        context_fn = strdup(fn);
        context_obj = read_context_file(fn);
        if (context_obj != NULL) {
            debug_return;
        }
    }
    debug("creating new context_obj\n");
    context_obj = json_object_new_object();
    debug_return;
}

void context_new(const char *fn) {
    debug_enter();
    context_fn = fn;
    debug("creating new context_obj\n");
    if (context_obj != NULL) {
        json_object_put(context_obj);
    }
    context_obj = json_object_new_object();
    debug_return;
}

int context_delete_system_prompt(void) {
    debug_enter();
    if (context_obj == NULL) {
        debug_return 1;
    }
    json_object_object_del(context_obj, CONTEXT_KEY_SYSTEM_PROMPT);
    debug_return 0;
}

int context_dump_history(void) {
    debug_enter();
    int result = 1;
    if (context_obj == NULL) {
        fprintf(stderr, "Error parsing context file\n");
        debug_return 1;
    }
    json_object *history_obj = NULL;
    json_object_object_get_ex(context_obj, CONTEXT_KEY_HISTORY, &history_obj);
    if (history_obj == NULL) {
        goto term;
    }
    int history_len = json_object_array_length(history_obj);
    for (int i = 0; i < history_len; i++) {
        json_object *query_obj = json_object_array_get_idx(history_obj, i);
        json_object *prompt_obj = NULL;
        json_object *response_obj = NULL;
        json_object *timestamp_obj = NULL;
        if (query_obj == NULL) {
            fprintf(stderr, "Error getting history item\n");
            goto term;
        }
        json_object_object_get_ex(query_obj, CONTEXT_KEY_PROMPT, &prompt_obj);
        if (prompt_obj == NULL) {
            fprintf(stderr, "Error getting prompt from history\n");
            goto term;
        }
        json_object_object_get_ex(query_obj, CONTEXT_KEY_RESPONSE, &response_obj);
        if (response_obj == NULL) {
            fprintf(stderr, "Error getting AI response from history\n");
            goto term;
        }
        json_object_object_get_ex(query_obj, CONTEXT_KEY_TIMESTAMP, &timestamp_obj);
        if (timestamp_obj == NULL) {
            fprintf(stderr, "Error getting query timestamp from history\n");
            goto term;
        }
        time_t ts = json_object_get_int64(timestamp_obj);
        printf("%s", ctime(&ts));
        printf("User: \"%s\"\n", json_object_get_string(prompt_obj));
        printf("AI: %s\n\n", json_object_get_string(response_obj));
    }
    result = 0;
term:
    debug_return result;
}

const char *context_get_ai_host(void) {
    debug_enter();
    json_object *ai_host_obj = NULL;
    const char *ai_host = NULL;
    const char *result = NULL;
    if (context_obj == NULL) {
        debug("context_obj is NULL\n");
        goto term;
    }
    if (!json_object_object_get_ex(context_obj, CONTEXT_KEY_AI_HOST, &ai_host_obj)) {
        debug("context_obj does not contain key \"%s\"\n", CONTEXT_KEY_AI_HOST);
        goto term;
    }
    ai_host = json_object_get_string(ai_host_obj);
term:
    debug_return ai_host;
}

const char *context_get_ai_provider(void) {
    debug_enter();
    json_object *ai_provider_obj = NULL;
    const char *ai_provider = NULL;
    const char *result = NULL;
    if (context_obj == NULL) {
        goto term;
    }
    if (!json_object_object_get_ex(context_obj, CONTEXT_KEY_AI_PROVIDER, &ai_provider_obj)) {
        goto term;
    }
    ai_provider = json_object_get_string(ai_provider_obj);
term:
    debug_return ai_provider;
}

const char *context_get_model(void) {
    debug_enter();
    json_object *model_obj = NULL;
    const char *model = NULL;
    const char *result = NULL;
    if (context_obj == NULL) {
        goto term;
    }
    if (!json_object_object_get_ex(context_obj, CONTEXT_KEY_MODEL, &model_obj)) {
        goto term;
    }
    if (model_obj != NULL) {
        model = json_object_get_string(model_obj);
    }
term:
    debug_return model;
}

const char *context_get_system_prompt(void) {
    debug_enter();
    json_object *system_prompt_obj = NULL;
    const char *system_prompt = NULL;
    const char *result = NULL;
    if (context_obj == NULL) {
        goto term;
    }
    if (!json_object_object_get_ex(context_obj, CONTEXT_KEY_SYSTEM_PROMPT, &system_prompt_obj)) {
        goto term;
    }
    system_prompt = json_object_get_string(system_prompt_obj);
    if (system_prompt == NULL) {
        goto term;
    }
    result = strdup(system_prompt);
term:
    debug_return result;
}

json_object *context_get_history(void) {
    debug_enter();
    if (context_obj == NULL) {
        debug_return NULL;
    }
    debug_return json_object_object_get(context_obj, CONTEXT_KEY_HISTORY);
}

const char *context_get_history_prompt(json_object *entry) {
    debug_enter();
    debug_return json_object_get_string(json_object_object_get(entry, CONTEXT_KEY_PROMPT));
}

const char *context_get_history_response(json_object *entry) {
    debug_enter();
    debug_return json_object_get_string(json_object_object_get(entry, CONTEXT_KEY_RESPONSE));
}

int64_t context_get_history_timestamp(json_object *entry) {
    debug_enter();
    debug_return json_object_get_int64(json_object_object_get(entry, CONTEXT_KEY_TIMESTAMP));
}

int context_set(const char *field, json_object *obj) {
    debug_enter();
    if (context_obj == NULL || obj == NULL || field == NULL) {
        debug_return 1;
    }
    if (json_object_object_add(context_obj, field, json_object_get(obj))) {
        fprintf(stderr, "Error adding field \"%s\" to context object\n", field);
        debug_return 1;
    }
    debug_return 0;
}

int context_set_ai_host(const char *s) {
    debug_enter();
    json_object *ai_host_obj = NULL;
    int result = 1;
    if (context_obj == NULL) {
        goto term;
    }
    ai_host_obj = json_object_new_string(s);
    if (ai_host_obj == NULL) {
        goto term;
    }
    json_object_object_add(context_obj, CONTEXT_KEY_AI_HOST, ai_host_obj);
    result = 0;
term:
    debug_return result;
}

int context_set_ai_provider(const char *s) {
    debug_enter();
    json_object *ai_provider_obj = NULL;
    int result = 1;
    if (context_obj == NULL) {
        goto term;
    }
    ai_provider_obj = json_object_new_string(s);
    if (ai_provider_obj == NULL) {
        goto term;
    }
    json_object_object_add(context_obj, CONTEXT_KEY_AI_PROVIDER, ai_provider_obj);
    result = 0;
term:
    debug_return result;
}

int context_set_model(const char *s) {
    debug_enter();
    json_object *model_obj = NULL;
    int result = 1;
    if (context_obj == NULL) {
        goto term;
    }
    model_obj = json_object_new_string(s);
    if (model_obj == NULL) {
        goto term;
    }
   json_object_object_add(context_obj, CONTEXT_KEY_MODEL, model_obj);
   result = 0;
term:
    debug_return result;
}

int context_set_system_prompt(const char *s) {
    debug_enter();
    if (s == NULL) {
        debug_return 0;
    }
    json_object *system_prompt_obj = NULL;
    if (context_obj == NULL) {
        debug_return 1;
    }
    system_prompt_obj = json_object_new_string(s);
    if (system_prompt_obj == NULL) {
        debug_return 1;
    }
    json_object_object_add(context_obj, CONTEXT_KEY_SYSTEM_PROMPT, system_prompt_obj);
    debug_return 0;
}

void context_update(void) {
    debug_enter();
    debug("writing context file \"%s\"\n", context_fn);
    const char *s = json_object_to_json_string_ext(context_obj, JSON_C_TO_STRING_PRETTY);
    debug("context:\n\"%s\"\n", s);
    file_write(context_fn, s);
    debug_return;
}

json_object *read_context_file(const char *fn) {
    debug_enter();
    const char *context = NULL;
    json_tokener *json = NULL;
    json_object *context_obj = NULL;
    if (fn == NULL) {
        fprintf(stderr, "No context filename\n");
        debug_return NULL;
    }
    context = file_read(fn);
    if (context == NULL) {
        debug_return NULL;
    }
    json = json_tokener_new();
    if (json == NULL) {
        goto term;
    }
    context_obj = json_tokener_parse_ex(json, context, strlen(context));
term:
    if (context != NULL) {
        free((void *)context);
    }
    if (json != NULL) {
        json_tokener_free(json);
    }
    debug_return context_obj;
}

static int write_context_file(const char *fn, json_object *context_obj) {
    debug_enter();
    if (fn == NULL) {
        fprintf(stderr, "No context filename\n");
        debug_return 1;
    }
    if (context_obj == NULL) {
        fprintf(stderr, "No context object\n");
        debug_return 1;
    }
    const char *context_json = json_object_to_json_string_ext(context_obj, JSON_C_TO_STRING_PRETTY);
    if (context_json == NULL) {
        fprintf(stderr, "Error converting context object to JSON string\n");
        debug_return 1;
    }
    file_write(fn, context_json);
    debug_return 0;
}
