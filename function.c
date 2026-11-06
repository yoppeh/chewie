/**
 * @file function.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief LUA function implmentation.
 * @version 0.1.0
 * @date 2024-05-25
 * @copyright Copyright (c) 2024
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <json-c/linkhash.h>

#include "chewie.h"
#include "file.h"
#include "function.h"
#include "setting.h"

static void function_exit(void);
static json_object *parse_enum(void);
static json_object *parse_functions(void);
static json_object *parse_parameters(void);

int function_init(void) {
  debug_enter();
  int result = 1;
  atexit(function_exit);
  result = 0;
term:
  debug_return result;
}

const char *function_invoke(const char *name, json_object *args) {
  debug_enter();
  const char *result = NULL;
  debug("Function result: %s\n", result);
term:
  debug_return result;
}

int function_load(const char *fn, json_object *settings) {
  (void)settings;
  debug_enter();
  /* Function support is not available (Lua was removed). Don't fail the
     whole run because a function file was requested or saved in the
     context; just say it's being ignored. */
  fprintf(stderr,
          "Warning: function support is not available; ignoring function "
          "file \"%s\" (reset the context with \"r\" to stop this warning)\n",
          fn);
  debug_return 0;
}

static void function_exit(void) {
  debug_enter();
  debug_return;
}
