/**
 * option.c
 * 
 * Functions for parsing command line options.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chewie.h"
#include "configure.h"
#include "option.h"

static option_t *match_option(option_t **options, char *s);

option_t **option_merge(option_t **options1, option_t **options2) {
    debug_enter();
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
        debug_exit NULL;
    }
    if (options1 != NULL) {
        memcpy(options, options1, n1 * sizeof(option_t *));
    } 
    if (options2 != NULL) {
        memcpy((void *)(((char *)options) + (n1 * sizeof(option_t *))), options2, n2 * sizeof(option_t *));
    }
    options[n - 1] = NULL;
    debug_exit options;
}

int option_parse_args(option_t **options, int ac, char **av, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    for (int i = 1; i < ac; i++) {
        option_t *option = match_option(options, av[i]);
        if (option == NULL) {
            fprintf(stderr, "Unrecognized option \"%s\"\n", av[i]);
            debug_exit 1;
        }
        option->present = true;
        const char *equal = strchr(av[i], '=');
        switch (option->arg_type) {
            case option_arg_none:
                if (equal != NULL) {
                    fprintf(stderr, "Option \"%s\" does not take an argument\n", option->name);
                    debug_exit 1;
                }
                break;
            case option_arg_required:
                if (equal == NULL) {
                    fprintf(stderr, "Option \"%s\" requires an argument\n", option->name);
                    debug_exit 1;
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
                debug_exit 1;
            }
        }
    }
    debug_exit 0;
}

int option_set_missing(option_t **options, json_object *actions_obj, json_object *settings_obj) {
    debug_enter();
    for (int i = 0; options[i] != NULL; i++) {
        if (options[i]->set_missing != NULL) {
            if (options[i]->set_missing(options[i], actions_obj, settings_obj) != 0) {
                debug_exit 1;
            }
        }
    }
    debug_exit 0;
}

static option_t *match_option(option_t **options, char *s) {
    debug_enter();
    int equal = 0;
    for (equal = 0; s[equal] != '\0'; equal++) {
        if (s[equal] == '=') {
            break;
        }
    }
    for (int i = 0; options[i] != NULL; i++) {
        char *name = NULL;
        int l = strlen(options[i]->name);
        if (options[i]->api != NULL) {
            l += strlen(options[i]->api) + 1;
        }
        name = malloc(l + 1);
        if (name == NULL) {
            debug_exit NULL;
        }
        if (options[i]->api != NULL) {
            strcpy(name, options[i]->api);
            strcat(name, ".");
        } else {
            name[0] = '\0';
        }
        strcat(name, options[i]->name);
        if (equal > l) {
            l = equal;
        }
        if (strncmp(name, s, l) == 0) {
            debug_exit options[i];
        }
    }
    debug_exit NULL;
}

void option_show_help(option_t **options) {
    debug_enter();
    int fld_width = 0;
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    for (int i = 0; options[i] != NULL; i++) {
        int len = strlen(options[i]->name);
        if (options[i]->api != NULL) {
            len += strlen(options[i]->api) + 1;
        }
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
        debug_exit;
    }
    for (int i = 0; options[i] != NULL; i++) {
        char *sp = s;
        if (options[i]->api != NULL && (options[i]->api != options[i - 1]->api)) {
            printf("%s API options:\n", options[i]->api);
        }
        memset(s, '.', fld_width);
        s[fld_width] = '\0';
        int l = strlen(options[i]->name);
        if (options[i]->api != NULL) {
            int al = strlen(options[i]->api);
            memcpy(s, options[i]->api, al);
            sp += al;
            sp++;
        }
        memcpy(sp, options[i]->name, l);
        sp += l;
        if (options[i]->arg_type == option_arg_required) {
            memcpy(sp, "=value ", 7);
        } else if (options[i]->arg_type == option_arg_optional) {
            memcpy(sp, "[=value] ", 9);
        } else {
            *sp = ' ';
        }
        printf("    %s %s\n", s, options[i]->description);
    }
    debug_exit;
}
