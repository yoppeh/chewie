/**
 * @file input.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Handle reading input from stdin.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chewie.h"
#include "file.h"
#include "input.h"

#define INPUT_BUFFER_SIZE 1024

char *input_get(void) {
    debug_enter();
    static char input_buffer[INPUT_BUFFER_SIZE];
    FILE *tmp_file = NULL;
    while (fgets(input_buffer, INPUT_BUFFER_SIZE, stdin) != NULL) {
        file_append_tmp(&tmp_file, input_buffer);
    }
    debug_return file_read_tmp(&tmp_file);
}
