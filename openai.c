/**
 * @file openai.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Interface to OpenAI API.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>
#include <json-c/json.h>

#include "chewie.h"
#include "action.h"
#include "api.h"
#include "context.h"
#include "file.h"
#include "function.h"
#include "option.h"
#include "openai.h"
#include "setting.h"

#define SETTING_KEY_EMBEDDING_MODEL    "embedding_model"

typedef size_t (*setup_curl_callback_t)(void *, size_t, size_t, void *);

static const char default_host[] = "https://api.openai.com";
static const char api_query_endpoint[] = "/v1/chat/completions";
static const char api_get_embeddings_endpoint[] = "/v1/embeddings";
static const char api_listmodels_endpoint[] = "/v1/models";
static const char default_model[] = "gpt-3.5-turbo";
static const char auth_prefix[] = "Authorization: Bearer ";
static const char default_embedding_model[] = "text-embedding-ada-002";
static const char ai_provider[] = "openai";

static CURL *curl = NULL;

static struct json_tokener *json = NULL;
static const char *context_fn = NULL;
static char *auth_header = NULL;
static FILE *tmp_response = NULL;
static int64_t timestamp = 0;
static json_object *messages_obj = NULL;

static const char *get_access_token(void);
static const char *get_api_name(void);
static const char *get_default_host(void);
static const char *get_default_model(void);
static action_t **get_actions(void);
static option_t **get_options(void);
static int get_embeddings(json_object *settings);
static size_t get_embeddings_callback(void *contents, size_t size, size_t nmemb, void *user_data);
static char *get_endpoint(const char *host, const char *endpoint);
static void openai_exit(void);
static int openai_init(void);
static int print_model_list(json_object *options);
static size_t print_model_list_callback(void *contents, size_t size, size_t nmemb, void *user_data);
static const char *query(json_object *options);
static size_t query_callback(void *contents, size_t size, size_t nmemb, void *user_data);
static json_object *query_get_history(json_object *options);
static void setup_curl(json_object *json_obj, const char *endpoint, setup_curl_callback_t callback, json_object *response_obj);
static int string_compare(const void *a, const void *b);
static int option_emd_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_emd(option_t *option, json_object *actions_obj, json_object *settings_obj);
static json_object *use_tool(json_object *tool_response);

static api_interface_t openai_api_interface = {
    .get_actions = get_actions,
    .get_options = get_options,
    .get_default_host = get_default_host,
    .get_default_model = get_default_model,
    .get_embeddings = get_embeddings,
    .get_api_name = get_api_name,
    .print_model_list = print_model_list,
    .query = query
};

static option_t option_emd = {
    .name = "emd",
    .description = "Set the language model for embeddings.",
    .arg_type = option_arg_required,
    .value = NULL,
    .validate = option_emd_validate,
    .set_missing = set_missing_emd,
    .api = ai_provider
};

static option_t *options[] = {
    &option_emd,
    NULL
};

const char *get_access_token(void) {
    debug_enter();
    debug_return getenv("OPENAI_API_KEY");
}

const api_interface_t *openai_get_aip_interface(void) {
    debug_enter();
    debug_return &openai_api_interface;
}

static const char *get_api_name(void) {
    debug_enter();
    debug_return ai_provider;
}

static action_t **get_actions(void) {
    debug_enter();
    debug_return NULL;
}

static option_t **get_options(void) {
    debug_enter();
    debug_return options;
}

static const char *get_default_host(void) {
    debug_enter();
    char *host = NULL;
    const char *s = getenv("OPENAI_HOST");
    if (s != NULL) {
        if (strncmp(s, "http://", sizeof("http://") - 1) != 0 && strncmp(s, "https://", sizeof("https://") - 1) != 0) {
            int l = strlen(s) + sizeof("https://");
            host = malloc(l);
            if (host != NULL) {
                strncpy(host, "https://", l);
                strncat(host, s, l);
            }
        } else {
            host = strdup(s);
        }
    } else {
        host = strdup(default_host);
    }
    debug_return host;
}

static const char *get_default_model(void) {
    debug_enter();
    debug_return strdup(default_model);
}

static int get_embeddings(json_object *settings) {
    debug_enter();
    CURLcode res;
    json_object *query_obj = NULL; 
    json_object *json_obj = NULL;
    char *endpoint = NULL;
    const char *host = default_host;
    if (openai_init()) {
        debug_return 1;
    }
    if (json_object_object_get_ex(settings, SETTING_KEY_AI_HOST, &json_obj)) {
        host = json_object_get_string(json_obj);
    }
    if ((endpoint = get_endpoint(host, api_get_embeddings_endpoint)) == NULL) {
        debug_return 1;
    }
    query_obj = json_object_new_object();
    if (query_obj == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        debug_return 1;
    }
    json_object_object_add(query_obj, "input", json_object_object_get(settings, SETTING_KEY_PROMPT));
    json_object_object_add(query_obj, "model", json_object_object_get(settings, SETTING_KEY_EMBEDDING_MODEL));
    setup_curl(query_obj, endpoint, get_embeddings_callback, NULL);
    debug("openai get_embeddings: %s\n", json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PLAIN));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        debug_return 1;
    }
    openai_exit();
    debug_return 0;
}

static size_t get_embeddings_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    debug_enter();
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, contents, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        debug_return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        debug_return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type != json_type_object) {
        fprintf(stderr, "Response doesn't appear to be a JSON object\n");
        debug_return 0;
    }
    json_object *error_obj = json_object_object_get(json_obj, "error");
    if (error_obj != NULL) {
        json_object *message_obj = json_object_object_get(error_obj, "message");
        if (message_obj != NULL) {
            fprintf(stderr, "API error: %s\n", json_object_get_string(message_obj));
        } else {
            fprintf(stderr, "Error getting message from response\n");
        }
        debug_return 0;
    }
    json_obj = json_object_object_get(json_obj, "data");
    for (int x = 0, y = json_object_array_length(json_obj); x < y; x++) {
        json_object *embedding_obj = json_object_array_get_idx(json_obj, x);
        json_object *object = json_object_object_get(embedding_obj, "object");
        if (strcmp(json_object_get_string(object), "embedding") == 0) {
            json_object *embedding = json_object_object_get(embedding_obj, "embedding");
            for (int i = 0, j = json_object_array_length(embedding); i < j; i++) {
                json_object *v = json_object_array_get_idx(embedding, i);
                printf("%s\n", json_object_to_json_string_ext(v, JSON_C_TO_STRING_PLAIN));
            }
        }
    }
    debug_return nmemb;
}

static char *get_endpoint(const char *host, const char *api_endpoint) {
    debug_enter();
    char *endpoint = NULL;
    long int endpoint_size = strlen(host) + strlen(api_endpoint) + 1;
    endpoint = malloc(endpoint_size);
    if (endpoint == NULL) {
        fprintf(stderr, "Error allocating memory for API endpoint\n");
        debug_return NULL;
    }
    strncpy(endpoint, host, endpoint_size);
    strncat(endpoint, api_endpoint, endpoint_size);
    debug_return endpoint;
}

static void openai_exit(void) {
    debug_enter();
    if (json != NULL) {
        json_tokener_free(json);
        json = NULL;
    }
    if (curl != NULL) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    curl_global_cleanup();
    debug_return;
}

static int openai_init(void) {
    debug_enter();
    CURLcode res;
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        debug_return 1;
    }
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "Error initializing curl\n");
        debug_return 1;
    }
    json = json_tokener_new();
    if (json == NULL) {
        fprintf(stderr, "JSON parser error: couldn't initialize JSON parser\n");
        debug_return 1;
    }
    debug_return 0;
}

static int print_model_list(json_object *options) {
    debug_enter();
    CURLcode res;
    json_object *field_obj = NULL;
    const char *host = default_host;
    char *endpoint = NULL;
    if (openai_init()) {
        debug_return 1;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_AI_HOST, &field_obj)) {
        host = json_object_get_string(field_obj);
    }
    if ((endpoint = get_endpoint(host, api_listmodels_endpoint)) == NULL) {
        debug_return 1;
    }
    printf("ENDPOINT: %s\n", endpoint);
    setup_curl(NULL, endpoint, print_model_list_callback, NULL);
    printf("Models available at %s:\n", host);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        debug_return 1;
    }
    openai_exit();
    debug_return 0;
}

static size_t print_model_list_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    debug_enter();
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, contents, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        debug_return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        debug_return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type != json_type_object) {
        fprintf(stderr, "Response doesn't appear to be a JSON object\n");
        debug_return 0;
    }
    json_obj = json_object_object_get(json_obj, "data");
    array_list *models = json_object_get_array(json_obj);
    if (models == NULL) {
        fprintf(stderr, "Error getting models from response\n");
        debug_return 0;
    }
    int model_count = array_list_length(models);
    char **model_names = malloc(model_count * sizeof(char *));
    if (model_names == NULL) {
        fprintf(stderr, "Error allocating %zu bytes of memory for model names\n", model_count * sizeof(char *));
        debug_return 0;
    }
    for (int i = 0; i < model_count; i++) {
        json_object *model = array_list_get_idx(models, i);
        json_object *id = NULL;
        if (json_object_object_get_ex(model, "id", &id)) {
            model_names[i] = (char *)json_object_get_string(id);
        } else {
            fprintf(stderr, "Error getting model id from response\n");
            debug_return 0;
        }
    }
    qsort(model_names, model_count, sizeof(char *), string_compare);
    for (int i = 0; i < model_count; i++) {
        printf("    %s\n", model_names[i]);
    }
    debug_return nmemb;
}

static const char *query(json_object *options) {
    debug_enter();
    CURLcode res;
    json_object *query_obj = NULL; 
    json_object *json_obj = NULL;
    json_object *field_obj = NULL;
    json_object *prompt_obj = NULL;
    json_object *response_obj = NULL;
    json_object *tool_outputs = NULL;
    json_object *user_obj = NULL;
    json_object *tools = NULL;
    const char *response = NULL;
    const char *prompt_str = NULL;
    const char *endpoint = NULL;
    const char *host = default_host;
    const char *model = default_model;
    if (openai_init()) {
        debug_return NULL;
    }
    messages_obj = query_get_history(options);
    if (messages_obj == NULL) {
        messages_obj = json_object_new_array();
    }
    if (json_object_object_get_ex(options, SETTING_KEY_AI_HOST, &json_obj)) {
        host = json_object_get_string(json_obj);
    }
    if (json_object_object_get_ex(options, SETTING_KEY_AI_MODEL, &json_obj)) {
        model = json_object_get_string(json_obj);
    }
    if ((endpoint = get_endpoint(host, api_query_endpoint)) == NULL) {
        goto term;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_PROMPT, &prompt_obj)) {
        response_obj = json_object_new_object();
        if (response_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            goto term;
        }
        user_obj = json_object_new_object();
        if (user_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            goto term;
        }
        if (json_object_object_add(user_obj, "role", json_object_new_string("user")) != 0) {
            fprintf(stderr, "Error adding role to JSON object\n");
            goto term;
        }
        if (json_object_object_add(user_obj, "content", prompt_obj) != 0) {
            fprintf(stderr, "Error adding content to JSON object\n");
            goto term;
        }
        if (json_object_array_add(messages_obj, user_obj) != 0) {
            fprintf(stderr, "Error adding user message to JSON object\n");
            goto term;
        }
    }
    while (1) {
        json_object *tool_calls;
        query_obj = json_object_new_object();
        if (query_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            goto term;
        }
        json_object_object_add(query_obj, "model", json_object_new_string(model));
        if (tool_outputs != NULL) {
            json_object *new_entry = json_object_new_object();
            json_object_object_add(new_entry, "role", json_object_new_string("assistant"));
            json_object_object_add(new_entry, "content", NULL);
            json_object_object_add(new_entry, "tool_calls", tool_calls);
            json_object_array_add(messages_obj, new_entry);
            size_t array_len = json_object_array_length(tool_outputs);
            for (size_t i = 0; i < array_len; i++) {
                json_object *tool_output = json_object_array_get_idx(tool_outputs, i);
                if (tool_output != NULL) {
                    json_object_array_add(messages_obj, json_object_get(tool_output));
                }
            }
        } 
        json_object_object_add(query_obj, "messages", messages_obj);
        if (json_object_object_get_ex(options, SETTING_KEY_TOOLS, &tools)) {
            if (tools == NULL) {
                fprintf(stderr, "Error getting tools from options\n");
                goto term;
            }
            json_object_object_add(query_obj, "tools", tools);
        }
        setup_curl(query_obj, endpoint, query_callback, response_obj);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            if (json_object_object_get_ex(response_obj, "tool_calls", &tool_calls)) {
                if (tool_calls != NULL) {
                    tool_outputs = use_tool(tool_calls);
                    curl_easy_reset(curl);
                    continue;
                }
            }
            goto term;
        }
        break;
    }
    timestamp = time(NULL);
    if (json_object_object_get_ex(options, SETTING_KEY_PROMPT, &prompt_obj)) {
        prompt_str = json_object_get_string(prompt_obj);
    }
    if (json_object_object_get_ex(options, SETTING_KEY_SYSTEM_PROMPT, &field_obj)) {
        json_object_object_add(query_obj, SETTING_KEY_SYSTEM_PROMPT, field_obj);
    }
    response = file_read_tmp(&tmp_response);
    if (response == NULL) {
        fprintf(stderr, "Error getting response from temporary file\n");
        goto term;
    }
    context_add_history(prompt_str, response, timestamp);
    context_update();
term:
    openai_exit();
    debug_return NULL;
}

static size_t query_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    debug_enter();
    static json_object *json_obj = NULL;
    json_object *response_obj = (json_object *)user_data;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, contents, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        debug("Response: %s\n", (char *)contents);
        debug_return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        debug_return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type == json_type_object) {
        debug("Response: %s\n", json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN));
        json_object *choices = NULL;
        json_object *error = NULL;
        if (json_object_object_get_ex(json_obj, "choices", &choices)) {
            json_object *choice = json_object_array_get_idx(choices, 0);
            json_object *content = NULL;
            json_object *message = NULL;
            json_object *tools = NULL;
            if (choice == NULL) {
                fprintf(stderr, "Error getting choice from response\n");
                debug_return 0;
            }
            if (json_object_object_get_ex(choice, "message", &message)) {
                if (json_object_object_get_ex(message, "tool_calls", &tools)) {
                    json_object_object_add(response_obj, "tool_calls", tools);
                    debug("Received: %s\n", json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN));
                    debug("Received tool call: %s\n", json_object_to_json_string_ext(tools, JSON_C_TO_STRING_PLAIN));
                    debug_return 0;
                }
                if (json_object_object_get_ex(message, "content", &content)) {
                    const char *s = (char *)json_object_get_string(content);
                    printf("%s\n", s);
                    file_append_tmp(&tmp_response, s);
                } else {
                    fprintf(stderr, "Error getting content from response\n");
                    debug_return 0;
                }
            } else {
                fprintf(stderr, "Error getting message from response\n");
                debug_return 0;
            }
        } else if (json_object_object_get_ex(json_obj, "error", &error)) {
            json_object *message = NULL;
            if (json_object_object_get_ex(error, "message", &message)) {
                fprintf(stderr, "API error: %s\n", json_object_get_string(message));
            } else {
                fprintf(stderr, "Error getting message from response\n");
            }
            debug_return 0;
        } else {
            fprintf(stderr, "Error getting response from API\n");
            debug_return 0;
        }
    } else {
        fprintf(stderr, "Response doesn't appear to be a JSON object:\n%*s\n", (int)nmemb, (char *)contents);
        debug_return 0;
    }
    debug_return nmemb;
}

static json_object *query_get_history(json_object *options) {
    debug_enter();
    json_object *history_obj = NULL;
    json_object *system_prompt_obj = NULL;
    json_object *context_history_obj = NULL;
    json_object *new_entry;
    json_object *context_fn_obj = NULL;
    int context_history_count = 0;
    const char *system_prompt_str = NULL;
    if (context_fn == NULL) {
        if (!json_object_object_get_ex(options, SETTING_KEY_CONTEXT_FILENAME, &context_fn_obj)) {
            fprintf(stderr, "Error getting context file name from options\n");
            debug_return NULL;
        }
        context_fn = json_object_get_string(context_fn_obj);
    }
    if (context_fn != NULL) {
        debug("Loading context from %s\n", context_fn);
        context_load(context_fn);
    } else {
        fprintf(stderr, "Error getting context file name\n");
        debug_return NULL;
    }
    history_obj = json_object_new_array();
    if (history_obj == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        debug_return NULL;
    }
    system_prompt_str = context_get_system_prompt();
    system_prompt_obj = json_object_new_string(system_prompt_str);
    if (system_prompt_obj != NULL) {
        new_entry = json_object_new_object();
        if (new_entry == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            debug_return NULL;
        }
        json_object_object_add(new_entry, "role", json_object_new_string("system"));
        json_object_object_add(new_entry, "content", system_prompt_obj);
        json_object_array_add(history_obj, new_entry);
    }
    context_history_obj = context_get_history();
    if (context_history_obj == NULL) {
        debug_return history_obj;
    }
    context_history_count = json_object_array_length(context_history_obj);
    for (int i = 0; i < context_history_count; i++) {
        json_object *entry = json_object_array_get_idx(context_history_obj, i);
        const char *prompt_str = NULL;
        const char *response_str = NULL;
        if (entry != NULL) {
            json_object *role_obj = NULL;
            json_object_object_get_ex(entry, "role", &role_obj);
            if (role_obj != NULL) {
                if (strcmp(json_object_get_string(role_obj), "system") != 0) {
                    prompt_str = context_get_history_prompt(entry);
                    response_str = context_get_history_response(entry);
                    if (prompt_str != NULL) {
                        new_entry = json_object_new_object();
                        if (new_entry == NULL) {
                            fprintf(stderr, "Error creating new JSON object\n");
                            debug_return NULL;
                        }
                        if (system_prompt_obj != NULL && json_object_get_boolean(system_prompt_obj)) {
                            json_object_object_add(new_entry, "role", json_object_new_string("system"));
                        } else {
                            json_object_object_add(new_entry, "role", json_object_new_string("user"));
                        }
                        json_object_object_add(new_entry, "content", json_object_new_string(prompt_str));
                        json_object_array_add(history_obj, new_entry);
                    }
                    if (response_str != NULL) {
                        new_entry = json_object_new_object();
                        if (new_entry == NULL) {
                            fprintf(stderr, "Error creating new JSON object\n");
                            debug_return NULL;
                        }
                        json_object_object_add(new_entry, "role", json_object_new_string("assistant"));
                        json_object_object_add(new_entry, "content", json_object_new_string(response_str));
                        json_object_array_add(history_obj, new_entry);
                    }
                }
            }
        }
    }
    debug_return history_obj;
}

static void setup_curl(json_object *query_obj, const char *endpoint, setup_curl_callback_t callback, json_object *response_obj) {
    debug_enter();
    const char *token = get_access_token();
    if (token == NULL) {
        fprintf(stderr, "Error getting access token\n");
        exit(1);
    }
    int auth_header_size = sizeof(auth_prefix) + strlen(token);
    auth_header = malloc(auth_header_size);
    if (auth_header == NULL) {
        fprintf(stderr, "Error allocating %d bytes of memory for auth header\n", auth_header_size);
        exit(1);
    }
    sprintf(auth_header, "%s%s", auth_prefix, token);
    struct curl_slist *headers = curl_slist_append(NULL, auth_header);
    if (query_obj) {
        const char *s = json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PLAIN);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(s));
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } 
    curl_easy_setopt(curl, CURLOPT_URL, endpoint);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response_obj);
    debug_return;
}

static int string_compare(const void *a, const void *b) {
    debug_enter();
    debug_return strcmp(*(char **)a, *(char **)b);
}

static int option_emd_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *api_object = NULL;
    if (!json_object_object_get_ex(settings_obj, SETTING_KEY_AI_PROVIDER, &api_object)) {
        fprintf(stderr, "Error getting AI provider from settings\n");
        debug_return 1;
    }
    if (api_object == NULL || strcmp(json_object_get_string(api_object), ai_provider) != 0) {
        fprintf(stderr, "Error: AI provider is not %s\n", ai_provider);
        debug_return 1;
    }
    json_object_object_add(settings_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(option->value));
    json_object *openai_obj = context_get(ai_provider);
    if (openai_obj == NULL) {
        openai_obj = json_object_new_object();
        if (openai_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            debug_return 1;
        }
    }
    json_object_object_add(openai_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(option->value));
    context_set(ai_provider, openai_obj);
    debug_return 0;
}

static int set_missing_emd(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    json_object *value;
    json_object *openai_obj = context_get(ai_provider);
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_EMBEDDING_MODEL, &value)) {
        debug("openai embedding model is set to %s\n", json_object_get_string(value));
        debug_return 0;
    }
    const char *m = NULL;
    if (openai_obj != NULL) {
        json_object_object_get_ex(openai_obj, SETTING_KEY_EMBEDDING_MODEL, &value);
        if (value != NULL) {
            m = json_object_get_string(value);
        }
    }
    if (m == NULL) {
        m = default_embedding_model;
    }
    debug("model in context file is %s\n", m);
    json_object_object_add(settings_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(m));
    if (openai_obj == NULL) {
        openai_obj = json_object_new_object();
        if (openai_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            debug_return 1;
        }
    }
    json_object_object_add(openai_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(m));
    debug("wrote %s to context file\n", json_object_to_json_string_ext(openai_obj, JSON_C_TO_STRING_PRETTY));
    context_set(ai_provider, openai_obj);
    debug_return 0;
}

static json_object *use_tool(json_object *tool_response) {
    debug_enter();
    json_object *results = json_object_new_array();
    json_object *result = json_object_new_object();
    array_list *tools = json_object_get_array(tool_response);
    size_t j = array_list_length(tools);
    if (result == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        debug_return NULL;
    }
    for (size_t i = 0; i < j; i++) {
        json_object *tool = array_list_get_idx(tools, i);
        json_object *tool_id = NULL;
        json_object *tool_type = NULL;
        json_object *tool_function = NULL;
        const char *tool_id_str = NULL;
        if (json_object_object_get_ex(tool, "type", &tool_type)) {
            if (strcmp(json_object_get_string(tool_type), "function") != 0) {
                fprintf(stderr, "Error unrecognzied tool type: %s\n", json_object_get_string(tool_type));
                debug_return NULL;
            }
        }
        if (json_object_object_get_ex(tool, "id", &tool_id)) {
            tool_id_str = json_object_get_string(tool_id);
        }
        if (json_object_object_get_ex(tool, "function", &tool_function)) {
            json_object *name = NULL;
            json_object *arguments = NULL;
            if (!json_object_object_get_ex(tool_function, "name", &name)) {
                fprintf(stderr, "Error getting tool function name\n");
                debug_return NULL;
            }
            if (!json_object_object_get_ex(tool_function, "arguments", &arguments)) {
                fprintf(stderr, "Error getting tool function arguments\n");
                debug_return NULL;
            }
            const char *sr = function_invoke(json_object_get_string(name), arguments);
            if (sr == NULL) {
                fprintf(stderr, "Tool function returned a NULL result\n");
                debug_return NULL;
            }
            json_object_object_add(result, "role", json_object_new_string("tool"));
            json_object_object_add(result, "name", name);
            json_object_object_add(result, "tool_call_id", json_object_new_string(tool_id_str));
            json_object_object_add(result, "content", json_object_new_string(sr));
            json_object_array_add(results, json_object_get(result));
        }
    }
    return results;
}
