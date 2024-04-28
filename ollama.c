/**
 * @file ollama.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Interface to Ollama API.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#include <curl/curl.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "chewie.h"
#include "action.h"
#include "api.h"
#include "context.h"
#include "file.h"
#include "ollama.h"
#include "setting.h"

typedef size_t (*curl_callback_t)(char *ptr, size_t size, size_t nmemb, void *userdata);

static const char default_host[] = "http://localhost:11434";
static const char api_query_endpoint[] = "/api/generate";
static const char api_listmodels_endpoint[] = "/api/tags";
static const char api_embeddings_endpoint[] = "/api/embeddings";
static const char default_model[] = "codellama:7b-instruct";

static CURL *curl = NULL; 
static struct json_tokener *json = NULL;
static const char *prompt_str = NULL;
static json_object *query_obj = NULL;
static int64_t timestamp = 0;

static action_t **get_actions(void);
static option_t **get_options(void);
static const char *get_api_name(void);
static const char *get_default_host(void);
static const char *get_default_model(void);
static char *get_endpoint(const char *host, const char *endpoint);
static size_t list_models_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
static int get_embeddings(json_object *options);
static size_t get_embeddings_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
static void ollama_exit(void);
static int ollama_init(void);
static size_t print_curl_data(char *data, size_t len);
static int print_model_list(json_object *options);
static const char *query(json_object *json_obj);
static size_t query_callback(char *ptr, size_t size, size_t nmemb, void *user_data);
static void setup_curl(json_object *query_obj, const char *endpoint, curl_callback_t callback);
static int string_compare(const void *a, const void *b);

static api_interface_t ollama_api_interface = {
    .get_actions = get_actions,
    .get_options = get_options,
    .get_default_host = get_default_host,
    .get_default_model = get_default_model,
    .get_api_name = get_api_name,
    .get_embeddings = get_embeddings,
    .print_model_list = print_model_list,
    .query = query
};

const api_interface_t *ollama_get_aip_interface(void) {
    debug_enter();
    debug_return &ollama_api_interface;
}

static const char *get_api_name(void) {
    debug_enter();
    debug_return "ollama";
}

static action_t **get_actions(void) {
    debug_enter();
    debug_return NULL;
}

static option_t **get_options(void) {
    debug_enter();
    debug_return NULL;
}

static const char *get_default_host(void) {
    debug_enter();
    char *host = NULL;
    char *s = getenv("OLLAMA_HOST");
    if (s != NULL) {
        int l = strlen(s) + 1;
        if (strncmp(s, "http://", sizeof("http://") - 1) != 0) {
            l += strlen("http://");
            host = malloc(l);
            if (host != NULL) {
                strncpy(host, "http://", l);
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

static size_t list_models_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    debug_enter();
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, ptr, nmemb);
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
    json_object *models = json_object_object_get(json_obj, "models");
    if (models == NULL) {
        fprintf(stderr, "No models debug_returned\n");
        debug_return 0;
    }
    array_list *model_array = json_object_get_array(models);
    if (model_array == NULL) {
        fprintf(stderr, "Error reading list of models\n");
        debug_return 0;
    }
    int model_count = json_object_array_length(models);
    char **model_list = malloc(model_count * sizeof(char *));
    for (int i = 0; i < model_count; i++) {
        json_object *model = json_object_array_get_idx(models, i);
        if (model == NULL) {
            fprintf(stderr, "Error reading model information\n");
            debug_return 0;
        }
        json_object *name = json_object_object_get(model, "name");
        if (name == NULL) {
            fprintf(stderr, "Couldn't find model name\n");
            debug_return 0;
        }
        const char *model_name = json_object_get_string(name);
        if (model_name == NULL) {
            fprintf(stderr, "Couldn't get model name\n");
            debug_return 0;
        }
        model_list[i] = (char *)model_name;
    }
    qsort(model_list, model_count, sizeof(char *), string_compare);
    for (int i = 0; i < model_count; i++) {
        printf("    %s\n", model_list[i]);
    }
    debug_return nmemb;
}

static void ollama_exit(void) {
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

static int ollama_init(void) {
    debug_enter();
    CURLcode res;
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        debug_return 1;
    }
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "API request error: couldn't initialize curl\n");
        debug_return 1;
    }
    json = json_tokener_new();
    if (json == NULL) {
        fprintf(stderr, "JSON parser error: couldn't initialize JSON parser\n");
        debug_return 1;
    }
    debug_return 0;
}

static size_t print_curl_data(char *data, size_t len) {
    debug_enter();
    char *s = malloc(len + 1);
    if (s == NULL) {
        fprintf(stderr, "Error allocating buffer for input\n");
        debug_return 0;
    }
    memcpy(s, data, len);
    s[len] = 0;
    fprintf(stdout, "%s", s);
    fflush(stdout);
    free(s);
    debug_return len;
}

static int print_model_list(json_object *options) {
    debug_enter();
    CURLcode res;
    json_object *field_obj = NULL;
    const char *host = default_host;
    char *endpoint = NULL;
    if (ollama_init()) {
        debug_return 1;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_AI_HOST, &field_obj)) {
        host = json_object_get_string(field_obj);
    }
    if ((endpoint = get_endpoint(host, api_listmodels_endpoint)) == NULL) {
        debug_return 1;
    }
    setup_curl(NULL, endpoint, list_models_callback);
    printf("Models available at %s:\n", host);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        debug_return 1;
    }
    ollama_exit();
    debug_return 0;
}

static const char *query(json_object *options) {
    debug_enter();
    CURLcode res;
    char *endpoint = NULL;
    char *response = NULL;
    json_object *ollama_obj = NULL;
    json_object *embeddings_obj = NULL;
    json_object *field_obj = NULL;
    json_object *options_obj = NULL;
    json_object *system_prompt_obj = NULL;
    const char *host = default_host;
    const char *model = default_model;
    const char *embeddings = NULL;
    const char *system_prompt = NULL;
    enum json_tokener_error jerr;
    if (ollama_init()) {
        debug_return NULL;
    } 
    if (json_object_object_get_ex(options, SETTING_KEY_AI_HOST, &field_obj)) {
        host = json_object_get_string(field_obj);
    }
    if (json_object_object_get_ex(options, SETTING_KEY_PROMPT, &field_obj)) {
        prompt_str = json_object_get_string(field_obj);
    } else {
        jerr = json_tokener_get_error(json);
        fprintf(stderr, "JSON parse error: %s\n", json_tokener_error_desc(jerr));
        goto term;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_AI_MODEL, &field_obj)) {
        model = json_object_get_string(field_obj);
    }
    ollama_obj = context_get("ollama");
    if (ollama_obj) {
        embeddings_obj = json_object_object_get(ollama_obj, "embeddings");
        if (embeddings_obj) {
            embeddings = json_object_get_string(embeddings_obj);
        }
    }
    options_obj = json_object_new_object();
    json_object_object_add(options_obj, "num_ctx", json_object_new_int(4096));
    query_obj = json_object_new_object();
    if (query_obj == NULL) {
        fprintf(stderr, "Error constructing JSON query object\n");
        goto term;
    }
    json_object_object_add(query_obj, "model", json_object_new_string(model));
    json_object_object_add(query_obj, "prompt", json_object_new_string(prompt_str));
    json_object_object_add(query_obj, "options", options_obj);
    if (embeddings != NULL && embeddings[0] == '[') {
        field_obj = json_tokener_parse_ex(json, embeddings, strlen(embeddings));
        json_object_object_add(query_obj, "context", field_obj);
    }
    if (json_object_object_get_ex(options, SETTING_KEY_BUFFERED, &field_obj)) {
        int b = json_object_get_boolean(field_obj);
        json_object_object_add(query_obj, "stream", json_object_new_boolean(!b));
    }
    if ((endpoint = get_endpoint(host, api_query_endpoint)) == NULL) {
        goto term;
    }
    if (json_object_object_get_ex(options, SETTING_KEY_SYSTEM_PROMPT, &system_prompt_obj)) {
        system_prompt = json_object_get_string(system_prompt_obj);
        if (system_prompt != NULL) {
            json_object_object_add(query_obj, "system", json_object_new_string(system_prompt));
        }
    }
    debug("query() post data: %s\n", json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PRETTY));
    setup_curl(query_obj, endpoint, query_callback);
    timestamp = time(NULL);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        goto term;
    }
    fprintf(stdout, "\n");
term:
    ollama_exit();
    debug_return NULL;
}

static size_t query_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    debug_enter();
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    static FILE *tmp_response = NULL;
    static FILE *tmp_context = NULL;
    json_obj = json_tokener_parse_ex(json, ptr, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        debug_return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        debug_return 0;
    }
    enum json_type type = json_object_get_type(json_obj);
    if (type == json_type_object) {
        json_object *data = NULL;
        if (json_object_object_get_ex(json_obj, "response", &data)) {
            const char *response = json_object_get_string(data);
            fprintf(stdout, "%s", response);
            fflush(stdout);
            file_append_tmp(&tmp_response, response);
        }
        if (json_object_object_get_ex(json_obj, "context", &data)) {
            const char *embeddings = json_object_get_string(data);
            file_append_tmp(&tmp_context, embeddings);
        }
        if (json_object_object_get_ex(json_obj, "done", &data)) {
            if (json_object_get_boolean(data)) {
                json_object *ollama_obj = json_object_new_object();
                if (!ollama_obj) {
                    fprintf(stderr, "Error constructing JSON updates object\n");
                    debug_return 0;
                }
                const char *response = file_read_tmp(&tmp_response);
                const char *embeddings = file_read_tmp(&tmp_context);
                json_object_object_add(ollama_obj, "embeddings", json_tokener_parse_ex(json, embeddings, strlen(embeddings)));
                context_add_history(prompt_str, response, timestamp);
                context_set("ollama", ollama_obj);
                context_update();
                debug_return nmemb;
            }

        }
        if (json_object_object_get_ex(json_obj, "error", &data)) {
            printf("\n>> Error: %s\n", json_object_get_string(data));
            debug_return nmemb;
        } 
    }
    debug_return nmemb;
}

static int get_embeddings(json_object *settings) {
    debug_enter();
    CURLcode res;
    char *endpoint = NULL;
    char *response = NULL;
    json_object *ollama_obj = NULL;
    json_object *embeddings_obj = NULL;
    json_object *field_obj = NULL;
    json_object *settings_obj = NULL;
    json_object *system_prompt_obj = NULL;
    const char *host = default_host;
    const char *model = default_model;
    const char *embeddings = NULL;
    const char *system_prompt = NULL;
    enum json_tokener_error jerr;
    if (ollama_init()) {
        debug_return 1;
    } 
    if (json_object_object_get_ex(settings, SETTING_KEY_AI_HOST, &field_obj)) {
        host = json_object_get_string(field_obj);
    }
    if (json_object_object_get_ex(settings, SETTING_KEY_PROMPT, &field_obj)) {
        prompt_str = json_object_get_string(field_obj);
    } else {
        jerr = json_tokener_get_error(json);
        fprintf(stderr, "JSON parse error: %s\n", json_tokener_error_desc(jerr));
        debug_return 1;
    }
    if (json_object_object_get_ex(settings, SETTING_KEY_AI_MODEL, &field_obj)) {
        model = json_object_get_string(field_obj);
    }
    query_obj = json_object_new_object();
    if (query_obj == NULL) {
        fprintf(stderr, "Error constructing JSON query object\n");
        debug_return 1;;
    }
    json_object_object_add(query_obj, "model", json_object_new_string(model));
    json_object_object_add(query_obj, "prompt", json_object_new_string(prompt_str));
    if ((endpoint = get_endpoint(host, api_embeddings_endpoint)) == NULL) {
        debug_return 1;
    }
    debug("get_embeddings() post data: %s\n", json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PRETTY));
    setup_curl(query_obj, endpoint, get_embeddings_callback);
    timestamp = time(NULL);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "API request error: %s\n", curl_easy_strerror(res));
        debug_return 1;
    }
    fprintf(stdout, "\n");
term:
    ollama_exit();
    debug_return 0;
}

static size_t get_embeddings_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    debug_enter();
    static json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    json_obj = json_tokener_parse_ex(json, ptr, nmemb);
    jerr = json_tokener_get_error(json);
    if (jerr == json_tokener_continue) {
        debug_return nmemb;
    }
    if (jerr != json_tokener_success) {
        fprintf(stderr, "Error parsing JSON response: %s\n", json_tokener_error_desc(jerr));
        debug_return 0;
    }
    json_object *embeddings = json_object_object_get(json_obj, "embedding");
    if (embeddings == NULL) {
        debug_return nmemb;
    }
    for (int i = 0, j = json_object_array_length(embeddings); i < j; i++) {
        json_object *embedding = json_object_array_get_idx(embeddings, i);
        if (embedding == NULL) {
            fprintf(stderr, "Error reading embedding\n");
            debug_return 0;
        }
        const char *embedding_str = json_object_get_string(embedding);
        if (embedding_str == NULL) {
            fprintf(stderr, "Error reading embedding\n");
            debug_return 0;
        }
        fprintf(stdout, "%s\n", embedding_str);
    }
    debug_return nmemb;
}

static void setup_curl(json_object *query_obj, const char *endpoint, curl_callback_t callback) {
    debug_enter();
    if (query_obj) {
        const char *s = json_object_to_json_string_ext(query_obj, JSON_C_TO_STRING_PLAIN);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(s));
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_URL, endpoint);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    debug_return;
}

static int string_compare(const void *a, const void *b) {
    debug_enter();
    debug_return strcmp(*(char **)a, *(char **)b);
}
