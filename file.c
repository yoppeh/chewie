/**
 * @file file.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief File I/O operations.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#include <dirent.h> 
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "chewie.h"
#include "file.h"

static int dir_exists(const char *path);
static int mk_dir(const char* path, mode_t mode);

int file_append_tmp(FILE **f, const char *s) {
    debug_enter();
    if (*f == NULL) {
        *f = tmpfile();
        if (*f == NULL) {
            perror("Error creating a temporary context file");
            debug_return 1;
        }
    }
    debug("appending \"%s\" to temporary context file\n", s);
    fprintf(*f, "%s", s);
    debug("flushing temporary context file\n");
    fflush(*f);
    debug("exiting\n");
    debug_return 0;
}

int file_create_path(const char *path) {   
    debug_enter();
    char *_path = NULL;
    char *p; 
    int result = -1;
    mode_t mode = 0777;
    errno = 0;
    if (dir_exists(path)) {
        debug_return 0;
    }
    _path = strdup(path);
    if (_path == NULL)
        goto out;
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mk_dir(_path, mode) != 0)
                goto out;
            *p = '/';
        }
    }   
    if (mk_dir(_path, mode) != 0)
        goto out;
    result = 0;
out:
    free(_path);
    debug_return result;
}

char *file_read(const char *filename) {
    debug_enter();
    char *c = NULL;
    struct stat sb;
    int fd = -1;
    if (stat(filename, &sb) == -1) {
        goto err;
    }
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Unable to open file \"%s\"\n", filename);
        goto err;
    }
    if (fstat(fd, &sb) != 0) {
        fprintf(stderr, "Unable to stat file \"%s\"\n", filename);
        goto err;
    }
    if (sb.st_size == 0) {
        debug_return NULL;
    }
    c = malloc(sb.st_size + 1);
    if (c == NULL) {
        fprintf(stderr, "Unable to allocate %ld byte buffer for reading file\n", sb.st_size);
        goto err;
    }
    if (read(fd, c, sb.st_size) == -1) {
        fprintf(stderr, "Unable to read file \"%s\"\n", filename);
        goto err;
    }
    c[sb.st_size] = 0; // Null-terminate
    close(fd);
    debug_return c;
err:
    if (fd > -1) {
        close(fd);
    }
    debug_return NULL;
}

extern char *file_read_tmp(FILE **f) {
    debug_enter();
    if (*f != NULL) {
        if (fseek(*f, 0L, SEEK_END) == -1) {
            perror("Unable to determine size of temporary context file");
            debug_return NULL;
        }
        long len = ftell(*f);
        rewind(*f);
        char *result = malloc(len + 1);
        fread(result, 1, len, *f);
        result[len] = 0; // Null-terminate
        fclose(*f);
        *f = NULL;
        debug_return result;
    }
    debug_return NULL;
}

void file_truncate(const char *filename) {
    debug_enter();
    mode_t mode = 0;
    mode |= S_IRUSR | S_IWUSR;
    mode |= S_IRGRP | S_IWGRP;
    mode |= S_IROTH;
    int fd;
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode)) > -1) {
        close(fd);
    }
    debug_return;
}

void file_write(const char *filename, const char *data) {
    debug_enter();
    mode_t mode = 0;
    char *c = NULL;
    struct stat sb;
    int fd = -1;
    mode |= S_IRUSR | S_IWUSR;
    mode |= S_IRGRP | S_IWGRP;
    mode |= S_IROTH;
    fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, mode);
    if (fd == -1) {
        fprintf(stderr, "Unable to open/create context file \"%s\"\n", filename);
        goto err;
    }
    if (write(fd, data, strlen(data)) == -1) {
        fprintf(stderr, "Unable to write context file \"%s\"\n", filename);
        goto err;
    }
    close(fd);
    debug_return;
err:
    if (fd > -1) {
        close(fd);
    }
    debug_return;
}

static int dir_exists(const char *path) {
    debug_enter();
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        debug_return 1;
    }
    debug_return 0;
}

static int mk_dir(const char* path, mode_t mode) {
    debug_enter();
    struct stat st;
    errno = 0;
    if (mkdir(path, mode) == 0)
        debug_return 0;
    if (errno != EEXIST)
        debug_return -1;
    if (stat(path, &st) != 0)
        debug_return -1;
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        debug_return -1;
    }
    errno = 0;
    debug_return 0;
}
