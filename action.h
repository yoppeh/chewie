/**
 * @file action.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Actions that are performed in response to certain command line arguments.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */
 
#ifndef _ACTION_H
#define _ACTION_H

#include <json-c/json_object.h>

#define ACTION_KEY_AI_HOST              "ai-host"
#define ACTION_KEY_AI_MODEL             "ai-model"
#define ACTION_KEY_AI_PROVIDER          "ai-provider"
#define ACTION_KEY_CONTEXT_FILENAME     "context-filename"
#define ACTION_KEY_HELP                 "help"
#define ACTION_KEY_VERSION              "version"
#define ACTION_KEY_LIST_APIS            "list-apis"
#define ACTION_KEY_LIST_MODELS          "list-models"
#define ACTION_KEY_LOAD_FUNCTION_FILE   "load-function-file"
#define ACTION_KEY_RESET_CONTEXT        "reset-context"
#define ACTION_KEY_BUFFERED             "buffered"
#define ACTION_KEY_DUMP_QUERY_HISTORY   "dump-query-history"
#define ACTION_KEY_SET_SYSTEM_PROMPT    "system-prompt"
#define ACTION_KEY_UPDATE_CONTEXT       "update-context"
#define ACTION_KEY_GET_EMBEDDINGS       "get-embeddings"
#define ACTION_KEY_QUERY                "query"

/**
 * @brief The result of an action. These are sent back to the caller so it
 * knows whether to continue processing or not.
 */
typedef enum action_result_t {
    ACTION_END,     // The action was successful and processing should end. 
    ACTION_ERROR,   // An error occurred and processing should end.
    ACTION_CONTINUE // The action was successful and processing should continue.
} action_result_t;

/**
 * @brief Represents an action that can be executed. Actions are built up 
 * during option processing and then executed in order.
 */
typedef struct action_t {
    const char *name;
    action_result_t (*callback)(json_object *settings, json_object *data);
} action_t;

/**
 * @brief Execute all actions specified in the given options object.
 * @param actions json_object containing the actions to execute.
 * @param settings json_object containing the settings to use.
 * @return action_result_t 
 */
extern action_result_t action_execute_all(json_object *actions, json_object *settings);

#endif // _ACTION_H
