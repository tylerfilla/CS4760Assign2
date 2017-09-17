/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shared.h"

char* image_path = NULL;

void perrorf(const char* format, ...)
{
    // Make sure perror(3) will actually print something
    if (!errno)
        return;

    // Format into a buffer
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    perror(buffer);
}

char* create_shared_buffer(size_t size, int* shm_id)
{
    // Try to create unique IPC key for shared memory
    // TODO: Base this on a file we own
    key_t ipc_key = ftok("/bin/echo", 'Q');
    if (ipc_key == -1)
    {
        perrorf("%s: ftok(3) failed", image_path);
        exit(1);
    }

    // Allocate and attach memory for shared buffer
    *shm_id = shmget(ipc_key, size, IPC_CREAT | IPC_EXCL | 0600);
    if (*shm_id == -1)
    {
        perrorf("%s: shmget(2) failed", image_path);
        exit(1);
    }
    char* shared_buffer = shmat(*shm_id, NULL, 0);
    if (shared_buffer == (void*) -1)
    {
        perrorf("%s: shmat(2) failed", image_path);
        exit(1);
    }

    return shared_buffer;
}

void destroy_shared_buffer(char* shared_buffer, int shm_id)
{
    // Detach and remove shared buffer memory
    if (shmdt(shared_buffer) == -1)
    {
        perrorf("%s: shmdt(2) failed", image_path);
        exit(1);
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
    {
        perrorf("%s: shmctl(2) failed", image_path);
        exit(1);
    }
}

const char* get_shared_buffer()
{
    // Create matching IPC key for shared memory
    // TODO: Base this on a file we own
    key_t ipc_key = ftok("/bin/echo", 'Q');
    if (ipc_key == -1)
    {
        perrorf("%s: ftok(3) failed", image_path);
        exit(1);
    }

    // Attach shared buffer as read-only memory
    int shm_id = shmget(ipc_key, 0, 0);
    if (shm_id == -1)
    {
        perrorf("%s: shmget(2) failed", image_path);
        exit(1);
    }
    const char* shared_buffer = shmat(shm_id, NULL, SHM_RDONLY);
    if (shared_buffer == (void*) -1)
    {
        perrorf("%s: shmat(2) failed", image_path);
        exit(1);
    }

    return shared_buffer;
}

size_t get_shared_num_strings(const char* buffer)
{
    size_t num_strings;
    memcpy(&num_strings, buffer, sizeof(size_t));
    return num_strings;
}

void set_shared_num_strings(char* buffer, size_t num_strings)
{
    memcpy(buffer, &num_strings, sizeof(size_t));
}

const char* get_shared_string(const char* buffer, size_t index)
{
    // Use lookup table to find the desired string's offset and get it
    size_t string_offset;
    memcpy(&string_offset, buffer + sizeof(size_t) + index * sizeof(size_t), sizeof(size_t));
    return &buffer[string_offset];
}

void add_shared_string(char* buffer, size_t index, size_t count, size_t* mass, const char* string)
{
    size_t str_len = strlen(string);

    // The destination for the shared string
    char* str_dest = buffer + (1 + count) * sizeof(size_t) + *mass;
    size_t str_off = str_dest - buffer;

    // Pack string into string area of shared buffer
    memcpy(str_dest, string, str_len);
    str_dest[str_len] = '\0';
    *mass += str_len + 1;

    // Pack string offset into lookup table
    memcpy(buffer + (1 + index) * sizeof(size_t), &str_off, sizeof(size_t));
}
