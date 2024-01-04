/**
 * action.h
 *
 * Actions that are performed in response to command line arguments.
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
#define ACTION_KEY_RESET_CONTEXT        "reset-context"
#define ACTION_KEY_BUFFERED             "buffered"
#define ACTION_KEY_DUMP_QUERY_HISTORY   "dump-query-history"
#define ACTION_KEY_SET_SYSTEM_PROMPT    "system-prompt"
#define ACTION_KEY_UPDATE_CONTEXT       "update-context"

///  The result of an action. These are sent back to the caller so it
///  knows whether to continue processing or not.
typedef enum action_result_t {
    ACTION_END,     // The action was successful and processing should end. 
    ACTION_ERROR,   // An error occurred and processing should end.
    ACTION_CONTINUE // The action was successful and processing should continue.
} action_result_t;

typedef struct action_t {
    const char *name;
    action_result_t (*callback)(json_object *settings, json_object *data);
} action_t;

/// Execute all actions specified in the given options object.
extern action_result_t action_execute_all(json_object *actions, json_object *settings);

#endif // _ACTION_H
