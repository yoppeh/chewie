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
#define debug(...) printf(__VA_ARGS__)
#else 
#define debug(...)
#endif

#endif // _CHEWIE_H
