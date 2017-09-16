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

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

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
        fprintf(stderr, "%s: ", argv[0]);
        perror("unable to allocate initial buffer capacity");
        return 1;
    }

    // Open input strings file for read
    FILE* strings_file = fopen("test.in", "r"); // TODO: Get file path from cmd args

    if (strings_file == NULL)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("unable to read strings file");
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
                fprintf(stderr, "%s: ", argv[0]);
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
        fprintf(stderr, "%s: ", argv[0]);
        perror("getdelim(3) failed");
        return 1;
    }

    // Close input strings file
    fclose(strings_file);

    // Calculate size for shared buffer
    // See README for information about its composition
    // In short: number of strings, string lookup table, and string mass area
    size_t shared_buffer_size = sizeof(size_t) + strings.num * sizeof(size_t) + total_string_mass;

    // Try to create unique IPC key for shared memory
    // TODO: Base this on a file we own
    key_t ipc_key = ftok("/bin/echo", 'Q');
    if (ipc_key == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("ftok(3) failed");
        return 1;
    }

    // Allocate and attach memory for shared buffer
    int shm_id = shmget(ipc_key, shared_buffer_size, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("shmget(2) failed");
        return 1;
    }
    char* shared_buffer = shmat(shm_id, NULL, 0);
    if (shared_buffer == (void*) -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("shmat(2) failed");
        return 1;
    }

    // Pack string count into shared buffer
    memcpy(shared_buffer, &strings.num, sizeof(size_t));

    // Pack strings into shared buffer
    size_t string_mass = 0;
    for (size_t li = 0; li < strings.num; ++li)
    {
        // The input string
        char* str = strings.buf[li];
        size_t str_len = strlen(str);

        // The destination for the shared string
        char* str_dest = shared_buffer + (1 + strings.num) * sizeof(size_t) + string_mass;
        size_t str_off = str_dest - shared_buffer;

        // Pack string into string area of shared buffer
        memcpy(str_dest, str, str_len);
        str_dest[str_len] = '\0';
        string_mass += str_len + 1;

        // Pack string offset into lookup table
        memcpy(shared_buffer + (1 + li) * sizeof(size_t), &str_off, sizeof(size_t));
    }

    // Free local string buffer
    free(strings.buf);
    memset(&strings, 0, sizeof(strings));

    // FIXME: This whole fork thing is just a test
    pid_t p = fork();
    if (p == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("fork(2) failed");
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

    // Detach and remove shared buffer memory
    if (shmdt(shared_buffer) == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("shmdt(2) failed");
        return 1;
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("shmctl(2) failed");
        return 1;
    }

    return 0;
}
