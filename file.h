/**
 * file.h
 *
 * File I/O operations.
 */

#ifndef _FILE_H
#define _FILE_H

/// Append the given string to the temporary file. The file is created if it
/// does not exist.
extern int file_append_tmp(FILE **f, const char *s);
/// Create the given path. Programmatic version of `mkdir -p`.
extern int file_create_path(const char *s);
/// Read the contents of the given file.
extern char *file_read(const char *filename);
/// Get the contents of the temporary file and close the file.
extern char *file_read_tmp(FILE **f);
/// Truncate the given file.
extern void file_truncate(const char *filename);
/// Write the given data to the given file.
extern void file_write(const char *filename, const char *data);

#endif // _FILE_H
