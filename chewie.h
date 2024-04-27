/**
 * chewie.h
 *
 * Version information.
 */

#ifndef _CHEWIE_H
#define _CHEWIE_H

#define VERSION_MAJOR   0
#define VERSION_MINOR   0
#define VERSION_REV     1

#ifdef DEBUG
#include <stdio.h>
extern int indent_level;
extern char *indent_string;
#define debug(...) do { printf("%.*s%s  %d  ", indent_level, indent_string, __FILE__, __LINE__); printf(__VA_ARGS__); } while(0)
#define debug_indent_inc() do { indent_level += 4; } while(0)
#define debug_indent_dec() do { indent_level -= 4; if (indent_level < 0) indent_level = 0; } while(0)
#define debug_enter() do { debug("%s()\n", __PRETTY_FUNCTION__); indent_level += 4; } while(0)
#define debug_exit indent_level -= 4; return
#else 
#define debug(...)
#define debug_enter()
#define debug_exit return
#endif

#endif // _CHEWIE_H
