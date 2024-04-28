/**
 * @file option.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Functions for parsing command line options.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#ifndef _OPTION_H
#define _OPTION_H

#include <stdbool.h>

#include <json-c/json.h>

/// Type of argument an option takes.
typedef enum option_arg_t {
    option_arg_none = 0, // No argument.
    option_arg_required = 1, // Argument required.
    option_arg_optional = 2 // Argument optional.
} option_arg_t;

/// Describes an option. Set name and arg_type appropriately, and set value to
/// NULL. validate() will be called any time the corresponding option is
/// specified on the command line. If validate() returns 0, the option is
/// considered valid. If validate() returns non-zero, the option is considered
/// invalid and option_parse_args() will return 1. The set_missing callback is
/// called for options that are not specified on the command line. The
/// option_set_missing() function should be called after option_parse_args().
/// It will scan all option_t structures and call set_missing() for each one
/// that wasn't specified on the command-line. The supplied set_missing() 
/// function should return 0 on success or 1 on error. For api-specific options,
/// the api field should be set to the same api name that is returned by the
/// api_get_name() function.
typedef struct option_t {
    const char *name;
    const char *description;
    option_arg_t arg_type;
    const char *value;
    int (*validate)(struct option_t *option, json_object *actions_obj, json_object *setting_obj);
    int (*set_missing)(struct option_t *option, json_object *actions_obj, json_object *setting_obj);
    const char *api;
    bool present;
} option_t;

/// Merge two NULL-terminated arrays of option_t pointers into a single
/// NULL-terminated array. The resulting array is allocated with malloc() and
/// must be freed with free().
extern option_t **option_merge(option_t **options1, option_t **options2);
/// Parse command line arguments into options. Returns 0 on success, 1 on
/// failure. Options is a NULL-terminated array of option_t pointers. ac and av
/// are the argument count and argument vector, respectively, as passed to main.
extern int option_parse_args(option_t **options, int ac, char **av, json_object *actions_obj, json_object *settings_obj);
/// Show help text for options. Options is a NULL-terminated array of option_t.
extern int option_set_missing(option_t **options, json_object *actions_obj, json_object *settings_obj);
/// Show help text for options. Options is a NULL-terminated array of option_t.
extern void option_show_help(option_t **options);

#endif // _OPTION_H
