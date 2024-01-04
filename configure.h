/**
 * configure.h
 *
 * Configure according to command line arguments and environment. Uses getopt()
 * to parse command line arguments and json-c to store the results in a JSON
 * object. That object is then sent to the action processor to execute any
 * actions specified, such as dumping the query. The object is then passed on
 * to the API interface to be used to send the proper query to the API.
 */

#ifndef _CONFIGURE_H
#define _CONFIGURE_H

#include <json-c/json_object.h>

#include "option.h"

/// Character used to indicate that a list of items should be displayed.
extern const char *list_argument;
/// Program name, taken from av[0].
extern const char *program_name;

/// Strings representing recognized environment variables.
#define ENV_KEY_AI_HOST             "CHEWIE_AI_HOST"
#define ENV_KEY_AI_PROVIDER         "CHEWIE_AI_PROVIDER"
#define ENV_KEY_CONTEXT_FILENAME    "CHEWIE_CONTEXT_FILENAME"
#define ENV_KEY_MODEL               "CHEWIE_MODEL"

/// Parse command line arguments into JSON object.
extern int configure(json_object *actions, json_object *settings, int ac, char **av);

#endif // _CONFIGURE_H
