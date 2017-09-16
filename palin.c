/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

//
// palin.c
// This is the main file for the palin executable.
//

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

/**
 * Determine if an ASCII string is a palindrome.
 *
 * @param first A pointer to the first character
 * @param last A pointer to the last character (must be >= first)
 * @return Nonzero if such is the case, otherwise zero
 */
int is_palindrome(const char* first, const char* last)
{
    // Recursive base case
    if (first >= last)
        return 1;

    if (*first == *last)
        return is_palindrome(first + 1, last - 1);

    return 0;
}

/**
 * The file path to the executable image of this process.
 */
static const char* image_path;

/**
 * Print the error message for errno in the following format:
 *
 * <program>: <pid>: <user msg>: <details>
 *
 * @param fmt The user message
 */
void printf_error(const char* fmt, ...)
{
    // Make sure perror(3) will actually print something
    if (!errno)
        return;

    // Format into a buffer
    va_list args;
    va_start(args, fmt);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Prefix standard perror(3) with executable name
    fprintf(stderr, "%s: %d: ", image_path, getpid());
    perror(buffer);
}

int main(int argc, char* argv[])
{
    image_path = argv[0];

    //
    // Parse Command-Line Arguments
    //

    // Quick and dirty argument check
    // This program isn't intended to be used directly by humans
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <seq> <str>\n", image_path);
        return 1;
    }

    // Parse seq argument
    char* fail_seq = { '\0' };
    size_t seq = strtoul(argv[1], &fail_seq, 10);

    if (*fail_seq != '\0')
    {
        fprintf(stderr, "invalid seq: %s\n", argv[1]);
        return 1;
    }

    // Parse str argument
    char* fail_str = { '\0' };
    size_t str = strtoul(argv[2], &fail_str, 10);

    if (*fail_str != '\0')
    {
        fprintf(stderr, "invalid str: %s\n", argv[2]);
        return 1;
    }

    //
    // Connect to Shared Memory
    //

    // Create matching IPC key for shared memory
    // TODO: Base this on a file we own
    key_t ipc_key = ftok("/bin/echo", 'Q');
    if (ipc_key == -1)
    {
        printf_error("ftok(3) failed");
        return 1;
    }

    // Attach shared buffer as read-only memory
    int shm_id = shmget(ipc_key, 0, 0);
    if (shm_id == -1)
    {
        printf_error("shmget(2) failed");
        return 1;
    }
    const char* shared_buffer = shmat(shm_id, NULL, SHM_RDONLY);
    if (shared_buffer == (void*) -1)
    {
        printf_error("shmat(2) failed");
        return 1;
    }

    //
    // Get Desired String
    //

    // Extract total number of strings from shared buffer
    size_t num_strings;
    memcpy(&num_strings, shared_buffer, sizeof(size_t));

    if (str >= num_strings)
    {
        fprintf(stderr, "str out of range: %ld\n", str);
        return 1;
    }

    // Use lookup table to find the desired string's offset
    size_t string_offset;
    memcpy(&string_offset, shared_buffer + sizeof(size_t) + str * sizeof(size_t), sizeof(size_t));

    // Get actual string and find its length
    const char* string = &shared_buffer[string_offset];
    size_t string_length = strlen(string);

    printf("%s is palindrome? %d\n", string, is_palindrome(string, string + string_length));

    return 0;
}
