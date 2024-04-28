/**
 * @file configure.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Configure environment and setup actions./**
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

#ifndef _CONFIGURE_H
#define _CONFIGURE_H

#include <json-c/json_object.h>

#include "option.h"

/** 
 * @brief Character used to indicate that a list of items should be 
 * displayed. 
 */
extern const char *list_argument;
/** @brief Program name, taken from av[0]. */
extern const char *program_name;

/** @brief Strings representing recognized environment variables. */
#define ENV_KEY_AI_HOST             "CHEWIE_AI_HOST"
#define ENV_KEY_AI_PROVIDER         "CHEWIE_AI_PROVIDER"
#define ENV_KEY_CONTEXT_FILENAME    "CHEWIE_CONTEXT_FILENAME"
#define ENV_KEY_MODEL               "CHEWIE_MODEL"

/**
 * @brief Parse command line arguments into JSON object.
 * @param actions json_object to store the actions.
 * @param settings json_object to store the settings.
 * @param ac argc parameter passed to main().
 * @param av argv parameter passed to main().
 * @return * int 0 if successful, non-zero otherwise.
 */
extern int configure(json_object *actions, json_object *settings, int ac, char **av);

#endif // _CONFIGURE_H
