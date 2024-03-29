/**
 * openai.c
 *
 * Interface to OpenAI API.
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
static json_object *query_obj = NULL;
static json_object *updates_obj = NULL;
static const char *context_fn = NULL;
static char *auth_header = NULL;
static FILE *tmp_response = NULL;
static const char *prompt_str = NULL;
static int64_t timestamp = 0;

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
static void setup_curl(json_object *json_obj, const char *endpoint, setup_curl_callback_t callback);
static int string_compare(const void *a, const void *b);
static int option_emd_validate(option_t *option, json_object *actions_obj, json_object *settings_obj);
static int set_missing_emd(option_t *option, json_object *actions_obj, json_object *settings_obj);

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
    return getenv("OPENAI_API_KEY");
}

const api_interface_t *openai_get_aip_interface(void) {
    return &openai_api_interface;
}

static const char *get_api_name(void) {
    return ai_provider;
}

static action_t **get_actions(void) {
    return NULL;
}

static option_t **get_options(void) {
    return options;
}

static const char *get_default_host(void) {
    char *host = NULL;
    char *s = getenv("OPENAI_HOST");
    if (s != NULL) {
        int l = strlen(s) + 1;
        if (strncmp(s, "http://", sizeof("http://") - 1) != 0 && strncmp(s, "https://", sizeof("https://") - 1) != 0) {
            l += strlen("https://");
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
    return host;
}

static const char *get_default_model(void) {
    return strdup(default_model);
}

static int get_embeddings(json_object *settings) {
    debug("openai get_embeddings()\n");
    CURLcode res;
    json_object *query_obj = NULL; 
    json_object *json_obj = NULL;
    json_object *field_obj = NULL;
    const char *response = NULL;
    char *endpoint = NULL;
    const char *host = default_host;
    const char *model = default_model;
    const char *context = NULL;
    enum json_tokener_error jerr;
    if (openai_init()) {
        return 1;
    }
    if (json_object_object_get_ex(settings, SETTING_KEY_AI_HOST, &json_obj)) {
        host = json_object_get_string(json_obj);
    }
    if ((endpoint = get_endpoint(host, api_get_embeddings_endpoint)) == NULL) {
        return 1;
    }
    query_obj = json_object_new_object();
    if (query_obj == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        return 1;
    }
    json_object_object_add(query_obj, "input", json_object_object_get(settings, SETTING_KEY_PROMPT));
    json_object_object_add(query_obj, "model", json_object_object_get(settings, SETTING_KEY_EMBEDDING_MODEL));
    setup_curl(query_obj, endpoint, get_embeddings_callback);
    debug("openai get_embeddings: %s\n", json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PLAIN));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        return 1;
    }
term:
    openai_exit();
    return 0;
}

static size_t get_embeddings_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, contents, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type != json_type_object) {
        fprintf(stderr, "Response doesn't appear to be a JSON object\n");
        return 0;
    }
    json_object *error_obj = json_object_object_get(json_obj, "error");
    if (error_obj != NULL) {
        json_object *message_obj = json_object_object_get(error_obj, "message");
        if (message_obj != NULL) {
            fprintf(stderr, "API error: %s\n", json_object_get_string(message_obj));
        } else {
            fprintf(stderr, "Error getting message from response\n");
        }
        return 0;
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
    return nmemb;
}

static char *get_endpoint(const char *host, const char *api_endpoint) {
    char *endpoint = NULL;
    long int endpoint_size = strlen(host) + strlen(api_endpoint) + 1;
    endpoint = malloc(endpoint_size);
    if (endpoint == NULL) {
        fprintf(stderr, "Error allocating memory for API endpoint\n");
        return NULL;
    }
    strncpy(endpoint, host, endpoint_size);
    strncat(endpoint, api_endpoint, endpoint_size);
    return endpoint;
}

static void openai_exit(void) {
    if (json != NULL) {
        json_tokener_free(json);
        json = NULL;
    }
    if (curl != NULL) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    curl_global_cleanup();
}

static int openai_init(void) {
    CURLcode res;
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        return 1;
    }
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "Error initializing curl\n");
        return 1;
    }
    json = json_tokener_new();
    if (json == NULL) {
        fprintf(stderr, "JSON parser error: couldn't initialize JSON parser\n");
        return 1;
    }
    updates_obj = json_object_new_object();
    if (updates_obj == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        return 1;
    }
    return 0;
}

static int print_model_list(json_object *options) {
    CURLcode res;
    json_object *field_obj = NULL;
    const char *host = default_host;
    char *endpoint = NULL;
    if (openai_init()) {
        return 1;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_AI_HOST, &field_obj)) {
        host = json_object_get_string(field_obj);
    }
    if ((endpoint = get_endpoint(host, api_listmodels_endpoint)) == NULL) {
        return 1;
    }
    printf("ENDPOINT: %s\n", endpoint);
    setup_curl(NULL, endpoint, print_model_list_callback);
    printf("Models available at %s:\n", host);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        return 1;
    }
term:
    openai_exit();
    return 0;
}

static size_t print_model_list_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, contents, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type != json_type_object) {
        fprintf(stderr, "Response doesn't appear to be a JSON object\n");
        return 0;
    }
    json_obj = json_object_object_get(json_obj, "data");
    array_list *models = json_object_get_array(json_obj);
    if (models == NULL) {
        fprintf(stderr, "Error getting models from response\n");
        return 0;
    }
    int model_count = array_list_length(models);
    char **model_names = malloc(model_count * sizeof(char *));
    if (model_names == NULL) {
        fprintf(stderr, "Error allocating %lu bytes of memory for model names\n", model_count * sizeof(char *));
        return 0;
    }
    for (int i = 0; i < model_count; i++) {
        json_object *model = array_list_get_idx(models, i);
        json_object *id = NULL;
        if (json_object_object_get_ex(model, "id", &id)) {
            model_names[i] = (char *)json_object_get_string(id);
        } else {
            fprintf(stderr, "Error getting model id from response\n");
            return 0;
        }
    }
    qsort(model_names, model_count, sizeof(char *), string_compare);
    for (int i = 0; i < model_count; i++) {
        printf("    %s\n", model_names[i]);
    }
    return nmemb;
}

static const char *query(json_object *options) {
    debug("openai query()\n");
    CURLcode res;
    json_object *messages_obj = NULL;
    json_object *query_obj = NULL; 
    json_object *json_obj = NULL;
    json_object *field_obj = NULL;
    const char *response = NULL;
    char *endpoint = NULL;
    const char *host = default_host;
    const char *model = default_model;
    const char *context = NULL;
    enum json_tokener_error jerr;
    if (openai_init()) {
        return NULL;
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
    messages_obj = query_get_history(options);
    query_obj = json_object_new_object();
    if (query_obj == NULL || messages_obj == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        goto term;
    }
    json_object_object_add(query_obj, "model", json_object_new_string(model));
    json_object_object_add(query_obj, "messages", messages_obj);
    setup_curl(query_obj, endpoint, query_callback);
    debug("openai query: %s\n", json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PLAIN));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        goto term;
    }
    timestamp = time(NULL);
    json_object *prompt_obj = NULL;
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
    return NULL;
}

static size_t query_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    debug("query_callack(): response = \"%*s\"\n", (int)nmemb, (char *)contents);
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, contents, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type == json_type_object) {
        json_object *choices = NULL;
        json_object *error = NULL;
        if (json_object_object_get_ex(json_obj, "choices", &choices)) {
            json_object *choice = json_object_array_get_idx(choices, 0);
            json_object *content = NULL;
            json_object *message = NULL;
            if (choice == NULL) {
                fprintf(stderr, "Error getting choice from response\n");
                return 0;
            }
            if (json_object_object_get_ex(choice, "message", &message)) {
                if (json_object_object_get_ex(message, "content", &content)) {
                    char *s = (char *)json_object_get_string(content);
                    printf("%s\n", s);
                    file_append_tmp(&tmp_response, s);
                } else {
                    fprintf(stderr, "Error getting content from response\n");
                    return 0;
                }
            } else {
                fprintf(stderr, "Error getting message from response\n");
                return 0;
            }
        } else if (json_object_object_get_ex(json_obj, "error", &error)) {
            json_object *message = NULL;
            if (json_object_object_get_ex(error, "message", &message)) {
                fprintf(stderr, "API error: %s\n", json_object_get_string(message));
            } else {
                fprintf(stderr, "Error getting message from response\n");
            }
            return 0;
        }
    } else {
        fprintf(stderr, "Response doesn't appear to be a JSON object:\n%*s\n", (int)nmemb, (char *)contents);
        return 0;
    }
    return nmemb;
}

static json_object *query_get_history(json_object *options) {
    const char *fn = NULL;
    const char *context = NULL;
    json_object *context_obj = NULL;
    json_object *history_obj = NULL;
    json_object *system_prompt_obj = NULL;
    json_object *context_history_obj = NULL;
    json_object *field_obj = NULL;
    json_object *new_entry;
    array_list *context_history_array = NULL;
    bool system_prompt = false;
    int context_history_count = 0;
    int history_count = 1;
    history_obj = json_object_new_array();
    if (history_obj == NULL) {
        fprintf(stderr, "Error creating new JSON object\n");
        return NULL;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_SYSTEM_PROMPT, &system_prompt_obj)) {
        new_entry = json_object_new_object();
        if (new_entry == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            return NULL;
        }
        json_object_object_add(new_entry, "role", json_object_new_string("system"));
        json_object_object_add(new_entry, "content", system_prompt_obj);
        json_object_array_add(history_obj, new_entry);
        system_prompt = true;
    }
    if ((context_history_obj = context_get_history()) != NULL) {
        context_history_array = json_object_get_array(context_history_obj);
        context_history_count = json_object_array_length(context_history_obj);
        history_count += context_history_count;
    }
    for (int i = 0; i < context_history_count; i++) {
        json_object *entry = json_object_array_get_idx(context_history_obj, i);
        const char *prompt_str = NULL;
        const char *response_str = NULL;
        if (entry != NULL) {
            prompt_str = context_get_history_prompt(entry);
            response_str = context_get_history_response(entry);
            if (prompt_str != NULL) {
                new_entry = json_object_new_object();
                if (new_entry == NULL) {
                    fprintf(stderr, "Error creating new JSON object\n");
                    return NULL;
                }
                if (system_prompt_obj != NULL && json_object_get_boolean(system_prompt_obj)) {
                    json_object_object_add(new_entry, "role", json_object_new_string("system"));
                } else {
                    json_object_object_add(new_entry, "role", json_object_new_string("user"));
                }
                json_object_object_add(new_entry, "content", json_object_new_string(prompt_str));
            }
            json_object_array_add(history_obj, new_entry);
            if (response_str != NULL) {
                new_entry = json_object_new_object();
                if (new_entry == NULL) {
                    fprintf(stderr, "Error creating new JSON object\n");
                    return NULL;
                }
                json_object_object_add(new_entry, "role", json_object_new_string("assistant"));
                json_object_object_add(new_entry, "content", json_object_new_string(response_str));
            }
            json_object_array_add(history_obj, new_entry);
        }
    }
    if (!system_prompt) {
        new_entry = json_object_new_object();
        if (new_entry == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            return NULL;
        }
        json_object_object_add(new_entry, "role", json_object_new_string("user"));
        json_object_object_add(new_entry, "content", json_object_object_get(options, SETTING_KEY_PROMPT));
        json_object_array_add(history_obj, new_entry);
    }
    return history_obj;
}

static void setup_curl(json_object *query_obj, const char *endpoint, setup_curl_callback_t callback) {
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
}

static int string_compare(const void *a, const void *b) {
    return strcmp(*(char **)a, *(char **)b);
}

static int option_emd_validate(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("option_emd_validate()\n");
    json_object_object_add(settings_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(option->value));
    json_object *openai_obj = context_get(ai_provider);
    if (openai_obj == NULL) {
        openai_obj = json_object_new_object();
        if (openai_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            return 1;
        }
    }
    json_object_object_add(openai_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(option->value));
    context_set(ai_provider, openai_obj);
    return 0;
}

static int set_missing_emd(option_t *option, json_object *actions_obj, json_object *settings_obj) {
    debug("set_missing_emd()\n");
    json_object *value;
    json_object *openai_obj = context_get(ai_provider);
    if (json_object_object_get_ex(settings_obj, SETTING_KEY_EMBEDDING_MODEL, &value)) {
        debug("    openai embedding model is set to %s\n", json_object_get_string(value));
        return 0;
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
    debug("    model in context file is %s\n", m);
    json_object_object_add(settings_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(m));
    if (openai_obj == NULL) {
        openai_obj = json_object_new_object();
        if (openai_obj == NULL) {
            fprintf(stderr, "Error creating new JSON object\n");
            return 1;
        }
    }
    json_object_object_add(openai_obj, SETTING_KEY_EMBEDDING_MODEL, json_object_new_string(m));
    debug("    wrote %s to context file\n", json_object_to_json_string_ext(openai_obj, JSON_C_TO_STRING_PRETTY));
    context_set(ai_provider, openai_obj);
    return 0;
}
