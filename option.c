/**
 * option.c
 * 
 * Functions for parsing command line options.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configure.h"
#include "option.h"

static option_t *match_option(option_t **options, char *s);

option_t **option_merge(option_t **options1, option_t **options2) {
    int n1 = 0, n2 = 0;
    if (options1 != NULL) {
        while (options1[n1] != NULL) {
            n1++;
        }
    }
    if (options2 != NULL) {
        while (options2[n2] != NULL) {
            n2++;
        }
    }
    int n = n1 + n2 + 1;
    option_t **options = malloc((n) * sizeof(option_t *));
    if (options == NULL) {
        return NULL;
    }
    if (options1 != NULL) {
        memcpy(options, options1, n1 * sizeof(option_t *));
    } 
    if (options2 != NULL) {
        memcpy((void *)(((char *)options) + (n1 * sizeof(option_t *))), options2, n2 * sizeof(option_t *));
    }
    options[n - 1] = NULL;
    return options;
}

int option_parse_args(option_t **options, int ac, char **av, json_object *actions_obj, json_object *settings_obj) {
    for (int i = 1; i < ac; i++) {
        option_t *option = match_option(options, av[i]);
        if (option == NULL) {
            fprintf(stderr, "Unrecognized option \"%s\"\n", av[i]);
            return 1;
        }
        option->present = true;
        const char *equal = strchr(av[i], '=');
        switch (option->arg_type) {
            case option_arg_none:
                if (equal != NULL) {
                    fprintf(stderr, "Option \"%s\" does not take an argument\n", option->name);
                    return 1;
                }
                break;
            case option_arg_required:
                if (equal == NULL) {
                    fprintf(stderr, "Option \"%s\" requires an argument\n", option->name);
                    return 1;
                }
                option->value = equal + 1;
                break;
            case option_arg_optional:
                if (equal != NULL) {
                    option->value = equal + 1;
                }
                break;
        }
        if (option->validate != NULL) {
            int r = option->validate(option, actions_obj, settings_obj);
            if (r != 0) {
                return 1;
            }
        }
    }
    return 0;
}

int option_set_missing(option_t **options, json_object *actions_obj, json_object *settings_obj) {
    for (int i = 0; options[i] != NULL; i++) {
        if (options[i]->set_missing != NULL) {
            if (options[i]->set_missing(options[i], actions_obj, settings_obj) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

static option_t *match_option(option_t **options, char *s) {
    int equal = 0;
    for (equal = 0; s[equal] != '\0'; equal++) {
        if (s[equal] == '=') {
            break;
        }
    }
    for (int i = 0; options[i] != NULL; i++) {
        int l = strlen(options[i]->name);
        if (equal > l) {
            l = equal;
        }
        if (strncmp(options[i]->name, s, l) == 0) {
            return options[i];
        }
    }
    return NULL;
}

void option_show_help(option_t **options) {
    int fld_width = 0;
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    for (int i = 0; options[i] != NULL; i++) {
        int len = strlen(options[i]->name);
        if (options[i]->arg_type == option_arg_required) {
            len += 7;
        } else if (options[i]->arg_type == option_arg_optional) {
            len += 9;
        }
        if (len > fld_width) {
            fld_width = len;
        }
    }
    fld_width += 3;
    char *s = malloc(fld_width + 1);
    if (s == NULL) {
        return;
    }
    for (int i = 0; options[i] != NULL; i++) {
        memset(s, '.', fld_width);
        s[fld_width] = '\0';
        int l = strlen(options[i]->name);
        memcpy(s, options[i]->name, l);
        if (options[i]->arg_type == option_arg_required) {
            memcpy(s + l, "=value ", 7);
        } else if (options[i]->arg_type == option_arg_optional) {
            memcpy(s + l, "[=value] ", 9);
        } else {
            s[l] = ' ';
        }
        printf("    %s %s\n", s, options[i]->description);
    }
}
