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

int main(int argc, char* argv[])
{
    // Local (non-shared) string buffer
    // This is working memory for organizing the input strings
    struct
    {
        char** buf;
        size_t cap;
        size_t num;
    } strings = { NULL, 0, 0 };

    // The size of all strings combined, assuming they're packed tightly
    // In this model, string boundaries are delimited by null terminators
    // This will be used when allocating the shared memory buffer
    size_t total_string_mass = 0;

    // Allocate initial capacity of ten
    strings.buf = calloc(10, sizeof(char*));
    strings.cap = 10;

    if (strings.buf == NULL)
    {
        perror("unable to allocate initial buffer capacity");
        return 1;
    }

    // Open input strings file for read
    FILE* strings_file = fopen("../test.in", "r"); // TODO: Get file path from cmd args

    if (strings_file == NULL)
    {
        perror("unable to open strings file for read");
        return 1;
    }

    // Read strings from file into buffer
    // For short strings, getdelim(3) can be memory inefficient, but this shouldn't matter
    size_t line_buffer_size = 0;
    while (getdelim(&strings.buf[strings.num++], &line_buffer_size, '\n', strings_file) != -1)
    {
        char* line = strings.buf[strings.num - 1];
        size_t line_len = strlen(line);

        // Trim newline characters from string
        while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
        {
            line[line_len - 1] = '\0';
            line_len--;
        }

        // Add packed line length (including null terminator) to the total string mass
        total_string_mass += line_len + 1;

        // If buffer has reached capacity
        if (strings.num == strings.cap)
        {
            // Attempt to double the buffer size
            size_t new_cap = strings.cap * 2;
            char** new_buf = realloc(strings.buf, new_cap * sizeof(char*));

            if (new_buf == NULL)
            {
                perror("unable to increase buffer size");
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
        perror("getdelim(3) failed");
        return 1;
    }

    // Close input strings file
    fclose(strings_file);

    // TODO: Pack strings into shared memory

    free(strings.buf);

    /*
    // Try to create unique IPC key for shared memory
    key_t ipc_key = ftok(argv[0], 'Q');
    if (ipc_key == -1)
    {
        perror("ftok(3) failed");
        return 1;
    }

    // Allocate and attach shared memory for strarr instance
    int shm_id = shmget(ipc_key, sizeof(strarr_t), IPC_CREAT | 0600);
    if (shm_id == -1)
    {
        perror("shmget(2) failed");
        return 1;
    }
    strarr_t* strings_array = shmat(shm_id, NULL, 0);
    if (strings_array == NULL)
    {
        perror("shmat(2) failed");
        return 1;
    }
    strarr_construct(strings_array);

    // Detach and free strarr instance on shared memory
    strarr_destruct(strings_array);
    if (shmdt(strings_array) == -1)
    {
        perror("shmdt(2) failed");
        return 1;
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
    {
        perror("shmctl(2) failed");
        return 1;
    }

    /*
    // Try to open strings file for read
    FILE* strings_file = fopen("../test.in", "r");
    if (strings_file == NULL)
    {
        perror("unable to open file");
        return 1;
    }

    fclose(strings_file);
    */

    return 0;
}
