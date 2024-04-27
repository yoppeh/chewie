/**
 * input.c
 *
 * Handle reading input from stdin.
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
    debug_exit file_read_tmp(&tmp_file);
}
