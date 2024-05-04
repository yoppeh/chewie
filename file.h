/**
 * @file file.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief File I/O operations.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>

/** 
 * @brief Append the given string to the temporary file. The file is created if it
 * does not exist.
 * @param f The file to append to.
 * @param s The string to append.
 * @return 0 on success, 1 on failure.
 */
extern int file_append_tmp(FILE **f, const char *s);

/** 
 * @brief Create the given path. Programmatic version of `mkdir -p`.
 * @param s The path to create.
 * @return 0 on success, 1 on failure.
 */
extern int file_create_path(const char *s);

/** 
 * @brief Read the contents of the given file.
 * @param filename The file to read.
 * @return The contents of the file.
 */
extern char *file_read(const char *filename);

/** 
 * @brief Get the contents of the temporary file and close the file.
 * @param f The file to read.
 * @return The contents of the file or NULL on error
 */
extern char *file_read_tmp(FILE **f);

/** 
 * @brief Truncate the given file.
 * @param filename The file to truncate.
 */
extern void file_truncate(const char *filename);

/** 
 * @brief Write the given data to the given file.
 * @param filename The file to write to.
 * @param data The data to write.
 */
extern void file_write(const char *filename, const char *data);

#endif // _FILE_H
