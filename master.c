/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

//
// master.c
// This is the main file for the master executable.
//

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

#include "shared.h"

void handle_sigint(int sig)
{
    // TODO: Handle ^C
}

int main(int argc, char* argv[])
{
    image_path = argv[0];

    // Handle ^C from terminal
    signal(SIGINT, NULL);

    // TODO: Read command-line arguments

    //
    // Read Input Strings File
    //

    // Local (non-shared) string buffer
    // This is the working memory for organizing the input strings before we share them
    // We can't getdelim(3) into shared memory, so we need to use normal memory first
    struct
    {
        char** buf;
        size_t cap;
        size_t num;
    } strings = { NULL, 0, 0 };

    // The size of all strings combined, assuming they're packed tightly
    size_t total_strings_area_size = 0;

    // Allocate initial capacity of ten
    strings.buf = calloc(10, sizeof(char*));
    strings.cap = 10;

    if (strings.buf == NULL)
    {
        perrorf("%s: unable to allocate initial buffer capacity", image_path);
        return 1;
    }

    // Open input strings file for read
    FILE* strings_file = fopen("test.in", "r"); // TODO: Get file path from cmd args

    if (strings_file == NULL)
    {
        perrorf("%s: unable to read strings file", image_path);
        return 1;
    }

    // Read strings from file into buffer
    // For short strings, getdelim(3) can be memory inefficient, but this shouldn't matter
    size_t line_buffer_size = 0;
    while (getdelim(&strings.buf[strings.num], &line_buffer_size, '\n', strings_file) != -1)
    {
        strings.num++;
        char* line = strings.buf[strings.num - 1];
        size_t line_len = strlen(line);

        // Trim newline characters from string
        while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
        {
            line[line_len - 1] = '\0';
            line_len--;
        }

        // Add packed line length (including null terminator) to the total strings area size
        total_strings_area_size += line_len + 1;

        // If buffer has reached capacity
        if (strings.num == strings.cap)
        {
            // Attempt to double the buffer size
            size_t new_cap = strings.cap * 2;
            char** new_buf = realloc(strings.buf, new_cap * sizeof(char*));

            if (new_buf == NULL)
            {
                perrorf("%s: unable to increase buffer size", image_path);
                return 1;
            }

            // Zero out new elements in buffer and start using it
            size_t cap_diff = new_cap - strings.cap;
            memset(new_buf + cap_diff, 0, cap_diff * sizeof(char*));
            strings.buf = new_buf;
            strings.cap = new_cap;
        }

        // Reset line buffer size to zero
        // getdelim(3) changes it after each call, but wants it to be zero!
        line_buffer_size = 0;
    }

    // If getdelim(3) exited with an error
    if (errno)
    {
        perrorf("%s: getdelim(3) failed", image_path);
        return 1;
    }

    // Close input strings file
    fclose(strings_file);

    // Create shared buffer
    // See README for information on its structure
    int shm_id;
    char* shared_buffer = create_shared_buffer((1 + strings.num) * sizeof(size_t) + total_strings_area_size, &shm_id);

    // Pack total number of strings into shared buffer
    set_shared_num_strings(shared_buffer, strings.num);

    // Pack strings into shared buffer
    size_t current_strings_area_size = 0;
    for (size_t i = 0; i < strings.num; ++i)
    {
        add_shared_string(shared_buffer, i, strings.num, &current_strings_area_size, strings.buf[i]);
    }

    // Free local string buffer
    // All strings are in shared memory now
    free(strings.buf);

    //
    // Spawn Worker Processes
    //

    // FIXME: This whole fork thing is just a test
    pid_t p = fork();
    if (p == -1)
    {
        perrorf("%s: fork(2) failed", image_path);
        return 1;
    }
    else if (p == 0)
    {
        // In child
        execv("./palin", (char* []) { "./palin", "1", "4999", NULL });
        _exit(0);
    }
    else
    {
        // In parent
        wait(NULL);
    }

    // Clean up shared memory
    destroy_shared_buffer(shared_buffer, shm_id);

    return 0;
}
